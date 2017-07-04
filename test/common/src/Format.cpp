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
#include "Format.h"

NS_SP_BEGIN

struct TermInfo {
	TermInfo() : colorSupport(false) {
		auto term = getenv("TERM");
		if (term && term[0]) {
			colorSupport = true;
		}
	}

	bool colorSupport;
};

static TermInfo s_info;

String format::color(Color value) {
	if (s_info.colorSupport) {
		switch (value) {
		case Color::Black: return "\033[30m"; break;
		case Color::Red: return "\033[31m"; break;
		case Color::Green: return "\033[32m"; break;
		case Color::Yellow: return "\033[33m"; break;
		case Color::Blue: return "\033[34m"; break;
		case Color::Magenta: return "\033[35m"; break;
		case Color::Cyan: return "\033[36m"; break;
		case Color::LightGray: return "\033[37m"; break;
		case Color::DarkGray: return "\033[90m"; break;
		case Color::LightRed: return "\033[91m"; break;
		case Color::LightGreen: return "\033[92m"; break;
		case Color::LightYellow: return "\033[93m"; break;
		case Color::LightBlue: return "\033[94m"; break;
		case Color::LightMagenta: return "\033[95m"; break;
		case Color::LightCyan: return "\033[96m"; break;
		case Color::White: return "\033[97m"; break;
		}
		return "";
	} else {
		return "";
	}
}

String format::drop() {
	if (s_info.colorSupport) {
		return "\033[0m";
	} else {
		return "";
	}
}

String format::bold() {
	if (s_info.colorSupport) {
		return "\033[1m";
	} else {
		return "";
	}
}

String format::dim() {
	if (s_info.colorSupport) {
		return "\033[2m";
	} else {
		return "";
	}
}

String format::underline() {
	if (s_info.colorSupport) {
		return "\033[4m";
	} else {
		return "";
	}
}

NS_SP_END
