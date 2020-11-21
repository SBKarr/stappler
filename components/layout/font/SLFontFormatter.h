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

#ifndef LAYOUT_FONT_SLFONTFORMATTER_H_
#define LAYOUT_FONT_SLFONTFORMATTER_H_

#include "SLFont.h"
#include "SLStyle.h"

typedef struct _HyphenDict HyphenDict;

NS_LAYOUT_BEGIN

using TextParameters = style::TextLayoutParameters;
using TextAlign = style::TextAlign;
using WhiteSpace = style::WhiteSpace;
using TextTransform = style::TextTransform;
using Hyphens = style::Hyphens;
using TextDecoration = style::TextDecoration;
using VerticalAlign = style::VerticalAlign;
using FontVariant = style::FontVariant;

struct LineSpec { // 12 bytes
	uint32_t start = 0;
	uint32_t count = 0;
	uint16_t pos = 0;
	uint16_t height = 0;
};

struct RangeSpec { // 32 bytes
	bool colorDirty = false;
	bool opacityDirty = false;
	TextDecoration decoration = TextDecoration::None;
	VerticalAlign align = VerticalAlign::Baseline;

	uint32_t start = 0;
	uint32_t count = 0;

	Color4B color;
	uint16_t height = 0;

	Rc<FontLayout> layout;
};

class FormatSpec : public Ref {
public:
	struct RangeLineIterator {
		Vector< RangeSpec >::const_iterator range;
		Vector< LineSpec >::const_iterator line;

		uint32_t start() const {
			return std::max(range->start, line->start);
		}
		uint32_t count() const {
			return std::min(range->start + range->count, line->start + line->count) - start();
		}
		uint32_t end() const {
			return std::min(range->start + range->count, line->start + line->count);
		}

		RangeLineIterator &operator++() {
			const auto rangeEnd = range->start + range->count;
			const auto lineEnd = line->start + line->count;
			if (rangeEnd < lineEnd) {
				++ range;
			} else if (rangeEnd > lineEnd) {
				++ line;
			} else {
				++ range;
				++ line;
			}

			return *this;
		}

		bool operator==(const RangeLineIterator &other) const {
			return other.line == line && other.range == range;
		}
		bool operator!=(const RangeLineIterator &other) const {
			return !(*this == other);
		}
	};

	Vector< RangeSpec > ranges;
	Vector< CharSpec > chars;
	Vector< LineSpec > lines;

	uint16_t width = 0;
	uint16_t height = 0;
	uint16_t maxLineX = 0;
	bool overflow = false;

	FormatSpec();
	explicit FormatSpec(size_t);
	FormatSpec(size_t, size_t);

	bool init(size_t, size_t = 1);
	void reserve(size_t, size_t = 1);
	void clear();

	enum SelectMode {
		Center,
		Prefix,
		Suffix,
		Best,
	};

	// on error maxOf<uint32_t> returned
	Pair<uint32_t, SelectMode> getChar(int32_t x, int32_t y, SelectMode = Center) const;
	const LineSpec *getLine(uint32_t charIndex) const;
	uint32_t getLineNumber(uint32_t charIndex) const;

	RangeLineIterator begin() const;
	RangeLineIterator end() const;

	WideString str(bool filterAlign = true) const;
	WideString str(uint32_t, uint32_t, size_t maxWords = maxOf<size_t>(), bool ellipsis = true, bool filterAlign = true) const;
	Pair<uint32_t, uint32_t> selectWord(uint32_t originChar) const;
	uint32_t selectChar(int32_t x, int32_t y, SelectMode = Center) const;

	Rect getLineRect(uint32_t lineId, float density, const Vec2 & = Vec2()) const;
	Rect getLineRect(const LineSpec &, float density, const Vec2 & = Vec2()) const;

	uint16_t getLineForCharId(uint32_t id) const;
	Vector<Rect> getLabelRects(uint32_t first, uint32_t last, float density, const Vec2 & = Vec2(), const Padding &p = Padding()) const;
	void getLabelRects(Vector<Rect> &, uint32_t first, uint32_t last, float density, const Vec2 & = Vec2(), const Padding &p = Padding()) const;
};

class HyphenMap : public Ref {
public:
	virtual ~HyphenMap();
	bool init();

	void addHyphenDict(CharGroupId id, FilePath file);
	void addHyphenDict(CharGroupId id, BytesView data);
	Vector<uint8_t> makeWordHyphens(const char16_t *ptr, size_t len);
	Vector<uint8_t> makeWordHyphens(const WideStringView &);
	void purgeHyphenDicts();

protected:
	String convertWord(HyphenDict *, const char16_t *ptr, size_t len);

	Map<CharGroupId, HyphenDict *> _dicts;
};

class Formatter {
public:
	using LinePositionCallback = Function<Pair<uint16_t, uint16_t>(uint16_t &, uint16_t &, float density)>;

