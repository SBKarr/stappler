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

#include "Material.h"
#include "MaterialColors.h"

NS_SP_EXT_BEGIN(rich_text)

namespace mdcolors {
	bool getColor(const std::string &str, uint32_t &color);
	std::string getName(uint32_t);
}

NS_SP_EXT_END(rich_text)

#define MD_COLOR_SPEC_COMPONENT(Name, Id, Value, Index, Group) \
	Color Color::Name ## _ ## Id(0x ## Value, Group * 16 + Index);

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

NS_MD_BEGIN

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


Color Color::White(0xFFFFFF, (int16_t)19 * 16 + 0);
Color Color::Black(0x000000, (int16_t)19 * 16 + 1);


struct _ColorHsl {
	float h;
	float s;
	float l;
};

static _ColorHsl rgb_to_hsl(uint32_t color) {
	_ColorHsl ret;
    float
        r = float((color >> (2 * 8)) & maxOf<uint8_t>()) / maxOf<uint8_t>(),
        g = float((color >> (1 * 8)) & maxOf<uint8_t>()) / maxOf<uint8_t>(),
        b = float((color >> (0 * 8)) & maxOf<uint8_t>()) / maxOf<uint8_t>();

    float
        maxv = std::max(std::max(r, g), b),
        minv = std::min(std::min(r, g), b),
		d = maxv - minv;

    ret.l = (maxv + minv) / 2;

    if (maxv != minv) {
    	ret.s = ret.l > 0.5f ? d / (2 - maxv - minv) : d / (maxv + minv);
        if (maxv == r) {
        	ret.h = (g - b) / d + (g < b ? 6 : 0);
        } else if (maxv == g) {
        	ret.h = (b - r) / d + 2;
        } else if (maxv == b) {
        	ret.h = (r - g) / d + 4;
        }
        ret.h /= 6;
    }
    return ret;
}

static float hue_to_rgb(float v1, float v2, float vH) {
	if (vH < 0)
		vH += 1;

	if (vH > 1)
		vH -= 1;

	if ((6 * vH) < 1)
		return (v1 + (v2 - v1) * 6 * vH);

	if ((2 * vH) < 1)
		return v2;

	if ((3 * vH) < 2)
		return (v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);

	return v1;
}

static uint32_t hsl_to_rgb(const _ColorHsl &color, uint32_t source) {
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;

	if (color.s == 0.0f) {
		r = g = b = (uint8_t)(color.l * 255);
	} else {
		float v1, v2;
		const float hue = color.h;

		v2 = (color.l < 0.5) ? (color.l * (1 + color.s)) : ((color.l + color.s) - (color.l * color.s));
		v1 = 2 * color.l - v2;

		r = (uint8_t)(255 * hue_to_rgb(v1, v2, hue + (1.0f / 3)));
		g = (uint8_t)(255 * hue_to_rgb(v1, v2, hue));
		b = (uint8_t)(255 * hue_to_rgb(v1, v2, hue - (1.0f / 3)));
	}

    return (uint32_t(r) << (2 * 8)) |
           (uint32_t(g) << (1 * 8)) |
           (uint32_t(b) << (0 * 8)) |
           (uint32_t((source >> (3 * 8)) & maxOf<uint8_t>()) << (3 * 8));
}

static uint32_t make_lighter(uint32_t color, uint8_t index) {
    _ColorHsl hsl = rgb_to_hsl(color);

    const float tmp = (1.0f - hsl.l) * 11.0f;
    if (tmp < 0.5f || tmp >= 10.5f) {
    	return color;
    }
    uint8_t id = uint8_t(roundf(tmp)) - 1;
	if (id < index) {
		id = 0;
	} else {
		id = (id + 10 - index) % 10 + 1;
	}
	hsl.l = 1.0f - (id + 1) / 11.0f;

    return hsl_to_rgb(hsl, color);
}
static uint32_t make_darker(uint32_t color, uint8_t index) {
    _ColorHsl hsl = rgb_to_hsl(color);

    const float tmp = (1.0f - hsl.l) * 11.0f;
    if (tmp < 0.5f || tmp >= 10.5f) {
    	return color;
    }

    uint8_t id = uint8_t(roundf(tmp)) - 1;
	if (id + index > 9) {
		id = 9;
	} else {
		id = (id + 10 + index) % 10;
	}
	hsl.l = 1.0f - (id + 1) / 11.0f;

    return hsl_to_rgb(hsl, color);
}

static uint32_t make_specific(uint32_t color, uint8_t index) {
    _ColorHsl hsl = rgb_to_hsl(color);

    if (index == 10) {
    	index = 1;
    } else if (index == 11) {
    	index = 2;
    } else if (index == 12) {
    	index = 4;
    } else if (index == 13) {
    	index = 7;
    } else {
    	index = 5;
    }

	hsl.l = 1.0f - (index + 1) / 11.0f;

    return hsl_to_rgb(hsl, color);
}

std::map<int16_t, Color> *s_colors = nullptr;

Color Color::getById(int16_t index) {
	auto it = s_colors->find(index);
	if (it != s_colors->end()) {
		return it->second;
	} else {
		return Color(0);
	}
}

int16_t Color::getColorIndex(uint32_t value) {
	for (auto &it : (*s_colors)) {
		if (value == it.second._value) {
			return it.first;
		}
	}
	return -1;
}

Color::Color(uint32_t value, int16_t index) {
	_value = value;
	_index = index;

	if (_index != -1) {
		if (!s_colors) {
			s_colors = new std::map<int16_t, Color>();
		}
		s_colors->insert(std::make_pair(_index, *this));
	}
}

