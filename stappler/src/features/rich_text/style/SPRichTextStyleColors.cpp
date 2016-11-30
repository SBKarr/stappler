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

#include "SPDefine.h"
#include "SPRichTextStyle.h"
#include "SPRichTextParser.h"
#include "SPString.h"

NS_SP_EXT_BEGIN(rich_text)

namespace mdcolors {
	Map<String, uint32_t> colors;
	bool init = false;

	void initColors() {

#define MD_COLOR_SPEC_COMPONENT(Name, Id, Value, Index, Group) \
		colors[ #Name #Id ] = 0x ## Value;

#define MD_COLOR_SPEC_BASE_DEFINE(Name, Group, b50, b100, b200, b300, b400, b500, b600, b700, b800, b900 ) \
	MD_COLOR_SPEC_COMPONENT(Name, 50, b50, 0, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 100, b100, 1, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 200, b200, 2, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 300, b300, 3, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 400, b400, 4, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 500, b500, 5, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 600, b600, 6, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 700, b700, 7, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 800, b800, 8, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 900, b900, 9, Group)

#define MD_COLOR_SPEC_ACCENT_DEFINE(Name, Group, a100, a200, a400, a700 ) \
	MD_COLOR_SPEC_COMPONENT(Name, A100, a100, 10, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, A200, a200, 11, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, A400, a400, 12, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, A700, a700, 13, Group)

#define MD_COLOR_SPEC_DEFINE(Name, Group, b50, b100, b200, b300, b400, b500, b600, b700, b800, b900, a100, a200, a400, a700) \
		MD_COLOR_SPEC_BASE_DEFINE(Name, Group, b50, b100, b200, b300, b400, b500, b600, b700, b800, b900) \
		MD_COLOR_SPEC_ACCENT_DEFINE(Name, Group, a100, a200, a400, a700)

MD_COLOR_SPEC_DEFINE(Red, 0,
		ffebee, // 50
		ffcdd2, // 100
		ef9a9a, // 200
		e57373, // 300
		ef5350, // 400
		f44336, // 500
		e53935, // 600
		d32f2f, // 700
		c62828, // 800
		b71c1c, // 900
		ff8a80, // a100
		ff5252, // a200
		ff1744, // a400
		d50000  // a700
		);

MD_COLOR_SPEC_DEFINE(Pink, 1,
		fce4ec, // 50
		f8bbd0, // 100
		f48fb1, // 200
		f06292, // 300
		ec407a, // 400
		e91e63, // 500
		d81b60, // 600
		c2185b, // 700
		ad1457, // 800
		880e4f, // 900
		ff80ab, // a100
		ff4081, // a200
		f50057, // a400
		c51162  // a700
		);

MD_COLOR_SPEC_DEFINE(Purple, 2,
		f3e5f5,
		e1bee7,
		ce93d8,
		ba68c8,
		ab47bc,
		9c27b0,
		8e24aa,
		7b1fa2,
		6a1b9a,
		4a148c,
		ea80fc,
		e040fb,
		d500f9,
		aa00ff
		);

MD_COLOR_SPEC_DEFINE(DeepPurple, 3,
		ede7f6,
		d1c4e9,
		b39ddb,
		9575cd,
		7e57c2,
		673ab7,
		5e35b1,
		512da8,
		4527a0,
		311b92,
		b388ff,
		7c4dff,
		651fff,
		6200ea
		);

MD_COLOR_SPEC_DEFINE(Indigo, 4,
		e8eaf6,
		c5cae9,
		9fa8da,
		7986cb,
		5c6bc0,
		3f51b5,
		3949ab,
		303f9f,
		283593,
		1a237e,
		8c9eff,
		536dfe,
		3d5afe,
		304ffe
		);

MD_COLOR_SPEC_DEFINE(Blue, 5,
		e3f2fd,
		bbdefb,
		90caf9,
		64b5f6,
		42a5f5,
		2196f3,
		1e88e5,
		1976d2,
		1565c0,
		0d47a1,
		82b1ff,
		448aff,
		2979ff,
		2962ff
		);

MD_COLOR_SPEC_DEFINE(LightBlue, 6,
		e1f5fe,
		b3e5fc,
		81d4fa,
		4fc3f7,
		29b6f6,
		03a9f4,
		039be5,
		0288d1,
		0277bd,
		01579b,
		80d8ff,
		40c4ff,
		00b0ff,
		0091ea
		);



MD_COLOR_SPEC_DEFINE(Cyan, 7,
		e0f7fa,
		b2ebf2,
		80deea,
		4dd0e1,
		26c6da,
		00bcd4,
		00acc1,
		0097a7,
		00838f,
		006064,
		84ffff,
		18ffff,
		00e5ff,
		00b8d4
		);


MD_COLOR_SPEC_DEFINE(Teal, 8,
		e0f2f1,
		b2dfdb,
		80cbc4,
		4db6ac,
		26a69a,
		009688,
		00897b,
		00796b,
		00695c,
		004d40,
		a7ffeb,
		64ffda,
		1de9b6,
		00bfa5
		);


MD_COLOR_SPEC_DEFINE(Green, 9,
		e8f5e9,
		c8e6c9,
		a5d6a7,
		81c784,
		66bb6a,
		4caf50,
		43a047,
		388e3c,
		2e7d32,
		1b5e20,
		b9f6ca,
		69f0ae,
		00e676,
		00c853
		);


MD_COLOR_SPEC_DEFINE(LightGreen, 10,
		f1f8e9,
		dcedc8,
		c5e1a5,
		aed581,
		9ccc65,
		8bc34a,
		7cb342,
		689f38,
		558b2f,
		33691e,
		ccff90,
		b2ff59,
		76ff03,
		64dd17
		);


MD_COLOR_SPEC_DEFINE(Lime, 11,
		f9fbe7,
		f0f4c3,
		e6ee9c,
		dce775,
		d4e157,
		cddc39,
		c0ca33,
		afb42b,
		9e9d24,
		827717,
		f4ff81,
		eeff41,
		c6ff00,
		aeea00
		);


MD_COLOR_SPEC_DEFINE(Yellow, 12,
		fffde7,
		fff9c4,
		fff59d,
		fff176,
		ffee58,
		ffeb3b,
		fdd835,
		fbc02d,
		f9a825,
		f57f17,
		ffff8d,
		ffff00,
		ffea00,
		ffd600
		);


MD_COLOR_SPEC_DEFINE(Amber, 13,
		fff8e1,
		ffecb3,
		ffe082,
		ffd54f,
		ffca28,
		ffc107,
		ffb300,
		ffa000,
		ff8f00,
		ff6f00,
		ffe57f,
		ffd740,
		ffc400,
		ffab00
		);


MD_COLOR_SPEC_DEFINE(Orange, 14,
		fff3e0,
		ffe0b2,
		ffcc80,
		ffb74d,
		ffa726,
		ff9800,
		fb8c00,
		f57c00,
		ef6c00,
		e65100,
		ffd180,
		ffab40,
		ff9100,
		ff6d00
		);


MD_COLOR_SPEC_DEFINE(DeepOrange, 15,
		fbe9e7,
		ffccbc,
		ffab91,
		ff8a65,
		ff7043,
		ff5722,
		f4511e,
		e64a19,
		d84315,
		bf360c,
		ff9e80,
		ff6e40,
		ff3d00,
		dd2c00
		);


MD_COLOR_SPEC_BASE_DEFINE(Brown, 16,
		efebe9,
		d7ccc8,
		bcaaa4,
		a1887f,
		8d6e63,
		795548,
		6d4c41,
		5d4037,
		4e342e,
		3e2723
		);


MD_COLOR_SPEC_BASE_DEFINE(Grey, 17,
		fafafa,
		f5f5f5,
		eeeeee,
		e0e0e0,
		bdbdbd,
		9e9e9e,
		757575,
		616161,
		424242,
		212121
		);


MD_COLOR_SPEC_BASE_DEFINE(BlueGrey, 18,
		eceff1,
		cfd8dc,
		b0bec5,
		90a4ae,
		78909c,
		607d8b,
		546e7a,
		455a64,
		37474f,
		263238
		);

		colors["White"] = 0xFFFFFF;
		colors["Black"] = 0x000000;
		init = true;
	}

	bool getColor(const String &str, Color3B &color) {
		if (!init) {
			initColors();
		}

		auto it = colors.find(str);
		if (it != colors.end()) {
			color.r = (uint8_t) ( ( it->second >> 16 ) & 0xFF );
			color.g = (uint8_t) ( ( it->second >> 8 ) & 0xFF );
			color.b = (uint8_t) ( ( it->second >> 0 ) & 0xFF );
			return true;
		}
		return false;
	}

	bool getColor(const String &str, uint32_t &color) {
		if (!init) {
			initColors();
		}

		auto it = colors.find(str);
		if (it != colors.end()) {
			color = it->second;
			return true;
		}
		return false;
	}

	String getName(uint32_t index) {
		if (!init) {
			initColors();
		}

		for (auto &it : colors) {
			if (index == it.second) {
				return it.first;
			}
		}
		return "";
	}

	String getName(const Color3B &color) {
		return getName((uint32_t)( ((color.r << 16) & 0xFF0000) | ((color.g << 8) & 0xFF00) | (color.b & 0xFF) ));
	}
}

namespace style {

void setHslColor(cocos2d::Color3B &color, float h, float sl, float l) {
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

bool readColorDigits(const String &origStr, float *b, int num, bool isRgb) {
	size_t idx = 0;
	while(isspace(origStr[idx]) && idx < origStr.length()) {
		idx++;
	}
	if (origStr[idx] != '(' || idx >= origStr.length()) {
		return false;
	}
	String str = origStr.substr(idx + 1);
	idx = 0;
	for (int i = 0; i < num; i++) {
		b[i] = stappler::stof(str, &idx);
		if (idx == 0 || idx >= str.length()) {
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

		while(isspace(str[idx]) && idx < str.length()) {
			idx++;
		}

		if (idx >= origStr.length()) {
			return false;
		}

		if (str[idx] == '%') {
			if (b[i] > 100.0f) {
				b[i] = 100.0f; // for percent values - max is 100
			}
			if (isRgb) {
				b[i] = 255.0f * b[i] / 100.0f; // translate RGB values to (0-255)
			} else if ((!isRgb && i == 0) || i == 3) {
				return false; // for H in HSL and any alpha percent values is denied
			}
			idx++;
			while(isspace(str[idx]) && idx < origStr.length()) {
				idx++;
			}
		} else if (!isRgb && (i == 1 || i == 2) && str[idx] != '%') {
			return false; // for S and L in HSL only percent values allowed
		}

		if (idx >= origStr.length()) {
			return false;
		}

		if (str[idx] == ',') {
			str = str.substr(idx + 1);
		} else if (str[idx] == ')' && i == num - 1) {
			return true;
		} else {
			return false; // invalid digits divider, must be ','
		}
	}

	return true;
}

bool readRgbaColor(const String &origStr, Color3B &color, uint8_t &opacity) {
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

bool readRgbColor(const String &origStr, Color3B &color) {
	float b[3] = { 0.0f };
	if (readColorDigits(origStr, b, 3, true)) {
		color.r = (uint8_t)b[0];
		color.g = (uint8_t)b[1];
		color.b = (uint8_t)b[2];
		return true;
	}
	return false;
}

bool readHslaColor(const String &origStr, Color3B &color, uint8_t &opacity) {
	float b[4] = { 0.0f };
	if (readColorDigits(origStr, b, 4, false)) {
		setHslColor(color, b[0], b[1], b[2]);
		opacity = (uint8_t)b[3];
		return true;
	}
	return false;
}

bool readHslColor(const String &origStr, Color3B &color) {
	float b[3] = { 0.0f };
	if (readColorDigits(origStr, b, 3, false)) {
		setHslColor(color, b[0], b[1], b[2]);
		return true;
	}
	return false;
}

bool readHashColor(const String &origStr, Color3B &color) {
	uint8_t b[6] = { 0 };
	auto str = origStr.substr(1);
	if (str.length() == 6) {
		for (int i = 0; i < 6; i++) {
			auto c = str[i];
			if (c >= '0' && c <= '9') {
				b[i] = c - '0';
			} else if (c >='a' && c <= 'z') {
				b[i] = c - 'a' + 10;
			} else if (c >='A' && c <= 'Z') {
				b[i] = c - 'A' + 10;
			} else {
				return false;
			}
		}
	} else if (str.length() == 3) {
		for (int i = 0; i < 3; i++) {
			auto c = str[i];
			if (c >= '0' && c <= '9') {
				b[i * 2] = c - '0';
				b[i * 2 + 1] = c - '0';
			} else if (c >='a' && c <= 'z') {
				b[i * 2] = c - 'a' + 10;
				b[i * 2 + 1] = c - 'a' + 10;
			} else if (c >='A' && c <= 'Z') {
				b[i * 2] = c - 'A' + 10;
				b[i * 2 + 1] = c - 'A' + 10;
			} else {
				return false;
			}
		}
	} else {
		return false;
	}

	color.r = b[0] << 4 | b[1];
	color.g = b[2] << 4 | b[3];
	color.b = b[4] << 4 | b[5];
	return true;
}

bool readNamedColor(const String &origStr, Color3B &color) {
	if (origStr == "white") {
		color = cocos2d::Color3B::WHITE;
	} else if (origStr == "silver") {
		color = cocos2d::Color3B(192,192,192);
	} else if (origStr == "gray") {
		color = cocos2d::Color3B(128,128,128);
	} else if (origStr == "black") {
		color = cocos2d::Color3B::BLACK;
	} else if (origStr == "maroon") {
		color = cocos2d::Color3B(128,0,0);
	} else if (origStr == "red") {
		color = cocos2d::Color3B(255,0,0);
	} else if (origStr == "orange") {
		color = cocos2d::Color3B(255,165,0);
	} else if (origStr == "yellow") {
		color = cocos2d::Color3B(255,255,0);
	} else if (origStr == "olive") {
		color = cocos2d::Color3B(128,128,0);
	} else if (origStr == "lime") {
		color = cocos2d::Color3B(0,255,0);
	} else if (origStr == "green") {
		color = cocos2d::Color3B(0,128,0);
	} else if (origStr == "aqua") {
		color = cocos2d::Color3B(0,255,255);
	} else if (origStr == "blue") {
		color = cocos2d::Color3B(0,0,255);
	} else if (origStr == "navy") {
		color = cocos2d::Color3B(0,0,128);
	} else if (origStr == "teal") {
		color = cocos2d::Color3B(0,128,128);
	} else if (origStr == "fuchsia") {
		color = cocos2d::Color3B(255,0,255);
	} else if (origStr == "purple") {
		color = cocos2d::Color3B(128,0,128);
	} else if (!mdcolors::getColor(origStr, color)) {
		return false;
	}
	return true;
}

bool readColor(const String &str, Color4B &color4) {
	Color3B color;
	uint8_t opacity = 0;
	if (str.compare(0, 4, "rgba") == 0) {
		if (readRgbaColor(str.substr(4), color, opacity)) {
			color4 = cocos2d::Color4B(color, opacity);
			return true;
		}
	} else if (str.compare(0, 4, "hsla") == 0) {
		if (readHslaColor(str.substr(4), color, opacity)) {
			color4 = cocos2d::Color4B(color, opacity);
			return true;
		}
	} else if (str.compare(0, 3, "rgb") == 0) {
		if (readRgbColor(str.substr(3), color)) {
			color4 = cocos2d::Color4B(color);
			return true;
		}
	} else if (str.compare(0, 3, "hsl") == 0) {
		if (readHslColor(str.substr(3), color)) {
			color4 = cocos2d::Color4B(color);
			return true;
		}
	} else if (str.front() == '#') {
		if (readHashColor(str, color)) {
			color4 = cocos2d::Color4B(color);
			return true;
		}
	} else {
		if (readNamedColor(str, color)) {
			color4 = cocos2d::Color4B(color);
			return true;
		}
	}
	return false;
}

}

NS_SP_EXT_END(rich_text)
