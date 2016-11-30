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

#ifndef STAPPLER_SRC_FEATURES_RESOURCES_SPFONTFORMATTER_H_
#define STAPPLER_SRC_FEATURES_RESOURCES_SPFONTFORMATTER_H_

#include "SPFont.h"
#include "SPRichTextStyle.h"

typedef struct _HyphenDict HyphenDict;

NS_SP_EXT_BEGIN(font)

using CharGroupId = chars::CharGroupId;
using TextParameters = rich_text::style::TextLayoutParameters;
using TextAlign = rich_text::style::TextAlign;
using WhiteSpace = rich_text::style::WhiteSpace;
using TextTransform = rich_text::style::TextTransform;
using Hyphens = rich_text::style::Hyphens;
using TextDecoration = rich_text::style::TextDecoration;
using VerticalAlign = rich_text::style::VerticalAlign;
using FontVariant = rich_text::style::FontVariant;

struct LineSpec { // 12 bytes
	uint32_t start = 0;
	uint32_t count = 0;
	uint16_t pos = 0;
	uint16_t height = 0;
};

struct RangeSpec { // 20-24 bytes
	uint32_t start = 0;
	uint32_t count = 0;

	Arc<FontLayout> layout;

	Color4B color;
	uint16_t height = 0;
	uint8_t underline = 0;
	VerticalAlign align = VerticalAlign::Baseline;

	bool colorDirty = false;
	bool opacityDirty = false;
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

	FormatSpec();
	explicit FormatSpec(size_t);
	FormatSpec(size_t, size_t);

	bool init(size_t, size_t = 1);
	void reserve(size_t, size_t = 1);
	void clear();

	const LineSpec *getLine(size_t) const;

	RangeLineIterator begin() const;
	RangeLineIterator end() const;
};

class HyphenMap : public Ref {
public:
	bool init();

	void addHyphenDict(CharGroupId id, const String &file);
	Vector<uint8_t> makeWordHyphens(const char16_t *ptr, size_t len);
	Vector<uint8_t> makeWordHyphens(const CharReaderUcs2 &);
	void purgeHyphenDicts();

protected:
	String convertWord(HyphenDict *, const char16_t *ptr, size_t len);

	Map<CharGroupId, HyphenDict *> _dicts;
};

class Formatter {
public:
	using LinePositionCallback = Function<Pair<uint16_t, uint16_t>(uint16_t &, uint16_t &, float density)>;

public:
	// You MUST ensure that source and output exists until formatter is finalized
	Formatter(Source *, FormatSpec *, float density = 1.0f);

	void setLinePositionCallback(const LinePositionCallback &);
	void setWidth(uint16_t width);
	void setTextAlignment(TextAlign align);
	void setLineHeightAbsolute(uint16_t);
	void setLineHeightRelative(float);

	void setMaxWidth(uint16_t);
	void setMaxLines(size_t);
	void setOpticalAlignment(bool value);
	void setFillerChar(char16_t);
	void setHyphens(HyphenMap *);

	void begin(uint16_t indent, uint16_t blockMargin = 0);
	bool read(const FontParameters &f, const TextParameters &s, const WideString &str, uint16_t front = 0, uint16_t back = 0);
	bool read(const FontParameters &f, const TextParameters &s, const char16_t *str, size_t len, uint16_t front = 0, uint16_t back = 0);
	bool read(const FontParameters &f, const TextParameters &s, uint16_t width, uint16_t height);

	void finalize();
	void reset(FormatSpec *);

	uint16_t getHeight() const;
	uint16_t getWidth() const;
	uint16_t getMaxLineX() const;
	uint16_t getLineHeight() const;

protected:
	bool isSpecial(char16_t c) const;
	uint16_t checkBullet(uint16_t first, uint16_t len) const;

	void parseWhiteSpace(WhiteSpace whiteSpacePolicy);
	void parseFontLineHeight(uint16_t);
	void updatePosition(uint16_t &linePos, uint16_t &lineHeight);

	Arc<FontLayout> getLayout(uint16_t pos) const;

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

	bool readChars(CharReaderUcs2 &r, const Vector<uint8_t> & = Vector<uint8_t>());
	void pushLineFiller(bool replaceLastChar = false);
	bool pushChar(char16_t c);
	bool pushSpace(bool wrap = true);
	bool pushLine(uint16_t first, uint16_t len, bool forceAlign);
	bool pushLine(bool forceAlign);
	bool pushLineBreak();

	void updateLineHeight(uint16_t first, uint16_t last);

	Source *  source = nullptr;
	FormatSpec *  output = nullptr;
	HyphenMap * _hyphens = nullptr;

	size_t charPosition = 0;

	float density = 1.0f;

	Arc<FontLayout> primaryFont;
	Arc<FontData> primaryData;

	TextParameters textStyle;

	bool preserveLineBreaks = false;
	bool collapseSpaces = true;
	bool wordWrap = false;
	bool opticalAlignment = true;

	char16_t b = 0;
	char16_t c = 0;

	uint16_t defaultWidth = 0;
	uint16_t width = 0;
	uint16_t lineOffset = 0;
	uint16_t lineX = 0;
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
};

NS_SP_EXT_END(font)

#endif /* STAPPLER_SRC_FEATURES_RESOURCES_SPFONTFORMATTER_H_ */
