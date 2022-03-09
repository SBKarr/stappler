// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017-2018 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPThreadManager.h"
#include "SPTextureCache.h"
#include "SPResource.h"
#include "SPDynamicLabel.h"
#include "SPDevice.h"
#include "SPDynamicBatchScene.h"
#include "base/CCDirector.h"
#include "renderer/CCTexture2D.h"

NS_SP_EXT_BEGIN(font)

class FontLibrary;
class FontLibraryCache;

class FontLibraryCache : public layout::FreeTypeInterface {
public:
	bool init(const String &, FontLibrary *);
	bool init(BytesView, FontLibrary *);

	void update();
	Time getTimer() const;

protected:
	FontLibrary *library = nullptr;
	Time timer;
};

class FontLibrary {
public:
	static FontLibrary *getInstance();
	FontLibrary();
	~FontLibrary();
	void update(float dt);

	Rc<FontLibraryCache> getCache();

	Vector<size_t> getQuadsCount(const layout::FormatSpec *format, const Map<String, Vector<CharTexture>> &layouts, size_t texSize);

	bool writeTextureQuads(uint32_t v, FontSource *, const layout::FormatSpec *, const Vector<Rc<cocos2d::Texture2D>> &,
			Vector<Rc<DynamicQuadArray>> &, Vector<Vector<bool>> &);

	bool writeTextureQuads(uint32_t v, FontSource *, const layout::FontTextureMap &, const layout::FormatSpec *,
			const Vector<Rc<cocos2d::Texture2D>> &, Vector<Rc<DynamicQuadArray>> &, Vector<Vector<bool>> &);

	bool writeTextureRects(uint32_t v, FontSource *, const layout::FormatSpec *, float scale, Vector<Rect> &);

	bool isSourceRequestValid(layout::FontSource *, uint32_t);

	void cleanupSource(FontSource *);
	Rc<layout::FontTextureLayout> getSourceLayout(FontSource *);
	void setSourceLayout(FontSource *, Rc<layout::FontTextureLayout> &&);

protected:
	void writeTextureQuad(const layout::FormatSpec *format, const layout::Metrics &m, const layout::CharSpec &c, const layout::CharLayout &l, const layout::CharTexture &t,
			const layout::RangeSpec &range, const layout::LineSpec &line, Vector<bool> &cMap, const cocos2d::Texture2D *tex, DynamicQuadArray *quad);

	std::mutex _mutex;
	Time _timer;
	Map<uint64_t, Rc<FontLibraryCache>> _cache;

	std::mutex _sourcesMutex;
	Map<FontSource *, Rc<layout::FontTextureLayout>> _sources;

	static FontLibrary *s_instance;
	static std::mutex s_mutex;
};

bool FontLibraryCache::init(const String &str, FontLibrary *lib) {
	if (!FreeTypeInterface::init(str)) {
		return false;
	}

	library = lib;
	timer = Time::now();

	return true;
}

bool FontLibraryCache::init(BytesView b, FontLibrary *lib) {
	if (!FreeTypeInterface::init(b)) {
		return false;
	}

	library = lib;
	timer = Time::now();

	return true;
}

void FontLibraryCache::update() {
	timer = Time::now();
}

Time FontLibraryCache::getTimer() const {
	return timer;
}

FontLibrary *FontLibrary::s_instance = nullptr;
std::mutex FontLibrary::s_mutex;

void FontLibrary::cleanupSource(FontSource *source) {
	_sourcesMutex.lock();
	if (_sources.erase(source) == 0) {
		log::text("FontLibrary", "Warning: cleanup empty source!");
	}
	_sourcesMutex.unlock();
}

Rc<layout::FontTextureLayout> FontLibrary::getSourceLayout(FontSource *source) {
	Rc<layout::FontTextureLayout> ret;
	_sourcesMutex.lock();
	auto it = _sources.find(source);
	if (it != _sources.end()) {
		ret = it->second;
	}
	_sourcesMutex.unlock();
	return ret;
}

void FontLibrary::setSourceLayout(FontSource *source, Rc<layout::FontTextureLayout> &&map) {
	Rc<layout::FontTextureLayout> ret;
	_sourcesMutex.lock();
	auto it = _sources.find(source);
	if (it != _sources.end()) {
		it->second = move(map);
	} else {
		_sources.emplace(source, move(map));
	}
	_sourcesMutex.unlock();
}

bool FontLibrary::isSourceRequestValid(layout::FontSource *source, uint32_t v) {
	return source->isTextureRequestValid(v);
}

FontLibrary *FontLibrary::getInstance() {
	s_mutex.lock();
	if (!s_instance) {
		s_instance = new FontLibrary;
	}
	s_mutex.unlock();
	return s_instance;
}

