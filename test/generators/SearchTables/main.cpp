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

#include "SPCommon.h"
#include "SPStringView.h"
#include "SPData.h"

#include <string>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include "SPFilesystem.h"

namespace stappler {

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

SP_EXTERN_C int _spMain(argc, argv) {
	data::Value opts = data::parseCommandLineOptions(argc, argv, &parseOptionSwitch, &parseOptionString);

	Vector<Set<char>> groups{
		chars::CharGroup<char, CharGroupId::PunctuationBasic>::toSet(),
		chars::CharGroup<char, CharGroupId::Numbers>::toSet(),
		chars::CharGroup<char, CharGroupId::WhiteSpace>::toSet(),
		chars::CharGroup<char, CharGroupId::LatinLowercase>::toSet(),
		chars::CharGroup<char, CharGroupId::LatinUppercase>::toSet(),
		chars::CharGroup<char, CharGroupId::Hexadecimial>::toSet(),
		chars::CharGroup<char, CharGroupId::Base64>::toSet(),
		chars::CharGroup<char, CharGroupId::TextPunctuation>::toSet()
	};

	size_t i = 0;
	uint16_t buf[256] = { 0 };
	for (auto &it : groups) {
		for (auto c : it) {
			buf[((const uint8_t *)&c)[0]] |= (1 << i);
		}
		++ i;
	}

	for (size_t j = 0; j < 16; ++ j) {
		for (size_t i = 0; i < 16; ++ i) {
			std::cout << std::right << std::setw(4) << buf[16 * j + i] << ",";
		}
		std::cout << "\n";
	}

	return 0;
}

}