Color Color::text() const {
	float r = ((_value >> 16) & 0xFF) / 255.0f;
	float g = ((_value >> 8) & 0xFF) / 255.0f;
	float b = (_value & 0xFF) / 255.0f;

	float l = 0.2989f * r + 0.5870f * g + 0.1140f * b;

	if (l <= 0.55f) {
		return Color::White;
	} else {
		return Color::Black;
	}
}

Color::Color(uint32_t value) : Color(value, getColorIndex(value)) { }

Color::Color(const cocos2d::Color3B &color)
: Color((uint32_t)(((color.r << 16) & 0xFF0000) | ((color.g << 8) & 0xFF00) | (color.b & 0xFF))) { }

Color::Color(const cocos2d::Color4B &color)
: Color((uint32_t)(((color.r << 16) & 0xFF0000) | ((color.g << 8) & 0xFF00) | (color.b & 0xFF))) { }

Color::Color(Tone tone, Level level) {
	*this = getById((int16_t)tone * 16 | (int16_t)level);
}

Color Color::previous() {
	return lighter(1);
}
Color Color::next() {
	return darker(1);
}

Color Color::lighter(uint8_t index) {
	if (_index == -1) {
		return Color(make_lighter(_value, index));
	}

	if (index > 0 && _index == Color::Black._index) {
		_index = Color::Grey_900._index;
		index --;
	}

	uint16_t color = _index & 0xFFF0;
	uint16_t id = _index & 0x0F;
	if (id >= 0 && id <= 9) {
        if (id < index) {
            return getById(color | 0);
        }
		id = (id + 10 - index) % 10;
		return getById(color | id);
    } else if (id >= 10 && id <= 13) {
        if (id - 10 < index) {
            return getById(color | 10);
        }
		id = (id - index);
		return getById(color | id);
	} else {
		return Color(0);
	}
}
Color Color::darker(uint8_t index) {
	if (_index == -1) {
		return Color(make_darker(_value, index));
	}

	if (index > 0 && _index == Color::White._index) {
		_index = Color::Grey_50._index;
		index --;
	}

	uint16_t color = _index &0xFFF0;
	uint16_t id = _index & 0x0F;
	if (id >= 0 && id <= 9) {
        if (id + index >= 9) {
            return getById(color | 9);
        }
		id = (id + index) % 10;
		return getById(color | id);
    } else if (id >= 10 && id <= 13) {
        if (id + index >= 13) {
            return getById(color | 13);
        }
		id = (id + index);
		return getById(color | id);
	} else {
		return Color(0);
	}
}

Color Color::medium() {
	if (_index == -1) {
		return make_specific(_value, 5);
	}
	uint16_t color = _index &0xFFF0;
	return getById(color | 5);
}
Color Color::specific(uint8_t index) {
	if (_index == -1) {
		return make_specific(_value, index);
	}
	uint16_t color = _index &0xFFF0;
	return getById(color | index);
}

Color Color::specific(Level tone) {
	return specific((uint8_t)tone);
}

std::string Color::name() const {
	auto ret = rich_text::mdcolors::getName(_value);
	if (ret.empty()) {
		ret = toString("rgb(", uint32_t(_value >> 16 & 0xFF), ", ", uint32_t(_value >> 8 & 0xFF), ", ", uint32_t(_value & 0xFF));
	}
	return ret;
}

static uint32_t readHashColor(const std::string &origStr) {
	uint8_t b[6] = { 0 };
	uint32_t color = 0;

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
				return color;
			}
		}
	} else if (str.length() == 3) {
		for (int i = 0; i < 3; i++) {
			auto c = str[i];
			if (c >= '0' && c <= '9') {
				b[i * 2] = c - '0';
				b[i * 2 + 1] = c - '0';
			} else if (c >='a' && c <= 'z') {
				b[i * 2] = c - 'a';
				b[i * 2 + 1] = c - 'a';
			} else if (c >='A' && c <= 'Z') {
				b[i * 2] = c - 'A';
				b[i * 2 + 1] = c - 'A';
			} else {
				return color;
			}
		}
	} else {
		return color;
	}

	color |= (b[0] << 4 | b[1]) << 16;
	color |= (b[2] << 4 | b[3]) << 8;
	color |= (b[4] << 4 | b[5]);

	return color;
}

Color Color::getColorByName(const std::string &str, const Color &def) {
	uint32_t value;
	if (!str.empty()) {
		if (str.front() == '#') {
			return Color(readHashColor(str));
		} else {
			if (rich_text::mdcolors::getColor(str, value)) {
				return Color(value);
			}
		}
	}
	return def;
}

Color Color::progress(const Color &a, const Color &b, float fp) {
	uint8_t p = (uint8_t) (fp * 255.0f);
	uint32_t val = (
			( ( (a._value >> 16 & 0xFF) * (255 - p) + (b._value >> 16 & 0xFF) * p ) / 255 << 16 & 0xFF0000 ) |
			( ( (a._value >> 8 & 0xFF) * (255 - p) + (b._value >> 8 & 0xFF) * p ) / 255 << 8 & 0xFF00) |
			( ( (a._value & 0xFF) * (255 - p) + (b._value & 0xFF) * p ) / 255 & 0xFF)
	);
	// log::format("Color", "%06X %06X %f -> %06X", a._value, b._value, fp, val);
	return Color(val);
}

NS_MD_END
