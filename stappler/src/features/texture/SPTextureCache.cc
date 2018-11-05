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
#include "SPTextureCache.h"
#include "SPBitmap.h"
#include "SPFilesystem.h"
#include "SPUrl.h"
#include "SPThread.h"
#include "SPDevice.h"
#include "SPAsset.h"
#include "SPPlatform.h"
#include "SPDataListener.h"
#include "SPAssetLibrary.h"
#include "SPDrawCanvas.h"
#include "SPDrawCache.h"
#include "SPApplication.h"

#include "base/CCDirector.h"
#include "base/CCScheduler.h"
#include "platform/CCImage.h"
#include "platform/CCGLView.h"

NS_SP_BEGIN

static Thread s_textureCacheThread("TextureCacheThread");
static TextureCache *s_sharedTextureCache = nullptr;

String TextureCache::getPathForUrl(const String &url) {
	auto dir = filesystem::writablePath("NetworkSprites");
	filesystem::mkdir(dir);
	return toString(dir, "/", string::stdlibHashUnsigned(url));
}

bool TextureCache::isCachedTextureUrl(const String &url) {
	auto lib = AssetLibrary::getInstance();
	auto cache = TextureCache::getInstance();

	auto path = getPathForUrl(url);
	auto assetId = lib->getAssetId(url, path);

	if (lib->isLiveAsset(assetId) && cache->hasTexture(path)) {
		return true;
	}
	return false;
}

TextureCache *TextureCache::getInstance() {
	if (!s_sharedTextureCache) {
		s_sharedTextureCache = new TextureCache();
	}
	return s_sharedTextureCache;
}

Thread &TextureCache::thread() {
	if (!s_sharedTextureCache) {
		s_sharedTextureCache = new TextureCache();
	}
	return s_textureCacheThread;
}

void TextureCache::update(float dt) {
	if (_reloadDirty) {
		for (auto &it : _textures) {
			reloadTexture(it.second, it.first);
		}
		_reloadDirty = false;
	}

	Map<TextureIndex, cocos2d::Texture2D *> release;
	for (auto &it : _texturesScore) {
		if (it.first->getReferenceCount() > 1) {
			it.second.first = GetDelayTime();
		} else {
			if (it.second.first <= 0) {
				release.emplace(it.second.second, it.first);
			} else {
				it.second.first -= dt;
			}
		}
	}

	for (auto &it : release) {
		_textures.erase(it.first);
		_texturesScore.erase(it.second);
	}

	if (_textures.empty()) {
		unregisterWithDispatcher();
	}
}

Rc<cocos2d::Texture2D> TextureCache::renderImage(cocos2d::Texture2D *tex, TextureFormat fmt, const layout::Image &image,
		const Size &contentSize, layout::style::Autofit autofit, const Vec2 &autofitPos, float density) {
	return doRenderImage(tex, fmt, image, contentSize, autofit, autofitPos, density);
}

void TextureCache::renderImageInBackground(const Callback &cb, cocos2d::Texture2D *tex, TextureFormat fmt, const layout::Image &image,
		const Size &contentSize, layout::style::Autofit autofit, const Vec2 &autofitPos, float density) {
	auto texPtr = new Rc<cocos2d::Texture2D>(tex);
	auto imagePtr = new Rc<layout::Image>(image.clone());

	s_textureCacheThread.perform([this, texPtr, imagePtr, fmt, contentSize, autofit, autofitPos, density] (const Task &) -> bool {
		return performWithGL([&] {
			*texPtr = doRenderImage(texPtr->get(), fmt, *imagePtr->get(), contentSize, autofit, autofitPos, density);
			return true;
		});
	}, [cb, texPtr, imagePtr] (const Task &, bool) {
		cb(*texPtr);
		delete texPtr;
		delete imagePtr;
	});
}

GLProgramSet *TextureCache::getPrograms() const {
	if (s_textureCacheThread.isOnThisThread()) {
		return _threadProgramSet;
	} else {
		return _mainProgramSet;
	}
}

cocos2d::Texture2D::PixelFormat TextureCache::getPixelFormat(Bitmap::PixelFormat fmt) {
	switch (fmt) {
	case Bitmap::PixelFormat::A8:
		return cocos2d::Texture2D::PixelFormat::A8;
		break;
	case Bitmap::PixelFormat::I8:
		return cocos2d::Texture2D::PixelFormat::I8;
		break;
	case Bitmap::PixelFormat::IA88:
		return cocos2d::Texture2D::PixelFormat::AI88;
		break;
	case Bitmap::PixelFormat::RGBA8888:
		return cocos2d::Texture2D::PixelFormat::RGBA8888;
		break;
	case Bitmap::PixelFormat::RGB888:
		return cocos2d::Texture2D::PixelFormat::RGB888;
		break;
	default:
		break;
	}
	return cocos2d::Texture2D::PixelFormat::AUTO;
}


