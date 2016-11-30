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
#include "base/CCDirector.h"
#include "base/CCScheduler.h"
#include "platform/CCImage.h"
#include "platform/CCGLView.h"

#include "SPThread.h"
#include "SPDevice.h"
#include "SPFilesystem.h"
#include "SPAsset.h"
#include "SPBitmap.h"

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

GLProgramSet *TextureCache::getBatchPrograms() const {
	return _batchDrawing;
}
GLProgramSet *TextureCache::getRawPrograms() const {
	return _rawDrawing;
}

cocos2d::Texture2D::PixelFormat TextureCache::getPixelFormat(Bitmap::Format fmt) {
	switch (fmt) {
	case Bitmap::Format::A8:
		return cocos2d::Texture2D::PixelFormat::A8;
		break;
	case Bitmap::Format::I8:
		return cocos2d::Texture2D::PixelFormat::I8;
		break;
	case Bitmap::Format::IA88:
		return cocos2d::Texture2D::PixelFormat::AI88;
		break;
	case Bitmap::Format::RGBA8888:
		return cocos2d::Texture2D::PixelFormat::RGBA8888;
		break;
	case Bitmap::Format::RGB888:
		return cocos2d::Texture2D::PixelFormat::RGB888;
		break;
	default:
		break;
	}
	return cocos2d::Texture2D::PixelFormat::AUTO;
}

void TextureCache::addTexture(const std::string &file, const Callback &cb, bool forceReload) {
	if (file.empty()) {
		return;
	}
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
					tex->initWithData(bitmap.dataPtr(), bitmap.size(), getPixelFormat(bitmap.format()), bitmap.width(), bitmap.height());
					if (bitmap.alpha() == Bitmap::Alpha::Premultiplied) {
						tex->setPremultipliedAlpha(true);
					}
					*imagePtr = tex;
				});
			} else {
				log::format("TextureCache", "fail to open bitmap: %s", file.c_str());
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
	_batchDrawing = Rc<GLProgramSet>::create(GLProgramSet::DrawNodeSet);
	_rawDrawing = Rc<GLProgramSet>::create(GLProgramSet::RawDrawingSet);

	onEvent(Device::onAndroidReset, [this] (const Event *) {
		for (auto &it : _textures) {
			Bitmap bitmap(filesystem::readFile(it.first));
			if (bitmap) {
				//log::format("Texture", "%s", it.first.c_str());
				it.second->initWithDataThreadSafe(bitmap.dataPtr(), bitmap.size(), it.second->getPixelFormat(), bitmap.width(), bitmap.height(), 0);
				if (bitmap.alpha() == Bitmap::Alpha::Premultiplied) {
					it.second->setPremultipliedAlpha(true);
				}
			}
		}

		_batchDrawing->reloadPrograms();
		_rawDrawing->reloadPrograms();
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
	tex = uploadTexture(bmp);
}

void TextureCache::uploadTextureBackground(Vector<Rc<cocos2d::Texture2D>> &texs, const Vector<Bitmap> &bmps) {
	for (auto &it : bmps) {
		texs.emplace_back(uploadTexture(it));
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
	if (_contextRetained > 0) {
		-- _contextRetained;
		if (_contextRetained == 0) {
			glFinish();
			platform::render::_disableOffscreenContext();
		}
	}
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

NS_SP_END
