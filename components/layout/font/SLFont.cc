// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPLayout.h"
#include "SLFont.h"
#include "SPFilesystem.h"

NS_LAYOUT_BEGIN

void FontCharString::addChar(char16_t c) {
	auto it = std::lower_bound(chars.begin(), chars.end(), c);
	if (it == chars.end() || *it != c) {
		chars.insert(it, c);
	}
}

void FontCharString::addString(const String &str) {
	addString(string::toUtf16(str));
}

void FontCharString::addString(const WideString &str) {
	addString(str.data(), str.size());
}

void FontCharString::addString(const char16_t *str, size_t len) {
	for (size_t i = 0; i < len; ++ i) {
		const char16_t &c = str[i];
		auto it = std::lower_bound(chars.begin(), chars.end(), c);
		if (it == chars.end() || *it != c) {
			chars.insert(it, c);
		}
	}
}

bool FontData::init() {
	return true;
}
bool FontData::init(const FontData &data) {
	metrics = data.metrics;
	chars = data.chars;
	kerning = data.kerning;
	return true;
}

uint16_t FontData::getHeight() const {
	return metrics.height;
}
int16_t FontData::getAscender() const {
	return metrics.ascender;
}
int16_t FontData::getDescender() const {
	return metrics.descender;
}

CharLayout FontData::getChar(char16_t c) const {
	auto it = std::lower_bound(chars.begin(), chars.end(), c);
	if (it != chars.end() && *it == c) {
		return *it;
	} else {
		return CharLayout{0};
	}
}

uint16_t FontData::xAdvance(char16_t c) const {
	auto it = std::lower_bound(chars.begin(), chars.end(), c);
	if (it != chars.end() && *it == c) {
		return it->xAdvance;
	} else {
		return 0;
	}
}

int16_t FontData::kerningAmount(char16_t first, char16_t second) const {
	uint32_t key = (first << 16) | (second & 0xffff);
	auto it = kerning.find(key);
	if (it != kerning.end()) {
		return it->second;
	}
	return 0;
}

bool FontLayout::init(const FontSource * source, const String &name, const StringView &family, FontSize size, const FontFace &face,
		const ReceiptCallback &cb, const MetricCallback &mcb, const UpdateCallback &ucb) {
	_name = name;
	_family = family.str();
	_dsize = size;
	_face = face;
	_callback = cb;
	_metricCallback = mcb;
	_updateCallback = ucb;
	_source = source;

	_data = Rc<FontData>::create();
	_data->metrics = _metricCallback(_source, face.src, _dsize, _callback);

	return true;
}

FontLayout::~FontLayout() { }

void FontLayout::addString(const String &str) {
	addString(string::toUtf16(str));
}
void FontLayout::addString(const WideString &str) {
	addString(str.data(), str.size());
}
void FontLayout::addString(const char16_t *str, size_t len) {
	Vector<char16_t> chars;
	for (size_t i = 0; i < len; ++ i) {
		const char16_t &c = str[i];
		auto it = std::lower_bound(chars.begin(), chars.end(), c);
		if (it == chars.end() || *it != c) {
			chars.insert(it, c);
		}
	}

	merge(chars);
}
void FontLayout::addString(const FontCharString &str) {
	merge(str.chars);
}

void FontLayout::addSortedChars(const Vector<char16_t> &vec) {
	merge(vec);
}

void FontLayout::merge(const Vector<char16_t> &chars) {
	Rc<FontData> data(getData());
	while (true) {
		Vector<char16_t> charsToUpdate; charsToUpdate.reserve(chars.size());
		for (auto &it : chars) {
			if (!std::binary_search(data->chars.begin(), data->chars.end(), CharLayout{it})) {
				charsToUpdate.push_back(it);
			}
		}
		if (charsToUpdate.empty()) {
			return;
		}

		Rc<FontData> newData(_updateCallback(_source, _face.src, data, charsToUpdate, _callback));
		_mutex.lock();
		if (_data == data) {
			_data = newData;
			_mutex.unlock();
			return;
		} else {
			data = _data;
		}
		_mutex.unlock();
	}
}

Rc<FontData> FontLayout::getData() {
	Rc<FontData> ret;
	_mutex.lock();
	ret = _data;
	_mutex.unlock();
	return ret;
}

const String &FontLayout::getName() const {
	return _name;
}
const String &FontLayout::getFamily() const {
	return _family;
}

const ReceiptCallback &FontLayout::getCallback() const {
	return _callback;
}

const FontFace &FontLayout::getFontFace() const {
	return _face;
}

//uint8_t FontLayout::getOriginalSize() const {
//	return _size;
//}

FontSize FontLayout::getSize() const {
	return _dsize;
}

FontParameters FontLayout::getStyle() const {
	return _face.getStyle(_family, _dsize);
}

