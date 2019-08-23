// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPLog.h"
#include "MMDHtmlOutputProcessor.h"

#include <stdlib.h>

NS_SP_EXT_BEGIN(app)

void processFile(const String &path, bool print) {
	auto text = filesystem::readTextFile(path);
	if (!text.empty()) {
		auto name = filepath::name(path);
		auto target = filesystem::currentDir("output/" + name + ".html");

		if (!print) {
			std::ofstream fstream(target);
			mmd::HtmlOutputProcessor::run(&fstream, text);
			fstream.close();

			StringStream cmd;
			cmd << "diff -u \"" << filesystem::currentDir("html/" + name + ".html") << "\" \"" << target << "\"";

			auto cmdStr = cmd.str();

			std::cout << "==== Test: " << name << "\n";

			system(cmdStr.data());
			std::cout << "==== End of diff " << name << "\n";

		} else {
			mmd::HtmlOutputProcessor::run(&std::cout, text);
		}
	}
}

NS_SP_EXT_END(app)

using namespace stappler;

int parseOptionSwitch(data::Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	} else if (c == 'p') {
		ret.setBool(true, "print");
	}
	return 1;
}

int parseOptionString(data::Value &ret, const String &str, int argc, const char * argv[]) {
	if (str == "print") {
		ret.setBool(true, "print");
	} else if (str == "verbose") {
		ret.setBool(true, "verbose");
	}
	return 1;
}

int _spMain(argc, argv) {
	data::Value opts = data::parseCommandLineOptions(argc, argv,
			&parseOptionSwitch, &parseOptionString);

	stappler::data::Value &args = opts.getValue("args");

	bool print = opts.getBool("print");

	if (!print) {
		auto baseDir = filesystem::currentDir("output");
		filesystem::remove(baseDir, true, true);
		filesystem::mkdir(baseDir);
	}

	String dir("mmd");

	if (args.size() >= 2) {
		dir = args.getString(1);
	}

	std::cout << filesystem::currentDir(dir) << "\n";

	filesystem::ftw(filesystem::currentDir(dir), [&] (const String &path, bool isFile) {
		if (isFile) {
			app::processFile(path, print);
		}
	});

	return 0;
}
