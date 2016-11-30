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

NS_SP_EXT_BEGIN(font)

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

FontLayout::FontLayout(const Source * source, const String &name, const String &family, uint8_t size, const FontFace &face, const ReceiptCallback &cb, float d)
: _density(d), _name(name), _family(family), _size(size), _face(face), _callback(cb), _source(source) {
	_data = Arc<Data>::create();
	_data->metrics = requestMetrics(_source, face.src, uint16_t(roundf(size * d)), _callback);
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

void FontLayout::addSortedChars(const Vector<char16_t> &vec) {
	merge(vec);
}

void FontLayout::merge(const Vector<char16_t> &chars) {
	Arc<Data> data(getData());
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

		Arc<Data> newData(requestLayoutUpgrade(_source, _face.src, data, charsToUpdate, _callback));
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

Arc<FontLayout::Data> FontLayout::getData() {
	Arc<FontLayout::Data> ret;
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
float FontLayout::getDensity() const {
	return _density;
}
uint8_t FontLayout::getOriginalSize() const {
	return _size;
}

uint16_t FontLayout::getSize() const {
	return roundf(_size * _density);
}

FontParameters FontLayout::getStyle() const {
	return _face.getStyle(_family, _size);
}


SP_DECLARE_EVENT(Source, "DynamicFontSource", onTextureUpdated);

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

Bytes Source::acquireFontData(const Source *source, const String &path, const ReceiptCallback &cb) {
	if (cb) {
		Bytes ret = cb(source, path);
		if (!ret.empty()) {
			return ret;
		}
	}
	if (!resource::isReceiptUrl(path)) {
		if (filesystem::exists(path) && !filesystem::isdir(path)) {
			return filesystem::readFile(path);
		}
		auto &dirs = source->getSearchDirs();
		for (auto &it : dirs) {
			auto ipath = filepath::merge(it, path);
			if (filesystem::exists(ipath) && !filesystem::isdir(ipath)) {
				return filesystem::readFile(ipath);
			}
		}
	} else {
		auto &map = source->getAssetMap();
		auto it = map.find(path);
		if (it != map.end()) {
			return it->second->readFile();
		}
	}
	return Bytes();
}

Source::~Source() {
	unschedule();
	cleanup();
}

void Source::schedule() {
	if (!_scheduled) {
		cocos2d::Director::getInstance()->getScheduler()->scheduleUpdate(this, 0, false);
		_scheduled = true;
	}
}

void Source::unschedule() {
	if (_scheduled) {
		_scheduled = false;
		cocos2d::Director::getInstance()->getScheduler()->unscheduleUpdate(this);
	}
}

void Source::preloadChars(const FontParameters &style, const Vector<char16_t> &chars) {
	auto l = getLayout(style);
	if (l) {
		l->addSortedChars(chars);
		addTextureString(l->getName(), chars.data(), chars.size());
	}
}

bool Source::init(FontFaceMap &&map, const ReceiptCallback &cb, float scale, SearchDirs &&dirs, AssetMap &&assets, bool scheduled) {
	_fontFaces = std::move(map);
	_fontScale = scale;
	_callback = cb;
	_version = 0;
	_assets = std::move(assets);
	_searchDirs = std::move(dirs);
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

	if (scheduled) {
		schedule();
	}

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
			ret = (_layouts.emplace(name, Arc<FontLayout>::create(this, name, family, style.fontSize, *face,
					std::bind(&Source::acquireFontData, std::placeholders::_1, std::placeholders::_2, _callback),
					screen::density() * _fontScale)).first->second);
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

bool Source::hasLayout(const FontParameters &style) {
	bool ret = false;
	auto family = style.fontFamily;
	if (family.empty()) {
		family = "default";
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

bool Source::hasLayout(const String &name) {
	bool ret = false;
	_mutex.lock();
	auto l_it = _layouts.find(name);
	if (l_it != _layouts.end()) {
		ret = true;
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

	if (!face) {
		family = "default";
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

void Source::update(float dt) {
	if (_dirty) {
		auto v = (++ _version);
		_dirty = false;
		updateTexture(v, _textureLayouts);
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

const Map<String, Vector<char16_t>> &Source::getTextureLayoutMap() const {
	return _textureLayouts;
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

void Source::onTextureResult(Map<String, Vector<char16_t>> &&map, Vector<Rc<cocos2d::Texture2D>> &&tex) {
	if (_dirty) {
		_textureLayouts = std::move(map);
		_textures = std::move(tex);

		onTextureUpdated(this);

		Thread::onMainThread([] {
			// old textures can be cached in autobatching materials, so, better to clean them up
			if (auto scene = dynamic_cast<DynamicBatchScene *>(cocos2d::Director::getInstance()->getRunningScene())) {
				scene->clearCachedMaterials();
			}
		});
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

const Source::SearchDirs &Source::getSearchDirs() const {
	return _searchDirs;
}

const Source::AssetMap &Source::getAssetMap() const {
	return _assets;
}

SP_DECLARE_EVENT(Controller, "DynamicFontController", onUpdate);
SP_DECLARE_EVENT(Controller, "DynamicFontController", onSource);

void Controller::mergeFontFace(FontFaceMap &target, const FontFaceMap &map) {
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

Controller::~Controller() {
	cocos2d::Director::getInstance()->getScheduler()->unscheduleUpdate(this);
}

bool Controller::init(FontFaceMap && map, Vector<String> && searchDir, const ReceiptCallback &cb) {
	if (_fontFaces.empty()) {
		_fontFaces = std::move(map);
	} else {
		mergeFontFace(_fontFaces, map);
	}
	_searchDir = std::move(searchDir);

	_callback = cb;
	_dirty = true;
	_dirtyFlags = DirtyFontFace;

	cocos2d::Director::getInstance()->getScheduler()->scheduleUpdate(this, 0, false);

	return true;
}

bool Controller::init(FontFaceMap && map, Vector<String> && searchDir, float scale, const ReceiptCallback &cb) {
	if (_fontFaces.empty()) {
		_fontFaces = std::move(map);
	} else {
		mergeFontFace(_fontFaces, map);
	}
	_searchDir = std::move(searchDir);

	_callback = cb;
	_scale = scale;
	_dirty = true;
	_dirtyFlags = DirtyFontFace;

	cocos2d::Director::getInstance()->getScheduler()->scheduleUpdate(this, 0, false);

	return true;
}

void Controller::setSearchDirs(Vector<String> && map) {
	_searchDir = std::move(map);
	_dirtyFlags |= DirtySearchDirs;
	_dirty = true;
}
void Controller::addSearchDir(const Vector<String> &map) {
	for (auto &it : map) {
		_searchDir.push_back(it);
	}
	_dirtyFlags |= DirtySearchDirs;
	_dirty = true;
}
void Controller::addSearchDir(const String &it) {
	_searchDir.push_back(it);
	_dirtyFlags |= DirtySearchDirs;
	_dirty = true;
}
const Vector<String> &Controller::getSearchDir() const {
	return _searchDir;
}

void Controller::setFontFaceMap(FontFaceMap && map) {
	_fontFaces = std::move(map);
	_dirty = true;
	_dirtyFlags |= DirtyFontFace;
}
void Controller::addFontFaceMap(const FontFaceMap & map) {
	mergeFontFace(_fontFaces, map);
	_dirtyFlags |= DirtyFontFace;
	_dirty = true;
}
void Controller::addFontFace(const String &family, FontFace &&face) {
	auto it = _fontFaces.find(family);
	if (it == _fontFaces.end()) {
		it = _fontFaces.emplace(family, Vector<FontFace>()).first;
	}

	it->second.emplace_back(std::move(face));
	_dirtyFlags |= DirtyFontFace;
	_dirty = true;
}
const Controller::FontFaceMap &Controller::getFontFaceMap() const {
	return _fontFaces;
}

void Controller::setFontScale(float value) {
	if (_scale != value) {
		_scale = value;
		_dirty = true;
	}
}
float Controller::getFontScale() const {
	return _scale;
}

void Controller::setReceiptCallback(const ReceiptCallback &cb) {
	_callback = cb;
	_dirty = true;
	_dirtyFlags |= DirtyReceiptCallback;
}
const ReceiptCallback & Controller::getReceiptCallback() const {
	return _callback;
}

void Controller::update(float dt) {
	if (_dirty) {
		onUpdate(this);
		updateSource();
	}
}

bool Controller::empty() const {
	return _source != nullptr;
}

Source * Controller::getSource() const {
	return _source;
}

void Controller::updateSource() {
	_urls.clear();

	for (auto &f_it : _fontFaces) {
		for (auto &vec_it : f_it.second) {
			for (auto &src : vec_it.src) {
				if (resource::isReceiptUrl(src)) {
					_urls.insert(src);
				}
			}
		}
	}

	if (_urls.empty()) {
		performSourceUpdate();
	} else {
		retain();
		resource::acquireFontAsset(_urls, std::bind(&Controller::onAssets, this, std::placeholders::_1));
	}

	_dirty = false;
}

void Controller::performSourceUpdate() {
	if (_assets.empty()) {
		onSourceUpdated(makeSource(AssetMap()));
	} else {
		const AssetMap *currentMap = nullptr;
		if (_source) {
			currentMap = &_source->getAssetMap();
		}

		AssetMap assets;
		bool assetsDirty = false;
		for (auto &a : _assets) {
			if (a->isReadAvailable() && a->tryReadLock(this)) {
				if (currentMap) {
					auto it = currentMap->find(a->getUrl());
					if (it != currentMap->end()) {
						if (it->second->match(a)) {
							assets.emplace(a->getUrl(), it->second);
							continue;
						}
					}
				}
				auto file = a->cloneFile();
				if (file) {
					assets.emplace(a->getUrl(), std::move(file));
					assetsDirty = true;
				}
				a->releaseReadLock(this);
			}
		}

		if (!assetsDirty && currentMap) {
			assetsDirty = (currentMap->size() != assets.size());
		}

		if (!assetsDirty && _dirtyFlags == DirtyAssets) {
			_dirty = false;
			_dirtyFlags = None;
			return;
		}

		if (_dirtyFlags & DirtyFontFace) {
			onSourceUpdated(makeSource(std::move(assets)));
		} else {
			auto source = makeSource(std::move(assets));
			if (_source) {
				// we can clone layouts form previous source
				source->clone(_source, [this] (Source *source) {
					onSourceUpdated(source);
				});
			} else {
				onSourceUpdated(source);
			}
		}
	}

	_dirty = false;
	_dirtyFlags = None;
}

Rc<Source> Controller::makeSource(AssetMap && map) {
	return Rc<Source>::create(FontFaceMap(_fontFaces), _callback, _scale, SearchDirs(_searchDir), std::move(map));
}

void Controller::onSourceUpdated(Source *s) {
	_source = s;
	setDirty();
	onSource(this, s);
}

void Controller::onAssets(const Vector<Asset *> &assets) {
	auto oldAssets = std::move(_assets);
	_assets.clear();
	for (auto &it : assets) {
		_assets.emplace_back(std::bind(&Controller::onAssetUpdated, this, it), it);
		if (it->isDownloadAvailable()) {
			it->download();
		}
	}
	if (getReferenceCount() > 1) {
		if (_assets.empty()) {
			_dirtyFlags |= DirtyAssets;
			performSourceUpdate();
		} else {
			bool update = true;
			for (auto &it : _assets) {
				if (!it->isReadAvailable()) {
					update = false;
					break;
				}
			}
			if (update) {
				_dirtyFlags |= DirtyAssets;
				performSourceUpdate();
			}
		}
	}
	release();
}

void Controller::onAssetUpdated(Asset *a) {
	bool downloadReady = true;
	for (auto &it : _assets) {
		if (it->isDownloadInProgress() || it->isWriteLocked() || !it->isReadAvailable()) {
			downloadReady = false;
			break;
		}
	}

	if (downloadReady) {
		_dirtyFlags |= DirtyAssets;
		performSourceUpdate();
	}
}

NS_SP_EXT_END(font)
