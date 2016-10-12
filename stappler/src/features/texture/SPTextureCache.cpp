/*
 * SPTextureCache.cpp
 *
 *  Created on: 24 мая 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPTextureCache.h"
#include "base/CCDirector.h"
#include "base/CCScheduler.h"
#include "platform/CCImage.h"
#include "platform/CCGLView.h"
#include "renderer/CCTexture2D.h"

#include "SPThread.h"
#include "SPDevice.h"
#include "SPFilesystem.h"
#include "SPAsset.h"
#include "SPImage.h"

#include "SPPlatform.h"

NS_SP_BEGIN

static Thread s_textureCacheThread("TextureCacheThread");
static TextureCache *s_sharedTextureCache = nullptr;

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
	std::map<std::string, cocos2d::Texture2D *> release;
	for (auto &it : _texturesScore) {
		//log("%u %s", it.first->getReferenceCount(), filepath::name(it.second.second).c_str());
		if (it.first->getReferenceCount() > 1) {
			it.second.first = GetDelayTime();
		} else {
			if (it.second.first <= 0) {
				release.insert(std::make_pair(it.second.second, it.first));
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

void TextureCache::addTexture(const std::string &file, const Callback &cb, bool forceReload) {
	if (!forceReload) {
		auto it = _textures.find(file);
		if (it != _textures.end()) {
			if (cb) {
				cb(it->second);
			}
			return;
		}
	}

	auto cbIt = _callbackMap.find(file);
	if (cbIt != _callbackMap.end()) {
		if (cb) {
			cbIt->second.push_back(cb);
		}
	} else {
		CallbackVec vec;
		if (cb) {
			vec.push_back(cb);
		}
		_callbackMap.insert(std::make_pair(file, vec));

		auto &thread = s_textureCacheThread;
        auto imagePtr = new Rc<cocos2d::Texture2D>;
		thread.perform([this, file, imagePtr] (cocos2d::Ref *) -> bool {
			Bitmap bitmap(filesystem::readFile(file));
			if (bitmap) {
				performWithGL([&] {
					auto tex = Rc<cocos2d::Texture2D>::alloc();
					tex->initWithData(bitmap.dataPtr(), bitmap.size(), Image::getPixelFormat(bitmap.format()), bitmap.width(), bitmap.height());
					if (bitmap.alpha() == Bitmap::Alpha::Premultiplied) {
						tex->setPremultipliedAlpha(true);
					}
					*imagePtr = tex;
				});
			}
			return true;
		}, [this, file, imagePtr] (cocos2d::Ref *, bool) {
			auto image = *imagePtr;
			if (image) {
				auto texIt = _textures.find(file);
				if (texIt != _textures.end()) {
					_texturesScore.erase(texIt->second);
				}

				_textures.insert(file, image);
				_texturesScore.insert(std::make_pair(image, std::make_pair(GetDelayTime(), file)));

				auto it = _callbackMap.find(file);
				if (it != _callbackMap.end()) {
					for (auto &cb : it->second) {
						cb(image);
					}
				}
				_callbackMap.erase(it);
	            registerWithDispatcher();
	        } else {
				auto it = _callbackMap.find(file);
				if (it != _callbackMap.end()) {
					for (auto &cb : it->second) {
						cb(nullptr);
					}
				}
				_callbackMap.erase(it);
	        }
			delete imagePtr;
		});
	}
}

void TextureCache::addTexture(Asset *a, const Callback & cb, bool forceReload) {
	auto &file = a->getFilePath();
	if (!forceReload) {
		auto it = _textures.find(file);
		if (it != _textures.end()) {
			if (cb) {
				cb(it->second);
			}
			return;
		}
	}

	a->retainReadLock(this, [this, a, cb, file, forceReload] {
		addTexture(file, [this, a, cb] (cocos2d::Texture2D *tex) {
			if (cb) {
				cb(tex);
			}
			a->releaseReadLock(this);
		}, forceReload);
	});
}

bool TextureCache::hasTexture(const std::string &path) {
	auto it = _textures.find(path);
	return it != _textures.end();
}

TextureCache::TextureCache() {
	onEvent(Device::onAndroidReset, [this] (const Event *) {
		for (auto &it : _textures) {
			Bitmap bitmap(filesystem::readFile(it.first));
			it.second->initWithData(bitmap.dataPtr(), bitmap.size(), Image::getPixelFormat(bitmap.format()), bitmap.width(), bitmap.height());
			if (bitmap.alpha() == Bitmap::Alpha::Premultiplied) {
				it.second->setPremultipliedAlpha(true);
			}
		}
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

	s_textureCacheThread.perform([this, bmpPtr, texPtr] (Ref *) -> bool {
		performWithGL([&] {
			uploadTextureBackground(*texPtr, *bmpPtr);
		});
		return true;
	}, [bmpPtr, texPtr, cb] (Ref *, bool) {
		cb(*texPtr);
		delete texPtr;
		delete bmpPtr;
	}, ref);
}

void TextureCache::uploadBitmap(Vector<Bitmap> &&bmp, const Function<void(Vector<Rc<cocos2d::Texture2D>> &&tex)> &cb, Ref *ref) {
	auto bmpPtr = new Vector<Bitmap>(std::move(bmp));
	auto texPtr = new Vector<Rc<cocos2d::Texture2D>>;

	s_textureCacheThread.perform([this, bmpPtr, texPtr] (Ref *) -> bool {
		performWithGL([&] {
			uploadTextureBackground(*texPtr, *bmpPtr);
		});
		return true;
	}, [bmpPtr, texPtr, cb] (Ref *, bool) {
		cb(std::move(*texPtr));
		delete texPtr;
		delete bmpPtr;
	}, ref);
}

void TextureCache::uploadTextureBackground(Rc<cocos2d::Texture2D> &tex, const Bitmap &bmp) {
	tex = Rc<cocos2d::Texture2D>::alloc();
	tex->initWithData(bmp.dataPtr(), bmp.size(), Image::getPixelFormat(bmp.format()), bmp.width(), bmp.height());
	glFinish();
}

void TextureCache::uploadTextureBackground(Vector<Rc<cocos2d::Texture2D>> &texs, const Vector<Bitmap> &bmps) {
	for (auto &it : bmps) {
		Rc<cocos2d::Texture2D> newTex = Rc<cocos2d::Texture2D>::alloc();
		newTex->initWithDataThreadSafe(it.dataPtr(), it.size(), Image::getPixelFormat(it.format()), it.width(), it.height(), 0);
		texs.emplace_back(std::move(newTex));
	}
}

void TextureCache::addLoadedTexture(const String &str, cocos2d::Texture2D *tex) {
	_textures.insert(str, tex);
}
void TextureCache::removeLoadedTexture(const String &str) {
	_textures.erase(str);
}

bool TextureCache::makeCurrentContext() {
#ifdef DEBUG
	assert(s_textureCacheThread.isOnThisThread());
#endif
	return platform::render::_enableOffscreenContext();
}
void TextureCache::freeCurrentContext() {
#ifdef DEBUG
	assert(s_textureCacheThread.isOnThisThread());
#endif
	glFinish();

	platform::render::_disableOffscreenContext();
}

NS_SP_END
