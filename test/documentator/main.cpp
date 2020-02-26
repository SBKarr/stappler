// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "DocUnit.h"

namespace stappler {

static constexpr auto HELP_STRING(
R"HelpString(doc <options> <command>
Options are one of:
    -v (--verbose)
    -h (--help))HelpString");


int parseOptionSwitch(data::Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	} else if (c == 'v') {
		ret.setBool(true, "verbose");
	}
	return 1;
}

int parseOptionString(data::Value &ret, const StringView &str, int argc, const char * argv[]) {
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

	doc::Unit unit;

	unit.exclude("lz4.h");
	unit.exclude("lz4hc.h");
	unit.exclude("xxhash.h");

	filesystem::ftw("/home/sbkarr/android/StapplerNext/stappler/components/common", [&] (StringView path, bool isFile) {
		if (isFile && filepath::lastExtension(path) == "h") {
			auto name = filepath::lastComponent(path);
			unit.addFile(path, name);
		}
	});

	unit.parseDefines();

	auto &defs = unit.getDefines();
	for (auto &it : defs) {
		if (!it.second.isGuard) {
			std::cout << it.first;
			if (it.second.isFunctional) {
				std::cout << " ( ";
				bool first = true;
				for (auto &iit : it.second.args) {
					if (first) { first = false; } else { std::cout << ", "; }
					std::cout << iit;
				}
				std::cout << " )";
			}
			std::cout << ":\n";
			for (auto &iit : it.second.refs) {
				auto pos = unit.getLineNumber(iit.first->content, iit.second->token);
				std::cout << "\t" << iit.first->key << ":" << pos.first << ":\n";
				std::cout << "\t\t" << pos.second << "\n";
			}
		}
	}

	/*auto file = unit.addFile("/home/sbkarr/android/StapplerNext/stappler/components/common/thirdparty/lz4/lib/lz4hc.h", "lz4hc.h");
	if (file) {
		file->token->describe(std::cout);
	}*/

	return 0;
}

}
