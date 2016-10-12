/****************************************************************************
 Copyright (c) 2013      Zynga Inc.
 Copyright (c) 2013-2014 Chukong Technologies Inc.

 http://www.cocos2d-x.org

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
 ****************************************************************************/

#include "SPDefine.h"
#include "SPFont.h"
#include "SPImage.h"
#include "renderer/CCTexture2D.h"
#include "SPFilesystem.h"
#include "SPAsset.h"
#include "SPDevice.h"
#include "SPResource.h"
#include "SPThread.h"
#include "SPDynamicBatchScene.h"
#include "SPTextureCache.h"

NS_SP_BEGIN

Font::Font(const std::string &name, Metrics &&metrics, CharSet &&chars, KerningMap &&kerning, Image *image)
: _name(name), _metrics(std::move(metrics)), _chars(std::move(chars)), _kerning(std::move(kerning)), _image(image) {
	for (auto &c : _chars) {
		const_cast<Font::Char &>(c).font = this;
	}
}

Font::~Font() { }

cocos2d::Texture2D *Font::getTexture() const {
	auto tex = getImage()->retainTexture();
	tex->setAliasTexParameters();
	return tex;
}

int16_t Font::kerningAmount(uint16_t first, uint16_t second) const {
	uint32_t key = (first << 16) | (second & 0xffff);
	auto it = _kerning.find(key);
	if (it != _kerning.end()) {
		return it->second;
	}
	return 0;
}

void Font::setFontSet(FontSet *set) {
	_fontSet = set;
}

FontRequest::FontRequest(const std::string &name, uint16_t size, const std::u16string &chars)
: name(name), size(size), chars(chars), charGroupMask(CharGroup::None) { }

FontRequest::FontRequest(const std::string &name, const std::string &file, uint16_t size)
: name(name), size(size), receipt{file}, charGroupMask(CharGroup::None)  { }

FontRequest::FontRequest(const std::string &name, const std::string &file, uint16_t size, const std::u16string &chars)
: name(name), size(size), chars(chars), receipt{file}, charGroupMask(CharGroup::None) { }

FontRequest::FontRequest(const std::string &name, const std::string &file, uint16_t size, CharGroup mask)
: name(name), size(size), receipt{file}, charGroupMask(mask) { }

FontRequest::FontRequest(const std::string &name, Receipt &&receipt, uint16_t size, const std::u16string &chars)
: name(name), size(size), chars(chars), receipt(std::move(receipt)), charGroupMask(CharGroup::None) { }

FontSource::FontSource() { }

FontSource::~FontSource() { }

bool FontSource::init(const std::string &name, uint16_t v, const std::vector<FontRequest> &req, const ReceiptCallback &cb, bool w) {
	_name = name;
	_version = v;
	_requests = req;
	return init(cb, w);
}

bool FontSource::init(const std::string &name, uint16_t v, std::vector<FontRequest> &&req, const ReceiptCallback &cb, bool w) {
	_name = name;
	_version = v;
	_requests = std::move(req);
	return init(cb, w);
}

bool FontSource::init(const std::vector<FontRequest> &req, const ReceiptCallback &cb, bool w) {
	_requests = req;
	return init(cb, w);
}

bool FontSource::init(std::vector<FontRequest> &&req, const ReceiptCallback &cb, bool w) {
	_requests = std::move(req);
	return init(cb, w);
}

bool FontSource::init(const ReceiptCallback &cb, bool wait) {
	_waitForAllAssets = wait;
	_receiptCallback = cb;
	updateRequest();

	onEvent(Device::onNetwork, [this] (const Event *) {
		if (Device::getInstance()->isNetworkOnline()) {
			for (auto &it : _assets) {
				if (it->isDownloadAvailable()) {
					it->download();
				}
			}
		}
	});

	return true;
}

void FontSource::setRequest(const std::vector<FontRequest> &req) {
	_requests = req;
	updateRequest();
}

void FontSource::setRequest(std::vector<FontRequest> &&req) {
	_requests = req;
	updateRequest();
}

FontSet *FontSource::getFontSet() const {
	return _fontSet;
}

void FontSource::onAssets(const std::vector<Asset *> &assets) {
	auto oldAssets = std::move(_assets);
	_assets.clear();
	for (auto &it : assets) {
		_assets.emplace_back(std::bind(&FontSource::onAssetUpdated, this, it), it);
		if (it->isDownloadAvailable()) {
			it->download();
		}
	}
	if (getReferenceCount() > 1) {
		if (!_waitForAllAssets || _assets.empty()) {
			updateFontSet();
		} else {
			bool update = true;
			for (auto &it : _assets) {
				if (!it->isReadAvailable()) {
					update = false;
					break;
				}
			}
			if (update) {
				updateFontSet();
			}
		}
	}
	release();
}