FontLibrary::FontLibrary() {
	Thread::onMainThread([this] {
		auto scheduler = cocos2d::Director::getInstance()->getScheduler();
		scheduler->scheduleUpdate(this, 0, false);
	});
	_timer = Time::now();
}

FontLibrary::~FontLibrary() {
	cocos2d::Director::getInstance()->getScheduler()->unscheduleUpdate(this);
}

void FontLibrary::update(float dt) {
	auto now = Time::now();
	if (now - _timer > TimeInterval::seconds(1)) {
		_timer = Time::now();

		_mutex.lock();

		Vector<uint64_t> expired;

		for (auto &it : _cache) {
			if (now - it.second->getTimer() < TimeInterval::seconds(1)) {
				expired.emplace_back(it.first);
			}
		}

		for (auto &it : expired) {
			_cache.erase(it);
		}

		_mutex.unlock();
	}
}

Rc<FontLibraryCache> FontLibrary::getCache() {
	Rc<FontLibraryCache> ret;
	_mutex.lock();
	auto id = ThreadManager::getInstance()->getNativeThreadId();
	auto it = _cache.find(id);
	if (it != _cache.end()) {
		ret = it->second;
		ret->update();
	} else {
		ret = _cache.emplace(id, Rc<FontLibraryCache>::create(resource::getFallbackFont(), this)).first->second;
	}


	_mutex.unlock();
	return ret;
}

Vector<size_t> FontLibrary::getQuadsCount(const layout::FormatSpec *format, const Map<String, Vector<CharTexture>> &layouts, size_t texSize) {
	Vector<size_t> ret; ret.resize(texSize);

	const layout::RangeSpec *targetRange = nullptr;
	Map<String, Vector<CharTexture>>::const_iterator texVecIt;

	for (auto it = format->begin(); it != format->end(); ++ it) {
		if (&(*it.range) != targetRange) {
			targetRange = &(*it.range);
			texVecIt = layouts.find(format->source->getFontName(it.range->layout));
			if (texVecIt == layouts.cend()) {
				return Vector<size_t>();
			}
		}

		const auto start = it.start();
		auto end = start + it.count();
		if (it.line->start + it.line->count == end) {
			const layout::CharSpec &c = format->chars[end - 1];
			if (!string::isspace(c.charID) && c.charID != char16_t(0x0A)) {
				const auto texIt = std::lower_bound(texVecIt->second.cbegin(), texVecIt->second.cend(), c.charID);
				if (texIt != texVecIt->second.cend() && texIt->charID == c.charID) {
					++ (ret[texIt->texture]);
				}
			}
			end -= 1;
		}

		for (auto charIdx = start; charIdx < end; ++ charIdx) {
			const layout::CharSpec &c = format->chars[charIdx];
			if (!string::isspace(c.charID) && c.charID != char16_t(0x0A) && c.charID != char16_t(0x00AD)) {
				const auto texIt = std::lower_bound(texVecIt->second.cbegin(), texVecIt->second.cend(), c.charID);
				if (texIt != texVecIt->second.cend() && texIt->charID == c.charID && texIt->texture != 0xFF) {
					++ (ret[texIt->texture]);
				}
			}
		}
	}

	return ret;
}

void FontLibrary::writeTextureQuad(const layout::FormatSpec *format, const layout::Metrics &m, const layout::CharSpec &c, const layout::CharLayout &l, const layout::CharTexture &t,
		const layout::RangeSpec &range, const layout::LineSpec &line, Vector<bool> &cMap, const cocos2d::Texture2D *tex, DynamicQuadArray *quad) {
	cMap.push_back(range.colorDirty);
	cMap.push_back(range.opacityDirty);
	switch (range.align) {
	case layout::VerticalAlign::Sub:
		quad->drawChar(m, l, t, c.pos, format->height - line.pos + m.descender / 2, range.color, range.decoration, tex->getPixelsWide(), tex->getPixelsHigh());
		break;
	case layout::VerticalAlign::Super:
		quad->drawChar(m, l, t, c.pos, format->height - line.pos + m.ascender / 2, range.color, range.decoration, tex->getPixelsWide(), tex->getPixelsHigh());
		break;
	default:
		quad->drawChar(m, l, t, c.pos, format->height - line.pos, range.color, range.decoration, tex->getPixelsWide(), tex->getPixelsHigh());
		break;
	}
}

bool FontLibrary::writeTextureQuads(uint32_t v, FontSource *source, const layout::FormatSpec *format, const Vector<Rc<cocos2d::Texture2D>> &texs,
		Vector<Rc<DynamicQuadArray>> &quads, Vector<Vector<bool>> &colorMap) {
	if (!isSourceRequestValid(source, v)) {
		return false;
	}

	auto layoutsRef = getSourceLayout(source);
	if (!layoutsRef) {
		return false;
	}

	if (layoutsRef->index != v) {
		return false;
	}

	return writeTextureQuads(v, source, layoutsRef->map, format, texs, quads, colorMap);
}

