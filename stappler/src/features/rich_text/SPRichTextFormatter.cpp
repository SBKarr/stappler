/*
 * SPRichTextRendererTextReader.cpp
 *
 *  Created on: 05 мая 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPRichTextFormatter.h"
#include "SPString.h"

NS_SP_EXT_BEGIN(rich_text)

Formatter::Formatter(FontSet *set, CharVec *chars, LineVec *lines, float density)
: fontSet(set), chars(chars), lines(lines), density(density) { }

void Formatter::setPositionsVec(PosVec *pos, size_t offset) {
	positions = pos;
	charPositionOffset = offset;
}
void Formatter::setLinePositionCallback(const LinePositionCallback &func) {
	linePositionFunc = func;
}

void Formatter::setWidth(uint16_t w) {
	defaultWidth = w;
	width = w;
}

void Formatter::setTextAlignment(style::TextAlign align) {
	alignment = align;
}

void Formatter::setLineHeightAbsolute(uint16_t val) {
	lineHeight = val;
	currentLineHeight = val;
	lineHeightIsAbsolute = true;
	if (font) {
		parseFont(font);
	}
}

void Formatter::setLineHeightRelative(float val) {
	lineHeightMod = val;
	lineHeightIsAbsolute = false;
	if (font) {
		parseFont(font);
	}
}

void Formatter::setMaxWidth(uint16_t value) {
	maxWidth = value;
}
void Formatter::setMaxLines(size_t value) {
	maxLines = value;
}
void Formatter::setOpticalAlignment(bool value) {
	opticalAlignment = value;
}
void Formatter::setFillerChar(char16_t value) {
	_fillerChar = value;
}
void Formatter::setHyphens(HyphenMap *map) {
	_hyphens = map;
}

void Formatter::begin(uint16_t ind, uint16_t blockMargin) {
	lineX = ind;

	firstInLine = charNum;
	wordWrapPos = charNum;

	bufferedSpace = false;
	c = 0;
	b = 0;

	if (lineY != 0) {
		lineY += blockMargin;
	}
}

void Formatter::parseWhiteSpace(style::WhiteSpace whiteSpacePolicy) {
	switch (whiteSpacePolicy) {
	case style::WhiteSpace::Normal:
		preserveLineBreaks = false;
		collapseSpaces = true;
		wordWrap = true;
		break;
	case style::WhiteSpace::Nowrap:
		preserveLineBreaks = false;
		collapseSpaces = true;
		wordWrap = false;
		break;
	case style::WhiteSpace::Pre:
		preserveLineBreaks = true;
		collapseSpaces = false;
		wordWrap = false;
		break;
	case style::WhiteSpace::PreLine:
		preserveLineBreaks = true;
		collapseSpaces = true;
		wordWrap = true;
		break;
	case style::WhiteSpace::PreWrap:
		preserveLineBreaks = true;
		collapseSpaces = false;
		wordWrap = true;
		break;
	default:
		preserveLineBreaks = false;
		collapseSpaces = true;
		wordWrap = true;
		break;
	};
}

bool Formatter::parseFont(Font *f, Font *alt) {
	if ((f != font && f->getFontSet() != fontSet) || (alt && alt->getFontSet() != fontSet)) {
		return false;
	}
	font = f;
	altFont = alt;
	if (font && !lineHeightIsAbsolute) {
		if (lineHeight == 0) {
			lineHeight = font->getHeight();
		}
		float fontLineHeight = (uint16_t) (font->getHeight() * lineHeightMod);
		if (fontLineHeight > currentLineHeight) {
			currentLineHeight = fontLineHeight;
		}
	}
	return true;
}

void Formatter::updatePosition(uint16_t &linePos, uint16_t &height) {
	if (linePositionFunc) {
		std::tie(lineOffset, width) = linePositionFunc(linePos, height, density);
		width = MIN(width, defaultWidth);

		while (width < font->getHeight()) {
			linePos += lineHeight;
			std::tie(lineOffset, width) = linePositionFunc(linePos, height, density);
			width = MIN(width, defaultWidth);
		}
	}
}

uint16_t Formatter::getAdvance(const CharSpec &c) const {
	return c.xAdvance();
}

uint16_t Formatter::getAdvance(uint16_t pos) const {
	if (pos < chars->size()) {
		return getAdvance(chars->at(pos));
	} else {
		return 0;
	}
}

inline uint16_t Formatter::getAdvancePosition(const CharSpec &c) const {
	return c.xAdvancePosition();
}

inline uint16_t Formatter::getAdvancePosition(uint16_t pos) const {
	return (pos < chars->size())?getAdvancePosition(chars->at(pos)):(uint16_t)0;
}

inline uint16_t Formatter::getOriginPosition(const CharSpec &c) const {
	return c.posX;
}

inline uint16_t Formatter::getOriginPosition(uint16_t pos) const {
	return (pos < chars->size())?getOriginPosition(chars->at(pos)):(uint16_t)0;
}

bool Formatter::isSpecial(char16_t c) const {
	if (!opticalAlignment) {
		return false;
	}
	return chars::CharGroup<char16_t, chars::CharGroupId::OpticalAlignmentSpecial>::match(c);
}

uint16_t Formatter::checkBullet(uint16_t first, uint16_t len) const {
	if (!opticalAlignment) {
		return 0;
	}

	uint16_t offset = 0;
	for (uint16_t i = first; i < first + len - 1; i++) {
		auto c = chars->at(i).charId();
		if (chars::CharGroup<char16_t, chars::CharGroupId::OpticalAlignmentBullet>::match(c)) {
			offset ++;
		} else if (string::isspace(c) && offset >= 1) {
			return offset + 1;
		} else {
			break;
		}
	}

	return 0;
}

void Formatter::pushLineFiller(bool replaceLastChar) {
	if (_fillerChar == 0) {
		return;
	}
	auto charDef = font->getChar(_fillerChar);
	if (!charDef) {
		return;
	}

	CharSpec spec;
	spec.posX = lineX;

	if (style.verticalAlign == style::VerticalAlign::Sub) {
		spec.posY -= font->getDescender() / 2;
	} else if (style.verticalAlign == style::VerticalAlign::Super) {
		spec.posY -= font->getAscender() / 2;
	}

	if (replaceLastChar && !chars->empty()) {
		chars->back().set(charDef);
		if (positions) {
			positions->back() = WideString::npos;
		}
	} else {
		spec.set(charDef);
		spec.color = cocos2d::Color4B(style.color, style.opacity);
		chars->push_back(std::move(spec));
		charNum ++;
		if (positions) {
			positions->push_back(WideString::npos);
		}
	}
}

bool Formatter::pushChar(char16_t c) {
	if (style.textTransform == style::TextTransform::Uppercase) {
		c = string::toupper(c);
	} else if (style.textTransform == style::TextTransform::Lowercase) {
		c = string::tolower(c);
	}

	const Font::Char * charDef = nullptr;
	if (altFont && c != string::toupper(c)) {
		charDef = altFont->getChar(string::toupper(c));
	} else {
		charDef = font->getChar(c);
	}

	if (!charDef) {
		if (c == (char16_t)0x00AD) {
			charDef = font->getChar('-');
		} else {
			log::format("RichTextFormatter", "%s: Attempted to use undefined character: %d '%s'", font->getName().c_str(), c, string::toUtf8(c).c_str());
			return true;
		}
	}

	if (charNum == firstInLine && lineOffset > 0) {
		lineX += lineOffset;
	}

	auto posX = lineX;

	CharSpec spec;
	spec.posX = posX;

	if (style.verticalAlign == style::VerticalAlign::Sub) {
		spec.posY -= font->getDescender() / 2;
	} else if (style.verticalAlign == style::VerticalAlign::Super) {
		spec.posY -= font->getAscender() / 2;
	}

	spec.set(charDef);
	spec.color = cocos2d::Color4B(style.color, style.opacity);

	if (style.textDecoration == style::TextDecoration::Underline) {
		spec.underline = roundf(density);
	}

	if (c == (char16_t)0x00AD) {
		if (style.hyphens == style::Hyphens::Manual || style.hyphens == style::Hyphens::Auto) {
			wordWrapPos = charNum + 1;
		}
		spec.display = CharSpec::Hidden;
	} else if (c == u'-' || c == u'+' || c == u'*' || c == u'/' || c == u'\\') {
		auto pos = charNum;
		while(pos > firstInLine && (!string::isspace(chars->at(pos - 1).charId()))) {
			pos --;
		}
		if (charNum - pos > 2) {
			wordWrapPos = charNum + 1;
		}
		auto newlineX = lineX + charDef->xAdvance;
		if (maxWidth && lineX > maxWidth) {
			pushLineFiller();
			return false;
		}
		lineX = newlineX;
	} else if (charDef) {
		if (charNum == firstInLine && isSpecial(c)) {
			spec.posX -= charDef->xAdvance / 2;
			lineX += charDef->xAdvance / 2;
		} else {
			auto newlineX = lineX + charDef->xAdvance;
			if (maxWidth && lineX > maxWidth) {
				pushLineFiller(true);
				return false;
			}
			lineX = newlineX;
		}
	}
	charNum ++;
	chars->push_back(std::move(spec));

	if (positions) {
		positions->push_back(charPosition + charPositionOffset);
	}

	return true;
}

bool Formatter::pushChar(uint16_t w, uint16_t h) {
	if (charNum == firstInLine && lineOffset > 0) {
		lineX += lineOffset;
	}

	auto posX = lineX;

	CharSpec spec;
	spec.posX = posX;

	if (style.verticalAlign == style::VerticalAlign::Sub) {
		spec.posY -= font->getDescender() / 2;
	} else if (style.verticalAlign == style::VerticalAlign::Super) {
		spec.posY -= font->getAscender() / 2;
	}

	spec.set(w, h);
	spec.color = cocos2d::Color4B(style.color, style.opacity);

	if (style.textDecoration == style::TextDecoration::Underline) {
		spec.underline = roundf(density);
	}

	auto newlineX = lineX + spec.xAdvance();
	if (maxWidth && lineX > maxWidth) {
		pushLineFiller(true);
		return false;
	}
	lineX = newlineX;

	charNum ++;
	chars->push_back(std::move(spec));

	if (positions) {
		positions->push_back(charPosition + charPositionOffset);
	}

	return true;
}

bool Formatter::pushSpace(bool wrap) {
	if (pushChar(' ')) {
		if (wordWrap && wrap) {
			wordWrapPos = charNum;
		}
		return true;
	}
	return false;
}

uint16_t Formatter::getLineAdvancePos(uint16_t lastPos) {
	auto &origChar = chars->at(lastPos);
	auto c = origChar.charId();
	if (c == ' ' && lastPos > firstInLine) {
		lastPos --;
	}
	if (lastPos < firstInLine) {
		return 0;
	}

	auto a = getAdvancePosition(lastPos);
	auto &lastChar = chars->at(lastPos);
	c = lastChar.charId();
	if (isSpecial(c)) {
		if (c == '.' || c == ',') {
			a -= lastChar.xAdvance();
		} else {
			a -= lastChar.xAdvance() / 2;
		}
	}
	return a;
}

bool Formatter::pushLine(uint16_t first, uint16_t len, bool forceAlign) {
	if (maxLines && lines && lines->size() + 1 == maxLines && forceAlign) {
		pushLineFiller(true);
		return false;
	}

	uint16_t linePos = lineY + currentLineHeight;

	for (uint16_t i = first; i < first + len; i++) {
		chars->at(i).posY += linePos;
	}

	if (lines) {
		lines->push_back(LineSpec{first, len, currentLineHeight});
	}
	if (len > 0) {
		uint16_t advance = getLineAdvancePos(first + len - 1);
		uint16_t offsetLeft = (advance < (width + lineOffset))?((width + lineOffset) - advance):0;
		if (offsetLeft > 0 && alignment == style::TextAlign::Right) {
			for (uint16_t i = first; i < first + len; i++) {
				chars->at(i).posX += offsetLeft;
			}
		} else if (offsetLeft > 0 && alignment == style::TextAlign::Center) {
			offsetLeft /= 2;
			for (uint16_t i = first; i < first + len; i++) {
				chars->at(i).posX += offsetLeft;
			}
		} else if ((offsetLeft > 0 || (advance > width + lineOffset)) && alignment == style::TextAlign::Justify && forceAlign && len > 0) {
			int16_t joffset = (advance > width + lineOffset)?(width + lineOffset - advance):offsetLeft;
			uint16_t spacesCount = 0;
			if (first == 0) {
				auto b = checkBullet(first, len);
				first += b;
				len -= b;
			}

			for (uint16_t i = first; i < first + len - 1; i++) {
				auto c = chars->at(i).charId();
				if (string::isspace(c) && c != '\n') {
					spacesCount ++;
				}
			}

			int16_t offset = 0;
			for (uint16_t i = first; i < first + len; i++) {
				auto c = chars->at(i).charId();
				if (string::isspace(c) && c != '\n' && spacesCount > 0) {
					offset += joffset / spacesCount;
					joffset -= joffset / spacesCount;
					spacesCount --;
				} else {
					chars->at(i).posX += offset;
				}
			}
		}

		if (advance > maxLineX) {
			maxLineX = advance;
		}
	}

	lineY = linePos;
	firstInLine = charNum;
	wordWrapPos = firstInLine;
	bufferedSpace = false;
	currentLineHeight = lineHeight;
	parseFont(font, altFont);
	width = defaultWidth;
	if (defaultWidth >= font->getHeight()) {
		updatePosition(lineY, currentLineHeight);
	}
	b = 0;
	return true;
}

bool Formatter::pushLine(bool forceAlign) {
	uint16_t first = firstInLine;
	if (firstInLine <= charNum) {
		uint16_t len = charNum - firstInLine;
		return pushLine(first, len, forceAlign);
	}
	return true;
}

bool Formatter::pushLineBreak() {
	if (chars->back().charId() == ' ') {
		return true;
	}

	if (firstInLine >= wordWrapPos - 1 && (maxLines != 0 && lines->size() + 1 != maxLines)) {
		return true;
	}

	uint16_t wordStart = wordWrapPos;
	uint16_t wordEnd = charNum - 1;


	// PREV: check if we can wrap the word (word is larger then available width)
	// NOW: in this case we wrap word by characters
	if (lineX - getOriginPosition(wordWrapPos) > width || wordWrapPos == 0) {
		if (wordWrap) {
			lineX = lineOffset;
			if (!pushLine(firstInLine, wordEnd - firstInLine, true)) {
				return false;
			}

			firstInLine = wordEnd;
			wordWrapPos = wordEnd;

			auto &c = chars->at(wordEnd);

			c.posX = lineX;
			lineX += c.xAdvance();

			for (size_t i = wordEnd; i < charNum; ++i) {
				CharSpec &c = chars->at(i);
				if (c.display == CharSpec::Block) {
					if (c.height() > currentLineHeight) {
						currentLineHeight = c.height();
					}
				}
			}
		}
	} else {
		// we can wrap the word
		auto &c = chars->at((wordWrapPos - 1));
		if (!string::isspace(c.charId())) {
			if (c.display == CharSpec::Hidden) {
				c.display = CharSpec::Char;
			}
			if (!pushLine(firstInLine, (wordWrapPos) - firstInLine, true)) {
				return false;
			}
		} else {
			if (!pushLine(firstInLine, (wordWrapPos > firstInLine)?((wordWrapPos - 1) - firstInLine):0, true)) {
				return false;
			}
		}
		firstInLine = wordStart;
		for (size_t i = firstInLine; i < charNum; ++i) {
			CharSpec &c = chars->at(i);
			if (c.display == CharSpec::Block) {
				if (c.height() > currentLineHeight) {
					currentLineHeight = c.height();
				}
			}
		}

		uint16_t originOffset = getOriginPosition(wordStart);

		auto &b = chars->at((wordStart));
		if (isSpecial(b.charId())) {
			originOffset += b.xAdvance() / 2;
		}

		if (originOffset > lineOffset) {
			originOffset -= lineOffset;
		}

		for (uint32_t i = wordStart; i <= wordEnd; i++) {
			chars->at(i).posX -= originOffset;
		}
		lineX -= originOffset;
	}
	return true;
}

bool Formatter::readChars(CharReaderUcs2 &r, const Vector<uint8_t> &hyph) {
	size_t wordPos = 0;
	auto hIt = hyph.begin();
	bool startWhitespace = chars->empty();
	for (c = r[0]; !r.empty(); ++charPosition, ++wordPos, ++r, c = r[0]) {
		if (hIt != hyph.end() && wordPos == *hIt) {
			pushChar((char16_t)0x00AD);
			++ hIt;
		}

		if (c == '\n') {
			if (preserveLineBreaks) {
				if (!pushLine(false)) {
					return false;
				}
				lineX = 0;
			} else if (collapseSpaces) {
				if (!startWhitespace) {
					bufferedSpace = true;
				}
			}
			b = 0;
			continue;
		}

		if (c < 0x20) {
			continue;
		}

		if (c != 0x00A0 && string::isspace(c) && collapseSpaces) {
			if (!startWhitespace) {
				bufferedSpace = true;
			}
			b = 0;
			continue;
		}

		if (c == (char16_t)0x00A0) {
			if (!pushSpace(false)) {
				return false;
			}
			bufferedSpace = false;
			continue;
		}

		if (bufferedSpace || (!collapseSpaces && c != 0x00A0 && string::isspace(c))) {
			if (!pushSpace()) {
				return false;
			}
			if (!bufferedSpace) {
				continue;
			} else {
				bufferedSpace = false;
			}
		}

		auto kerning = font->kerningAmount(b, c);
		if (kerning != 0) {
			//log::format("Kerning", "%c %c %d", b, c, kerning);
		}
		lineX += kerning;
		if (!pushChar(c)) {
			return false;
		}
		startWhitespace = false;

		if (width + lineOffset > 0 && lineX > width + lineOffset) {
			lineX -= kerning;
			if (!pushLineBreak()) {
				return false;
			}
		}

		if (c != (char16_t)0x00AD) {
			b = c;
		}
	}
	return true;
}

bool Formatter::read(Font *f, const style::TextLayoutParameters &s, const WideString &str, uint16_t frontOffset, uint16_t backOffset) {
	return read(f, nullptr, s, str.c_str(), str.length(), frontOffset, backOffset);
}
bool Formatter::read(Font *f, const style::TextLayoutParameters &s, const char16_t *str, size_t len, uint16_t frontOffset, uint16_t backOffset) {
	return read(f, nullptr, s, str, len, frontOffset, backOffset);
}
bool Formatter::read(Font *f, Font *alt, const style::TextLayoutParameters &s, const WideString &str, uint16_t frontOffset, uint16_t backOffset) {
	return read(f, alt, s, str.c_str(), str.length(), frontOffset, backOffset);
}

bool Formatter::read(Font *f, Font *alt, const style::TextLayoutParameters &s, const char16_t *str, size_t len, uint16_t frontOffset, uint16_t backOffset) {
	charPosition = 0;
	if (bufferedSpace) {
		pushSpace();
		bufferedSpace = false;
	}

	if (!parseFont(f, alt) || !str) {
		return false;
	}

	style = s;
	parseWhiteSpace(style.whiteSpace);
	updatePosition(lineY, currentLineHeight);

	if (!chars->empty() && chars->back().charId() == ' ' && collapseSpaces) {
		while (len > 0 && ((string::isspace(str[0]) && str[0] != 0x00A0) || str[0] < 0x20)) {
			len --; str++;
		}
	}

	b = 0;

	lineX += frontOffset;
	charPosition = 0;
	CharReaderUcs2 r(str, len);
	if (s.hyphens == style::Hyphens::Auto && _hyphens) {
		while (!r.empty()) {
			CharReaderUcs2 tmp = r.readUntil<CharReaderUcs2::CharGroup<chars::CharGroupId::Latin>,
					CharReaderUcs2::CharGroup<chars::CharGroupId::Cyrillic>>();
			if (!tmp.empty()) {
				readChars(tmp);
			}
			tmp = r.readChars<CharReaderUcs2::CharGroup<chars::CharGroupId::Latin>,
					CharReaderUcs2::CharGroup<chars::CharGroupId::Cyrillic>>();
			if (!tmp.empty()) {
				readChars(tmp, _hyphens->makeWordHyphens(tmp));
			}
		}
	} else {
		readChars(r);
	}

	lineX += backOffset;

	return true;
}

bool Formatter::read(Font *f, const style::TextLayoutParameters &s, uint16_t blockWidth, uint16_t blockHeight) {
	charPosition = 0;
	if (bufferedSpace) {
		pushSpace();
		bufferedSpace = false;
	}

	if (!parseFont(f)) {
		return false;
	}

	style = s;
	parseWhiteSpace(style.whiteSpace);

	if (currentLineHeight < blockHeight) {
		currentLineHeight = blockHeight;
	}

	updatePosition(lineY, currentLineHeight);
	pushChar(blockWidth, blockHeight);
	if (width + lineOffset > 0 && lineX > width + lineOffset) {
		if (!pushLineBreak()) {
			return false;
		}
	}

	return true;
}

void Formatter::finalize() {
	if (firstInLine < charNum) {
		pushLine(false);
	}
}

void Formatter::reset(CharVec *charsVec, LineVec *linesVec) {
	chars = charsVec;
	lines = linesVec;

	b = 0;
	c = 0;

	defaultWidth = 0;
	width = 0;
	lineOffset = 0;
	lineX = 0;
	lineY = 0;

	maxLineX = 0;

	charNum = 0;
	lineHeight = 0;
	currentLineHeight = 0;

	lineHeightMod = 1.0f;
	lineHeightIsAbsolute = false;

	firstInLine = 0;
	wordWrapPos = 0;

	bufferedSpace = false;
}

uint16_t Formatter::getHeight() const {
	return lineY;
}
uint16_t Formatter::getWidth() const {
	return MAX(maxLineX, width);
}
uint16_t Formatter::getMaxLineX() const {
	return maxLineX;
}
uint16_t Formatter::getLineHeight() const {
	return lineHeight;
}

NS_SP_EXT_END(rich_text)
