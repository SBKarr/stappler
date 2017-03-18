// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SLParser.h"
#include "SPLayout.h"
#include "SLStyle.h"
#include "SPString.h"

NS_LAYOUT_BEGIN

namespace table {

// internal color table function,
bool getColor(const CharReaderBase &str, Color3B &color);

}

namespace style {

static void setHslColor(Color3B &color, float h, float sl, float l) {
	float v;

	h = h / 360.0f;
	sl = sl / 100.0f;
	l = l / 100.0f;

	float r,g,b;
	r = l;   // default to gray
	g = l;
	b = l;

	v = (l <= 0.5) ? (l * (1.0 + sl)) : (l + sl - l * sl);

	if (v > 0) {
		float m;
		float sv;
		int sextant;
		float fract, vsf, mid1, mid2;

		m = l + l - v;
		sv = (v - m ) / v;
		h *= 6.0;
		sextant = (int)h;
		fract = h - sextant;
		vsf = v * sv * fract;
		mid1 = m + vsf;
		mid2 = v - vsf;

		switch (sextant) {
		case 0: r = v; g = mid1; b = m; break;
		case 1: r = mid2; g = v; b = m; break;
		case 2: r = m; g = v; b = mid1; break;
		case 3: r = m; g = mid2; b = v; break;
		case 4: r = mid1; g = m; b = v; break;
		case 5: r = v; g = m; b = mid2; break;
		}
	}

	color.r = (uint8_t)(r * 255.0f);
	color.g = (uint8_t)(g * 255.0f);
	color.b = (uint8_t)(b * 255.0f);
}

static bool readColorDigits(const CharReaderBase &origStr, float *b, int num, bool isRgb) {
	CharReaderBase str(origStr);
	str.skipChars<CharReaderBase::CharGroup<CharGroupId::WhiteSpace>>();
	if (!str.is('(')) {
		return false;
	}
	++ str;

	for (int i = 0; i < num; i++) {
		b[i] = str.readFloat();
		if (IsErrorValue(b[i])) {
			return false;
		}

		if (b[i] < 0.0f) { b[i] = 0.0f; } // error - less then 0
		if (isRgb) {
			if (b[i] > 255.0f) {
				b[i] = 255.0f; // for RGB absolute values 255 is max
			}
		} else if (!isRgb && (i != 3)) {
			if (i == 0 && b[i] > 359.0f) {
				b[i] = 359.0f; // for HSL: H value max is 359
			} else if (b[i] > 100.0f) {
				b[i] = 100.0f; // for HSL: S and L max is 100%
			}
		} else if (i == 3 && b[i] > 1.0f) {
			b[i] = 1.0f; // for alpha - 1.0 is max
		}

		if (i == 3) {
			b[i] = b[i] * 255.0f; // translate alpha to (0-255)
		}

		str.skipChars<CharReaderBase::CharGroup<CharGroupId::WhiteSpace>>();

		if (str.empty()) {
			return false;
		}

		if (str.is('%')) {
			if (b[i] > 100.0f) {
				b[i] = 100.0f; // for percent values - max is 100
			}
			if (isRgb) {
				b[i] = 255.0f * b[i] / 100.0f; // translate RGB values to (0-255)
			} else if ((!isRgb && i == 0) || i == 3) {
				return false; // for H in HSL and any alpha percent values is denied
			}
			str.skipChars<CharReaderBase::CharGroup<CharGroupId::WhiteSpace>>();
		} else if (!isRgb && (i == 1 || i == 2) && !str.is('%')) {
			return false; // for S and L in HSL only percent values allowed
		}

		if (str.empty()) {
			return false;
		}

		str.skipChars<CharReaderBase::CharGroup<CharGroupId::WhiteSpace>, CharReaderBase::Chars<','>>();

		if (str.is(')') && i == num - 1) {
			return true;
		}
	}

	return true;
}

static bool readRgbaColor(const CharReaderBase &origStr, Color3B &color, uint8_t &opacity) {
	float b[4] = { 0.0f };
	if (readColorDigits(origStr, b, 4, true)) {
		color.r = (uint8_t)b[0];
		color.g = (uint8_t)b[1];
		color.b = (uint8_t)b[2];
		opacity = (uint8_t)b[3];
		return true;
	}
	return false;
}

static bool readRgbColor(const CharReaderBase &origStr, Color3B &color) {
	float b[3] = { 0.0f };
	if (readColorDigits(origStr, b, 3, true)) {
		color.r = (uint8_t)b[0];
		color.g = (uint8_t)b[1];
		color.b = (uint8_t)b[2];
		return true;
	}
	return false;
}

static bool readHslaColor(const CharReaderBase &origStr, Color3B &color, uint8_t &opacity) {
	float b[4] = { 0.0f };
	if (readColorDigits(origStr, b, 4, false)) {
		setHslColor(color, b[0], b[1], b[2]);
		opacity = (uint8_t)b[3];
		return true;
	}
	return false;
}

static bool readHslColor(const CharReaderBase &origStr, Color3B &color) {
	float b[3] = { 0.0f };
	if (readColorDigits(origStr, b, 3, false)) {
		setHslColor(color, b[0], b[1], b[2]);
		return true;
	}
	return false;
}

static bool readHashColor(const CharReaderBase &origStr, Color3B &color) {
	CharReaderBase str(origStr);
	++ str;
	if (str.size() == 6) {
		color.r = base16::hexToChar(str[0], str[1]);
		color.g = base16::hexToChar(str[2], str[3]);
		color.b = base16::hexToChar(str[4], str[5]);
	} else if (str.size() == 3) {
		color.r = base16::hexToChar(str[0], str[0]);
		color.g = base16::hexToChar(str[1], str[1]);
		color.b = base16::hexToChar(str[2], str[2]);
	} else {
		return false;
	}

	return true;
}

static bool readNamedColor(const CharReaderBase &origStr, Color3B &color) {
	if (origStr.compare("white")) {
		color = Color3B::WHITE;
	} else if (origStr.compare("silver")) {
		color = Color3B(192,192,192);
	} else if (origStr.compare("gray")) {
		color = Color3B(128,128,128);
	} else if (origStr.compare("black")) {
		color = Color3B::BLACK;
	} else if (origStr.compare("maroon")) {
		color = Color3B(128,0,0);
	} else if (origStr.compare("red")) {
		color = Color3B(255,0,0);
	} else if (origStr.compare("orange")) {
		color = Color3B(255,165,0);
	} else if (origStr.compare("yellow")) {
		color = Color3B(255,255,0);
	} else if (origStr.compare("olive")) {
		color = Color3B(128,128,0);
	} else if (origStr.compare("lime")) {
		color = Color3B(0,255,0);
	} else if (origStr.compare("green")) {
		color = Color3B(0,128,0);
	} else if (origStr.compare("aqua")) {
		color = Color3B(0,255,255);
	} else if (origStr.compare("blue")) {
		color = Color3B(0,0,255);
	} else if (origStr.compare("navy")) {
		color = Color3B(0,0,128);
	} else if (origStr.compare("teal")) {
		color = Color3B(0,128,128);
	} else if (origStr.compare("fuchsia")) {
		color = Color3B(255,0,255);
	} else if (origStr.compare("purple")) {
		color = Color3B(128,0,128);
	} else if (!table::getColor(origStr, color)) {
		return false;
	}
	return true;
}

bool readColor(const CharReaderBase &str, Color4B &color4) {
	Color3B color;
	uint8_t opacity = 0;
	if (str == "rgba") {
		if (readRgbaColor(CharReaderBase(str.data() + 4, str.size() - 4), color, opacity)) {
			color4 = Color4B(color, opacity);
			return true;
		}
	} else if (str == "hsla") {
		if (readHslaColor(CharReaderBase(str.data() + 4, str.size() - 4), color, opacity)) {
			color4 = Color4B(color, opacity);
			return true;
		}
	} else if (str == "rgb") {
		if (readRgbColor(CharReaderBase(str.data() + 3, str.size() - 3), color)) {
			color4 = Color4B(color);
			return true;
		}
	} else if (str == "hsl") {
		if (readHslColor(CharReaderBase(str.data() + 3, str.size() - 3), color)) {
			color4 = Color4B(color);
			return true;
		}
	} else if (str.is('#')) {
		if (readHashColor(str, color)) {
			color4 = Color4B(color);
			return true;
		}
	} else {
		if (readNamedColor(str, color)) {
			color4 = Color4B(color);
			return true;
		}
	}
	return false;
}

bool readColor(const CharReaderBase &str, Color3B &color) {
	if (str == "rgb") {
		if (readRgbColor(CharReaderBase(str.data() + 3, str.size() - 3), color)) {
			return true;
		}
	} else if (str == "hsl") {
		if (readHslColor(CharReaderBase(str.data() + 3, str.size() - 3), color)) {
			return true;
		}
	} else if (str.is('#')) {
		if (readHashColor(str, color)) {
			return true;
		}
	} else {
		if (readNamedColor(str, color)) {
			return true;
		}
	}
	return false;
}

}

NS_LAYOUT_END