void TextureCache::addTexture(const StringView &ifile, const Callback &cb, bool forceReload) {
	addTexture(ifile, screen::density(), BitmapFormat::Auto, cb, forceReload);
}
void TextureCache::addTexture(const StringView &ifile, float density, const Callback &cb, bool forceReload) {
	addTexture(ifile, density, BitmapFormat::Auto, cb, forceReload);
}
void TextureCache::addTexture(const StringView &ifile, float density, BitmapFormat fmt, const Callback &cb, bool forceReload) {
	if (ifile.empty()) {
		return;
	}

	TextureIndex index{ifile.str(), fmt, density};
	auto pos = index.file.find("://");
	if (pos != String::npos) {
		Url url(index.file);
		if (url.getScheme() == "file" && (url.getHost() == "localhost" || url.getHost().empty())) {
			index.file = String("/") + toStringConcat(url.getPath(), '/');
		} else {
			auto lib = AssetLibrary::getInstance();
			auto path = getPathForUrl(index.file);
			lib->getAsset([this, density, fmt, forceReload, cb] (Asset *a) {
				if (a) {
					addTexture(a, density, fmt, cb, forceReload);
				} else {
					cb(nullptr);
				}
			}, index.file, path, TimeInterval::seconds(10 * 24 * 60 * 60));
			return;
		}
	}

	if (!forceReload) {
		auto it = _textures.find(index);
		if (it != _textures.end()) {
			if (cb) {
				cb(it->second);
			}
			return;
		}
	}

	auto cbIt = _callbackMap.find(index);
	if (cbIt != _callbackMap.end()) {
		if (cb) {
			cbIt->second.push_back(cb);
		}
	} else {
		CallbackVec vec;
		if (cb) {
			vec.push_back(cb);
		}
		_callbackMap.emplace(index, vec);

		auto &thread = s_textureCacheThread;
        auto imagePtr = new Rc<cocos2d::Texture2D>;
		thread.perform([this, index, imagePtr] (const Task &) -> bool {
			*imagePtr = loadTexture(index);
			return true;
		}, [this, index, imagePtr] (const Task &, bool) {
			auto image = *imagePtr;
			if (image) {
				auto texIt = _textures.find(index);
				if (texIt != _textures.end()) {
					_texturesScore.erase(texIt->second);
				}

				_textures.emplace(index, image);
				_texturesScore.emplace(image, pair(GetDelayTime(), index));

				auto it = _callbackMap.find(index);
				if (it != _callbackMap.end()) {
					for (auto &cb : it->second) {
						cb(image);
					}
					_callbackMap.erase(it);
				}
	            registerWithDispatcher();
	        } else {
				auto it = _callbackMap.find(index);
				if (it != _callbackMap.end()) {
					for (auto &cb : it->second) {
						cb(nullptr);
					}
					_callbackMap.erase(it);
				}
	        }
			delete imagePtr;
		});
	}
}

struct TextureCache::AssetDownloader : public Ref {
	using Callback = Function<void(cocos2d::Texture2D *)>;

	bool init(Asset *a, float d, BitmapFormat fmt, const Callback & cb, bool forceReload) {
		format = fmt;
		density = d;
		force = forceReload;
		callback = cb;
		listener.setCallback(std::bind(&AssetDownloader::onUpdate, this, std::placeholders::_1));
		listener = a;
		if (listener->download()) {
			enabled = true;
			retain();
			return true;
		}
		return false;
	}

	void onUpdate(data::Subscription::Flags f) {
		if (enabled && (f.hasFlag(uint8_t(Asset::Update::FileUpdated)) || f.hasFlag(uint8_t(Asset::Update::DownloadFailed)))) {
			if (listener->isReadAvailable()) {
				TextureCache::getInstance()->addAssetTexture(listener, density, format, callback, force);
			} else {
				callback(nullptr);
			}
			enabled = false;
			release();
		}
	}

	BitmapFormat format = BitmapFormat::Auto;
	float density = screen::density();
	bool enabled = false;
	bool force = false;
	Callback callback;
	data::Listener<Asset> listener;
};