	enum class ContentRequest {
		Normal,
		Minimize,
		Maximize,
	};

public:
	Formatter();

	// You MUST ensure that source and output exists until formatter is finalized
	Formatter(FontSource *, FormatSpec *, float density = 1.0f);

	void init(FontSource *, FormatSpec *, float density = 1.0f);

	void setLinePositionCallback(const LinePositionCallback &);
	void setWidth(uint16_t width);
	void setTextAlignment(TextAlign align);
	void setLineHeightAbsolute(uint16_t);
	void setLineHeightRelative(float);

	void setMaxWidth(uint16_t);
	void setMaxLines(size_t);
	void setOpticalAlignment(bool value);
	void setEmplaceAllChars(bool value);
	void setFillerChar(char16_t);
	void setHyphens(HyphenMap *);
	void setRequest(ContentRequest);
	void setFontScale(float);

	void begin(uint16_t indent, uint16_t blockMargin = 0);
	bool read(const FontParameters &f, const TextParameters &s, WideStringView str, uint16_t front = 0, uint16_t back = 0);
	bool read(const FontParameters &f, const TextParameters &s, const char16_t *str, size_t len, uint16_t front = 0, uint16_t back = 0);
	bool read(const FontParameters &f, const TextParameters &s, uint16_t width, uint16_t height);

	void finalize();
	void reset(FormatSpec *);
	void reset(FontSource *, FormatSpec *, float density = 1.0f);

	uint16_t getHeight() const;
	uint16_t getWidth() const;
	uint16_t getMaxLineX() const;
	uint16_t getLineHeight() const;

	float getDensity() const;

	FormatSpec *getOutput() const;

protected:
	bool isSpecial(char16_t c) const;
	uint16_t checkBullet(uint16_t first, uint16_t len) const;

	void parseWhiteSpace(WhiteSpace whiteSpacePolicy);
	void parseFontLineHeight(uint16_t);
	bool updatePosition(uint16_t &linePos, uint16_t &lineHeight);

	Rc<FontLayout> getLayout(uint16_t pos) const;

	uint16_t getAdvance(const CharSpec &c) const;
	uint16_t getAdvance(uint16_t pos) const;

	inline uint16_t getAdvancePosition(const CharSpec &c) const;
	inline uint16_t getAdvancePosition(uint16_t pos) const;

	uint16_t getLineAdvancePos(uint16_t lastPos);

	inline uint16_t getOriginPosition(const CharSpec &c) const;
	inline uint16_t getOriginPosition(uint16_t pos) const;

	void clearRead();

	bool readWithRange(RangeSpec &&, const TextParameters &s, const char16_t *str, size_t len, uint16_t front = 0, uint16_t back = 0);
	bool readWithRange(RangeSpec &&, const TextParameters &s, uint16_t width, uint16_t height);

	bool readChars(WideStringView &r, const Vector<uint8_t> & = Vector<uint8_t>());
	void pushLineFiller(bool replaceLastChar = false);
	bool pushChar(char16_t c);
	bool pushSpace(bool wrap = true);
	bool pushTab();
	bool pushLine(uint16_t first, uint16_t len, bool forceAlign);
	bool pushLine(bool forceAlign);
	bool pushLineBreak();
	bool pushLineBreakChar();

	void updateLineHeight(uint16_t first, uint16_t last);

	FontSource *  source = nullptr;
	FormatSpec *  output = nullptr;
	HyphenMap * _hyphens = nullptr;

	size_t charPosition = 0;

	float density = 1.0f;
	float fontScale = nan();

	Rc<FontLayout> primaryFont;
	Rc<FontData> primaryData;

	TextParameters textStyle;

	bool preserveLineBreaks = false;
	bool collapseSpaces = true;
	bool wordWrap = false;
	bool opticalAlignment = true;
	bool emplaceAllChars = false;

	char16_t b = 0;
	char16_t c = 0;

	uint16_t defaultWidth = 0;
	uint16_t width = 0;
	uint16_t lineOffset = 0;
	int16_t lineX = 0;
	uint16_t lineY = 0;

	uint16_t maxLineX = 0;

	uint16_t charNum = 0;
	uint16_t lineHeight = 0;
	uint16_t currentLineHeight = 0;
	uint16_t rangeLineHeight = 0;

	float lineHeightMod = 1.0f;
	bool lineHeightIsAbsolute = false;

	uint16_t firstInLine = 0;
	uint16_t wordWrapPos = 0;

	bool bufferedSpace = false;

	uint16_t maxWidth = 0;
	size_t maxLines = 0;

	char16_t _fillerChar = 0;
	TextAlign alignment = TextAlign::Left;
	LinePositionCallback linePositionFunc = nullptr;

	ContentRequest request = ContentRequest::Normal;
};

NS_LAYOUT_END

#endif /* LAYOUT_FONT_SLFONTFORMATTER_H_ */
