// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPLabelParameters.h"
#include "SLFontFormatter.h"
#include "SPLocale.h"

NS_SP_BEGIN

LabelParameters::DescriptionStyle::DescriptionStyle() {
	font.fontFamily = StringView("default");
	font.fontSize = FontSize(14);
	text.opacity = 222;
	text.color = Color3B::BLACK;
	text.whiteSpace = layout::style::WhiteSpace::PreWrap;
}

String LabelParameters::DescriptionStyle::getConfigName(bool caps) const {
	return font.getConfigName(caps);
}

LabelParameters::DescriptionStyle LabelParameters::DescriptionStyle::merge(Source *source, const Style &style) const {
	DescriptionStyle ret(*this);
	for (auto &it : style.params) {
		switch (it.name) {
		case Style::Name::TextTransform: ret.text.textTransform = it.value.textTransform; break;
		case Style::Name::TextDecoration: ret.text.textDecoration = it.value.textDecoration; break;
		case Style::Name::Hyphens: ret.text.hyphens = it.value.hyphens; break;
		case Style::Name::VerticalAlign: ret.text.verticalAlign = it.value.verticalAlign; break;
		case Style::Name::Color: ret.text.color = it.value.color; ret.colorDirty = true; break;
		case Style::Name::Opacity: ret.text.opacity = it.value.opacity; ret.opacityDirty = true; break;
		case Style::Name::FontSize: ret.font.fontSize = it.value.fontSize; break;
		case Style::Name::FontStyle: ret.font.fontStyle = it.value.fontStyle; break;
		case Style::Name::FontWeight: ret.font.fontWeight = it.value.fontWeight; break;
		case Style::Name::FontStretch: ret.font.fontStretch = it.value.fontStretch; break;
		case Style::Name::FontFamily: ret.font.fontFamily = source->getFamilyName(it.value.fontFamily); break;
		}
	}
	return ret;
}

bool LabelParameters::DescriptionStyle::operator == (const DescriptionStyle &style) const {
	return font == style.font && text == style.text && colorDirty == style.colorDirty && opacityDirty == style.opacityDirty;
}
bool LabelParameters::DescriptionStyle::operator != (const DescriptionStyle &style) const {
	return !((*this) == style);
}

LabelParameters::Style::Value::Value() {
	memset((void *)this, 0, sizeof(Value));
}