void TextureCache::addTexture(Asset *a, const Callback & cb, bool forceReload) {
	addTexture(a, screen::density(), BitmapFormat::Auto, cb, forceReload);
}
void TextureCache::addTexture(Asset *a, float density, const Callback & cb, bool forceReload) {
	addTexture(a, density, BitmapFormat::Auto, cb, forceReload);
}
void TextureCache::addTexture(Asset *a, float density, BitmapFormat fmt, const Callback & cb, bool forceReload) {
	auto file = a->getFilePath();
	if (!forceReload) {
		auto it = _textures.find(TextureIndex{file.str(), fmt, density});
		if (it != _textures.end()) {
			if (cb) {
				cb(it->second);
			}
			return;
		}
	}

	if (a->isDownloadAvailable()) {
		Rc<AssetDownloader>::create(a, density, fmt, cb, forceReload);
	} else {
		addAssetTexture(a, density, fmt, cb, forceReload);
	}
}

void TextureCache::addAssetTexture(Asset *a, float density, BitmapFormat fmt, const Callback &cb, bool forceReload) {
	auto file = a->getFilePath();
	a->retainReadLock(this, [this, a, density, fmt, cb, file = file.str(), forceReload] {
		addTexture(file, density, fmt, [this, a, cb] (cocos2d::Texture2D *tex) {
			if (cb) {
				cb(tex);
			}
			a->releaseReadLock(this);
		}, forceReload);
	});
}

bool TextureCache::hasTexture(const StringView &path, float d, BitmapFormat fmt) {
	auto it = _textures.find(TextureIndex{path.str(), fmt, d});
	return it != _textures.end();
}

TextureCache::TextureCache() {
	_mainProgramSet = Rc<GLProgramSet>::create();

	s_textureCacheThread.perform([this] (const Task &) -> bool {
		performWithGL([&] {
			_threadProgramSet = Rc<GLProgramSet>::create();
		});
		return true;
	});
}

TextureCache::~TextureCache() { }

void TextureCache::registerWithDispatcher() {
	if (!_registred) {
		auto dir = cocos2d::Director::getInstance();
		auto sc = dir->getScheduler();
		sc->scheduleUpdate(this, 0, false);
		_registred = true;
	}
}

void TextureCache::unregisterWithDispatcher() {
	if (_registred) {
		auto dir = cocos2d::Director::getInstance();
		auto sc = dir->getScheduler();
		sc->unscheduleUpdate(this);
		_registred = false;
	}
}

void TextureCache::uploadBitmap(Bitmap && bmp, const Function<void(cocos2d::Texture2D *)> &cb, Ref *ref) {
	auto bmpPtr = new Bitmap(std::move(bmp));
	auto texPtr = new Rc<cocos2d::Texture2D>;

	s_textureCacheThread.perform([this, bmpPtr, texPtr] (const Task &) -> bool {
		performWithGL([&] {
			uploadTextureBackground(*texPtr, *bmpPtr);
		});
		return true;
	}, [bmpPtr, texPtr, cb] (const Task &, bool) {
		cb(*texPtr);
		delete texPtr;
		delete bmpPtr;
	}, ref);
}

void TextureCache::uploadBitmap(Vector<Bitmap> &&bmp, const Function<void(Vector<Rc<cocos2d::Texture2D>> &&tex)> &cb, Ref *ref) {
	auto bmpPtr = new Vector<Bitmap>(std::move(bmp));
	auto texPtr = new Vector<Rc<cocos2d::Texture2D>>;

	s_textureCacheThread.perform([this, bmpPtr, texPtr] (const Task &) -> bool {
		performWithGL([&] {
			uploadTextureBackground(*texPtr, *bmpPtr);
		});
		return true;
	}, [bmpPtr, texPtr, cb] (const Task &, bool) {
		cb(std::move(*texPtr));
		delete texPtr;
		delete bmpPtr;
	}, ref);
}

void TextureCache::uploadTextureBackground(Rc<cocos2d::Texture2D> &tex, const Bitmap &bmp) {
	tex = uploadTexture(bmp);
}

void TextureCache::uploadTextureBackground(Vector<Rc<cocos2d::Texture2D>> &texs, const Vector<Bitmap> &bmps) {
	for (auto &it : bmps) {
		texs.emplace_back(uploadTexture(it));
	}
}