bool FontLibrary::writeTextureQuads(uint32_t v, FontSource *source, const layout::FontTextureMap &layouts, const layout::FormatSpec *format,
		const Vector<Rc<cocos2d::Texture2D>> &texs, Vector<Rc<DynamicQuadArray>> &quads, Vector<Vector<bool>> &colorMap) {
	colorMap.resize(texs.size());
	quads.reserve(texs.size());
	for (size_t i = 0; i < texs.size(); ++ i) {
		quads.push_back(Rc<DynamicQuadArray>::alloc());
	}

	auto sizes = getQuadsCount(format, layouts, texs.size());
	for (size_t i = 0; i < sizes.size(); ++ i) {
		quads[i]->setCapacity(sizes[i]);
		colorMap[i].reserve(sizes[i] * 2);
	}

	const layout::RangeSpec *targetRange = nullptr;
	Map<String, Vector<CharTexture>>::const_iterator texVecIt;
	Rc<layout::FontData> data;
	const layout::Metrics *metrics = nullptr;
	const Vector<layout::CharLayout> *charVec = nullptr;

	for (auto it = format->begin(); it != format->end(); ++ it) {
		if (it.count() == 0) {
			continue;
		}

		if (&(*it.range) != targetRange) {
			if (!isSourceRequestValid(source, v)) {
				return false;
			}
			targetRange = &(*it.range);
			texVecIt = layouts.find(format->source->getFontName(targetRange->layout));
			if (texVecIt == layouts.end()) {
				return false;
			}

			data = format->source->getData(targetRange->layout);
			metrics = &data->metrics;
			charVec = &data->chars;
		}

		const auto start = it.start();
		auto end = start + it.count();

		for (auto charIdx = start; charIdx < end; ++ charIdx) {
			const layout::CharSpec &c = format->chars[charIdx];
			if (!string::isspace(c.charID) && c.charID != char16_t(0x0A) && c.charID != char16_t(0x00AD)) {
				auto texIt = std::lower_bound(texVecIt->second.cbegin(), texVecIt->second.cend(), c.charID);
				auto charIt = std::lower_bound(charVec->cbegin(), charVec->cend(), c.charID);

				if (texIt != texVecIt->second.end() && texIt->charID == c.charID && charIt != charVec->end() && charIt->charID == c.charID && texIt->texture != maxOf<uint8_t>()) {
					writeTextureQuad(format, *metrics, c, *charIt, *texIt, *it.range, *it.line, colorMap[texIt->texture], texs[texIt->texture], quads[texIt->texture]);
				}
			}
		}

		if (it.line->start + it.line->count == end) {
			const layout::CharSpec &c = format->chars[end - 1];
			if (c.charID == char16_t(0x00AD)) {
				auto texIt = std::lower_bound(texVecIt->second.cbegin(), texVecIt->second.cend(), c.charID);
				auto charIt = std::lower_bound(charVec->cbegin(), charVec->cend(), c.charID);

				if (texIt != texVecIt->second.end() && texIt->charID == c.charID && charIt != charVec->end() && charIt->charID == c.charID && texIt->texture != maxOf<uint8_t>()) {
					writeTextureQuad(format, *metrics, c, *charIt, *texIt, *it.range, *it.line, colorMap[texIt->texture], texs[texIt->texture], quads[texIt->texture]);
				}
			}
			end -= 1;
		}

		if (it.count() > 0 && it.range->decoration != layout::style::TextDecoration::None) {
			const font::CharSpec &firstChar = format->chars[it.start()];
			const font::CharSpec &lastChar = format->chars[it.start() + it.count() - 1];

			auto color = it.range->color;
			color.a = uint8_t(0.75f * color.a);
			Rc<font::FontData> data(format->source->getData(it.range->layout));

			float offset = 0.0f;
			switch (it.range->decoration) {
			case layout::style::TextDecoration::None: break;
			case layout::style::TextDecoration::Overline:
				offset = data->metrics.height;
				break;
			case layout::style::TextDecoration::LineThrough:
				offset = data->metrics.height / 2.0f;
				break;
			case layout::style::TextDecoration::Underline:
				offset = data->metrics.height / 8.0f;
				break;
			}

			const float width = data->metrics.height / 16.0f;
			const float base = floorf(width);
			const float frac = width - base;

			quads[0]->drawRect(firstChar.pos, format->height - it.line->pos + offset, lastChar.pos + lastChar.advance - firstChar.pos, base,
					color, texs[0]->getPixelsWide(), texs[0]->getPixelsHigh());
			if (frac > 0.1) {
				color.a *= frac;
				quads[0]->drawRect(firstChar.pos, format->height - it.line->pos + offset - base + 1, lastChar.pos + lastChar.advance - firstChar.pos, 1,
						color, texs[0]->getPixelsWide(), texs[0]->getPixelsHigh());
			}
		}
	}

	return true;
}

