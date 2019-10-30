/**
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2011      Zynga Inc.
Copyright (c) 2013-2015 Chukong Technologies Inc.
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef LAYOUT_TYPES_SLCOLOR_H_
#define LAYOUT_TYPES_SLCOLOR_H_

#include "SPLayout.h"

NS_LAYOUT_BEGIN

struct Color3B;
struct Color4B;
struct Color4F;

/*
 * Based on cocos2d-x sources
 * stappler-cocos2d-x fork use this sources as a replacement of original
 */

/**
 * RGB color composed of bytes 3 bytes.
 */
struct Color3B : public AllocBase {
	static Color3B getColorByName(const String &, const Color3B & = Color3B::BLACK);
	static Color3B getColorByName(const StringView &, const Color3B & = Color3B::BLACK);

	Color3B();
	Color3B(uint8_t _r, uint8_t _g, uint8_t _b);
	explicit Color3B(const Color4B& color);
	explicit Color3B(const Color4F& color);

	bool operator==(const Color3B& right) const;
	bool operator==(const Color4B& right) const;
	bool operator==(const Color4F& right) const;
	bool operator!=(const Color3B& right) const;
	bool operator!=(const Color4B& right) const;
	bool operator!=(const Color4F& right) const;

	bool equals(const Color3B& other) {
		return (*this == other);
	}

	uint8_t r;
	uint8_t g;
	uint8_t b;

	static const Color3B WHITE;
	static const Color3B BLACK;

	static Color3B progress(const Color3B &a, const Color3B &b, float p);
};

/**
 * RGBA color composed of 4 bytes.
 */
struct Color4B : public AllocBase {
	static Color4B getColorByName(const String &, const Color4B & = Color4B::BLACK);
	static Color4B getColorByName(const StringView &, const Color4B & = Color4B::BLACK);

	Color4B();
	Color4B(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a);
	Color4B(const Color3B& color, uint8_t _a);
	explicit Color4B(const Color3B& color);
	explicit Color4B(const Color4F& color);

	bool operator==(const Color4B& right) const;
	bool operator==(const Color3B& right) const;
	bool operator==(const Color4F& right) const;
	bool operator!=(const Color4B& right) const;
	bool operator!=(const Color3B& right) const;
	bool operator!=(const Color4F& right) const;

	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;

	static Color4B white(uint8_t);
	static Color4B black(uint8_t);

	static const Color4B WHITE;
	static const Color4B BLACK;

	static Color4B progress(const Color4B &a, const Color4B &b, float p);
};

/**
 * RGBA color composed of 4 floats.
 */
struct Color4F : public AllocBase {
	Color4F();
	Color4F(float _r, float _g, float _b, float _a);
	explicit Color4F(const Color3B& color);
	explicit Color4F(const Color4B& color);

	bool operator==(const Color4F& right) const;
	bool operator==(const Color3B& right) const;
	bool operator==(const Color4B& right) const;
	bool operator!=(const Color4F& right) const;
	bool operator!=(const Color3B& right) const;
	bool operator!=(const Color4B& right) const;

	bool equals(const Color4F &other) {
		return (*this == other);
	}

	float r;
	float g;
	float b;
	float a;

	static const Color4F WHITE;
	static const Color4F BLACK;

	static Color4F progress(const Color4F &a, const Color4F &b, float p);
};

class Color : public AllocBase {
public:
	LAYOUT_COLOR_SPEC(Red); // 0
	LAYOUT_COLOR_SPEC(Pink); // 1
	LAYOUT_COLOR_SPEC(Purple); // 2
	LAYOUT_COLOR_SPEC(DeepPurple); // 3
	LAYOUT_COLOR_SPEC(Indigo); // 4
	LAYOUT_COLOR_SPEC(Blue); // 5
	LAYOUT_COLOR_SPEC(LightBlue); // 6
	LAYOUT_COLOR_SPEC(Cyan); // 7
	LAYOUT_COLOR_SPEC(Teal); // 8
	LAYOUT_COLOR_SPEC(Green); // 9
	LAYOUT_COLOR_SPEC(LightGreen); // 10
	LAYOUT_COLOR_SPEC(Lime); // 11
	LAYOUT_COLOR_SPEC(Yellow); // 12
	LAYOUT_COLOR_SPEC(Amber); // 13
	LAYOUT_COLOR_SPEC(Orange); // 14
	LAYOUT_COLOR_SPEC(DeepOrange); // 15

	LAYOUT_COLOR_SPEC_BASE(Brown); // 16
	LAYOUT_COLOR_SPEC_BASE(Grey); // 17
	LAYOUT_COLOR_SPEC_BASE(BlueGrey); // 18

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

	inline Color3B asColor3B() const {
		return Color3B((_value >> 16) & 0xFF, (_value >> 8) & 0xFF, _value & 0xFF);
	}

	inline Color4B asColor4B() const {
		return Color4B((_value >> 16) & 0xFF, (_value >> 8) & 0xFF, _value & 0xFF, 255);
	}

	inline Color4F asColor4F() const {
		return Color4F(
				((float)0xFF) / ((_value >> 16) & 0xFF),
				((float)0xFF) / ((_value >> 8) & 0xFF),
				((float)0xFF) / (_value & 0xFF),
				1.0f);
	}

	inline operator Color3B () const {
		return asColor3B();
	}
	inline operator Color4B () const {
		return asColor4B();
	}
	inline operator Color4F () const {
		return asColor4F();
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

	inline uint32_t value() const { return _value; }
	inline uint32_t index() const { return _index; }



	Color text() const;

	inline Level level() const { return (_index == maxOf<uint16_t>())?Level::Unknown:((Level)(_index & 0x0F)); }
	inline Tone tone() const { return (_index == maxOf<uint16_t>())?Tone::Unknown:((Tone)((_index & 0xFFF0) / 16)); }

	Color previous() const;
	Color next() const;

	Color lighter(uint8_t index = 1) const;
	Color darker(uint8_t index = 1) const;

	Color medium() const;
	Color specific(uint8_t index) const;
	Color specific(Level l) const;

	Color(Tone, Level);
	Color(uint32_t value);
	Color(uint32_t value, int16_t index);
	Color(const Color3B &color);
	Color(const Color4B &color);

	Color() = default;

	Color(const Color &) = default;
	Color(Color &&) = default;
	Color & operator=(const Color &) = default;
	Color & operator=(Color &&) = default;

	String name() const;

	static Color getColorByName(const StringView &, const Color & = Color::Black);
	static Color progress(const Color &a, const Color &b, float p);

private:
	static Color getById(uint16_t index);
	static uint16_t getColorIndex(uint32_t);

	uint32_t _value = 0;
	uint16_t _index = uint16_t(19 * 16 + 1);
};

NS_LAYOUT_END

NS_SP_BEGIN

template <> inline
layout::Color progress<layout::Color>(const layout::Color &a, const layout::Color &b, float p) {
	return layout::Color::progress(a, b, p);
}

template <> inline
layout::Color3B progress<layout::Color3B>(const layout::Color3B &a, const layout::Color3B &b, float p) {
	return layout::Color3B::progress(a, b, p);
}

template <> inline
layout::Color4B progress<layout::Color4B>(const layout::Color4B &a, const layout::Color4B &b, float p) {
	return layout::Color4B::progress(a, b, p);
}

template <> inline
layout::Color4F progress<layout::Color4F>(const layout::Color4F &a, const layout::Color4F &b, float p) {
	return layout::Color4F::progress(a, b, p);
}

NS_SP_END

#endif /* LAYOUT_TYPES_SLCOLOR_H_ */
