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

#ifndef CATALOGS_CLASSES_MATERIAL_MATERIALCOLORS_H_
#define CATALOGS_CLASSES_MATERIAL_MATERIALCOLORS_H_

#include "Material.h"

NS_MD_BEGIN

class Color {
public:
	MD_COLOR_SPEC(Red); // 0
	MD_COLOR_SPEC(Pink); // 1
	MD_COLOR_SPEC(Purple); // 2
	MD_COLOR_SPEC(DeepPurple); // 3
	MD_COLOR_SPEC(Indigo); // 4
	MD_COLOR_SPEC(Blue); // 5
	MD_COLOR_SPEC(LightBlue); // 6
	MD_COLOR_SPEC(Cyan); // 7
	MD_COLOR_SPEC(Teal); // 8
	MD_COLOR_SPEC(Green); // 9
	MD_COLOR_SPEC(LightGreen); // 10
	MD_COLOR_SPEC(Lime); // 11
	MD_COLOR_SPEC(Yellow); // 12
	MD_COLOR_SPEC(Amber); // 13
	MD_COLOR_SPEC(Orange); // 14
	MD_COLOR_SPEC(DeepOrange); // 15

	MD_COLOR_SPEC_BASE(Brown); // 16
	MD_COLOR_SPEC_BASE(Grey); // 17
	MD_COLOR_SPEC_BASE(BlueGrey); // 18

	static Color White;
	static Color Black;

	enum class Level : int16_t {
		Unknown = -1,
		b50 = 0,
		b100,
		b200,
		b300,
		b400,
		b500,
		b600,
		b700,
		b800,
		b900,
		a100,
		a200,
		a400,
		a700
	};

	enum class Tone : int16_t {
		Unknown = -1,
		Red = 0,
		Pink,
		Purple,
		DeepPurple,
		Indigo,
		Blue,
		LightBlue,
		Cyan,
		Teal,
		Green,
		LightGreen,
		Lime,
		Yellow,
		Amber,
		Orange,
		DeepOrange,
		Brown,
		Grey,
		BlueGrey,
		BlackWhite,
	};

	inline operator cocos2d::Color3B () const {
		return cocos2d::Color3B((_value >> 16) & 0xFF, (_value >> 8) & 0xFF, _value & 0xFF);
	}
	inline operator cocos2d::Color4B () const {
		return cocos2d::Color4B((_value >> 16) & 0xFF, (_value >> 8) & 0xFF, _value & 0xFF, 255);
	}
	inline operator cocos2d::Color4F () const {
		return cocos2d::Color4F(
				((float)0xFF) / ((_value >> 16) & 0xFF),
				((float)0xFF) / ((_value >> 8) & 0xFF),
				((float)0xFF) / (_value & 0xFF),
				1.0f);
	}

	inline bool operator == (const Color &other) const {
		return _value == other._value;
	}

	inline bool operator != (const Color &other) const {
		return _value != other._value;
	}

	inline uint8_t r() const { return (_value >> 16) & 0xFF; }
	inline uint8_t g() const { return (_value >> 8) & 0xFF; }
	inline uint8_t b() const { return _value  & 0xFF; }

	Color text() const;

	inline Level level() const { return (_index == -1)?Level::Unknown:((Level)(_index & 0x0F)); }
	inline Tone tone() const { return (_index == -1)?Tone::Unknown:((Tone)((_index & 0xFFF0) / 16)); }

	Color previous() const;
	Color next() const;

	Color lighter(uint8_t index = 1) const;
	Color darker(uint8_t index = 1) const;

	Color medium() const;
	Color specific(uint8_t index) const;
	Color specific(Level l) const;

	Color(Tone, Level);
	Color(uint32_t value);
	Color(const cocos2d::Color3B &color);
	Color(const cocos2d::Color4B &color);

	Color() = default;

	Color(const Color &) = default;
	Color(Color &&) = default;
	Color & operator=(const Color &) = default;
	Color & operator=(Color &&) = default;

	std::string name() const;

	static Color getColorByName(const std::string &, const Color & = Color::Black);

	static Color progress(const material::Color &a, const material::Color &b, float p);

private:
	static Color getById(int16_t index);
	static int16_t getColorIndex(uint32_t);

	Color(uint32_t value, int16_t index);

	uint32_t _value = 0;
	int16_t _index = -1;
};

NS_MD_END

NS_SP_BEGIN

template <> inline
material::Color progress<material::Color>(const material::Color &a, const material::Color &b, float p) {
	return material::Color::progress(a, b, p);
}

NS_SP_END

#endif /* CATALOGS_CLASSES_MATERIAL_MATERIALCOLORS_H_ */