bool FontLibrary::writeTextureRects(uint32_t v, FontSource *source, const layout::FormatSpec *format, float scale, Vector<Rect> &rects) {
	if (!isSourceRequestValid(source, v)) {
		return false;
	}

	rects.reserve(format->chars.size());

	auto layoutsRef = getSourceLayout(source);
	if (!layoutsRef) {
		return false;
	}

	if (layoutsRef->index != v) {
		return false;
	}

	auto &layouts = layoutsRef->map;

	const layout::RangeSpec *targetRange = nullptr;
	Map<String, Vector<CharTexture>>::const_iterator texVecIt;
	Rc<layout::FontData> data;
	const layout::Metrics *metrics = nullptr;
	const Vector<layout::CharLayout> *charVec = nullptr;

	for (auto it = format->begin(); it != format->end(); ++ it) {
		if (it.count() == 0) {
			continue;
		}

		if (&(*it.range) != targetRange) {
			if (!isSourceRequestValid(source, v)) {
				return false;
			}
			targetRange = &(*it.range);
			texVecIt = layouts.find(format->source->getFontName(targetRange->layout));
			if (texVecIt == layouts.end()) {
				return false;
			}
			data = format->source->getData(targetRange->layout);
			metrics = &data->metrics;
			charVec = &data->chars;
		}

		const auto start = it.start();
		const auto end = start + it.count();

		for (auto charIdx = start; charIdx < end; ++ charIdx) {
			const layout::CharSpec &c = format->chars[charIdx];
			if (!string::isspace(c.charID) && c.charID != char16_t(0x0A) && c.charID != char16_t(0x00AD)) {
				auto texIt = std::lower_bound(texVecIt->second.cbegin(), texVecIt->second.cend(), c.charID);
				auto charIt = std::lower_bound(charVec->cbegin(), charVec->cend(), c.charID);

				if (texIt != texVecIt->second.end() && texIt->charID == c.charID && charIt != charVec->end() && charIt->charID == c.charID && texIt->texture != maxOf<uint8_t>()) {
					const auto posX = c.pos + charIt->xOffset;
					const auto posY = format->height - it.line->pos - (charIt->yOffset + texIt->height) - metrics->descender;
					switch (it.range->align) {
					case layout::VerticalAlign::Sub:
						rects.emplace_back(posX * scale, (posY + metrics->descender / 2) * scale, texIt->width * scale, texIt->height * scale);
						break;
					case layout::VerticalAlign::Super:
						rects.emplace_back(posX * scale, (posY + metrics->ascender / 2) * scale, texIt->width * scale, texIt->height * scale);
						break;
					default:
						rects.emplace_back(posX * scale, posY * scale, texIt->width * scale, texIt->height * scale);
						break;
					}
				}
			}
		}
	}

	return true;
}


SP_DECLARE_EVENT(FontSource, "DynamicFontSource", onTextureUpdated);

