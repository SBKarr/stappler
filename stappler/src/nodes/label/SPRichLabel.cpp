/*
 * SPRichLabel.cpp
 *
 *  Created on: 08 мая 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPRichLabel.h"
#include "SPString.h"
#include "SPLocale.h"
#include "SPRichTextFormatter.h"
#include "SPGLProgramSet.h"
#include "SPLayer.h"

#include "renderer/CCGLProgram.h"
#include "renderer/CCGLProgramState.h"
#include "renderer/CCGLProgramCache.h"

NS_SP_BEGIN

float RichLabel::getStringWidth(Font *font, const std::string &str, float density) {
	if (!font || str.empty()) {
		return 0.0f;
	}

	if (density == 0.0f) {
		density = screen::density();
	}

	std::u16string u16str = string::toUtf16(str);
	CharVec vec;
	vec.reserve(u16str.length());

	rich_text::style::TextLayoutParameters baseParams;
	baseParams.whiteSpace = rich_text::style::WhiteSpace::Pre;

	rich_text::Formatter fmt(font->getFontSet(), &vec, nullptr, density);
	fmt.begin(0, 0);
	fmt.read(font, baseParams, u16str);
	fmt.finalize();

	return fmt.getWidth() / density;
}

RichLabel::Style::Value::Value() {
	memset(this, 0, sizeof(Value));
}

void RichLabel::Style::set(const Param &p, bool force) {
	if (force) {
		auto it = params.begin();
		while(it != params.end()) {
			if (it->name == p.name) {
				it = params.erase(it);
			} else {
				it ++;
			}
		}
	}
	params.push_back(p);
}
void RichLabel::Style::merge(const Style &style) {
	for (auto &it : style.params) {
		set(it, true);
	}
}

void RichLabel::Style::clear() {
	params.clear();
}

rich_text::TextStyle RichLabel::getDefaultStyle() {
	rich_text::TextStyle ret;

	ret.whiteSpace = rich_text::style::WhiteSpace::PreWrap;
	ret.textDecoration = Style::TextDecoration::None;
	ret.textTransform = Style::TextTransform::None;
	ret.verticalAlign = Style::VerticalAlign::Baseline;
	ret.hyphens = Style::Hyphens::Manual;

	return ret;
}

cocos2d::Size RichLabel::getLabelSize(Font *f, const std::string &s, float w, Alignment a, float density, const rich_text::TextStyle &style) {
	if (s.empty()) {
		return cocos2d::Size(0.0f, 0.0f);
	}
	return getLabelSize(f, string::toUtf16(s), w, a, density, style);
}

cocos2d::Size RichLabel::getLabelSize(Font *f, const std::u16string &s, float w, Alignment a, float density, const rich_text::TextStyle &style) {
	if (density == 0) {
		density = screen::density();
	}

	if (s.empty()) {
		return cocos2d::Size(0.0f, 0.0f);
	}

	std::vector< Font::CharSpec > chars;
	rich_text::Formatter fmt(f->getFontSet(), &chars, nullptr, density);
	fmt.setWidth((uint16_t)roundf(w * density));
	fmt.setTextAlignment(a);

	fmt.begin(0);
	fmt.read(f, style, s);
	fmt.finalize();

	return cocos2d::Size(fmt.getWidth() / density, fmt.getHeight() / density);
}

RichLabel::~RichLabel() { }

bool RichLabel::init(Font *font, const std::string &str, float width, Alignment alignment, float density) {
	cocos2d::Texture2D *tex = nullptr;
	FontSet *set = nullptr;
	if (font) {
		tex = font->getTexture();
	    set = font->getFontSet();
	}

    if (!DynamicBatchNode::init(tex, density)) {
    	return false;
    }

    _baseFont = font;
    if (set) {
    	_fontSet = set;
    }

    setNormalized(true);

    setString(str);
    setWidth(width);
    setAlignment(alignment);

    updateLabel();

    return true;
}

void RichLabel::updateLabel() {
	if (!_baseFont || !_texture) {
		_quads.clear();
		return;
	}

	_compiledStyles = compileStyle();

	rich_text::style::TextLayoutParameters baseParams;
	baseParams.color = _displayedColor;
	baseParams.opacity = _displayedOpacity;
	baseParams.whiteSpace = rich_text::style::WhiteSpace::PreWrap;

	baseParams.textDecoration = _textDecoration;
	baseParams.textTransform = _textTransform;
	baseParams.verticalAlign = _verticalAlign;
	baseParams.hyphens = _hyphens;

	Font *baseFont = _baseFont;

	_chars.clear();
	_lines.clear();
	_positions.clear();

	_chars.reserve(_string16.size());

	rich_text::Formatter formatter(_fontSet, &_chars, &_lines, _density);
	formatter.setWidth((uint16_t)roundf(_width * _density));
	formatter.setTextAlignment(_alignment);
	formatter.setMaxWidth((uint16_t)roundf(_maxWidth * _density));
	formatter.setMaxLines(_maxLines);
	formatter.setOpticalAlignment(_opticalAlignment);
	formatter.setFillerChar(_fillerChar);

	if (_lineHeight != 0.0f) {
		if (_isLineHeightAbsolute) {
			formatter.setLineHeightAbsolute((uint16_t)(_lineHeight * _density));
		} else {
			formatter.setLineHeightRelative(_lineHeight);
		}
	}

	formatter.begin((uint16_t)roundf(_textIndent * _density));

	size_t drawedChars = 0;
	for (auto &it : _compiledStyles) {
		formatter.setPositionsVec(&_positions, it.start);
		rich_text::style::TextLayoutParameters params = baseParams;
		Font *font = baseFont;
		for (auto &styleIt : it.style.params) {
			switch (styleIt.name) {
			case Style::Name::TextTransform:
				params.textTransform = styleIt.value.textTransform;
				break;
			case Style::Name::TextDecoration:
				params.textDecoration = styleIt.value.textDecoration;
				break;
			case Style::Name::Hyphens:
				params.hyphens = styleIt.value.hyphens;
				break;
			case Style::Name::VerticalAlign:
				params.verticalAlign = styleIt.value.verticalAlign;
				break;
			case Style::Name::Color:
				params.color = styleIt.value.color;
				break;
			case Style::Name::Opacity:
				params.opacity = styleIt.value.opacity;
				break;
			case Style::Name::Font:
				if (styleIt.value.font && styleIt.value.font->getImage() == baseFont->getImage()) {
					font = styleIt.value.font;
				}
				break;
			default:
				break;
			}
		}

		auto start = _string16.c_str() + it.start;
		auto len = it.length;

		if (_localeEnabled && hasLocaleTags(start, len)) {
			std::u16string str(resolveLocaleTags(start, len));

			start = str.c_str();
			len = str.length();

			if (_maxChars > 0 && drawedChars + len > _maxChars) {
				len = _maxChars - drawedChars;
			}
			if (!formatter.read(font, params, start, len)) {
				drawedChars += len;
				break;
			}
		} else {
			if (_maxChars > 0 && drawedChars + len > _maxChars) {
				len = _maxChars - drawedChars;
			}
			if (!formatter.read(font, params, start, len)) {
				drawedChars += len;
				break;
			}
		}
	}
	formatter.finalize();

	_charsWidth = formatter.getWidth();
	_charsHeight = formatter.getHeight();

	_quads.clear();
	_quads.drawCharsInvert(_chars, _charsHeight);

	setContentSize(cocos2d::Size(_charsWidth / _density, _charsHeight / _density));

	_reversePositions.clear();
	_reversePositions.reserve(_string16.length());
	size_t counter = 0;
	size_t idx = 0;
	for (auto &it : _positions) {
		if (it == std::u16string::npos) {
			break;
		}
		while (counter < it) {
			//log("char: %lu '%s'  symbol: max", counter, string::toUtf8(_string16[counter]).c_str());
			_reversePositions.push_back(std::u16string::npos);
			counter ++;
		}
		//log("char: %lu '%s'  symbol: %lu '%s'", counter, string::toUtf8(_string16[counter]).c_str(),
		//		idx, string::toUtf8(_chars[idx].charPtr->charID).c_str());
		_reversePositions.push_back(idx);
		counter++;
		idx ++;
	}

	while (_reversePositions.size() < _string16.length()) {
		//log("char: %lu '%s'  symbol: max", counter, string::toUtf8(_string16[counter]).c_str());
		_reversePositions.push_back(std::u16string::npos);
		counter++;
	}

	_labelDirty = false;
}

void RichLabel::visit(cocos2d::Renderer *r, const cocos2d::Mat4& t, uint32_t f, ZPath &zPath) {
	if (_labelDirty) {
		updateLabel();
	}
	DynamicBatchNode::visit(r, t, f, zPath);
}

bool RichLabel::isLabelDirty() const {
	return _labelDirty;
}

void RichLabel::updateColor() {
    _labelDirty = true;
}

void RichLabel::setString(const std::string &newString) {
	if (newString == _string8) {
		return;
	}

	_string8 = newString;
	_string16 = string::toUtf16(newString);
    _labelDirty = true;
}

void RichLabel::setString(const std::u16string &newString) {
	if (newString == _string16) {
		return;
	}

	_string8 = string::toUtf8(newString);
	_string16 = newString;
    _labelDirty = true;
}

const std::string &RichLabel::getString(void) const {
	return _string8;
}
const std::u16string &RichLabel::getString16(void) const {
	return _string16;
}

void RichLabel::erase16(size_t start, size_t len) {
	if (start >= _string16.length()) {
		return;
	}

	_string16.erase(start, len);
	_string8 = string::toUtf8(_string16);
	_labelDirty = true;
}

void RichLabel::erase8(size_t start, size_t len) {
	if (start >= _string8.length()) {
		return;
	}

	_string8.erase(start, len);
	_string16 = string::toUtf16(_string8);
	_labelDirty = true;
}

void RichLabel::append(const std::string& value) {
	_string8.append(value);
	_string16 = string::toUtf16(_string8);
	_labelDirty = true;
}
void RichLabel::append(const std::u16string& value) {
	_string16.append(value);
	_string8 = string::toUtf8(_string16);
	_labelDirty = true;
}

void RichLabel::prepend(const std::string& value) {
	_string8 = value + _string8;
	_string16 = string::toUtf16(_string8);
	_labelDirty = true;
}
void RichLabel::prepend(const std::u16string& value) {
	_string16 = value + _string16;
	_string8 = string::toUtf8(_string16);
	_labelDirty = true;
}

void RichLabel::setDensity(float density) {
	if (density != _density) {
		DynamicBatchNode::setDensity(density);
		_labelDirty = true;
	}
}

void RichLabel::setAlignment(Alignment alignment) {
	if (_alignment != alignment) {
		_alignment = alignment;
		_labelDirty = true;
	}
}

RichLabel::Alignment RichLabel::getAlignment() const {
	return _alignment;
}

void RichLabel::setWidth(float width) {
	if (_width != width) {
		_width = width;
		_labelDirty = true;
	}
}

float RichLabel::getWidth() const {
	return _width;
}

void RichLabel::setTextIndent(float value) {
	if (_textIndent != value) {
		_textIndent = value;
		_labelDirty = true;
	}
}
float RichLabel::getTextIndent() const {
	return _textIndent;
}

void RichLabel::setFont(Font *font) {
	if (font != _baseFont) {
		_fontSet = font->getFontSet();
		_baseFont = font;
		if (_baseFont) {
			setTexture(_baseFont->getTexture());
			_labelDirty = true;
		} else {
			setTexture(nullptr);
		}
	}
}

Font *RichLabel::getFont() const {
	return _baseFont;
}

void RichLabel::setTextTransform(const Style::TextTransform &value) {
	if (value != _textTransform) {
		_textTransform = value;
		_labelDirty = true;
	}
}

RichLabel::Style::TextTransform RichLabel::getTextTransform() const {
	return _textTransform;
}

void RichLabel::setTextDecoration(const Style::TextDecoration &value) {
	if (value != _textDecoration) {
		_textDecoration = value;
		_labelDirty = true;
	}
}
RichLabel::Style::TextDecoration RichLabel::getTextDecoration() const {
	return _textDecoration;
}

void RichLabel::setHyphens(const Style::Hyphens &value) {
	if (value != _hyphens) {
		_hyphens = value;
		_labelDirty = true;
	}
}
RichLabel::Style::Hyphens RichLabel::getHyphens() const {
	return _hyphens;
}

void RichLabel::setVerticalAlign(const Style::VerticalAlign &value) {
	if (value != _verticalAlign) {
		_verticalAlign = value;
		_labelDirty = true;
	}
}
RichLabel::Style::VerticalAlign RichLabel::getVerticalAlign() const {
	return _verticalAlign;
}

void RichLabel::setLineHeightAbsolute(float value) {
	if (!_isLineHeightAbsolute || _lineHeight != value) {
		_isLineHeightAbsolute = true;
		_lineHeight = value;
		_labelDirty = true;
	}
}
void RichLabel::setLineHeightRelative(float value) {
	if (_isLineHeightAbsolute || _lineHeight != value) {
		_isLineHeightAbsolute = false;
		_lineHeight = value;
		_labelDirty = true;
	}
}
float RichLabel::getLineHeight() const {
	return _lineHeight;
}
bool RichLabel::isLineHeightAbsolute() const {
	return _isLineHeightAbsolute;
}

void RichLabel::setMaxWidth(float value) {
	if (_maxWidth != value) {
		_maxWidth = value;
		_labelDirty = true;
	}
}
float RichLabel::getMaxWidth() const {
	return _maxWidth;
}

void RichLabel::setMaxLines(size_t value) {
	if (_maxLines != value) {
		_maxLines = value;
		_labelDirty = true;
	}
}
size_t RichLabel::getMaxLines() const {
	return _maxLines;
}

void RichLabel::setMaxChars(size_t value) {
	if (_maxChars != value) {
		_maxChars = value;
		_labelDirty = true;
	}
}
size_t RichLabel::getMaxChars() const {
	return _maxChars;
}

void RichLabel::setOpticalAlignment(bool value) {
	if (_opticalAlignment != value) {
		_opticalAlignment = value;
		_labelDirty = true;
	}
}
bool RichLabel::isOpticallyAligned() const {
	return _opticalAlignment;
}

void RichLabel::setFillerChar(char16_t c) {
	if (c != _fillerChar) {
		_fillerChar = c;
		_labelDirty = true;
	}
}
char16_t RichLabel::getFillerChar() const {
	return _fillerChar;
}

void RichLabel::setLocaleEnabled(bool value) {
	_localeEnabled = value;
}
bool RichLabel::isLocaleEnabled() const {
	return _localeEnabled;
}

void RichLabel::setTextRangeStyle(size_t start, size_t length, Style &&style) {
	if (length > 0) {
		_styles.emplace_back(start, length, std::move(style));
		_labelDirty = true;
	}
}

void RichLabel::appendTextWithStyle(const std::string &str, Style &&style) {
	auto start = _string16.length();
	append(str);
	setTextRangeStyle(start, _string16.length() - start, std::move(style));
}

void RichLabel::appendTextWithStyle(const std::u16string &str, Style &&style) {
	auto start = _string16.length();
	append(str);
	setTextRangeStyle(start, str.length(), std::move(style));
}

void RichLabel::prependTextWithStyle(const std::string &str, Style &&style) {
	auto len = _string16.length();
	prepend(str);
	setTextRangeStyle(0, _string16.length() - len, std::move(style));
}
void RichLabel::prependTextWithStyle(const std::u16string &str, Style &&style) {
	prepend(str);
	setTextRangeStyle(0, str.length(), std::move(style));
}

void RichLabel::clearStyles() {
	_styles.clear();
	_labelDirty = true;
}

const RichLabel::StyleVec &RichLabel::getStyles() const {
	return _styles;
}

const RichLabel::StyleVec &RichLabel::getCompiledStyles() const {
	return _compiledStyles;
}

void RichLabel::setStyles(StyleVec &&vec) {
	_styles = std::move(vec);
	_labelDirty = true;
}
void RichLabel::setStyles(const StyleVec &vec) {
	_styles = vec;
	_labelDirty = true;
}

RichLabel::SymbolIndex RichLabel::getSymbolIndex(CharIndex cidx) {
	if (cidx >= getCharCount()) {
		return SymbolIndex::max();
	}


	auto sidx = _reversePositions[cidx.get()];
	if (sidx == std::u16string::npos) {
		return SymbolIndex::max();
	} else {
		return SymbolIndex(sidx);
	}
}
RichLabel::CharIndex RichLabel::getCharIndex(SymbolIndex sidx) {
	if (sidx >= getSymbolCount()) {
		return CharIndex::max();
	}

	auto cidx = _positions[sidx.get()];
	if (cidx == std::u16string::npos) {
		return CharIndex::max();
	} else {
		return CharIndex(cidx);
	}
}

RichLabel::SymbolIndex RichLabel::getSymbolCount() {
	return SymbolIndex(_positions.size());
}
RichLabel::CharIndex RichLabel::getCharCount() {
	return CharIndex(_reversePositions.size());
}

Vec2 RichLabel::getCursorPosition(CharIndex cidx) {
	if (_string16.empty() || cidx.get() == 0) {
		return Vec2::ZERO;
	} else if (_positions.size() > 0 && cidx.get() >= _string16.length()) {
		return getSymbolAdvance(getSymbolCount() - SymbolIndex(1));
	}
	auto sidx = getSymbolIndex(cidx);
	return getCursorPosition(sidx);
}
Vec2 RichLabel::getCursorPosition(SymbolIndex sidx) {
	return getSymbolOrigin(sidx);
}

Vec2 RichLabel::getSymbolOrigin(SymbolIndex sidx) {
	if (sidx == SymbolIndex::max() || sidx >= getSymbolCount()) {
		return Vec2::ZERO;
	} else {
		CharSpec &charSpec = _chars[sidx.get()];
		return Vec2(charSpec.posX / _density, charSpec.posY / _density);
	}
}
Vec2 RichLabel::getSymbolAdvance(SymbolIndex sidx) {
	if (sidx == SymbolIndex::max() || sidx >= getSymbolCount()) {
		return Vec2::ZERO;
	} else {
		CharSpec &charSpec = _chars[sidx.get()];
		return Vec2((charSpec.posX + charSpec.xAdvance()) / _density, _contentSize.height - charSpec.posY / _density);
	}
}

static void RichLabel_dumpStyle(RichLabel::StyleVec &ret, size_t pos, size_t len, const RichLabel::Style &style) {
	if (len > 0) {
		ret.emplace_back(pos, len, style);
	}
}

RichLabel::StyleVec RichLabel::compileStyle() const {
	size_t pos = 0;
	size_t max = _string16.length();

	StyleVec ret;
	StyleVec vec = _styles;

	Style compiledStyle;

	size_t dumpPos = 0;

	for (pos = 0; pos < max; pos ++) {
		auto it = vec.begin();

		bool cleaned = false;
		// check for endings
		while(it != vec.end()) {
			if (it->start + it->length <= pos) {
				RichLabel_dumpStyle(ret, dumpPos, pos - dumpPos, compiledStyle);
				compiledStyle.clear();
				cleaned = true;
				dumpPos = pos;
				it = vec.erase(it);
			} else {
				it ++;
			}
		}

		// checks for continuations and starts
		it = vec.begin();
		while(it != vec.end()) {
			if (it->start == pos) {
				if (dumpPos != pos) {
					RichLabel_dumpStyle(ret, dumpPos, pos - dumpPos, compiledStyle);
					dumpPos = pos;
				}
				compiledStyle.merge(it->style);
			} else if (it->start <= pos && it->start + it->length > pos) {
				if (cleaned) {
					compiledStyle.merge(it->style);
				}
			}
			it ++;
		}
	}

	RichLabel_dumpStyle(ret, dumpPos, pos - dumpPos, compiledStyle);

	return ret;
}

cocos2d::GLProgramState *RichLabel::getProgramStateA8() const {
	return cocos2d::GLProgramState::getOrCreateWithGLProgram(GLProgramSet::getInstance()->getProgram(GLProgramSet::DynamicBatchA8Highp));
}

bool RichLabel::hasLocaleTags(const char16_t *str, size_t len) const {
	return locale::hasLocaleTags(str, len);
}

std::u16string RichLabel::resolveLocaleTags(const char16_t *str, size_t len) const {
	return locale::resolveLocaleTags(str, len);
}

NS_SP_END