void LabelParameters::Style::set(const Param &p, bool force) {
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

void LabelParameters::Style::merge(const Style &style) {
	for (auto &it : style.params) {
		set(it, true);
	}
}

void LabelParameters::Style::clear() {
	params.clear();
}


bool LabelParameters::ExternalFormatter::init(Source *s, float w, float density) {
	if (!s) {
		return false;
	}

	if (density == 0.0f) {
		_density = screen::density();
	} else {
		_density = density;
	}

	_spec.setSource(Rc<layout::FormatterFontSource>::alloc(s));
	_formatter.init(&_spec, _density);
	if (w > 0.0f) {
		_formatter.setWidth((uint16_t)roundf(w * _density));
	}
	return true;
}

void LabelParameters::ExternalFormatter::setLineHeightAbsolute(float value) {
	_formatter.setLineHeightAbsolute((uint16_t)(value * _formatter.getDensity()));
}

void LabelParameters::ExternalFormatter::setLineHeightRelative(float value) {
	_formatter.setLineHeightRelative(value);
}

void LabelParameters::ExternalFormatter::reserve(size_t chars, size_t ranges) {
	_spec.reserve(chars, ranges);
}

void LabelParameters::ExternalFormatter::addString(const DescriptionStyle &style, const StringView &str, bool localized) {
	addString(style, string::toUtf16(str), localized);
}
void LabelParameters::ExternalFormatter::addString(const DescriptionStyle &style, const WideStringView &str, bool localized) {
	if (!begin) {
		_formatter.begin(0, 0);
		begin = true;
	}
	if (localized && locale::hasLocaleTags(str)) {
		auto u16str = locale::resolveLocaleTags(str);
		_formatter.read(style.font, style.text, u16str.data(), u16str.size());
	} else {
		_formatter.read(style.font, style.text, str.data(), str.size());
	}
}

Size LabelParameters::ExternalFormatter::finalize() {
	_formatter.finalize();
	return Size(_spec.width / _density, _spec.height / _density);
}

WideString LabelParameters::getLocalizedString(const StringView &s) {
	return getLocalizedString(string::toUtf16(s));
}
WideString LabelParameters::getLocalizedString(const WideStringView &s) {
	if (locale::hasLocaleTags(s)) {
		return locale::resolveLocaleTags(s);
	}
	return s.str();
}

float LabelParameters::getStringWidth(Source *source, const DescriptionStyle &style, const StringView &str, float density, bool localized) {
	return getStringWidth(source, style, string::toUtf16(str), density, localized);
}

float LabelParameters::getStringWidth(Source *source, const DescriptionStyle &style, const WideStringView &str, float density, bool localized) {
	if (!source) {
		return 0.0f;
	}

	if (density == 0.0f) {
		density = screen::density();
	}

	layout::FormatSpec spec(Rc<layout::FormatterFontSource>::alloc(source));
	layout::Formatter fmt(&spec, density);
	fmt.begin(0, 0);

	if (localized && locale::hasLocaleTags(str)) {
		auto u16str = locale::resolveLocaleTags(str);
		spec.reserve(u16str.size());
		fmt.read(style.font, style.text, u16str.data(), u16str.size());
	} else {
		spec.reserve(str.size());
		fmt.read(style.font, style.text, str.data(), str.size());
	}

	fmt.finalize();
	return spec.width / density;
}

Size LabelParameters::getLabelSize(Source *source, const DescriptionStyle &style, const StringView &s, float w, float density, bool localized) {
	return getLabelSize(source, style, string::toUtf16(s), w, density, localized);
}

Size LabelParameters::getLabelSize(Source *source, const DescriptionStyle &style, const WideStringView &str, float w, float density, bool localized) {
	if (str.empty()) {
		return Size(0.0f, 0.0f);
	}

	if (!source) {
		return Size(0.0f, 0.0f);
	}

	if (density == 0.0f) {
		density = screen::density();
	}


	layout::FormatSpec spec(Rc<layout::FormatterFontSource>::alloc(source));
	layout::Formatter fmt(&spec, density);
	fmt.setWidth((uint16_t)roundf(w * density));
	fmt.begin(0, 0);

	if (localized && locale::hasLocaleTags(str)) {
		auto u16str = locale::resolveLocaleTags(str);
		spec.reserve(u16str.size());
		fmt.read(style.font, style.text, u16str.data(), u16str.size());
	} else {
		spec.reserve(str.size());
		fmt.read(style.font, style.text, str.data(), str.size());
	}

	fmt.finalize();
	return Size(spec.maxLineX / density, spec.height / density);
}

void LabelParameters::setAlignment(Alignment alignment) {
	if (_alignment != alignment) {
		_alignment = alignment;
		_labelDirty = true;
	}
}

LabelParameters::Alignment LabelParameters::getAlignment() const {
	return _alignment;
}

void LabelParameters::setWidth(float width) {
	if (_width != width) {
		_width = width;
		_labelDirty = true;
	}
}

float LabelParameters::getWidth() const {
	return _width;
}

void LabelParameters::setTextIndent(float value) {
	if (_textIndent != value) {
		_textIndent = value;
		_labelDirty = true;
	}
}
float LabelParameters::getTextIndent() const {
	return _textIndent;
}

void LabelParameters::setTextTransform(const TextTransform &value) {
	if (value != _style.text.textTransform) {
		_style.text.textTransform = value;
		_labelDirty = true;
	}
}

LabelParameters::TextTransform LabelParameters::getTextTransform() const {
	return _style.text.textTransform;
}

void LabelParameters::setTextDecoration(const TextDecoration &value) {
	if (value != _style.text.textDecoration) {
		_style.text.textDecoration = value;
		_labelDirty = true;
	}
}
LabelParameters::TextDecoration LabelParameters::getTextDecoration() const {
	return _style.text.textDecoration;
}

void LabelParameters::setHyphens(const Hyphens &value) {
	if (value != _style.text.hyphens) {
		_style.text.hyphens = value;
		_labelDirty = true;
	}
}
LabelParameters::Hyphens LabelParameters::getHyphens() const {
	return _style.text.hyphens;
}

void LabelParameters::setVerticalAlign(const VerticalAlign &value) {
	if (value != _style.text.verticalAlign) {
		_style.text.verticalAlign = value;
		_labelDirty = true;
	}
}
LabelParameters::VerticalAlign LabelParameters::getVerticalAlign() const {
	return _style.text.verticalAlign;
}

void LabelParameters::setFontSize(const uint16_t &value) {
	if (value != _style.font.fontSize.get()) {
		_style.font.fontSize = FontSize(value);
		_labelDirty = true;
	}
}
uint16_t LabelParameters::getFontSize() const {
	return _style.font.fontSize.get();
}

void LabelParameters::setFontStyle(const FontStyle &value) {
	if (value != _style.font.fontStyle) {
		_style.font.fontStyle = value;
		_labelDirty = true;
	}
}
LabelParameters::FontStyle LabelParameters::getFontStyle() const {
	return _style.font.fontStyle;
}

void LabelParameters::setFontWeight(const FontWeight &value) {
	if (value != _style.font.fontWeight) {
		_style.font.fontWeight = value;
		_labelDirty = true;
	}
}
LabelParameters::FontWeight LabelParameters::getFontWeight() const {
	return _style.font.fontWeight;
}

void LabelParameters::setFontStretch(const FontStretch &value) {
	if (value != _style.font.fontStretch) {
		_style.font.fontStretch = value;
		_labelDirty = true;
	}
}
LabelParameters::FontStretch LabelParameters::getFontStretch() const {
	return _style.font.fontStretch;
}

void LabelParameters::setFontFamily(const StringView &value) {
	if (value != _style.font.fontFamily) {
		_fontFamilyStorage = value.str();
		_style.font.fontFamily = _fontFamilyStorage;
		_labelDirty = true;
	}
}
StringView LabelParameters::getFontFamily() const {
	return _style.font.fontFamily;
}

void LabelParameters::setLineHeightAbsolute(float value) {
	if (!_isLineHeightAbsolute || _lineHeight != value) {
		_isLineHeightAbsolute = true;
		_lineHeight = value;
		_labelDirty = true;
	}
}
void LabelParameters::setLineHeightRelative(float value) {
	if (_isLineHeightAbsolute || _lineHeight != value) {
		_isLineHeightAbsolute = false;
		_lineHeight = value;
		_labelDirty = true;
	}
}
float LabelParameters::getLineHeight() const {
	return _lineHeight;
}
bool LabelParameters::isLineHeightAbsolute() const {
	return _isLineHeightAbsolute;
}

void LabelParameters::setMaxWidth(float value) {
	if (_maxWidth != value) {
		_maxWidth = value;
		_labelDirty = true;
	}
}
float LabelParameters::getMaxWidth() const {
	return _maxWidth;
}

void LabelParameters::setMaxLines(size_t value) {
	if (_maxLines != value) {
		_maxLines = value;
		_labelDirty = true;
	}
}
size_t LabelParameters::getMaxLines() const {
	return _maxLines;
}

void LabelParameters::setMaxChars(size_t value) {
	if (_maxChars != value) {
		_maxChars = value;
		_labelDirty = true;
	}
}
size_t LabelParameters::getMaxChars() const {
	return _maxChars;
}

void LabelParameters::setOpticalAlignment(bool value) {
	if (_opticalAlignment != value) {
		_opticalAlignment = value;
		_labelDirty = true;
	}
}
bool LabelParameters::isOpticallyAligned() const {
	return _opticalAlignment;
}

void LabelParameters::setFillerChar(char16_t c) {
	if (c != _fillerChar) {
		_fillerChar = c;
		_labelDirty = true;
	}
}
char16_t LabelParameters::getFillerChar() const {
	return _fillerChar;
}

void LabelParameters::setLocaleEnabled(bool value) {
	if (_localeEnabled != value) {
		_localeEnabled = value;
		_labelDirty = true;
	}
}
bool LabelParameters::isLocaleEnabled() const {
	return _localeEnabled;
}

void LabelParameters::setString(const StringView &newString) {
	if (newString == _string8) {
		return;
	}

	_string8 = newString.str();
	_string16 = string::toUtf16(newString);
	if (!_localeEnabled && locale::hasLocaleTagsFast(_string16)) {
		setLocaleEnabled(true);
	}
	_labelDirty = true;
	clearStyles();
}

void LabelParameters::setString(const WideStringView &newString) {
	if (newString == _string16) {
		return;
	}

	_string8 = string::toUtf8(newString);
	_string16 = newString.str();
	if (!_localeEnabled && locale::hasLocaleTagsFast(_string16)) {
		setLocaleEnabled(true);
	}
	_labelDirty = true;
	clearStyles();
}

void LabelParameters::setLocalizedString(size_t idx) {
	setString(localeIndex(idx));
	setLocaleEnabled(true);
}

WideStringView LabelParameters::getString() const {
	return _string16;
}

StringView LabelParameters::getString8() const {
	return _string8;
}

void LabelParameters::erase16(size_t start, size_t len) {
	if (start >= _string16.length()) {
		return;
	}

	_string16.erase(start, len);
	_string8 = string::toUtf8(_string16);
	_labelDirty = true;
}

void LabelParameters::erase8(size_t start, size_t len) {
	if (start >= _string8.length()) {
		return;
	}

	_string8.erase(start, len);
	_string16 = string::toUtf16(_string8);
	_labelDirty = true;
}

void LabelParameters::append(const String& value) {
	_string8.append(value);
	_string16 = string::toUtf16(_string8);
	_labelDirty = true;
}
void LabelParameters::append(const WideString& value) {
	_string16.append(value);
	_string8 = string::toUtf8(_string16);
	_labelDirty = true;
}

void LabelParameters::prepend(const String& value) {
	_string8 = value + _string8;
	_string16 = string::toUtf16(_string8);
	_labelDirty = true;
}
void LabelParameters::prepend(const WideString& value) {
	_string16 = value + _string16;
	_string8 = string::toUtf8(_string16);
	_labelDirty = true;
}

void LabelParameters::setTextRangeStyle(size_t start, size_t length, Style &&style) {
	if (length > 0) {
		_styles.emplace_back(start, length, std::move(style));
		_labelDirty = true;
	}
}

void LabelParameters::appendTextWithStyle(const String &str, Style &&style) {
	auto start = _string16.length();
	append(str);
	setTextRangeStyle(start, _string16.length() - start, std::move(style));
}

void LabelParameters::appendTextWithStyle(const WideString &str, Style &&style) {
	auto start = _string16.length();
	append(str);
	setTextRangeStyle(start, str.length(), std::move(style));
}

void LabelParameters::prependTextWithStyle(const String &str, Style &&style) {
	auto len = _string16.length();
	prepend(str);
	setTextRangeStyle(0, _string16.length() - len, std::move(style));
}
void LabelParameters::prependTextWithStyle(const WideString &str, Style &&style) {
	prepend(str);
	setTextRangeStyle(0, str.length(), std::move(style));
}

void LabelParameters::clearStyles() {
	_styles.clear();
	_labelDirty = true;
}

const LabelParameters::StyleVec &LabelParameters::getStyles() const {
	return _styles;
}

const LabelParameters::StyleVec &LabelParameters::getCompiledStyles() const {
	return _compiledStyles;
}

void LabelParameters::setStyles(StyleVec &&vec) {
	_styles = std::move(vec);
	_labelDirty = true;
}
void LabelParameters::setStyles(const StyleVec &vec) {
	_styles = vec;
	_labelDirty = true;
}

LabelParameters::~LabelParameters() { }

bool LabelParameters::isLabelDirty() const {
	return _labelDirty;
}

static void LabelParameters_dumpStyle(LabelParameters::StyleVec &ret, size_t pos, size_t len, const LabelParameters::Style &style) {
	if (len > 0) {
		ret.emplace_back(pos, len, style);
	}
}

LabelParameters::StyleVec LabelParameters::compileStyle() const {
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
				LabelParameters_dumpStyle(ret, dumpPos, pos - dumpPos, compiledStyle);
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
					LabelParameters_dumpStyle(ret, dumpPos, pos - dumpPos, compiledStyle);
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

	LabelParameters_dumpStyle(ret, dumpPos, pos - dumpPos, compiledStyle);

	return ret;
}

bool LabelParameters::hasLocaleTags(const WideStringView &str) const {
	return locale::hasLocaleTags(str);
}

WideString LabelParameters::resolveLocaleTags(const WideStringView &str) const {
	return locale::resolveLocaleTags(str);
}

NS_SP_END