size_t FontSource::getFontFaceScore(const FontParameters &params, const FontFace &face) {
	size_t ret = 0;
	if (face.fontStyle == style::FontStyle::Normal) {
		ret += 1;
	}
	if (face.fontWeight == style::FontWeight::Normal) {
		ret += 1;
	}
	if (face.fontStretch == style::FontStretch::Normal) {
		ret += 1;
	}

	if (face.fontStyle == params.fontStyle && (face.fontStyle == style::FontStyle::Oblique || face.fontStyle == style::FontStyle::Italic)) {
		ret += 100;
	} else if ((face.fontStyle == style::FontStyle::Oblique || face.fontStyle == style::FontStyle::Italic)
			&& (params.fontStyle == style::FontStyle::Oblique || params.fontStyle == style::FontStyle::Italic)) {
		ret += 75;
	}

	auto weightDiff = (int)toInt(style::FontWeight::W900) - abs((int)toInt(params.fontWeight) - (int)toInt(face.fontWeight));
	ret += weightDiff * 10;

	auto stretchDiff = (int)toInt(style::FontStretch::UltraExpanded) - abs((int)toInt(params.fontStretch) - (int)toInt(face.fontStretch));
	ret += stretchDiff * 5;

	return ret;
}

FontSource::~FontSource() { }

void FontSource::preloadChars(const FontParameters &style, const Vector<char16_t> &chars) {
	auto l = getLayout(style);
	if (l) {
		l->addSortedChars(chars);
		addTextureString(l->getName(), chars.data(), chars.size());
	}
}

bool FontSource::init(FontFaceMap &&map, const ReceiptCallback &cb, float scale, SearchDirs &&dirs) {
	_fontFaces = std::move(map);
	_fontScale = scale;
	_callback = cb;
	_version = 0;
	_searchDirs = std::move(dirs);

	for (auto &it : _fontFaces) {
		auto &family = it.first;
		_families.emplace(hash::hash32(family.c_str(), family.size()), family);
	}

	return true;
}

static auto FontSource_defaultFontFamily = "default";

Rc<FontLayout> FontSource::getLayout(const FontParameters &style, float scale) {
	Rc<FontLayout> ret = nullptr;

	auto family = style.fontFamily;
	if (family.empty()) {
		family = StringView(FontSource_defaultFontFamily);
	}

	if (isnan(scale)) {
		scale = _fontScale;
	}

	auto face = getFontFace(family, style);
	if (face) {
		auto dsize = FontSize(roundf(style.fontSize.get() * _density * scale));
		auto name = face->getConfigName(family, dsize);
		_mutex.lock();
		auto l_it = _layouts.find(name);
		if (l_it == _layouts.end()) {
			ret = (_layouts.emplace(name, Rc<FontLayout>::create(this, name, family, dsize, *face,
					_callback, _metricCallback, _layoutCallback)).first->second);
		} else {
			ret = (l_it->second);
		}
		_mutex.unlock();
	}

	return ret;
}

Rc<FontLayout> FontSource::getLayout(const FontLayout *l) {
	Rc<FontLayout> ret = nullptr;

	StringView family = l->getFamily();
	if (family.empty()) {
		family = StringView(FontSource_defaultFontFamily);
	}

	auto dstyle = l->getStyle();
	auto face = getFontFace(family, dstyle);
	if (face) {
		auto dsize = l->getSize();
		auto name = face->getConfigName(family, dsize);
		_mutex.lock();
		auto l_it = _layouts.find(name);
		if (l_it == _layouts.end()) {
			ret = (_layouts.emplace(name, Rc<FontLayout>::create(this, name, family, dsize, *face,
					_callback, _metricCallback, _layoutCallback)).first->second);
		} else {
			ret = (l_it->second);
		}
		_mutex.unlock();
	}

	return ret;
}

Rc<FontLayout> FontSource::getLayout(const String &name) {
	Rc<FontLayout> ret = nullptr;
	_mutex.lock();
	auto l_it = _layouts.find(name);
	if (l_it != _layouts.end()) {
		ret = (l_it->second);
	}
	_mutex.unlock();
	return ret;
}

bool FontSource::hasLayout(const FontParameters &style) {
	bool ret = false;
	auto family = style.fontFamily;
	if (family.empty()) {
		family = StringView(FontSource_defaultFontFamily);
	}

	auto face = getFontFace(family, style);
	if (face) {
		auto name = face->getConfigName(family, style.fontSize);
		_mutex.lock();
		auto l_it = _layouts.find(name);
		if (l_it != _layouts.end()) {
			ret = true;
		}
		_mutex.unlock();
	}
	return ret;
}

bool FontSource::hasLayout(const String &name) {
	bool ret = false;
	_mutex.lock();
	auto l_it = _layouts.find(name);
	if (l_it != _layouts.end()) {
		ret = true;
	}
	_mutex.unlock();
	return ret;
}

