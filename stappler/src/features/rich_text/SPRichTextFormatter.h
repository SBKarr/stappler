/*
 * SPRichTextRendererTextReader.h
 *
 *  Created on: 05 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTFORMATTER_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTFORMATTER_H_

#include "SPRichTextRendererTypes.h"
#include "SPFont.h"
#include "base/CCVector.h"

typedef struct _HyphenDict HyphenDict;

NS_SP_EXT_BEGIN(rich_text)

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
	using CharSpec = Font::CharSpec;
	struct LineSpec {
		uint32_t first;
		uint32_t second;
		uint16_t height;
	};

	using LinePositionCallback = Function<Pair<uint16_t, uint16_t>(uint16_t &, uint16_t &, float density)>;

	using CharVec = Vector< CharSpec >;
	using LineVec = Vector< LineSpec >;
	using PosVec = Vector< size_t >;

public:
	Formatter(FontSet *, CharVec *, LineVec * = nullptr, float density = 1.0f);

	void setPositionsVec(PosVec *, size_t offset);
	void setLinePositionCallback(const LinePositionCallback &);
	void setWidth(uint16_t width);
	void setTextAlignment(style::TextAlign align);
	void setLineHeightAbsolute(uint16_t);
	void setLineHeightRelative(float);

	void setMaxWidth(uint16_t);
	void setMaxLines(size_t);
	void setOpticalAlignment(bool value);
	void setFillerChar(char16_t);
	void setHyphens(HyphenMap *);

	void begin(uint16_t indent, uint16_t blockMargin = 0);
	bool read(Font *f, const style::TextLayoutParameters &s, const WideString &str, uint16_t front = 0, uint16_t back = 0);
	bool read(Font *f, const style::TextLayoutParameters &s, const char16_t *str, size_t len, uint16_t front = 0, uint16_t back = 0);
	bool read(Font *, Font *, const style::TextLayoutParameters &s, const WideString &str, uint16_t front = 0, uint16_t back = 0);
	bool read(Font *, Font *, const style::TextLayoutParameters &s, const char16_t *str, size_t len, uint16_t front = 0, uint16_t back = 0);
	bool read(Font *f, const style::TextLayoutParameters &s, uint16_t width, uint16_t height);

	void finalize();
	void reset(CharVec *, LineVec * = nullptr);

	uint16_t getHeight() const;
	uint16_t getWidth() const;
	uint16_t getMaxLineX() const;
	uint16_t getLineHeight() const;

protected:
	bool isSpecial(char16_t c) const;
	uint16_t checkBullet(uint16_t first, uint16_t len) const;

	void parseWhiteSpace(style::WhiteSpace whiteSpacePolicy);
	bool parseFont(Font *f, Font * = nullptr);
	void updatePosition(uint16_t &linePos, uint16_t &lineHeight);

	uint16_t getAdvance(const CharSpec &c) const;
	uint16_t getAdvance(uint16_t pos) const;

	inline uint16_t getAdvancePosition(const CharSpec &c) const;
	inline uint16_t getAdvancePosition(uint16_t pos) const;

	uint16_t getLineAdvancePos(uint16_t lastPos);

	inline uint16_t getOriginPosition(const CharSpec &c) const;
	inline uint16_t getOriginPosition(uint16_t pos) const;

	bool readChars(CharReaderUcs2 &r, const Vector<uint8_t> & = Vector<uint8_t>());
	void pushLineFiller(bool replaceLastChar = false);
	bool pushChar(char16_t c);
	bool pushChar(uint16_t w, uint16_t h);
	bool pushSpace(bool wrap = true);
	bool pushLine(uint16_t first, uint16_t len, bool forceAlign);
	bool pushLine(bool forceAlign);
	bool pushLineBreak();

	FontSet *fontSet = nullptr;
	CharVec * chars = nullptr;
	LineVec * lines = nullptr;
	HyphenMap *_hyphens = nullptr;

	PosVec * positions = nullptr;
	size_t charPosition = 0;
	size_t charPositionOffset = 0;

	float density = 1.0f;

	Font *font = nullptr;
	Font *altFont = nullptr;
	style::TextLayoutParameters style;

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

	float lineHeightMod = 1.0f;
	bool lineHeightIsAbsolute = false;

	uint16_t firstInLine = 0;
	uint16_t wordWrapPos = 0;

	bool bufferedSpace = false;

	uint16_t maxWidth = 0;
	size_t maxLines = 0;

	char16_t _fillerChar = 0;
	style::TextAlign alignment = style::TextAlign::Left;
	LinePositionCallback linePositionFunc = nullptr;
};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTFORMATTER_H_ */