void TextureCache::addLoadedTexture(const StringView &str, cocos2d::Texture2D *tex) {
	addLoadedTexture(str, screen::density(), BitmapFormat::Auto, tex);
}
void TextureCache::addLoadedTexture(const StringView &str, float d, cocos2d::Texture2D *tex) {
	addLoadedTexture(str, d, BitmapFormat::Auto, tex);
}
void TextureCache::addLoadedTexture(const StringView &str, float d, BitmapFormat fmt, cocos2d::Texture2D *tex) {
	_textures.emplace(TextureIndex{str.str(), fmt, d}, tex);
}

void TextureCache::removeLoadedTexture(const StringView &str, float d, BitmapFormat fmt) {
	_textures.erase(TextureIndex{str.str(), fmt, d});
}

bool TextureCache::makeCurrentContext() {
#ifdef DEBUG
	assert(s_textureCacheThread.isOnThisThread());
#endif
	if (!stappler::Application::isApplicationExists()) {
		return false;
	}

	if (_contextRetained == 0) {
		++ _contextRetained;
		return platform::render::_enableOffscreenContext();
	} else {
		++ _contextRetained;
		return true;
	}
}
void TextureCache::freeCurrentContext() {
#ifdef DEBUG
	assert(s_textureCacheThread.isOnThisThread());
#endif
	if (!stappler::Application::isApplicationExists()) {
		return;
	}

	if (_contextRetained > 0) {
		-- _contextRetained;
		if (_contextRetained == 0) {
			glFinish();
			platform::render::_disableOffscreenContext();
		}
	}
}

void TextureCache::reloadTextures() {
	_mainProgramSet->reloadPrograms();
	if (_vectorCanvas) {
		_vectorCanvas->drop();
		_vectorCanvas = nullptr;
	}

	s_textureCacheThread.perform([this] (const Task &) -> bool {
		if (_threadVectorCanvas) {
			_threadVectorCanvas->drop();
			_threadVectorCanvas = nullptr;
		}
		performWithGL([&] {
			_threadProgramSet->reloadPrograms();
		});
		return true;
	});

	for (auto &it : _textures) {
		if (auto img = draw::Cache::getInstance()->getImage(it.first.file)) {
			BitmapFormat fmt = it.first.fmt;
			if (fmt == BitmapFormat::Auto) {
				fmt = img->detectFormat();
			}

			it.second->init(getPixelFormat(fmt), it.second->getPixelsWide(), it.second->getPixelsHigh(), cocos2d::Texture2D::InitAs::RenderTarget);
		} else {
			it.second->init(it.second->getPixelFormat(), it.second->getPixelsWide(), it.second->getPixelsHigh());
		}
	}
	_reloadDirty = true;
}


Rc<cocos2d::Texture2D> TextureCache::uploadTexture(const Bytes &data, const Size &size, float density) {
	if (layout::Image::isSvg(data)) {
		layout::Image img;
		if (img.init(data)) {
			BitmapFormat fmt = img.detectFormat();

			return getInstance()->performWithGL([&] {
				Size target(size);
				if (target.width == 0.0f || target.height == 0.0f) {
					target = Size(img.getWidth(), img.getHeight());
				}

				target = target * density;

				if (target.width > 0.0f && target.height > 0.0f) {
					auto tex = Rc<cocos2d::Texture2D>::create(getPixelFormat(fmt),
							roundf(target.width), roundf(target.height),
							cocos2d::Texture2D::InitAs::RenderTarget);
					Rc<draw::Canvas> canvas;
					TextureCache *c = getInstance();
					if (TextureCache::thread().isOnThisThread()) {
						if (!c->_threadVectorCanvas) {
							c->_threadVectorCanvas = Rc<draw::Canvas>::create();
							c->_threadVectorCanvas->setQuality(draw::Canvas::QualityHigh);
						}
						canvas = c->_threadVectorCanvas;
					} else {
						canvas = c->_vectorCanvas;
					}
					canvas->begin(tex, Color4B());
					canvas->draw(img, Rect(0.0f, 0.0f, tex->getPixelsWide(), tex->getPixelsHigh()));
					canvas->end();
					return tex;
				}
				return Rc<cocos2d::Texture2D>();
			});
		}
	} else {
		Bitmap bitmap(data);
		if (bitmap) {
			if (size.width > 0.0f && size.height > 0.0f && (bitmap.width() > size.width || bitmap.height() > size.height)) {
				bitmap = bitmap.resample(roundf(size.width * density), roundf(size.height * density));
			}
			return getInstance()->performWithGL([&] {
				auto tex = Rc<cocos2d::Texture2D>::alloc();
				tex->initWithDataThreadSafe(bitmap.dataPtr(), bitmap.size(), getPixelFormat(bitmap.format()), bitmap.width(), bitmap.height(), 0);
				if (bitmap.alpha() == Bitmap::Alpha::Premultiplied) {
					tex->setPremultipliedAlpha(true);
				}
				return tex;
			});
		}
	}
	return nullptr;
}

