/*
 * SPFontFormatter.cpp
 *
 *  Created on: 22 окт. 2016 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPFontFormatter.h"
#include "SPString.h"
#include "SPFilesystem.h"
#include "hyphen.h"

NS_SP_EXT_BEGIN(font)

Formatter::Formatter(Source *s, FormatSpec *o, float density)
: source(s), output(o), density(density) { }

void Formatter::setLinePositionCallback(const LinePositionCallback &func) {
	linePositionFunc = func;
}

void Formatter::setWidth(uint16_t w) {
	defaultWidth = w;
	width = w;
}

void Formatter::setTextAlignment(TextAlign align) {
	alignment = align;
}

void Formatter::setLineHeightAbsolute(uint16_t val) {
	lineHeight = val;
	currentLineHeight = val;
	lineHeightIsAbsolute = true;
	parseFontLineHeight(rangeLineHeight);
}

void Formatter::setLineHeightRelative(float val) {
	lineHeightMod = val;
	lineHeightIsAbsolute = false;
	parseFontLineHeight(rangeLineHeight);
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

void Formatter::parseWhiteSpace(WhiteSpace whiteSpacePolicy) {
	switch (whiteSpacePolicy) {
	case WhiteSpace::Normal:
		preserveLineBreaks = false;
		collapseSpaces = true;
		wordWrap = true;
		break;
	case WhiteSpace::Nowrap:
		preserveLineBreaks = false;
		collapseSpaces = true;
		wordWrap = false;
		break;
	case WhiteSpace::Pre:
		preserveLineBreaks = true;
		collapseSpaces = false;
		wordWrap = false;
		break;
	case WhiteSpace::PreLine:
		preserveLineBreaks = true;
		collapseSpaces = true;
		wordWrap = true;
		break;
	case WhiteSpace::PreWrap:
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

void Formatter::parseFontLineHeight(uint16_t h) {
	if (!lineHeightIsAbsolute) {
		if (lineHeight == 0) {
			lineHeight = h;
		}
		float fontLineHeight = (uint16_t) (h * lineHeightMod);
		if (fontLineHeight > currentLineHeight) {
			currentLineHeight = fontLineHeight;
		}
	}
}

void Formatter::updatePosition(uint16_t &linePos, uint16_t &height) {
	if (linePositionFunc) {
		std::tie(lineOffset, width) = linePositionFunc(linePos, height, density);
		width = MIN(width, defaultWidth);

		while (width < primaryData->getHeight()) {
			linePos += lineHeight;
			std::tie(lineOffset, width) = linePositionFunc(linePos, height, density);
			width = MIN(width, defaultWidth);
		}
	}
}

Arc<FontLayout> Formatter::getLayout(uint16_t pos) const {
	for (const RangeSpec &it : output->ranges) {
		if (pos >= it.start && pos < it.start + it.count) {
			return it.layout;
		}
	}
	return primaryFont;
}

uint16_t Formatter::getAdvance(const CharSpec &c) const {
	return c.advance;
}

uint16_t Formatter::getAdvance(uint16_t pos) const {
	if (pos < output->chars.size()) {
		return getAdvance(output->chars.at(pos));
	} else {
		return 0;
	}
}

inline uint16_t Formatter::getAdvancePosition(const CharSpec &c) const {
	return c.pos + c.advance;
}

inline uint16_t Formatter::getAdvancePosition(uint16_t pos) const {
	return (pos < output->chars.size())?getAdvancePosition(output->chars.at(pos)):(uint16_t)0;
}

inline uint16_t Formatter::getOriginPosition(const CharSpec &c) const {
	return c.pos;
}

inline uint16_t Formatter::getOriginPosition(uint16_t pos) const {
	return (pos < output->chars.size())?getOriginPosition(output->chars.at(pos)):(uint16_t)0;
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
		auto c = output->chars.at(i).charID;
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
	auto charDef = primaryData->getChar(_fillerChar);
	if (!charDef) {
		return;
	}

	CharSpec spec{_fillerChar, lineX, charDef.xAdvance};
	if (replaceLastChar && !output->chars.empty()) {
		output->chars.back() = std::move(spec);
	} else {
		output->chars.push_back(std::move(spec));
		charNum ++;
	}
}

bool Formatter::pushChar(char16_t c) {
	if (textStyle.textTransform == TextTransform::Uppercase) {
		c = string::toupper(c);
	} else if (textStyle.textTransform == TextTransform::Lowercase) {
		c = string::tolower(c);
	}

	CharLayout charDef = primaryData->getChar(c);

	if (charDef.charID == 0) {
		if (c == (char16_t)0x00AD) {
			charDef = primaryData->getChar('-');
		} else {
			log::format("RichTextFormatter", "%s: Attempted to use undefined character: %d '%s'", primaryFont->getName().c_str(), c, string::toUtf8(c).c_str());
			return true;
		}
	}

	if (charNum == firstInLine && lineOffset > 0) {
		lineX += lineOffset;
	}

	auto posX = lineX;

	CharSpec spec{c, posX, charDef.xAdvance, CharSpec::Display::Char};

	if (c == (char16_t)0x00AD) {
		if (textStyle.hyphens == Hyphens::Manual || textStyle.hyphens == Hyphens::Auto) {
			wordWrapPos = charNum + 1;
		}
		spec.display = CharSpec::Display::Hidden;
	} else if (c == u'-' || c == u'+' || c == u'*' || c == u'/' || c == u'\\') {
		auto pos = charNum;
		while(pos > firstInLine && (!string::isspace(output->chars.at(pos - 1).charID))) {
			pos --;
		}
		if (charNum - pos > 2) {
			wordWrapPos = charNum + 1;
		}
		auto newlineX = lineX + charDef.xAdvance;
		if (maxWidth && lineX > maxWidth) {
			pushLineFiller();
			return false;
		}
		lineX = newlineX;
	} else if (charDef) {
		if (charNum == firstInLine && isSpecial(c)) {
			spec.pos -= charDef.xAdvance / 2;
			lineX += charDef.xAdvance / 2;
		} else {
			auto newlineX = lineX + charDef.xAdvance;
			if (maxWidth && lineX > maxWidth) {
				pushLineFiller(true);
				return false;
			}
			lineX = newlineX;
		}
	}
	charNum ++;
	output->chars.push_back(std::move(spec));

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
	auto &origChar = output->chars.at(lastPos);
	auto c = origChar.charID;
	if (c == ' ' && lastPos > firstInLine) {
		lastPos --;
	}
	if (lastPos < firstInLine) {
		return 0;
	}

	auto a = getAdvancePosition(lastPos);
	auto &lastChar = output->chars.at(lastPos);
	c = lastChar.charID;
	if (isSpecial(c)) {
		if (c == '.' || c == ',') {
			a -= lastChar.advance;
		} else {
			a -= lastChar.advance / 2;
		}
	}
	return a;
}

bool Formatter::pushLine(uint16_t first, uint16_t len, bool forceAlign) {
	if (maxLines && output->lines.size() + 1 == maxLines && forceAlign) {
		pushLineFiller(true);
		return false;
	}

	uint16_t linePos = lineY + currentLineHeight;

	if (len > 0) {
		output->lines.push_back(LineSpec{first, len, linePos, currentLineHeight});
		uint16_t advance = getLineAdvancePos(first + len - 1);
		uint16_t offsetLeft = (advance < (width + lineOffset))?((width + lineOffset) - advance):0;
		if (offsetLeft > 0 && alignment == TextAlign::Right) {
			for (uint16_t i = first; i < first + len; i++) {
				output->chars.at(i).pos += offsetLeft;
			}
		} else if (offsetLeft > 0 && alignment == TextAlign::Center) {
			offsetLeft /= 2;
			for (uint16_t i = first; i < first + len; i++) {
				output->chars.at(i).pos += offsetLeft;
			}
		} else if ((offsetLeft > 0 || (advance > width + lineOffset)) && alignment == TextAlign::Justify && forceAlign && len > 0) {
			int16_t joffset = (advance > width + lineOffset)?(width + lineOffset - advance):offsetLeft;
			uint16_t spacesCount = 0;
			if (first == 0) {
				auto b = checkBullet(first, len);
				first += b;
				len -= b;
			}

			for (uint16_t i = first; i < first + len - 1; i++) {
				auto c = output->chars.at(i).charID;
				if (string::isspace(c) && c != '\n') {
					spacesCount ++;
				}
			}

			int16_t offset = 0;
			for (uint16_t i = first; i < first + len; i++) {
				auto c = output->chars.at(i).charID;
				if (string::isspace(c) && c != '\n' && spacesCount > 0) {
					offset += joffset / spacesCount;
					joffset -= joffset / spacesCount;
					spacesCount --;
				} else {
					output->chars.at(i).pos += offset;
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
	parseFontLineHeight(rangeLineHeight);
	width = defaultWidth;
	if (defaultWidth >= primaryData->getHeight()) {
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

void Formatter::updateLineHeight(uint16_t first, uint16_t last) {
	if (!lineHeightIsAbsolute) {
		bool found = false;
		for (RangeSpec &it : output->ranges) {
			if (it.start <= first && it.start + it.count > first) {
				found = true;
			} else if (it.start > last) {
				break;
			}
			if (found) {
				parseFontLineHeight(it.height);
			}
		}
	}
}

bool Formatter::pushLineBreak() {
	if (output->chars.back().charID == ' ') {
		return true;
	}

	if (firstInLine >= wordWrapPos - 1 && (maxLines != 0 && output->lines.size() + 1 != maxLines)) {
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

			auto &c = output->chars.at(wordEnd);

			c.pos = lineX;
			lineX += c.advance;

			updateLineHeight(wordEnd, charNum);
		}
	} else {
		// we can wrap the word
		auto &c = output->chars.at((wordWrapPos - 1));
		if (!string::isspace(c.charID)) {
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

		uint16_t originOffset = getOriginPosition(wordStart);

		auto &b = output->chars.at((wordStart));
		if (isSpecial(b.charID)) {
			originOffset += b.advance / 2;
		}

		if (originOffset > lineOffset) {
			originOffset -= lineOffset;
		}

		for (uint32_t i = wordStart; i <= wordEnd; i++) {
			output->chars.at(i).pos -= originOffset;
		}
		lineX -= originOffset;
	}
	return true;
}

bool Formatter::readChars(CharReaderUcs2 &r, const Vector<uint8_t> &hyph) {
	size_t wordPos = 0;
	auto hIt = hyph.begin();
	bool startWhitespace = output->chars.empty();
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

		auto kerning = primaryData->kerningAmount(b, c);
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

bool Formatter::read(const FontParameters &f, const TextParameters &s, const WideString &str, uint16_t frontOffset, uint16_t backOffset) {
	return read(f, s, str.c_str(), str.length(), frontOffset, backOffset);
}
bool Formatter::read(const FontParameters &f, const TextParameters &s, const char16_t *str, size_t len, uint16_t frontOffset, uint16_t backOffset) {
	if (!str) {
		return false;
	}

	Arc<FontLayout> primary, secondary;
	clearRead();

	primary = source->getLayout(f);
	if (f.fontVariant == FontVariant::SmallCaps) {
		secondary = source->getLayout(f.getSmallCaps());

		FontCharString primaryStr;
		FontCharString secondaryStr;
		for (size_t i = 0; i < len; ++ i) {
			char16_t ch = str[i];
			if (s.textTransform == TextTransform::Uppercase) {
				ch = string::toupper(ch);
			} else if (s.textTransform == TextTransform::Lowercase) {
				ch = string::tolower(ch);
			}
			if (secondary && ch != string::toupper(ch)) {
				secondaryStr.addChar(string::toupper(ch));
			} else {
				primaryStr.addChar(ch);
			}
		}
		if (_fillerChar) {
			primaryStr.addChar(_fillerChar);
		}
		primaryStr.addChar('-');
		primaryStr.addChar(char16_t(0xAD));
		primary->addString(primaryStr);
		if (secondary) {
			secondary->addString(secondaryStr);
		}
	} else {
		FontCharString primaryStr;
		if (s.textTransform == TextTransform::None) {
			primaryStr.addString(str, len);
		} else {
			for (size_t i = 0; i < len; ++ i) {
				char16_t ch = str[i];
				if (s.textTransform == TextTransform::Uppercase) {
					ch = string::toupper(ch);
				} else if (s.textTransform == TextTransform::Lowercase) {
					ch = string::tolower(ch);
				}
				primaryStr.addChar(ch);
			}
		}
		if (_fillerChar) {
			primaryStr.addChar(_fillerChar);
		}
		primaryStr.addChar('-');
		primaryStr.addChar(char16_t(0xAD));

		primary->addString(primaryStr);
	}

	auto h = primary->getData()->getHeight();

	if (f.fontVariant == FontVariant::SmallCaps && s.textTransform != TextTransform::Uppercase) {
		size_t blockStart = 0;
		size_t blockSize = 0;
		bool caps = false;
		TextParameters capsParams = s;
		capsParams.textTransform = TextTransform::Uppercase;
		for (size_t idx = 0; idx < len; ++idx) {
			const char16_t c = (s.textTransform == TextTransform::None)?str[idx]:string::tolower(str[idx]);
			if (string::toupper(c) != c) { // char can be uppercased - use caps
				if (caps != true) {
					caps = true;
					if (blockSize > 0) {
						readWithRange(RangeSpec{uint32_t(output->chars.size()), 0, primary, Color4B(s.color, s.opacity), h,
							((s.textDecoration == TextDecoration::Underline)?uint8_t(roundf(density)):uint8_t(0)), s.verticalAlign},
								s, str + blockStart, blockSize, frontOffset, backOffset);
					}
					blockStart = idx;
					blockSize = 0;
				}
			} else {
				if (caps != false) {
					caps = false;
					if (blockSize > 0) {
						readWithRange(RangeSpec{uint32_t(output->chars.size()), 0, secondary, Color4B(s.color, s.opacity), h,
							((s.textDecoration == TextDecoration::Underline)?uint8_t(roundf(density)):uint8_t(0)), s.verticalAlign},
								capsParams, str + blockStart, blockSize, frontOffset, backOffset);
					}
					blockStart = idx;
					blockSize = 0;
				}
			}
			++ blockSize;
		}
		if (blockSize > 0) {
			if (caps) {
				return readWithRange(RangeSpec{uint32_t(output->chars.size()), 0, secondary, Color4B(s.color, s.opacity), h,
					((s.textDecoration == TextDecoration::Underline)?uint8_t(roundf(density)):uint8_t(0)), s.verticalAlign},
						capsParams, str + blockStart, blockSize, frontOffset, backOffset);
			} else {
				return readWithRange(RangeSpec{uint32_t(output->chars.size()), 0, primary, Color4B(s.color, s.opacity), h,
					((s.textDecoration == TextDecoration::Underline)?uint8_t(roundf(density)):uint8_t(0)), s.verticalAlign},
						s, str + blockStart, blockSize, frontOffset, backOffset);
			}
		}
	} else {
		return readWithRange(RangeSpec{uint32_t(output->chars.size()), 0, primary, Color4B(s.color, s.opacity), h,
			((s.textDecoration == TextDecoration::Underline)?uint8_t(roundf(density)):uint8_t(0)), s.verticalAlign},
				s, str, len, frontOffset, backOffset);
	}

	return true;
}

bool Formatter::read(const FontParameters &f, const TextParameters &s, uint16_t blockWidth, uint16_t blockHeight) {
	clearRead();

	auto primary = source->getLayout(f);

	return readWithRange(RangeSpec{uint32_t(output->chars.size()), 0, primary, Color4B(s.color, s.opacity), blockHeight,
		((s.textDecoration == TextDecoration::Underline)?uint8_t(roundf(density)):uint8_t(0)), s.verticalAlign},
			s, blockWidth, blockHeight);
}

void Formatter::clearRead() {
	primaryFont = nullptr;
	primaryData = nullptr;
}

bool Formatter::readWithRange(RangeSpec && range, const TextParameters &s, const char16_t *str, size_t len, uint16_t frontOffset, uint16_t backOffset) {
	primaryFont = range.layout;
	primaryData = primaryFont->getData();
	rangeLineHeight = range.height;

	charPosition = 0;
	if (bufferedSpace) {
		pushSpace();
		bufferedSpace = false;
	}

	parseFontLineHeight(rangeLineHeight);

	textStyle = s;
	parseWhiteSpace(textStyle.whiteSpace);
	updatePosition(lineY, currentLineHeight);

	if (!output->chars.empty() && output->chars.back().charID == ' ' && collapseSpaces) {
		while (len > 0 && ((string::isspace(str[0]) && str[0] != 0x00A0) || str[0] < 0x20)) {
			len --; str ++;
		}
	}

	b = 0;

	lineX += frontOffset;
	charPosition = 0;
	CharReaderUcs2 r(str, len);
	if (textStyle.hyphens == Hyphens::Auto && _hyphens) {
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

	range.count = uint32_t(output->chars.size() - range.start);
	if (range.count > 0) {
		output->ranges.emplace_back(std::move(range));
	}
	lineX += backOffset;

	return true;
}
bool Formatter::readWithRange(RangeSpec &&range, const TextParameters &s, uint16_t blockWidth, uint16_t blockHeight) {
	primaryFont = range.layout;
	primaryData = primaryFont->getData();
	rangeLineHeight = range.height;

	charPosition = 0;
	if (bufferedSpace) {
		pushSpace();
		bufferedSpace = false;
	}

	parseFontLineHeight(rangeLineHeight);

	textStyle = s;
	parseWhiteSpace(textStyle.whiteSpace);

	if (maxWidth && lineX + blockWidth > maxWidth) {
		pushLineFiller(false);
		return false;
	}

	if (width + lineOffset > 0) {
		if (lineX + blockWidth > width + lineOffset) {
			if (!pushLine(true)) {
				return false;
			}
		}
	}

	if (currentLineHeight < blockHeight) {
		currentLineHeight = blockHeight;
	}

	updatePosition(lineY, currentLineHeight);

	if (charNum == firstInLine && lineOffset > 0) {
		lineX += lineOffset;
	}

	CharSpec spec{char16_t(0xFFFF), lineX, blockWidth, CharSpec::Display::Block};
	lineX += spec.advance;
	charNum ++;
	output->chars.push_back(std::move(spec));

	if (width + lineOffset > 0 && lineX > width + lineOffset) {
		if (!pushLineBreak()) {
			return false;
		}
	}

	range.count = uint32_t(output->chars.size() - range.start);
	output->ranges.emplace_back(std::move(range));

	return true;
}

void Formatter::finalize() {
	if (firstInLine < charNum) {
		pushLine(false);
	}

	auto chars = output->chars.size();
	if (chars > 0 && output->ranges.size() > 0 && output->lines.size() > 0) {
		auto &lastRange = output->ranges.back();
		auto &lastLine = output->lines.back();

		if (lastLine.start + lastLine.count != chars) {
			lastLine.count = uint32_t(chars - lastLine.start);
		}

		if (lastRange.start + lastRange.count != chars) {
			lastRange.count = uint32_t(chars - lastRange.start);
		}
	}

	output->width = getWidth();
	output->height = getHeight();
	output->maxLineX = getMaxLineX();
}

void Formatter::reset(FormatSpec *o) {
	output = o;

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


FormatSpec::FormatSpec() { }

FormatSpec::FormatSpec(size_t res) {
	chars.reserve(res);
	lines.reserve(res / 60);
	ranges.reserve(1);
}

FormatSpec::FormatSpec(size_t res, size_t rang) {
	chars.reserve(res);
	lines.reserve(res / 60);
	ranges.reserve(rang);
}

bool FormatSpec::init(size_t res, size_t rang) {
	chars.reserve(res);
	lines.reserve(res / 60);
	ranges.reserve(rang);
	return true;
}

void FormatSpec::reserve(size_t res, size_t rang) {
	chars.reserve(res);
	lines.reserve(res / 60);
	ranges.reserve(rang);
}

void FormatSpec::clear() {
	chars.clear();
	lines.clear();
	ranges.clear();
}

const LineSpec *FormatSpec::getLine(size_t idx) const {
	const LineSpec *ret = nullptr;
	for (const LineSpec &it : lines) {
		if (it.start <= idx && it.start + it.count > idx) {
			ret = &it;
		}
	}
	return ret;
}

FormatSpec::RangeLineIterator FormatSpec::begin() const {
	return RangeLineIterator{ranges.begin(), lines.begin()};
}

FormatSpec::RangeLineIterator FormatSpec::end() const {
	return RangeLineIterator{ranges.end(), lines.end()};
}

bool HyphenMap::init() {
	return true;
}

void HyphenMap::addHyphenDict(CharGroupId id, const String &file) {
	auto data = filesystem::readTextFile(file);
	if (!data.empty()) {
		auto dict = hnj_hyphen_load_data(data.data(), data.size());
		if (dict) {
			auto it = _dicts.find(id);
			if (it == _dicts.end()) {
				_dicts.emplace(id, dict);
			} else {
				hnj_hyphen_free(it->second);
				it->second = dict;
			}
		}
	}
}
Vector<uint8_t> HyphenMap::makeWordHyphens(const char16_t *ptr, size_t len) {
	if (len < 4 || len >= 255) {
		return Vector<uint8_t>();
	}

	HyphenDict *dict = nullptr;
	for (auto &it : _dicts) {
		if (inCharGroup(it.first, ptr[0])) {
			dict = it.second;
			break;
		}
	}

	if (!dict) {
		return Vector<uint8_t>();
	}

	String word = convertWord(dict, ptr, len);
	if (!word.empty()) {
		Vector<char> buf; buf.resize(word.size() + 5);

		char ** rep = nullptr;
		int * pos = nullptr;
		int * cut = nullptr;
		hnj_hyphen_hyphenate2(dict, word.data(), int(word.size()), buf.data(), nullptr, &rep, &pos, &cut);

		Vector<uint8_t> ret;
		uint8_t i = 0;
		for (auto &it : buf) {
			if (it > 0) {
				if ((it - '0') % 2 == 1) {
					ret.push_back(i + 1);
				}
			} else {
				break;
			}
			++ i;
		}
		return ret;
	}
	return Vector<uint8_t>();
}
Vector<uint8_t> HyphenMap::makeWordHyphens(const CharReaderUcs2 &r) {
	return makeWordHyphens(r.data(), r.size());
}
void HyphenMap::purgeHyphenDicts() {
	for (auto &it : _dicts) {
		hnj_hyphen_free(it.second);
	}
}

String HyphenMap::convertWord(HyphenDict *dict, const char16_t *ptr, size_t len) {
	if (dict->utf8) {
		return string::toUtf8(ptr, len);
	} else {
		if (strcmp("KOI8-R", dict->cset) == 0) {
			return string::toKoi8r(ptr, len);
		}

		return String();
	}
}

NS_SP_EXT_END(font)
