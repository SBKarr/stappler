/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

#include "SPCommon.h"
#include "SPData.h"
#include "SPFilesystem.h"

NS_SP_BEGIN

struct Config {
	enum Flags {
		None = 0,
		PortsChanged = 1,
		AdminChanged = 2,
		RootServerNameChanged = 4,
		ServerNameChanged = 8,
		ServerAliasesChanged = 16,
		DBParamsChanged = 16,
		SessionKeyChanged = 32,
	};

	String httpPort = "8080";
	String httpsPort = "8443";

	String serverAdmin = "you@localhost";
	String rootServerName = "localhost";
	String serverName = "localhost";
	Set<String> serverAliases;

	String dbParams = "host=localhost dbname=postgres user=postgres password=postgres";
	String sessionKey = "SerenityDockerKey";

	String privateKey;
	String publicKey;
	String customConfig;

	Flags flags = None;
};

SP_DEFINE_ENUM_AS_MASK(Config::Flags);

static constexpr auto HELP_STRING(
R"HelpString(config <options> <handlers-conf-dir>
Options are one of:
    -v (--verbose)
    -h (--help))HelpString");

static int parseOptionSwitch(data::Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	} else if (c == 'v') {
		ret.setBool(true, "verbose");
	}
	return 1;
}

static int parseOptionString(data::Value &ret, const StringView &str, int argc, const char * argv[]) {
	if (str == "help") {
		ret.setBool(true, "help");
	} else if (str == "verbose") {
		ret.setBool(true, "verbose");
	}
	return 1;
}

struct HttpdConfig {
	Map<StringView, StringView> cfg;
	Map<StringView, Map<StringView, StringView>> vhosts;
};

static bool parse(HttpdConfig &cfg, StringView v) {
	Vector<Pair<StringView, Map<StringView, StringView> *>> stack;

	stack.emplace_back(v, &cfg.cfg);
	while (!v.empty()) {
		auto line = v.readUntil<StringView::Chars<'\n', '\r'>>();
		v.skipChars<StringView::Chars<'\n', '\r'>>();

		line.trimChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		if (!line.empty()) {
			if (line.is('<')) {
				++ line;
				auto tag = line.readUntil<StringView::Chars<'>'>>();
				auto name = tag.readUntil<StringView::Chars<':', ','>, StringView::CharGroup<CharGroupId::WhiteSpace>>();
				if (name.is('/')) {
					++ name;
					if (stack.back().first == name) {
						stack.pop_back();
					} else {
						std::cerr << "Invalid original config\n";
						return false;
					}
				} else if (name == "VirtualHost") {
					tag.skipChars<StringView::Chars<':', ','>, StringView::CharGroup<CharGroupId::WhiteSpace>>();
					auto &vhost = cfg.vhosts.emplace(tag, Map<StringView, StringView>()).first->second;
					stack.emplace_back(name, &vhost);
				} else {
					stack.emplace_back(name, nullptr);
				}
			} else {
				if (stack.back().second) {
					auto name = line.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
					if (!name.empty()) {
						line.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
						if (!line.empty()) {
							stack.back().second->emplace(name, line);
						} else {
							stack.back().second->emplace(name, StringView());
						}
					}
				}
			}

		}
	}
	return true;
}

bool process(Config &cfg, HttpdConfig &httpd) {
	for (auto &it : httpd.cfg) {
		if (it.first == "ServerAdmin") {
			cfg.serverAdmin = it.second.str();
		} else if (it.first == "ServerName") {
			cfg.rootServerName = it.second.str();
		}
	}

	for (auto &it : httpd.vhosts) {
		bool isSecure = false;
		auto port = it.first;
		port.skipUntil<StringView::Chars<':'>>();
		if (port.is(':')) {
			++ port;
		}

		if (!port.empty()) {
			auto iit = it.second.find("SSLEngine");
			if (iit != it.second.end() && string::tolower(iit->second) == "on") {
				cfg.httpsPort = port;
			} else {
				cfg.httpPort = port;
			}
		}

		for (auto &iit : it.second) {
			if (iit.first == "ServerName") {
				cfg.serverName = iit.second.str();
			} else if (iit.first == "ServerAlias") {
				cfg.serverAliases.emplace(iit.second.str());
			} else if (iit.first == "DBDParams") {
				cfg.dbParams = iit.second.str();
			} else if (iit.first == "SerenitySession") {
				auto v = iit.second;
				v.skipUntilString("key=");
				if (v.is("key=")) {
					auto key = v.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
					if (!key.empty()) {
						cfg.sessionKey = key.str();
					}
				}
			}
		}
	}

	return true;
}

SP_EXTERN_C int _spMain(argc, argv) {
	data::Value opts = data::parseCommandLineOptions(argc, argv,
			&parseOptionSwitch, &parseOptionString);
	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		return 0;
	}

	if (opts.getBool("verbose")) {
		std::cout << " Current work dir: " << stappler::filesystem::currentDir() << "\n";
		std::cout << " Documents dir: " << stappler::filesystem::documentsPath() << "\n";
		std::cout << " Cache dir: " << stappler::filesystem::cachesPath() << "\n";
		std::cout << " Writable dir: " << stappler::filesystem::writablePath() << "\n";
		std::cout << " Options: " << stappler::data::EncodeFormat::Pretty << opts << "\n";
	}

	auto args = opts.getValue("args");

	if (args.size() != 2) {
		return 1;
	}

	auto path = args.getString(1);
	if (!filesystem::exists(path)) {
		std::cerr << "Conf file not found: " << path << "\n";
		return 1;
	}

	Config cfg;
	HttpdConfig httpd;
	auto str = filesystem::readTextFile(path);
	if (!parse(httpd, str)) {
		std::cerr << "File : " << path << "\n";
		return 1;
	}

	process(cfg, httpd);


	return 0;
}

NS_SP_END
