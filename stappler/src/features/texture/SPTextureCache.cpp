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

#include "SPThread.h"
#include "SPDevice.h"
#include "SPFilesystem.h"
#include "SPAsset.h"

NS_SP_BEGIN

static TextureCache *s_sharedTextureCache = nullptr;
Thread s_textureCacheThread("TextureCacheThread");

TextureCache *TextureCache::getInstance() {
	if (!s_sharedTextureCache) {
		s_sharedTextureCache = new TextureCache();
	}
	return s_sharedTextureCache;
}

Thread &TextureCache::thread() {
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
        auto imagePtr = new (std::nothrow) (cocos2d::Image *)(nullptr);
		thread.perform([file, imagePtr] (cocos2d::Ref *) -> bool {
			auto image = new cocos2d::Image();
	        if (image && !image->initWithImageFile(file)) {
	        	delete image;
	        } else {
	        	(*imagePtr) = image;
	        }
			return true;
		}, [this, file, imagePtr] (cocos2d::Ref *, bool) {
			auto image = *imagePtr;
			if (image) {
				cocos2d::Texture2D *texture = nullptr;
				auto texIt = _textures.find(file);
				if (texIt != _textures.end()) {
					_texturesScore.erase(texIt->second);
					texture = texIt->second;
					texture->retain(); // retain to match extra-counter state after new
				} else {
					texture = new (std::nothrow) cocos2d::Texture2D();
				}

				texture->initWithImage(image);

				_textures.insert(file, texture);
				_texturesScore.insert(std::make_pair(texture, std::make_pair(GetDelayTime(), file)));

				auto it = _callbackMap.find(file);
				if (it != _callbackMap.end()) {
					for (auto &cb : it->second) {
						cb(texture);
					}
				}
				_callbackMap.erase(it);

				texture->release();
	            registerWithDispatcher();
	            image->release();
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
			cocos2d::Image image;
	        image.initWithImageFile(it.first);
	        it.second->initWithImage(&image);
		}
	});
}

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

NS_SP_END