void FontSource::onAssetUpdated(Asset *a) {
	bool downloadReady = true;
	for (auto &it : _assets) {
		if (it->isDownloadInProgress() || it->isWriteLocked() || !it->isReadAvailable()) {
			downloadReady = false;
			break;
		}
	}

	if (downloadReady) {
		bool assetModified = false;
		for (auto &it : _assets) {
			auto assetIt = _runningAssets.find(it);
			if (assetIt == _runningAssets.end()
					|| assetIt->second.first != it->getETag()
					|| assetIt->second.second != it->getMTime()) {
				assetModified = true;
				break;
			}
		}
		if (assetModified) {
			_runningAssets.clear();
			for (auto &it : _assets) {
				_runningAssets.insert(std::make_pair(it, std::make_pair(it->getETag(), it->getMTime())));
			}
			updateFontSet();
		}
	}
}

void FontSource::onFontSet(FontSet *set) {
	if (set != _fontSet) {
		_inUpdate = false;
		_fontSet = set;
		setDirty();
	}
}

void FontSource::updateFontSet() {
	if (!_requests.empty()) {
		retain();
		FontSet::generate(_name.empty()?FontSet::Config(_requests, _receiptCallback):FontSet::Config(_name, _version, _requests, _receiptCallback),
				[this] (FontSet *set) {
			onFontSet(set);
			release();
		});
	}
}

void FontSource::updateRequest() {
	_inUpdate = true;
	_urls = resource::getRequestsUrls(_requests);
	if (_urls.empty()) {
		updateFontSet();
	} else {
		retain();
		resource::acquireFontAsset(_urls, std::bind(&FontSource::onAssets, this, std::placeholders::_1));
	}
}


FontSet::Config::Config(const std::vector<FontRequest> &vec, const ReceiptCallback &cb)
: name(""), version(0), dynamic(true), requests(vec), receiptCallback(cb) { }

FontSet::Config::Config(std::vector<FontRequest> &&vec, const ReceiptCallback &cb)
: name(""), version(0), dynamic(true), requests(std::move(vec)), receiptCallback(cb) { }

FontSet::Config::Config(const std::string &name, uint16_t v, std::vector<FontRequest> &&vec, const ReceiptCallback &cb)
: name(name), version(v), dynamic(false), requests(vec), receiptCallback(cb) { }

FontSet::Config::Config(const std::string &name, uint16_t v, const std::vector<FontRequest> &vec, const ReceiptCallback &cb)
: name(name), version(v), dynamic(false), requests(std::move(vec)), receiptCallback(cb) { }


void FontSet::generate(Config &&cfg, const Callback &callback) {
	resource::generateFontSet(std::move(cfg), callback);
}

Font *FontSet::getFont(const std::string &key) const {
	auto it = _fonts.find(key);
	if (it == _fonts.end()) {
		auto aliasedKey = getFontNameForAlias(key);
		if (!aliasedKey.empty()) {
			it = _fonts.find(aliasedKey);
			if (it != _fonts.end()) {
				return it->second;
			}
		}
		return nullptr;
	}
	return it->second;
}

std::string FontSet::getFontNameForAlias(const std::string &alias) const {
	auto it = _aliases.find(alias);
	if (it == _aliases.end()) {
		return "";
	}
	return it->second;
}

FontSet::FontSet(Config &&cfg, cocos2d::Map<std::string, Font *> &&fonts, Image *image)
: _config(std::move(cfg)), _image(image), _fonts(std::move(fonts)) {
	for (auto &it : _fonts) {
		it.second->setFontSet(this);
	}

	for (auto &it : _config.requests) {
		for (auto &alias : it.aliases) {
			_aliases.insert(std::make_pair(alias, it.name));
		}
	}
}

FontSet::~FontSet() {
	for (auto &it : _fonts) {
		if (it.second->getFontSet() == this) {
			it.second->setFontSet(nullptr);
		}
	}
}

NS_SP_END

NS_SP_EXT_BEGIN(font)

size_t Source::getFontFaceScore(const FontParameters &params, const FontFace &face) {
	using namespace rich_text;
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

FontLayout::FontLayout(const String &name, const String &family, uint8_t size, const FontFace &face, const ReceiptCallback &cb, float d)
: _density(d), _name(name), _family(family), _size(size), _face(face), _callback(cb) {
	_data = Arc<Data>::create();
	_data->metrics = requestMetrics(face.src, uint16_t(roundf(size * d)), _callback);
}

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

void FontLayout::merge(const Vector<char16_t> &chars) {
	Arc<Data> data = _data;
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
		Arc<Data> newData = requestLayoutUpgrade(_face.src, data, charsToUpdate, _callback);
		if (_data.compare_swap(data, std::move(newData))) {
			return;
		}
	}
}

Arc<FontLayout::Data> FontLayout::getData() const {
	return _data;
}

const String &FontLayout::getName() const {
	return _name;
}

