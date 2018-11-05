// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include <string>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include "SPFilesystem.h"

NS_SP_EXT_BEGIN(app)

const char * stopwordList[] = {
	"test1",
	"test2"
};

auto HELP_STRING =
R"Text(stopwords
	- generate stopwords dictionary source
)Text";

auto LICENSE_STRING =
R"Text(/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

// Generated with PathGenerator (%STAPPLER_ROOT%/test/generators/StopwordsGenerator)
// stopwords from postgres repo: https://git.postgresql.org/git/postgresql.git
)Text";

void processDictFile(StringStream &stream, const StringView &data) {
	StringView r(data);
	while (!r.empty()) {
		auto d = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		stream << "\tStringView(\"" << d << "\", " << d.size() << "),\n";
	}
}

int parseOptionSwitch(data::Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	}
	return 1;
}

int parseOptionString(data::Value &ret, const String &str, int argc, const char * argv[]) {
	if (str == "help") {
		ret.setBool(true, "help");
	}
	return 1;
}

void sp_android_terminate () {
	stappler::log::text("Exception", "Crash on exceprion");
  __gnu_cxx::__verbose_terminate_handler();
}

SP_EXTERN_C int _spMain(argc, argv) {
	std::set_terminate(sp_android_terminate);

	data::Value opts = data::parseCommandLineOptions(argc, argv, &parseOptionSwitch, &parseOptionString);
	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		return 0;
	};

	auto dir = filesystem::currentDir("words");

	auto &args = opts.getValue("args");
	if (args.size() > 1) {
		auto path = args.getString(1);
		if (!filepath::isAbsolute(path)) {
			dir = filesystem::currentDir(path);
		} else {
			dir = path;
		}
	}

	StringStream stream;
	stream << LICENSE_STRING <<
R"Text(
#include "SPCommon.h"

NS_SP_EXT_BEGIN(search)

)Text";

	if (filesystem::exists(dir)) {
		filesystem::ftw(dir, [&] (const StringView &path, bool isFile) {
			if (isFile) {
				stream << "static StringView s_" << filepath::name(path) << "_stopwords[] = {\n";
				auto data = filesystem::readFile(path);
				processDictFile(stream, StringView((const char *)data.data(), data.size()));
				stream << "\tStringView()\n};\n";
			}
		});
	}

	stream <<
R"Text(
NS_SP_EXT_END(search)
)Text";

	auto d = filesystem::currentDir("gen");
	filesystem::mkdir(d);

	auto sourceFilePath = toString(d, "/SPSnowballStopwords.cc");

	filesystem::remove(sourceFilePath);;
	filesystem::write(sourceFilePath, stream.str());

	std::cout << "Source file: " << sourceFilePath << "\n";

	return 0;
}

NS_SP_EXT_END(app)
