// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPDefine.h"
#include "SPData.h"
#include "SPThreadManager.h"
#include "SPData.h"
#include "SPNetworkDataTask.h"
#include "SPDataStream.h"
#include "SLPath.h"
#include "SLImage.h"

#include <string>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include "SPFilesystem.h"

NS_SP_EXT_BEGIN(app)

struct FontInfo {
	String name;
	Bytes data;

	String path;
	String title;
};

void readFontDir(const StringView &dirPath, Map<String, stappler::app::FontInfo> &icons) {
	filesystem::ftw(dirPath, [&] (const StringView &path, bool isFile) {
		if (isFile) {
			auto bytes = filesystem::readFile(path);
			icons.emplace(filepath::name(path).str(), FontInfo{filepath::name(path).str(), std::move(bytes)});
		}
	}, 1);
}

NS_SP_EXT_END(app)

using namespace stappler;

auto HELP_STRING =
R"Text(fontgen <path-to-fonts-dir>
	- generate font source files
)Text";

auto LICENSE_STRING =
R"Text(/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

// Generated with FontGenerator (%STAPPLER_ROOT%/test/generators/FontGenerator)

)Text";

int parseOptionSwitch(data::Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	}
	return 1;
}

int parseOptionString(data::Value &ret, const StringView &str, int argc, const char * argv[]) {
	if (str == "help") {
		ret.setBool(true, "help");
	}
	return 1;
}

void sp_android_terminate () {
	stappler::log::text("Exception", "Crash on exceprion");
  __gnu_cxx::__verbose_terminate_handler();
}

int _spMain(argc, argv) {
	std::set_terminate(sp_android_terminate);

	data::Value opts = data::parseCommandLineOptions(argc, argv, &parseOptionSwitch, &parseOptionString);
	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		return 0;
	};

	Map<String, stappler::app::FontInfo> fonts;

	auto &args = opts.getValue("args");
	if (args.size() > 1) {
		auto path = args.getString(1);
		stappler::app::readFontDir(path, fonts);
	}

	auto makeVarName = [] (const StringView &var) -> String {
		String ret = var.str();
		for (auto &it : ret) {
			switch(it) {
			case '-':
			case ' ':
			case '.':
				it = '_';
				break;
			default:
				break;
			}
		}
		return ret;
	};

	if (!fonts.empty()) {
		auto dir = filesystem::currentDir("gen");
		filesystem::mkdir(dir);

		for (auto &it : fonts) {
			it.second.path = toString(dir, "/MaterialFont-", it.second.name, ".cc");
			it.second.title = makeVarName(it.second.name);

			auto text = base16::encode(CoderSource(it.second.data));
			auto d = text.data();

			StringStream fileData;
			fileData << LICENSE_STRING << "#include \"Material.h\"\n\nNS_MD_BEGIN\n";

			bool first = false;
			fileData << "static const unsigned char s_font_" << it.second.title << "[] = {";
			for (size_t i = 0; i < text.size() / 2; ++ i) {
				if (!first) {
					first = true;
				} else {
					fileData << ",";
				}
				if (i % 24 == 0) {
					fileData << "\n\t";
				}
				fileData << "0x" << d[i * 2] << d[i * 2 + 1];
			}
			fileData << "\n};\n";
			fileData << "NS_MD_END\n";
			std::cout << "File: " << it.second.path << "\n";

			filesystem::remove(it.second.path);
			filesystem::write(it.second.path, fileData.str());
		}

		StringStream sourceFile;
		sourceFile << LICENSE_STRING << "#include \"Material.h\"\n#include \"MaterialFontSource.h\"\n\n";

		for (auto &it : fonts) {
			sourceFile << "#include \"" << filepath::lastComponent(it.second.path) << "\"\n";
		}

		sourceFile << "\nNS_MD_BEGIN\n\nBytesView getSystemFont(SystemFontName name) {\n"
				"\tswitch (name) {\n";

		for (auto &it : fonts) {
			sourceFile << "\t\tcase SystemFontName::" << it.second.title
					<< ": return BytesView(s_font_" << it.second.title << ", " << it.second.data.size() << "); break;\n";
		}

		sourceFile << "\t}\n\n\treturn BytesView();\n}\n\nNS_MD_END\n";

		auto sourcePath = toString(dir, "/MaterialFontSource.cpp");

		filesystem::remove(sourcePath);
		filesystem::write(sourcePath, sourceFile.str());

		StringStream headerFile;
		headerFile << LICENSE_STRING << "#include \"Material.h\"\n\nNS_MD_BEGIN\n\nenum class SystemFontName {";
		bool first = true;
		for (auto &it : fonts) {
			if (first) { first = false; } else { headerFile << ","; }
			headerFile << "\n\t" << it.second.title;
		}
		headerFile << "\n};\n\n"
				"BytesView getSystemFont(SystemFontName);\n"
				"\nNS_MD_END\n";

		auto headerPath = toString(dir, "/MaterialFontSource.h");

		filesystem::remove(headerPath);
		filesystem::write(headerPath, headerFile.str());
	}

	return 0;
}