const ReceiptCallback &FontLayout::getCallback() const {
	return _callback;
}

const FontFace &FontLayout::getFontFace() const {
	return _face;
}

uint16_t FontLayout::getSize() const {
	return roundf(_size * _density);
}

SP_DECLARE_EVENT(Source, "DynamicFontSource", onLayoutUpdated);
SP_DECLARE_EVENT(Source, "DynamicFontSource", onTextureUpdated);

Source::~Source() {
	cocos2d::Director::getInstance()->getScheduler()->unscheduleUpdate(this);
	cleanup();
}

bool Source::init(FontFaceMap &&map, const ReceiptCallback &cb, float scale) {
	_fontFaces = std::move(map);
	_fontScale = scale;
	_callback = cb;
	_version = 0;
	for (auto &it : _fontFaces) {
		auto &family = it.first;
		_families.emplace(hash::hash32(family.c_str(), family.size()), family);
	}

	onEvent(Device::onAndroidReset, [this] (const Event *) {
		for (auto &it : _textures) {
			it->init(it->getPixelFormat(), it->getPixelsWide(), it->getPixelsHigh());
		}
		_dirty = true;
	});

	cocos2d::Director::getInstance()->getScheduler()->scheduleUpdate(this, 0, false);
	return true;
}

Arc<FontLayout> Source::getLayout(const FontParameters &style) {
	Arc<FontLayout> ret = nullptr;

	auto family = style.fontFamily;
	if (family.empty()) {
		family = "default";
	}

	auto face = getFontFace(family, style);
	if (face) {
		auto name = face->getConfigName(family, style.fontSize);
		_mutex.lock();
		auto l_it = _layouts.find(name);
		if (l_it == _layouts.end()) {
			ret = (_layouts.emplace(name, Arc<FontLayout>::create(name, family, style.fontSize, *face, _callback, screen::density() * _fontScale)).first->second);
		} else {
			ret = (l_it->second);
		}
		_mutex.unlock();
	}

	return ret;
}

Arc<FontLayout> Source::getLayout(const String &name) {
	Arc<FontLayout> ret = nullptr;
	_mutex.lock();
	auto l_it = _layouts.find(name);
	if (l_it != _layouts.end()) {
		ret = (l_it->second);
	}
	_mutex.unlock();
	return ret;
}

Map<String, Arc<FontLayout>> Source::getLayoutMap() {
	Map<String, Arc<FontLayout>> ret;
	_mutex.lock();
	ret = _layouts;
	_mutex.unlock();
	return ret;
}

FontFace * Source::getFontFace(const String &name, const FontParameters &it) {
	size_t score = 0;
	FontFace *face = nullptr;

	auto family = it.fontFamily;
	if (family.empty()) {
		family = "default";
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

	return face;
}

void Source::update(float dt) {
	if (_dirty) {
		auto v = (++ _version);
		updateTexture(v, _textureLayouts);
		_dirty = false;
	}
}

String Source::getFamilyName(uint32_t id) const {
	auto it = _families.find(id);
	if (it != _families.end()) {
		return it->second;
	}
	return String();
}

void Source::addTextureString(const String &layout, const String &str) {
	addTextureString(layout, string::toUtf16Html(str));
}
void Source::addTextureString(const String &layout, const WideString &str) {
	addTextureString(layout, str.data(), str.size());
}
void Source::addTextureString(const String &layout, const char16_t *str, size_t len) {
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

const Vector<char16_t> & Source::addTextureChars(const String &layout, const Vector<CharSpec> &l) {
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

const Vector<char16_t> & Source::addTextureChars(const String &layout, const Vector<CharSpec> &l, uint32_t start, uint32_t count) {
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

Vector<char16_t> &Source::getTextureLayout(const String &layout) {
	auto it = _textureLayouts.find(layout);
	if (it == _textureLayouts.end()) {
		it = _textureLayouts.emplace(layout, Vector<char16_t>()).first;
		it->second.reserve(128);
	}
	return it->second;
}
cocos2d::Texture2D *Source::getTexture(uint8_t idx) const {
	return _textures.at(idx);
}

const Vector<Rc<cocos2d::Texture2D>> &Source::getTextures() const {
	return _textures;
}

void Source::onTextureResult(Vector<Rc<cocos2d::Texture2D>> &&tex) {
	if (!_dirty) {
		_textures = std::move(tex);

		onTextureUpdated(this);

		// old textures can be cached in autobatching materials, so, better to clean them up
		if (auto scene = dynamic_cast<DynamicBatchScene *>(cocos2d::Director::getInstance()->getRunningScene())) {
			scene->clearCachedMaterials();
		}
	}
}

uint32_t Source::getVersion() const {
	return _version.load();
}

bool Source::isTextureRequestValid(uint32_t v) const {
	return _version.load() == v;
}

bool Source::isDirty() const {
	return _dirty;
}

NS_SP_EXT_END(font)