Rc<cocos2d::Texture2D> TextureCache::uploadTexture(const Bitmap &bmp) {
	return getInstance()->performWithGL([&] () -> Rc<cocos2d::Texture2D> {
		if (bmp) {
			auto ret = Rc<cocos2d::Texture2D>::alloc();
			ret->initWithDataThreadSafe(bmp.dataPtr(), bmp.size(), getPixelFormat(bmp.format()), bmp.width(), bmp.height(), 0);
			ret->setPremultipliedAlpha(bmp.alpha() == Bitmap::Alpha::Premultiplied);
			return ret;
		}
		return nullptr;
	});
}

bool TextureCache::TextureIndex::operator < (const TextureIndex &other) const {
	auto cmp = file.compare(other.file);
	if (cmp == 0) {
		if (fmt == other.fmt) {
			return density < other.density;
		} else {
			return toInt(fmt) < toInt(other.fmt);
		}
	}
	return cmp < 0;
}

bool TextureCache::TextureIndex::operator > (const TextureIndex &other) const {
	auto cmp = file.compare(other.file);
	if (cmp == 0) {
		if (fmt == other.fmt) {
			return density > other.density;
		} else {
			return toInt(fmt) > toInt(other.fmt);
		}
	}
	return cmp > 0;
}

bool TextureCache::TextureIndex::operator == (const TextureIndex &other) const {
	return file == other.file && fabs(density - other.density) <= std::numeric_limits<float>::epsilon() && fmt == other.fmt;
}

bool TextureCache::TextureIndex::operator != (const TextureIndex &other) const {
	return !(*this == other);
}

void TextureCache::reloadTexture(cocos2d::Texture2D *tex, const TextureIndex &index) {
	if (auto img = draw::Cache::getInstance()->getImage(index.file)) {
		if (!_vectorCanvas) {
			_vectorCanvas = Rc<draw::Canvas>::create();
		}
		_vectorCanvas->begin(tex, Color4B());
		_vectorCanvas->draw(*img, Rect(0.0f, 0.0f, tex->getPixelsWide(), tex->getPixelsHigh()));
		_vectorCanvas->end();
		return;
	}

	auto data = filesystem::readFile(index.file);
	if (layout::Image::isSvg(data)) {
		if (auto img = draw::Cache::getInstance()->addImage(index.file, data)) {
			if (!_vectorCanvas) {
				_vectorCanvas = Rc<draw::Canvas>::create();
			}
			_vectorCanvas->begin(tex, Color4B());
			_vectorCanvas->draw(*img, Rect(0.0f, 0.0f, tex->getPixelsWide(), tex->getPixelsHigh()));
			_vectorCanvas->end();
		}
	} else {
		Bitmap bitmap(data);
		if (bitmap) {
			if (index.fmt != BitmapFormat::Auto) {
				bitmap.convert(index.fmt);
			}
			tex->updateWithData(bitmap.dataPtr(), 0, 0, bitmap.width(), bitmap.height());
			if (bitmap.alpha() == Bitmap::Alpha::Premultiplied) {
				tex->setPremultipliedAlpha(true);
			}
		}
	}
}

