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
	String httpPort = "8080";
	String httpsPort = "8443";

	String serverAdmin = "you@localhost";
	String docuemntRoot = "www";
	String serverName = "localhost";
	Vector<String> serverAliases;

	String dbParams = "host=localhost dbname=postgres user=postgres password=postgres";
	String sessionKey = "SerenityDockerKey";

	String sslCert = "crt/server.crt";
	String sslKey = "crt/server.key";
	String sslCA = "crt/ca.crt";

	String source = "Lib.so:CreateComponent";

	String _privateKey;
	String _publicKey;
};

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
		std::cerr << "Conf dir not found: " << path << "\n";
		return 1;
	}

	return 0;
}

NS_SP_END
