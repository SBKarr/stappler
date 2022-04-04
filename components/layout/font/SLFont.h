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

#ifndef LAYOUT_FONT_SLFONT_H_
#define LAYOUT_FONT_SLFONT_H_

#include "SLStyle.h"
#include "SLNode.h"
#include "SPCharGroup.h"

NS_LAYOUT_BEGIN

using FontSize = style::FontSize;
using FontFace = style::FontFace;
using FontParameters = style::FontStyleParameters;

using FontStyle = style::FontStyle;
using FontWeight = style::FontWeight;
using FontStretch = style::FontStretch;
using ReceiptCallback = Function<Bytes(const FontSource *, const String &)>;

using FontLayoutId = ValueWrapper<uint16_t, class FontLayoutIdTag>;

struct Metrics final {
	uint16_t size = 0; // font size in pixels
	uint16_t height = 0; // default font line height
	int16_t ascender = 0; // The distance from the baseline to the highest coordinate used to place an outline point
	int16_t descender = 0; // The distance from the baseline to the lowest grid coordinate used to place an outline point
	int16_t underlinePosition = 0;
	int16_t underlineThickness = 0;
};

struct CharLayout final {
	char16_t charID = 0;
	int16_t xOffset = 0;
	int16_t yOffset = 0;
	uint16_t xAdvance = 0;

	operator char16_t() const { return charID; }
};

struct CharSpec final {
	char16_t charID = 0;
	int16_t pos = 0;
	uint16_t advance = 0;
	uint16_t face = 0;
};

struct CharTexture final {
	char16_t charID = 0;
	uint16_t x = 0;
	uint16_t y = 0;
	uint16_t width = 0;
	uint16_t height = 0;
	uint8_t texture = maxOf<uint8_t>();

	operator char16_t() const { return charID; }
};

struct FontCharString final {
	void addChar(char16_t);
	void addString(const String &);
	void addString(const WideString &);
	void addString(const char16_t *, size_t);

	Vector<char16_t> chars;
};

class FormatterSourceInterface : public Ref {
public:
	virtual ~FormatterSourceInterface() { }

	virtual FontLayoutId getLayout(const FontParameters &f, float scale) = 0;
	virtual void addString(FontLayoutId, const FontCharString &) = 0;
	virtual uint16_t getFontHeight(FontLayoutId) = 0;
	virtual int16_t getKerningAmount(FontLayoutId, char16_t first, char16_t second, uint16_t face) const = 0;
	virtual Metrics getMetrics(FontLayoutId) = 0;
	virtual CharLayout getChar(FontLayoutId, char16_t, uint16_t &face) = 0;
	virtual StringView getFontName(FontLayoutId) = 0;
};

struct EmplaceCharInterface {
	uint16_t (*getX) (void *) = nullptr;
	uint16_t (*getY) (void *) = nullptr;
	uint16_t (*getWidth) (void *) = nullptr;
	uint16_t (*getHeight) (void *) = nullptr;
	void (*setX) (void *, uint16_t) = nullptr;
	void (*setY) (void *, uint16_t) = nullptr;
	void (*setTex) (void *, uint16_t) = nullptr;
};

Extent2 emplaceChars(const EmplaceCharInterface &, const SpanView<void *> &,
		float totalSquare = std::numeric_limits<float>::quiet_NaN());

inline bool operator< (const CharTexture &t, const CharTexture &c) { return t.charID < c.charID; }
inline bool operator> (const CharTexture &t, const CharTexture &c) { return t.charID > c.charID; }
inline bool operator<= (const CharTexture &t, const CharTexture &c) { return t.charID <= c.charID; }
inline bool operator>= (const CharTexture &t, const CharTexture &c) { return t.charID >= c.charID; }

inline bool operator< (const CharTexture &t, const char16_t &c) { return t.charID < c; }
inline bool operator> (const CharTexture &t, const char16_t &c) { return t.charID > c; }
inline bool operator<= (const CharTexture &t, const char16_t &c) { return t.charID <= c; }
inline bool operator>= (const CharTexture &t, const char16_t &c) { return t.charID >= c; }

inline bool operator< (const CharLayout &l, const CharLayout &c) { return l.charID < c.charID; }
inline bool operator> (const CharLayout &l, const CharLayout &c) { return l.charID > c.charID; }
inline bool operator<= (const CharLayout &l, const CharLayout &c) { return l.charID <= c.charID; }
inline bool operator>= (const CharLayout &l, const CharLayout &c) { return l.charID >= c.charID; }

inline bool operator< (const CharLayout &l, const char16_t &c) { return l.charID < c; }
inline bool operator> (const CharLayout &l, const char16_t &c) { return l.charID > c; }
inline bool operator<= (const CharLayout &l, const char16_t &c) { return l.charID <= c; }
inline bool operator>= (const CharLayout &l, const char16_t &c) { return l.charID >= c; }

NS_LAYOUT_END

#endif
