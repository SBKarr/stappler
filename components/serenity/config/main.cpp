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
	bool hasHttps = false;

	Flags flags = None;
};

SP_DEFINE_ENUM_AS_MASK(Config::Flags);

static constexpr auto HELP_STRING(
R"HelpString(config <options> <handlers-conf-dir>
Options are one of:
    -v (--verbose)
    -h (--help)
	--db="<db-params>"
	--port=<http-port>
	--sport=<https-port>
	--name=<server-name>
	--alias=<server-alias>
	--root=<root-server-name>
	--admin=<server-admin>
	--session=<session-key>)HelpString");

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
	} else if (str.starts_with("db=")) {
		ret.setString(StringView(str, "db="_len, str.size() - "db="_len), "db");
	} else if (str.starts_with("port=")) {
		ret.setString(StringView(str, "port="_len, str.size() - "port="_len), "port");
	} else if (str.starts_with("sport=")) {
		ret.setString(StringView(str, "sport="_len, str.size() - "sport="_len), "sport");
	} else if (str.starts_with("name=")) {
		ret.setString(StringView(str, "name="_len, str.size() - "name="_len), "name");
	} else if (str.starts_with("alias=")) {
		ret.emplace("alias").addString(StringView(str, "alias="_len, str.size() - "alias="_len));
	} else if (str.starts_with("root=")) {
		ret.setString(StringView(str, "root="_len, str.size() - "root="_len), "root");
	} else if (str.starts_with("admin=")) {
		ret.setString(StringView(str, "admin="_len, str.size() - "admin="_len), "admin");
	} else if (str.starts_with("session=")) {
		ret.setString(StringView(str, "session="_len, str.size() - "session="_len), "session");
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

static bool process(Config &cfg, HttpdConfig &httpd) {
	for (auto &it : httpd.cfg) {
		if (it.first == "ServerAdmin") {
			cfg.serverAdmin = it.second.str();
		} else if (it.first == "ServerName") {
			cfg.rootServerName = it.second.str();
		}
	}

	for (auto &it : httpd.vhosts) {
		auto port = it.first;
		port.skipUntil<StringView::Chars<':'>>();
		if (port.is(':')) {
			++ port;
		}

		if (!port.empty()) {
			auto iit = it.second.find(StringView("SSLEngine"));
			if (iit != it.second.end() && string::tolower(iit->second) == "on") {
				cfg.httpsPort = port.str();
				cfg.hasHttps = true;
			} else {
				cfg.httpPort = port.str();
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
					v += "key="_len;
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

static bool write(Config &cfg, HttpdConfig &httpd, StringView path) {
	filesystem::remove(path);

	std::ofstream stream(path.data());
	stream << "# Generated by serenity docker config tool\n\n";
	stream << "Listen " << cfg.httpPort << "\n";
	if (cfg.hasHttps) {
		stream << "Listen " << cfg.httpsPort << "\n";
	}
	stream << "\n";

	for (auto &it : httpd.cfg) {
		if (it.first == "ServerAdmin" && (cfg.flags & Config::Flags::AdminChanged)) {
			stream << it.first << " " << cfg.serverAdmin << "\n";
		} else if (it.first == "ServerName" && (cfg.flags & Config::Flags::RootServerNameChanged)) {
			stream << it.first << " " << cfg.rootServerName << "\n";
		} else if (it.first != "Listen") {
			stream << it.first << " " << it.second << "\n";
		}
	}

	for (auto &it : httpd.vhosts) {
		stream << "\n<VirtualHost *:";
		auto iit = it.second.find(StringView("SSLEngine"));
		if (iit != it.second.end() && string::tolower(iit->second) == "on") {
			stream << cfg.httpsPort;
		} else {
			stream << cfg.httpPort;
		}
		stream << ">\n";

		for (auto &iit : it.second) {
			if (iit.first == "ServerName" && (cfg.flags & Config::Flags::ServerNameChanged)) {
				stream << "\t" << iit.first << " " << cfg.serverName << "\n";
			} else if (iit.first == "ServerAlias" && (cfg.flags & Config::Flags::ServerAliasesChanged)) {
				stream << "\t" << iit.first;
				for (auto &ait : cfg.serverAliases) {
					stream << " " << ait;
				}
				stream << "\n";
			} else if (iit.first == "DBDParams" && (cfg.flags & Config::Flags::DBParamsChanged)) {
				stream << "\t" << iit.first << " \"" << cfg.dbParams << "\"\n";
			} else if (iit.first == "SerenitySession" && (cfg.flags & Config::Flags::SessionKeyChanged)) {
				stream << "\t" << iit.first << " ";

				auto v = iit.second;
				stream << v.readUntilString("key=");
				if (v.is("key=")) {
					v += "key="_len;
					v.skipUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
					stream << "key=" << cfg.sessionKey;
				}
				stream << v << "\n";
			} else {
				stream << "\t" << iit.first << " " << iit.second << "\n";
			}
		}

		stream << "</VirtualHost>\n";
	}
	stream.close();
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

	if (args.size() < 2) {
		return 1;
	}

	auto path = args.getString(1);
	if (!filesystem::exists(path)) {
		std::cerr << "[Fail] Config file not found: " << path << "\n";
		return 1;
	}

	Config cfg;
	HttpdConfig httpd;
	auto str = filesystem::readTextFile(path);
	if (!parse(httpd, str)) {
		std::cerr << "[Fail] fail to parse httpd config\n";
		return 1;
	}

	if (!process(cfg, httpd)) {
		std::cerr << "[Fail] Fail to process httpd config\n";
		return 1;
	}

	for (auto &it : opts.asDict()) {
		if (it.first == "db") {
			cfg.dbParams = it.second.getString();
			cfg.flags |= Config::DBParamsChanged;
		} else if (it.first == "port") {
			cfg.httpPort = it.second.getString();
			cfg.flags |= Config::PortsChanged;
		} else if (it.first == "sport") {
			cfg.httpsPort = it.second.getString();
			cfg.flags |= Config::PortsChanged;
		} else if (it.first == "name") {
			cfg.serverName = it.second.getString();
			cfg.flags |= Config::ServerNameChanged;
		} else if (it.first == "alias") {
			cfg.serverAliases.clear();
			for (auto &iit : it.second.asArray()) {
				cfg.serverAliases.emplace(iit.asString());
			}
			cfg.flags |= Config::ServerAliasesChanged;
		} else if (it.first == "root") {
			cfg.rootServerName = it.second.getString();
			cfg.flags |= Config::RootServerNameChanged;
		} else if (it.first == "admin") {
			cfg.serverAdmin = it.second.getString();
			cfg.flags |= Config::AdminChanged;
		} else if (it.first == "session") {
			cfg.sessionKey = it.second.getString();
			cfg.flags |= Config::SessionKeyChanged;
		}
	}

	auto newName = toString(filepath::root(path), "/gen-", filepath::lastComponent(path));
	if (!write(cfg, httpd, newName)) {
		std::cerr << "[Fail] Fail to write new config to " << newName << "\n";
		return 1;
	}

	std::cout << "[Success] " << newName << "\n";
	return 0;
}

NS_SP_END