Rc<cocos2d::Texture2D> TextureCache::loadTexture(const TextureIndex &index) {
	auto renderVg = [&] (layout::Image *img) -> Rc<cocos2d::Texture2D> {
		BitmapFormat fmt = index.fmt;
		if (index.fmt == BitmapFormat::Auto) {
			fmt = img->detectFormat();
		}

		return performWithGL([&] {
			auto tex = Rc<cocos2d::Texture2D>::create(getPixelFormat(fmt),
					roundf(img->getWidth() * index.density), roundf(img->getHeight() * index.density),
					cocos2d::Texture2D::InitAs::RenderTarget);
			if (!_threadVectorCanvas) {
				_threadVectorCanvas = Rc<draw::Canvas>::create();
				_threadVectorCanvas->setQuality(draw::Canvas::QualityHigh);
			}
			_threadVectorCanvas->begin(tex, Color4B());
			_threadVectorCanvas->draw(*img, Rect(0.0f, 0.0f, tex->getPixelsWide(), tex->getPixelsHigh()));
			_threadVectorCanvas->end();

			return tex;
		});
	};

	if (auto img = draw::Cache::getInstance()->getImage(index.file)) {
		return renderVg(img);
	}

	auto data = filesystem::readFile(index.file);
	if (layout::Image::isSvg(data)) {
		if (auto img = draw::Cache::getInstance()->addImage(index.file, data)) {
			return renderVg(img);
		}
	} else {
		Bitmap bitmap(filesystem::readFile(index.file));
		if (bitmap) {
			if (index.fmt != BitmapFormat::Auto) {
				bitmap.convert(index.fmt);
			}
			return performWithGL([&] {
				auto tex = Rc<cocos2d::Texture2D>::alloc();
				tex->initWithDataThreadSafe(bitmap.dataPtr(), bitmap.size(), getPixelFormat(bitmap.format()), bitmap.width(), bitmap.height(), 0);
				if (bitmap.alpha() == Bitmap::Alpha::Premultiplied) {
					tex->setPremultipliedAlpha(true);
				}
				return tex;
			});
		} else {
			log::format("TextureCache", "fail to open bitmap: %s", index.file.data());
		}
	}
	return nullptr;
}

Rc<cocos2d::Texture2D> TextureCache::doRenderImage(cocos2d::Texture2D *tex, TextureFormat fmt, const layout::Image &image,
		const Size &size, layout::style::Autofit autofit, const Vec2 &autofitPos, float density) {
	auto baseWidth = image.getWidth();
	auto baseHeight = image.getHeight();

	if (baseWidth == 0 || baseHeight == 0) {
		return tex;
	}

	Rc<draw::Canvas> canvas;

	if (s_textureCacheThread.isOnThisThread()) {
		if (!_threadVectorCanvas) {
			_threadVectorCanvas = Rc<draw::Canvas>::create();
			_threadVectorCanvas->setQuality(draw::Canvas::QualityHigh);
		}

		canvas = _threadVectorCanvas;
	} else {
		if (!_vectorCanvas) {
			_vectorCanvas = Rc<draw::Canvas>::create();
			_vectorCanvas->setQuality(draw::Canvas::QualityHigh);
		}
		canvas = _vectorCanvas;
	}

	uint32_t width = ceilf(size.width * density);
	uint32_t height = ceilf(size.height * density);

	float scaleX = float(width) / float(baseWidth);
	float scaleY = float(height) / float(baseHeight);

	switch (autofit) {
	case layout::style::Autofit::Contain:
		scaleX = scaleY = std::min(scaleX, scaleY);
		break;
	case layout::style::Autofit::Cover:
		scaleX = scaleY = std::max(scaleX, scaleY);
		break;
	case layout::style::Autofit::Width:
		scaleY = scaleX;
		break;
	case layout::style::Autofit::Height:
		scaleX = scaleY;
		break;
	case layout::style::Autofit::None:
		break;
	}

	uint32_t nwidth = baseWidth * scaleX;
	uint32_t nheight = baseHeight * scaleY;

	float offsetX = 0.0f;
	float offsetY = 0.0f;

	if (nwidth > width) {
		offsetX = autofitPos.x * (nwidth - width);
	} else {
		width = nwidth;
	}

	if (nheight > height) {
		offsetY = autofitPos.y * (nheight - height);
	} else {
		height = nheight;
	}

	auto generateTexture = [] (cocos2d::Texture2D *tex, uint32_t w, uint32_t h, TextureFormat fmt) -> Rc<cocos2d::Texture2D> {
		if (tex && tex->getPixelsHigh() == int(h) && tex->getPixelsWide() == int(w) && fmt == tex->getReferenceFormat()) {
			return tex;
		}
		auto outtex = Rc<cocos2d::Texture2D>::create(fmt, w, h, cocos2d::Texture2D::InitAs::RenderTarget);
		if (outtex) {
			outtex->setAliasTexParameters();
		}
		return outtex;
	};

	auto t = generateTexture(tex, width, height, fmt);
	if (t) {
		canvas->begin(t.get(), Color4B(0, 0, 0, 0));
		canvas->save();
		canvas->translate(offsetX, offsetY);
		canvas->scale(scaleX, scaleY);
		canvas->transform(image.getViewBoxTransform());

		image.draw([&] (const layout::Path &path, const Vec2 &pos) {
			if (pos.isZero()) {
				canvas->draw(path);
			} else {
				canvas->draw(path, pos.x, pos.y);
			}
		});

		canvas->restore();
		canvas->end();
	}
	return t;
}

NS_SP_END
