// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
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

#include "SPLayout.h"
#include "SLFontFormatter.h"
#include "SPFilesystem.h"
#include "SPString.h"
#include "hyphen.h"

NS_LAYOUT_BEGIN

Formatter::Formatter(): source(nullptr), output(nullptr), density(1.0f) { }

Formatter::Formatter(FontSource *s, FormatSpec *o, float density)
: source(s), output(o), density(density) { }

void Formatter::init(FontSource *s, FormatSpec *o, float d) {
	reset(s, o, d);
}

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
void Formatter::setEmplaceAllChars(bool value) {
	emplaceAllChars = value;
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
		width = std::min(width, defaultWidth);

		while (width < primaryData->getHeight()) {
			linePos += lineHeight;
			std::tie(lineOffset, width) = linePositionFunc(linePos, height, density);
			width = std::min(width, defaultWidth);
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

uint16_t Formatter::getAdvance(const CharSpec &ch) const {
	return ch.advance;
}

uint16_t Formatter::getAdvance(uint16_t pos) const {
	if (pos < output->chars.size()) {
		return getAdvance(output->chars.at(pos));
	} else {
		return 0;
	}
}

inline uint16_t Formatter::getAdvancePosition(const CharSpec &ch) const {
	return ch.pos + ch.advance;
}

inline uint16_t Formatter::getAdvancePosition(uint16_t pos) const {
	return (pos < output->chars.size())?getAdvancePosition(output->chars.at(pos)):(uint16_t)0;
}

inline uint16_t Formatter::getOriginPosition(const CharSpec &ch) const {
	return ch.pos;
}

inline uint16_t Formatter::getOriginPosition(uint16_t pos) const {
	return (pos < output->chars.size())?getOriginPosition(output->chars.at(pos)):(uint16_t)0;
}

bool Formatter::isSpecial(char16_t ch) const {
	 // collapseSpaces can be disabled for manual optical alignment
	if (!opticalAlignment || !collapseSpaces) {
		return false;
	}
	return chars::CharGroup<char16_t, CharGroupId::OpticalAlignmentSpecial>::match(ch);
}

uint16_t Formatter::checkBullet(uint16_t first, uint16_t len) const {
	 // collapseSpaces can be disabled for manual optical alignment
	if (!opticalAlignment || !collapseSpaces) {
		return 0;
	}

	uint16_t offset = 0;
	for (uint16_t i = first; i < first + len - 1; i++) {
		auto ch = output->chars.at(i).charID;
		if (chars::CharGroup<char16_t, CharGroupId::OpticalAlignmentBullet>::match(ch)) {
			offset ++;
		} else if (string::isspace(ch) && offset >= 1) {
			return offset + 1;
		} else {
			break;
		}
	}

	return 0;
}

void Formatter::pushLineFiller(bool replaceLastChar) {
	output->overflow = true;
	if (_fillerChar == 0) {
		return;
	}
	auto charDef = primaryData->getChar(_fillerChar);
	if (!charDef) {
		return;
	}

	if (replaceLastChar && !output->chars.empty()) {
		auto &bc = output->chars.back();
		bc.charID = _fillerChar;
		bc.advance = charDef.xAdvance;
	} else {
		output->chars.emplace_back(CharSpec{_fillerChar, lineX, charDef.xAdvance});
		charNum ++;
	}
}

bool Formatter::pushChar(char16_t ch) {
	if (textStyle.textTransform == TextTransform::Uppercase) {
		ch = string::toupper(ch);
	} else if (textStyle.textTransform == TextTransform::Lowercase) {
		ch = string::tolower(ch);
	}

	CharLayout charDef = primaryData->getChar(ch);

	if (charDef.charID == 0) {
		if (ch == (char16_t)0x00AD) {
			charDef = primaryData->getChar('-');
		} else {
			log::format("RichTextFormatter", "%s: Attempted to use undefined character: %d '%s'", primaryFont->getName().c_str(), ch, string::toUtf8(ch).c_str());
			return true;
		}
	}

	if (charNum == firstInLine && lineOffset > 0) {
		lineX += lineOffset;
	}

	auto posX = lineX;

	CharSpec spec{ch, posX, charDef.xAdvance};

	if (ch == (char16_t)0x00AD) {
		if (textStyle.hyphens == Hyphens::Manual || textStyle.hyphens == Hyphens::Auto) {
			wordWrapPos = charNum + 1;
		}
	} else if (ch == u'-' || ch == u'+' || ch == u'*' || ch == u'/' || ch == u'\\') {
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
		if (charNum == firstInLine && isSpecial(ch)) {
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

bool Formatter::pushTab() {
	CharLayout charDef = primaryData->getChar(' ');

	auto posX = lineX;
	auto tabPos = (lineX + charDef.xAdvance) / (charDef.xAdvance * 4) + 1;
	lineX = tabPos * charDef.xAdvance * 4;

	charNum ++;
	output->chars.push_back(CharSpec{char16_t('\t'), posX, uint16_t(lineX - posX)});
	if (wordWrap) {
		wordWrapPos = charNum;
	}

	return true;
}

uint16_t Formatter::getLineAdvancePos(uint16_t lastPos) {
	auto &origChar = output->chars.at(lastPos);
	auto ch = origChar.charID;
	if (ch == ' ' && lastPos > firstInLine) {
		lastPos --;
	}
	if (lastPos < firstInLine) {
		return 0;
	}

	auto a = getAdvancePosition(lastPos);
	auto &lastChar = output->chars.at(lastPos);
	ch = lastChar.charID;
	if (isSpecial(ch)) {
		if (ch == '.' || ch == ',') {
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
				auto bc = checkBullet(first, len);
				first += bc;
				len -= bc;
			}

			for (uint16_t i = first; i < first + len - 1; i++) {
				auto ch = output->chars.at(i).charID;
				if (string::isspace(ch) && ch != '\n') {
					spacesCount ++;
				}
			}

			int16_t offset = 0;
			for (uint16_t i = first; i < first + len; i++) {
				auto ch = output->chars.at(i).charID;
				if (string::isspace(ch) && ch != '\n' && spacesCount > 0) {
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
	currentLineHeight = min(rangeLineHeight, lineHeight);
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
	if (chars::CharGroup<char16_t, CharGroupId::WhiteSpace>::match(output->chars.back().charID)) {
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

			auto &ch = output->chars.at(wordEnd);

			ch.pos = lineX;
			lineX += ch.advance;

			updateLineHeight(wordEnd, charNum);
		}
	} else {
		// we can wrap the word
		auto &ch = output->chars.at((wordWrapPos - 1));
		if (!string::isspace(ch.charID)) {
			if (!pushLine(firstInLine, (wordWrapPos) - firstInLine, true)) {
				return false;
			}
		} else {
			if (!pushLine(firstInLine, (wordWrapPos > firstInLine)?((wordWrapPos) - firstInLine):0, true)) {
				return false;
			}
		}
		firstInLine = wordStart;

		uint16_t originOffset = getOriginPosition(wordStart);

		auto &bc = output->chars.at((wordStart));
		if (isSpecial(bc.charID)) {
			originOffset += bc.advance / 2;
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

bool Formatter::pushLineBreakChar() {
	charNum ++;
	output->chars.push_back(CharSpec{char16_t(0x0A), lineX, 0});

	if (!pushLine(false)) {
		return false;
	}
	lineX = 0;

	return true;
}

bool Formatter::readChars(WideStringView &r, const Vector<uint8_t> &hyph) {
	size_t wordPos = 0;
	auto hIt = hyph.begin();
	bool startWhitespace = output->chars.empty();
	for (c = r[0]; !r.empty(); ++charPosition, ++wordPos, ++r, c = r[0]) {
		if (hIt != hyph.end() && wordPos == *hIt) {
			pushChar((char16_t)0x00AD);
			++ hIt;
		}

		if (c == char16_t('\n')) {
			if (preserveLineBreaks) {
				if (!pushLineBreakChar()) {
					return false;
				}
			} else if (collapseSpaces) {
				if (!startWhitespace) {
					bufferedSpace = true;
				}
			}
			b = 0;
			continue;
		}

		if (c == char16_t('\t') && !collapseSpaces) {
			if (!pushTab()) {
				return false;
			}
			continue;
		}

		if (c < char16_t(0x20)) {
			if (emplaceAllChars) {
				charNum ++;
				output->chars.push_back(CharSpec{char16_t(0xFFFF), lineX, 0});
			}
			continue;
		}

		if (c != char16_t(0x00A0) && string::isspace(c) && collapseSpaces) {
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
						readWithRange(RangeSpec{false, false,
							((s.textDecoration == TextDecoration::Underline)?uint8_t(roundf(density)):uint8_t(0)), s.verticalAlign,
							uint32_t(output->chars.size()), 0, Color4B(s.color, s.opacity), h, primary},
								s, str + blockStart, blockSize, frontOffset, backOffset);
					}
					blockStart = idx;
					blockSize = 0;
				}
			} else {
				if (caps != false) {
					caps = false;
					if (blockSize > 0) {
						readWithRange(RangeSpec{false, false,
							((s.textDecoration == TextDecoration::Underline)?uint8_t(roundf(density)):uint8_t(0)), s.verticalAlign,
							uint32_t(output->chars.size()), 0, Color4B(s.color, s.opacity), h, secondary},
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
				return readWithRange(RangeSpec{false, false,
					((s.textDecoration == TextDecoration::Underline)?uint8_t(roundf(density)):uint8_t(0)), s.verticalAlign,
					uint32_t(output->chars.size()), 0, Color4B(s.color, s.opacity), h, secondary},
						capsParams, str + blockStart, blockSize, frontOffset, backOffset);
			} else {
				return readWithRange(RangeSpec{false, false,
					((s.textDecoration == TextDecoration::Underline)?uint8_t(roundf(density)):uint8_t(0)), s.verticalAlign,
					uint32_t(output->chars.size()), 0, Color4B(s.color, s.opacity), h, primary},
						s, str + blockStart, blockSize, frontOffset, backOffset);
			}
		}
	} else {
		return readWithRange(RangeSpec{false, false,
			((s.textDecoration == TextDecoration::Underline)?uint8_t(roundf(density)):uint8_t(0)), s.verticalAlign,
			uint32_t(output->chars.size()), 0, Color4B(s.color, s.opacity), h, primary},
				s, str, len, frontOffset, backOffset);
	}

	return true;
}

bool Formatter::read(const FontParameters &f, const TextParameters &s, uint16_t blockWidth, uint16_t blockHeight) {
	clearRead();

	auto primary = source->getLayout(f);

	return readWithRange(RangeSpec{false, false,
		((s.textDecoration == TextDecoration::Underline)?uint8_t(roundf(density)):uint8_t(0)), s.verticalAlign,
		uint32_t(output->chars.size()), 0, Color4B(s.color, s.opacity), blockHeight, primary},
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
	WideStringView r(str, len);
	if (textStyle.hyphens == Hyphens::Auto && _hyphens) {
		while (!r.empty()) {
			WideStringView tmp = r.readUntil<WideStringView::CharGroup<CharGroupId::Latin>,
					WideStringView::CharGroup<CharGroupId::Cyrillic>>();
			if (!tmp.empty()) {
				readChars(tmp);
			}
			tmp = r.readChars<WideStringView::CharGroup<CharGroupId::Latin>,
					WideStringView::CharGroup<CharGroupId::Cyrillic>>();
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
			lineX = 0;
		}
	}

	parseFontLineHeight(rangeLineHeight);
	if (currentLineHeight < blockHeight) {
		currentLineHeight = blockHeight;
	}

	updatePosition(lineY, currentLineHeight);

	if (charNum == firstInLine && lineOffset > 0) {
		lineX += lineOffset;
	}

	CharSpec spec{char16_t(0xFFFF), lineX, blockWidth};
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

	if (!output->chars.empty() && output->chars.back().charID == char16_t(0x0A)) {
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
	rangeLineHeight = 0;

	lineHeightMod = 1.0f;
	lineHeightIsAbsolute = false;

	firstInLine = 0;
	wordWrapPos = 0;

	bufferedSpace = false;
}

void Formatter::reset(FontSource *s, FormatSpec *o, float d) {
	source = s;
	density = d;
	reset(o);
}

uint16_t Formatter::getHeight() const {
	return lineY;
}
uint16_t Formatter::getWidth() const {
	return std::max(maxLineX, width);
}
uint16_t Formatter::getMaxLineX() const {
	return maxLineX;
}
uint16_t Formatter::getLineHeight() const {
	return lineHeight;
}

FormatSpec *Formatter::getOutput() const {
	return output;
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
	overflow = false;
}

inline static bool isSpaceOrLineBreak(char16_t c) {
	return c == char16_t(0x0A) || string::isspace(c);
}

Pair<uint32_t, FormatSpec::SelectMode> FormatSpec::getChar(int32_t x, int32_t y, SelectMode mode) const {
	int32_t yDistance = maxOf<int32_t>();
	const LineSpec *pLine = nullptr;
	if (!lines.empty()) {
		for (auto &l : lines) {
			int32_t dst = maxOf<int32_t>();
			switch (mode) {
			case Center:
				dst = abs(y - (l.pos - l.height / 2));
				break;
			case Best:
				dst = abs(y - (l.pos - l.height * 3 / 4));
				break;
			case Prefix:
			case Suffix:
				dst = abs(y - (l.pos - l.height));
				break;
			};
			if (dst < yDistance) {
				pLine = &l;
				yDistance = dst;
			} else {
				break;
			}
		}

		if (chars.back().charID == char16_t(0x0A) && pLine == &lines.back() && (mode == Best || mode == Suffix)) {
			int32_t dst = maxOf<int32_t>();
			switch (mode) {
			case Center:
				dst = abs(y - (height - pLine->height / 2));
				break;
			case Best:
				dst = abs(y - (height - pLine->height * 3 / 4));
				break;
			case Prefix:
			case Suffix:
				dst = abs(y - (height - pLine->height));
				break;
			};
			if (dst < yDistance) {
				return pair(chars.size() - 1, Suffix);
			}
		}
	}

	if (!pLine || yDistance > pLine->height * 3 / 2) {
		return pair(maxOf<uint32_t>(), mode);
	}

	SelectMode nextMode = mode;
	int32_t xDistance = maxOf<int32_t>();
	const CharSpec *pChar = nullptr;
	uint32_t charNumber = pLine->start;
	for (uint32_t i = pLine->start; i < pLine->start + pLine->count; ++ i) {
		auto &c = chars[i];
		if (c.charID != char16_t(0xAD) && !isSpaceOrLineBreak(c.charID)) {
			int32_t dst = maxOf<int32_t>();
			SelectMode dstMode = mode;
			switch (mode) {
			case Center: dst = abs(x - (c.pos + c.advance / 2)); break;
			case Prefix: dst = abs(x - c.pos); break;
			case Suffix: dst = abs(x - (c.pos + c.advance)); break;
			case Best: {
				int32_t prefixDst = abs(x - c.pos);
				int32_t suffixDst = abs(x - (c.pos + c.advance));
				if (prefixDst <= suffixDst) {
					dst = prefixDst; dstMode = Prefix;
				} else {
					dst = suffixDst; dstMode = Suffix;
				}
			} break;
			};
			if (dst < xDistance) {
				pChar = &c;
				xDistance = dst;
				charNumber = i;
				nextMode = dstMode;
			} else {
				break;
			}
		}
	}
	if (pLine->count && chars[pLine->start + pLine->count - 1].charID == char16_t(0x0A)) {
		auto &c = chars[pLine->start + pLine->count - 1];
		int32_t dst = maxOf<int32_t>();
		switch (mode) {
		case Prefix: dst = abs(x - c.pos); break;
		case Best: dst = abs(x - c.pos); break;
		default: break;
		};
		if (dst < xDistance) {
			pChar = &c;
			xDistance = dst;
			charNumber = pLine->start + pLine->count - 1;
			nextMode = Prefix;
		}
	}

	if ((mode == Best || mode == Suffix) && pLine == &(lines.back())) {
		auto c = chars.back();
		int32_t dst = abs(x - (c.pos + c.advance));
		if (dst < xDistance) {
			pChar = &c;
			xDistance = dst;
			charNumber = uint32_t(chars.size() - 1);
			nextMode = Suffix;
		}
	}

	if (!pChar) {
		return pair(maxOf<uint32_t>(), mode);
	}

	return pair(charNumber, nextMode);
}

const LineSpec *FormatSpec::getLine(uint32_t idx) const {
	const LineSpec *ret = nullptr;
	for (const LineSpec &it : lines) {
		if (it.start <= idx && it.start + it.count > idx) {
			ret = &it;
			break;
		}
	}
	return ret;
}
uint32_t FormatSpec::getLineNumber(uint32_t id) const {
	uint16_t n = 0;
	for (auto &it : lines) {
		if (id >= it.start && id < it.start + it.count) {
			return n;
		}
		n++;
	}
	if (n >= lines.size()) {
		n = lines.size() - 1;
	}
	return n;
}

FormatSpec::RangeLineIterator FormatSpec::begin() const {
	return RangeLineIterator{ranges.begin(), lines.begin()};
}

FormatSpec::RangeLineIterator FormatSpec::end() const {
	return RangeLineIterator{ranges.end(), lines.end()};
}

WideString FormatSpec::str(bool filter) const {
	WideString ret; ret.reserve(chars.size());
	for (auto it = begin(); it != end(); ++ it) {
		const RangeSpec &range = *it.range;
		if (!filter || range.align == VerticalAlign::Baseline) {
			size_t end = it.start() + it.count() - 1;
			for (size_t i = it.start(); i <= end; ++ i) {
				const auto &spec = chars[i];
				if (spec.charID != char16_t(0xAD) && spec.charID != char16_t(0xFFFF)) {
					ret.push_back(spec.charID);
				}
			}
		}
	}
	return ret;
}

WideString FormatSpec::str(uint32_t s_start, uint32_t s_end, size_t maxWords, bool ellipsis, bool filter) const {
	WideString ret; ret.reserve(s_end - s_start + 2);
	if (ellipsis && s_start > 0) {
		ret.push_back(u'…');
		ret.push_back(u' ');
	}
	auto it = begin();
	while (it.end() < s_start) {
		++ it;
	}

	size_t counter = 0;
	for (; it != end(); ++ it) {
		const RangeSpec &range = *it.range;
		if (!filter || range.align == VerticalAlign::Baseline) {
			size_t _end = std::min(it.end() - 1, s_end);
			size_t _start = std::max(it.start(), s_start);
			if (maxWords != maxOf<size_t>()) {
				for (size_t i = _start; i <= _end; ++ i) {
					const auto &spec = chars[i];
					if (spec.charID != char16_t(0xAD) && spec.charID != char16_t(0xFFFF)) {
						ret.push_back(spec.charID);
						if (string::isspace(spec.charID)) {
							++ counter;
							if (counter >= maxWords) {
								break;
							}
						}
					}
				}
				if (counter >= maxWords) {
					if (ellipsis) {
						ret.push_back(u'…');
					}
					break;
				}
			} else {
				for (size_t i = _start; i <= _end; ++ i) {
					const auto &spec = chars[i];
					if (spec.charID != char16_t(0xAD) && spec.charID != char16_t(0xFFFF)) {
						ret.push_back(spec.charID);
					}
				}
			}
			if (it.end() >= s_end) {
				if (s_end < chars.size()  - 1 && ellipsis) {
					ret.push_back(u' ');
					ret.push_back(u'…');
				}
				break;
			}
		}
	}

	return ret;
}

Pair<uint32_t, uint32_t> FormatSpec::selectWord(uint32_t origin) const {
	Pair<uint32_t, uint32_t> ret(origin, origin);
	while (ret.second + 1 < chars.size() && !isSpaceOrLineBreak(chars[ret.second + 1].charID)) {
		++ ret.second;
	}
	while (ret.first > 0 && !isSpaceOrLineBreak(chars[ret.first - 1].charID)) {
		-- ret.first;
	}
	return ret;
}

uint32_t FormatSpec::selectChar(int32_t x, int32_t y, SelectMode mode) const {
	return getChar(x, y, mode).first;
}

HyphenMap::~HyphenMap() {
	for (auto &it : _dicts) {
		hnj_hyphen_free(it.second);
	}
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
Vector<uint8_t> HyphenMap::makeWordHyphens(const WideStringView &r) {
	return makeWordHyphens(r.data(), r.size());
}
void HyphenMap::purgeHyphenDicts() {
	for (auto &it : _dicts) {
		hnj_hyphen_free(it.second);
	}
}

String HyphenMap::convertWord(HyphenDict *dict, const char16_t *ptr, size_t len) {
	if (dict->utf8) {
		return string::toUtf8(WideStringView(ptr, len));
	} else {
		if (strcmp("KOI8-R", dict->cset) == 0) {
			return string::toKoi8r(WideStringView(ptr, len));
		}

		return String();
	}
}

static Rect getLabelLineStartRect(const FormatSpec &f, uint16_t lineId, float density, uint32_t c) {
	Rect rect;
	const LineSpec &line = f.lines.at(lineId);
	if (line.count > 0) {
		const CharSpec & firstChar = f.chars.at(std::max(line.start, c));
		const CharSpec & lastChar = f.chars.at(line.start + line.count - 1);
		rect.origin = Vec2((firstChar.pos) / density, (line.pos) / density - line.height / density);
		rect.size = Size((lastChar.pos + lastChar.advance - firstChar.pos) / density, line.height / density);
	}

	return rect;
}

static Rect getLabelLineEndRect(const FormatSpec &f, uint16_t lineId, float density, uint32_t c) {
	Rect rect;
	const LineSpec &line = f.lines.at(lineId);
	if (line.count > 0) {
		const CharSpec & firstChar = f.chars.at(line.start);
		const CharSpec & lastChar = f.chars.at(std::min(line.start + line.count - 1, c));
		rect.origin = Vec2((firstChar.pos) / density, (line.pos) / density - line.height / density);
		rect.size = Size((lastChar.pos + lastChar.advance - firstChar.pos) / density, line.height / density);
	}
	return rect;
}

static Rect getCharsRect(const FormatSpec &f, uint32_t lineId, uint32_t firstCharId, uint32_t lastCharId, float density) {
	Rect rect;
	const LineSpec & line = f.lines.at(lineId);
	const CharSpec & firstChar = f.chars.at(firstCharId);
	const CharSpec & lastChar = f.chars.at(lastCharId);
	rect.origin = Vec2((firstChar.pos) / density, (line.pos) / density - line.height / density);
	rect.size = Size((lastChar.pos + lastChar.advance - firstChar.pos) / density, line.height / density);
	return rect;
}

Rect FormatSpec::getLineRect(uint32_t lineId, float density, const Vec2 &origin) const {
	if (lineId >= lines.size()) {
		return Rect::ZERO;
	}
	return getLineRect(lines[lineId], density, origin);
}

Rect FormatSpec::getLineRect(const LineSpec &line, float density, const Vec2 &origin) const {
	Rect rect;
	if (line.count > 0) {
		const CharSpec & firstChar = chars.at(line.start);
		const CharSpec & lastChar = chars.at(line.start + line.count - 1);
		rect.origin = Vec2((firstChar.pos) / density + origin.x, (line.pos) / density - line.height / density + origin.y);
		rect.size = Size((lastChar.pos + lastChar.advance - firstChar.pos) / density, line.height / density);
	}
	return rect;
}

Vector<Rect> FormatSpec::getLabelRects(uint32_t firstCharId, uint32_t lastCharId, float density, const Vec2 &origin, const Padding &p) const {
	Vector<Rect> ret;
	getLabelRects(ret, firstCharId, lastCharId, density, origin, p);
	return ret;
}

void FormatSpec::getLabelRects(Vector<Rect> &ret, uint32_t firstCharId, uint32_t lastCharId, float density, const Vec2 &origin, const Padding &p) const {
	auto firstLine = getLineNumber(firstCharId);
	auto lastLine = getLineNumber(lastCharId);

	if (firstLine == lastLine) {
		auto rect = getCharsRect(*this, firstLine, firstCharId, lastCharId, density);
		rect.origin.x += origin.x - p.left;
		rect.origin.y += origin.y - p.top;
		rect.size.width += p.left + p.right;
		rect.size.height += p.bottom + p.top;
		if (!rect.equals(Rect::ZERO)) {
			ret.push_back(rect);
		}
	} else {
		auto first = getLabelLineStartRect(*this, firstLine, density, firstCharId);
		if (!first.equals(Rect::ZERO)) {
			first.origin.x += origin.x;
			first.origin.y += origin.y;
			if (first.origin.x - p.left < 0.0f) {
				first.size.width += (first.origin.x);
				first.origin.x = 0.0f;
			} else {
				first.origin.x -= p.left;
				first.size.width += p.left;
			}
			first.origin.y -= p.top;
			first.size.height += p.bottom + p.top;
			ret.push_back(first);
		}

		for (auto i = firstLine + 1; i < lastLine; i++) {
			auto rect = getLineRect(i, density);
			rect.origin.x += origin.x;
			rect.origin.y += origin.y - p.top;
			rect.size.height += p.bottom + p.top;
			if (!rect.equals(Rect::ZERO)) {
				ret.push_back(rect);
			}
		}

		auto last = getLabelLineEndRect(*this, lastLine, density, lastCharId);
		if (!last.equals(Rect::ZERO)) {
			last.origin.x += origin.x;
			last.origin.y += origin.y - p.top;
			last.size.width += p.right;
			last.size.height += p.bottom + p.top;
			ret.push_back(last);
		}
	}
}

NS_LAYOUT_END