Bytes FontSource::acquireFontData(const layout::FontSource *lsource, const String &path, const layout::ReceiptCallback &cb) {
	auto source = static_cast<const FontSource *>(lsource);
	if (cb) {
		Bytes ret = cb(source, path);
		if (!ret.empty()) {
			return ret;
		}
	}
	if (!resource::isReceiptUrl(path)) {
		if (filesystem::exists(path) && !filesystem::isdir(path)) {
			return filesystem::readIntoMemory(path);
		}
		auto &dirs = source->getSearchDirs();
		for (auto &it : dirs) {
			auto ipath = filepath::merge(it, path);
			if (filesystem::exists(ipath) && !filesystem::isdir(ipath)) {
				return filesystem::readIntoMemory(ipath);
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

bool FontSource::init(FontFaceMap &&map, const ReceiptCallback &cb, float scale, SearchDirs &&dirs, AssetMap &&assets, bool scheduled) {
	if (!layout::FontSource::init(std::move(map),
			std::bind(&FontSource::acquireFontData, std::placeholders::_1, std::placeholders::_2, cb),
			scale * screen::density(), std::move(dirs))) {
		return false;
	}

	_updateCallback = [this] (uint32_t v, const Map<String, Vector<char16_t>> &m) {
		updateTexture(v, m);
	};

	_metricCallback = [] (const layout::FontSource *source, const Vector<FontFace::FontFaceSource> &srcs,
			layout::FontSize size, const ReceiptCallback &cb) {
		return requestMetrics(source, srcs, size, cb);
	};

	_layoutCallback = [] (const layout::FontSource *source, const Vector<FontFace::FontFaceSource> &srcs,
			const Rc<FontData> &data, const Vector<char16_t> &chars, const ReceiptCallback &cb) {
		return requestLayoutUpgrade(source, srcs, data, chars, cb);
	};

	_assets = std::move(assets);

	onEvent(Device::onRegenerateResources, [this] (const Event &) {
		for (auto &it : _textures) {
			it->init(it->getPixelFormat(), it->getPixelsWide(), it->getPixelsHigh());
		}
		_dirty = true;
		log::text("FontSource", "onAndroidReset");
	});

	if (scheduled) {
		schedule();
	}

	return true;
}

FontSource::~FontSource() {
	unschedule();
	cleanup();
}

void FontSource::update(float dt) {
	layout::FontSource::update();
}

void FontSource::schedule() {
	if (!_scheduled) {
		cocos2d::Director::getInstance()->getScheduler()->scheduleUpdate(this, 0, false);
		_scheduled = true;
	}
}

void FontSource::unschedule() {
	if (_scheduled) {
		_scheduled = false;
		cocos2d::Director::getInstance()->getScheduler()->unscheduleUpdate(this);
	}
}

void FontSource::cleanup() {
	FontSource *s = this;
	auto &thread = TextureCache::thread();
	thread.perform([s] (const thread::Task &) -> bool {
		auto cache = FontLibrary::getInstance();
		cache->cleanupSource(s);
		return true;
	});
}

FontSource::FontTextureMap FontSource::updateTextures(const Map<String, Vector<char16_t>> &l, Vector<Rc<cocos2d::Texture2D>> &tPtr) {
	layout::FreeTypeInterface::FontTextureInterface iface;
	iface.emplaceTexture = [&] (uint16_t w, uint16_t h) -> size_t {
		tPtr.emplace_back(Rc<cocos2d::Texture2D>::create(cocos2d::Texture2D::PixelFormat::A8, w, h));
		tPtr.back()->updateWithData("\xFF", w-1, h-1, 1, 1);
		tPtr.back()->setAliasTexParameters();
		return tPtr.size() - 1;
	};
	iface.draw = [&] (size_t idx, const void *data, uint16_t offsetX, uint16_t offsetY, uint16_t width, uint16_t height) -> bool {
		cocos2d::Texture2D * t = tPtr.at(idx);
		t->updateWithData(data, offsetX, offsetY, width, height);
		return true;
	};

	auto lib = FontLibrary::getInstance();
	auto cache = lib->getCache();
	return cache->updateTextureWithSource(maxOf<uint32_t>(), this, l, iface);
}

void FontSource::onTextureResult(Vector<Rc<cocos2d::Texture2D>> &&tex, uint32_t v) {
	if (!_dirty) {
		onResult(v);
		_textures = std::move(tex);

		onTextureUpdated(this);

		// old textures can be cached in autobatching materials, so, better to clean them up
		if (auto scene = dynamic_cast<DynamicBatchScene *>(cocos2d::Director::getInstance()->getRunningScene())) {
			scene->clearCachedMaterials();
		}
	}
}

void FontSource::onTextureResult(Map<String, Vector<char16_t>> &&map, Vector<Rc<cocos2d::Texture2D>> &&tex, uint32_t v) {
	onResult(v);
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

bool FontSource::doUpdateTexture(uint32_t v, Vector<Rc<cocos2d::Texture2D>> &tPtr, const Map<String, Vector<char16_t>> &lPtr) {
	layout::FreeTypeInterface::FontTextureInterface iface;
	iface.emplaceTexture = [&] (uint16_t w, uint16_t h) -> size_t {
		tPtr.emplace_back(Rc<cocos2d::Texture2D>::create(cocos2d::Texture2D::PixelFormat::A8, w, h));
		tPtr.back()->updateWithData("\xFF", w-1, h-1, 1, 1);
		tPtr.back()->setAliasTexParameters();
		return tPtr.size() - 1;
	};
	iface.draw = [&] (size_t idx, const void *data, uint16_t offsetX, uint16_t offsetY, uint16_t width, uint16_t height) -> bool {
		cocos2d::Texture2D * t = tPtr.at(idx);
		t->updateWithData(data, offsetX, offsetY, width, height);
		return true;
	};

	auto lib = FontLibrary::getInstance();
	auto cache = lib->getCache();
	auto uret = cache->updateTextureWithSource(v, this, lPtr, iface);
	if (!uret.empty()) {
		lib->setSourceLayout(this, Rc<layout::FontTextureLayout>::create(v, move(uret)));
		return true;
	}
	return false;
}

void FontSource::updateTexture(uint32_t v, const Map<String, Vector<char16_t>> &l) {
	auto &thread = TextureCache::thread();
	if (thread.isOnThisThread()) {
		Vector<Rc<cocos2d::Texture2D>> tPtr;
		TextureCache::getInstance()->performWithGL([&] {
			if (doUpdateTexture(v, tPtr, l)) {
				onTextureResult(std::move(tPtr), v);
			}
		});
	} else {
		auto lPtr = new Map<String, Vector<char16_t>>(l);
		auto tPtr = new Vector<Rc<cocos2d::Texture2D>>();
		thread.perform([this, lPtr, tPtr, v] (const thread::Task &) -> bool {
			auto ret = TextureCache::getInstance()->performWithGL([&] {
				return doUpdateTexture(v, *tPtr, *lPtr);
			});
			delete lPtr;
			return ret;
		}, [this, tPtr, v] (const thread::Task &, bool success) {
			if (success) {
				onTextureResult(std::move(*tPtr), v);
			}
			delete tPtr;
		}, this);
	}
}

void FontSource::clone(FontSource *source, const Function<void(FontSource *)> &cb) {
	if (source->getLayoutMap().empty()) {
		if (cb) {
			cb(this);
		}
	}
	auto lPtr = new Map<String, Rc<FontLayout>>(source->getLayoutMap());
	auto tlPtr = new Map<String, Vector<char16_t>>(source->getTextureLayoutMap());
	auto tPtr = new Vector<Rc<cocos2d::Texture2D>>();
	auto sPtr = new Rc<FontSource>(source);
	uint32_t *vPtr = new uint32_t(0);

	auto &thread = TextureCache::thread();
	thread.perform([this, lPtr, tlPtr, tPtr, vPtr] (const thread::Task &) -> bool {
		Vector<char16_t> vec;;
		for (auto &it : (*lPtr)) {
			Rc<FontLayout> l(it.second);
			auto data = l->getData();
			vec.clear();
			if (vec.capacity() < data->chars.size()) {
				vec.reserve(data->chars.size());
			}
			for (auto &c : data->chars) {
				vec.emplace_back(c);
			}

			getLayout(l)->addSortedChars(vec);
		}

		auto ret = TextureCache::getInstance()->performWithGL([&] {
			layout::FreeTypeInterface::FontTextureInterface iface;
			iface.emplaceTexture = [tPtr] (uint16_t w, uint16_t h) -> size_t {
				tPtr->emplace_back(Rc<cocos2d::Texture2D>::create(cocos2d::Texture2D::PixelFormat::A8, w, h));
				tPtr->back()->updateWithData("\xFF", w-1, h-1, 1, 1);
				tPtr->back()->setAliasTexParameters();
				return tPtr->size() - 1;
			};
			iface.draw = [tPtr] (size_t idx, const void *data, uint16_t offsetX, uint16_t offsetY, uint16_t width, uint16_t height) -> bool {
				cocos2d::Texture2D * t = tPtr->at(idx);
				t->updateWithData(data, offsetX, offsetY, width, height);
				return true;
			};

			auto lib = FontLibrary::getInstance();
			auto cache = lib->getCache();
			*vPtr = _version.load();
			auto ret = cache->updateTextureWithSource(*vPtr, this, *tlPtr, iface);
			if (!ret.empty()) {
				lib->setSourceLayout(this, Rc<layout::FontTextureLayout>::create(*vPtr, move(ret)));
				return true;
			}
			return false;
		});
		delete lPtr;
		return ret;
	}, [this, tlPtr, tPtr, sPtr, vPtr, cb] (const thread::Task &, bool success) {
		if (success) {
			onTextureResult(std::move(*tlPtr), std::move(*tPtr), *vPtr);
			if (cb) {
				cb(this);
			}
		}
		delete tlPtr;
		delete tPtr;
		delete sPtr;
		delete vPtr;
	}, this);
}

Metrics FontSource::requestMetrics(const layout::FontSource *source, const Vector<FontFace::FontFaceSource> &srcs,
		layout::FontSize size, const ReceiptCallback &cb) {
	auto cache = FontLibrary::getInstance()->getCache();
	return cache->requestMetrics(source, srcs, size, cb);
}

Rc<FontData> FontSource::requestLayoutUpgrade(const layout::FontSource *source, const Vector<FontFace::FontFaceSource> &srcs, const Rc<FontData> &data, const Vector<char16_t> &chars, const ReceiptCallback &cb) {
	auto cache = FontLibrary::getInstance()->getCache();
	return cache->requestLayoutUpgrade(source, srcs, data, chars, cb);
}

const FontSource::AssetMap &FontSource::getAssetMap() const {
	return _assets;
}

cocos2d::Texture2D *FontSource::getTexture(uint8_t idx) const {
	return _textures.at(idx);
}

const Vector<Rc<cocos2d::Texture2D>> &FontSource::getTextures() const {
	return _textures;
}

SP_DECLARE_EVENT(FontController, "DynamicFontController", onUpdate);
SP_DECLARE_EVENT(FontController, "DynamicFontController", onSource);

FontController::~FontController() {
	cocos2d::Director::getInstance()->getScheduler()->unscheduleUpdate(this);
}

bool FontController::init(FontFaceMap && map, Vector<String> && searchDir, const ReceiptCallback &cb, bool scheduled) {
	if (_fontFaces.empty()) {
		_fontFaces = std::move(map);
	} else {
		FontSource::mergeFontFace(_fontFaces, map);
	}
	_searchDir = std::move(searchDir);

	_callback = cb;
	_scheduled = scheduled;
	_dirty = true;
	_dirtyFlags = DirtyFontFace;

	cocos2d::Director::getInstance()->getScheduler()->scheduleUpdate(this, 0, false);

	return true;
}

bool FontController::init(FontFaceMap && map, Vector<String> && searchDir, float scale, const ReceiptCallback &cb, bool scheduled) {
	if (_fontFaces.empty()) {
		_fontFaces = std::move(map);
	} else {
		FontSource::mergeFontFace(_fontFaces, map);
	}
	_searchDir = std::move(searchDir);

	_callback = cb;
	_scheduled = scheduled;
	_scale = scale;
	_dirty = true;
	_dirtyFlags = DirtyFontFace;

	cocos2d::Director::getInstance()->getScheduler()->scheduleUpdate(this, 0, false);

	return true;
}

void FontController::setSearchDirs(Vector<String> && map) {
	_searchDir = std::move(map);
	_dirtyFlags |= DirtySearchDirs;
	_dirty = true;
}
void FontController::addSearchDir(const Vector<String> &map) {
	for (auto &it : map) {
		_searchDir.push_back(it);
	}
	_dirtyFlags |= DirtySearchDirs;
	_dirty = true;
}
void FontController::addSearchDir(const String &it) {
	_searchDir.push_back(it);
	_dirtyFlags |= DirtySearchDirs;
	_dirty = true;
}
const Vector<String> &FontController::getSearchDir() const {
	return _searchDir;
}

void FontController::setFontFaceMap(FontFaceMap && map) {
	_fontFaces = std::move(map);
	_dirty = true;
	_dirtyFlags |= DirtyFontFace;
}
void FontController::addFontFaceMap(const FontFaceMap & map) {
	FontSource::mergeFontFace(_fontFaces, map);
	_dirtyFlags |= DirtyFontFace;
	_dirty = true;
}
void FontController::addFontFace(const String &family, FontFace &&face) {
	auto it = _fontFaces.find(family);
	if (it == _fontFaces.end()) {
		it = _fontFaces.emplace(family, Vector<FontFace>()).first;
	}

	it->second.emplace_back(std::move(face));
	_dirtyFlags |= DirtyFontFace;
	_dirty = true;
}
const FontController::FontFaceMap &FontController::getFontFaceMap() const {
	return _fontFaces;
}

void FontController::setFontScale(float value) {
	if (_scale != value) {
		_scale = value;
		_dirty = true;
	}
}
float FontController::getFontScale() const {
	return _scale;
}

void FontController::setReceiptCallback(const ReceiptCallback &cb) {
	_callback = cb;
	_dirty = true;
	_dirtyFlags |= DirtyReceiptCallback;
}
const ReceiptCallback & FontController::getReceiptCallback() const {
	return _callback;
}

void FontController::update(float dt) {
	if (_dirty) {
		onUpdate(this);
		updateSource();
	}
}

bool FontController::empty() const {
	return _source != nullptr;
}

FontSource * FontController::getSource() const {
	return _source;
}

void FontController::updateSource() {
	_urls.clear();

	for (auto &f_it : _fontFaces) {
		for (auto &vec_it : f_it.second) {
			for (auto &src : vec_it.src) {
				if (!src.file.empty() && resource::isReceiptUrl(src.file)) {
					_urls.insert(src.file);
				}
			}
		}
	}

	if (_urls.empty()) {
		performSourceUpdate();
	} else {
		auto id = retain();
		resource::acquireFontAsset(_urls, std::bind(&FontController::onAssets, this, id, std::placeholders::_1));
	}

	_dirty = false;
}

void FontController::performSourceUpdate() {
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
							assets.emplace(a->getUrl().str(), it->second);
							continue;
						}
					}
				}
				auto file = a->cloneFile();
				if (file) {
					assets.emplace(a->getUrl().str(), std::move(file));
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
			auto source = makeSource(std::move(assets), _source == nullptr);
			if (_source) {
				// we can clone layouts form previous source
				source->clone(_source, [this] (FontSource *source) {
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

Rc<FontSource> FontController::makeSource(AssetMap && map, bool schedule) {
	return Rc<FontSource>::create(FontFaceMap(_fontFaces), _callback, _scale, SearchDirs(_searchDir), std::move(map), _scheduled && schedule);
}

void FontController::onSourceUpdated(FontSource *s) {
	_source = s;
	if (_source && _scheduled) {
		_source->schedule();
	}
	setDirty();
	onSource(this, s);
}

void FontController::onAssets(uint64_t callId, const Vector<Rc<Asset>> &assets) {
	auto oldAssets = std::move(_assets);
	_assets.clear();
	for (auto &it : assets) {
		_assets.emplace_back(std::bind(&FontController::onAssetUpdated, this, it), it);
		if (it->isDownloadAvailable()) {
			it.get()->download();
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
	release(callId);
}

void FontController::onAssetUpdated(Asset *a) {
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


NS_SP_BEGIN

void DynamicLabel::updateQuadsBackground(Source *source, FormatSpec *format) {
	auto v = source->getTextureVersion();
	auto time = Time::now();
	auto sourceRef = new Rc<Source>(source);
	auto formatRef = new Rc<FormatSpec>(format);
	auto tPtr = new Vector<Rc<cocos2d::Texture2D>>(source->getTextures());
	auto qPtr = new Vector<Rc<DynamicQuadArray>>();
	auto cPtr = new Vector<Vector<bool>>();

	auto &thread = TextureCache::thread();
	thread.perform([sourceRef, formatRef, tPtr, qPtr, cPtr, v] (const thread::Task &) -> bool {
		auto cache = font::FontLibrary::getInstance();
		return cache->writeTextureQuads(v, *sourceRef, *formatRef, *tPtr, *qPtr, *cPtr);
	}, [this, time, sourceRef, formatRef, tPtr, qPtr, cPtr] (const thread::Task &, bool success) {
		if (success) {
			onQuads(time, *tPtr, std::move(*qPtr), std::move(*cPtr));
		}
		delete sourceRef;
		delete formatRef;
		delete tPtr;
		delete qPtr;
		delete cPtr;
	}, this);
}

void DynamicLabel::updateQuadsForeground(Source *source, const FormatSpec *format) {
	auto v = source->getTextureVersion();
	auto time = Time::now();

	Vector<Rc<cocos2d::Texture2D>> tPtr(source->getTextures());
	Vector<Rc<DynamicQuadArray>> qPtr;
	Vector<Vector<bool>> cPtr;

	auto cache = font::FontLibrary::getInstance();
	if (cache->writeTextureQuads(v, source, format, tPtr, qPtr, cPtr)) {
		onQuads(time, tPtr, std::move(qPtr), std::move(cPtr));
	}
}

void DynamicLabel::updateQuadsStandalone(Source *source, const FormatSpec *format) {
	uint32_t v = maxOf<uint32_t>();
	auto time = Time::now();

	Vector<Rc<DynamicQuadArray>> qPtr;
	Vector<Vector<bool>> cPtr;

	auto cache = font::FontLibrary::getInstance();
	if (cache->writeTextureQuads(v, source, _standaloneMap, format, _standaloneTextures, qPtr, cPtr)) {
		onQuads(time, _standaloneTextures, std::move(qPtr), std::move(cPtr));
	}
}

void DynamicLabel::makeLabelQuads(Source *source, const FormatSpec *format,
		const Function<void(const TextureVec &newTex, QuadVec &&newQuads, ColorMapVec &&cMap)> &cb) {
	auto v = source->getVersion();

	auto &tPtr = source->getTextures();
	Vector<Rc<DynamicQuadArray>> qPtr;
	Vector<Vector<bool>> cPtr;

	auto cache = font::FontLibrary::getInstance();
	if (cache->writeTextureQuads(v, source, format, tPtr, qPtr, cPtr)) {
		cb(tPtr, std::move(qPtr), std::move(cPtr));
	}
}

void DynamicLabel::makeLabelRects(Source *source, const FormatSpec *format, float scale, const Function<void(const Vector<Rect> &)> &cb) {
	auto v = source->getVersion();
	Vector<Rect> rects;

	auto cache = font::FontLibrary::getInstance();
	if (cache->writeTextureRects(v, source, format, scale, rects)) {
		cb(rects);
	}
}

NS_SP_END