Map<String, Rc<FontLayout>> FontSource::getLayoutMap() {
	Map<String, Rc<FontLayout>> ret;
	_mutex.lock();
	ret = _layouts;
	_mutex.unlock();
	return ret;
}

FontFace * FontSource::getFontFace(const StringView &name, const FontParameters &it) {
	size_t score = 0;
	FontFace *face = nullptr;

	auto family = it.fontFamily;
	if (family.empty()) {
		family = StringView(FontSource_defaultFontFamily);
	}

	auto f_it = _fontFaces.find(family);
	if (f_it != _fontFaces.end()) {
		auto &faces = f_it->second;
		for (auto &face_it : faces) {
			auto newScore = getFontFaceScore(it, face_it);
			if (newScore >= score) {
				score = newScore;
				face = &face_it;
			}
		}
	}

	if (!face) {
		family = StringView(FontSource_defaultFontFamily);
		auto f_it = _fontFaces.find(family);
		if (f_it != _fontFaces.end()) {
			auto &faces = f_it->second;
			for (auto &face_it : faces) {
				auto newScore = getFontFaceScore(it, face_it);
				if (newScore >= score) {
					score = newScore;
					face = &face_it;
				}
			}
		}
	}

	return face;
}

float FontSource::getFontScale() const {
	return _fontScale;
}

void FontSource::update() {
	if (_dirty) {
		auto v = (++ _version);
		if (v == maxOf<uint32_t>()) {
			v = _version = 0;
		}
		_dirty = false;
		if (_updateCallback) {
			_updateCallback(v, _textureLayouts);
		}
	}
}

String FontSource::getFamilyName(uint32_t id) const {
	auto it = _families.find(id);
	if (it != _families.end()) {
		return it->second;
	}
	return String();
}

void FontSource::addTextureString(const String &layout, const String &str) {
	addTextureString(layout, string::toUtf16Html(str));
}
void FontSource::addTextureString(const String &layout, const WideString &str) {
	addTextureString(layout, str.data(), str.size());
}
void FontSource::addTextureString(const String &layout, const char16_t *str, size_t len) {
	auto &vec = getTextureLayout(layout);
	for (size_t i = 0; i < len; ++i) {
		const char16_t &c = str[i];
		auto char_it = std::lower_bound(vec.begin(), vec.end(), c);
		if (char_it == vec.end() || *char_it != c) {
			vec.emplace(char_it, c);
			_dirty = true;
		}
	}
}

const Vector<char16_t> & FontSource::addTextureChars(const String &layout, const Vector<CharSpec> &l) {
	auto &vec = getTextureLayout(layout);
	for (auto &it : l) {
		const char16_t &c = it.charID;
		auto char_it = std::lower_bound(vec.begin(), vec.end(), c);
		if (char_it == vec.end() || *char_it != c) {
			vec.emplace(char_it, c);
			_dirty = true;
		}
	}
	return vec;
}

const Vector<char16_t> & FontSource::addTextureChars(const String &layout, const Vector<CharSpec> &l, uint32_t start, uint32_t count) {
	auto &vec = getTextureLayout(layout);
	const uint32_t end = start + count;
	for (uint32_t i = start; i < end; ++ i) {
		const char16_t &c = l[i].charID;
		auto char_it = std::lower_bound(vec.begin(), vec.end(), c);
		if (char_it == vec.end() || *char_it != c) {
			vec.emplace(char_it, c);
			_dirty = true;
		}
	}
	return vec;
}

Vector<char16_t> &FontSource::getTextureLayout(const String &layout) {
	auto it = _textureLayouts.find(layout);
	if (it == _textureLayouts.end()) {
		it = _textureLayouts.emplace(layout, Vector<char16_t>()).first;
		it->second.reserve(128);
	}
	return it->second;
}

const Map<String, Vector<char16_t>> &FontSource::getTextureLayoutMap() const {
	return _textureLayouts;
}

void FontSource::onResult(uint32_t v) {
	if (!_dirty) {
		_textureVersion = v;
	}
}

uint32_t FontSource::getVersion() const {
	return _version.load();
}

uint32_t FontSource::getTextureVersion() const {
	return _textureVersion;
}

bool FontSource::isTextureRequestValid(uint32_t v) const {
	return v == maxOf<uint32_t>() ||  _version.load() == v;
}

bool FontSource::isDirty() const {
	return _dirty;
}

const FontSource::SearchDirs &FontSource::getSearchDirs() const {
	return _searchDirs;
}

void FontSource::mergeFontFace(FontFaceMap &target, const FontFaceMap &map) {
	for (auto &m_it : map) {
		auto it = target.find(m_it.first);
		if (it == target.end()) {
			target.emplace(m_it.first, m_it.second);
		} else {
			for (auto &v_it : m_it.second) {
				it->second.emplace_back(v_it);
			}
		}
	}
}

NS_LAYOUT_END
