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
#include "SPNetworkSprite.h"
#include "SPAssetLibrary.h"
#include "renderer/CCTexture2D.h"
#include "SPFilesystem.h"
#include "SPThreadManager.h"
#include "SPTaskManager.h"
#include "SPTextureCache.h"
#include "SPString.h"

NS_SP_BEGIN

std::string NetworkSprite::getPathForUrl(const std::string &url) {
	auto dir = filesystem::writablePath("NetworkSprites");
	filesystem::mkdir(dir);
	return toString(dir, "/", string::stdlibHashSigned(url));
}

bool NetworkSprite::isCachedTextureUrl(const std::string &url) {
	auto lib = AssetLibrary::getInstance();
	auto cache = TextureCache::getInstance();

	auto path = getPathForUrl(url);
	auto assetId = lib->getAssetId(url, path);

	if (lib->isLiveAsset(assetId) && cache->hasTexture(path)) {
		return true;
	}
	return false;
}

NetworkSprite::~NetworkSprite() {
	storeForFrames(_asset.get(), 10);
}

bool NetworkSprite::init(const std::string &url, float density) {
	if (!DynamicSprite::init(nullptr, cocos2d::Rect::ZERO, density)) {
		return false;
	}

	_asset.setCallback(std::bind(&NetworkSprite::onAssetUpdated, this, std::placeholders::_1));
	_url = url;

	return true;
}

void NetworkSprite::visit(cocos2d::Renderer *r, const cocos2d::Mat4 &t, uint32_t f, ZPath &zPath) {
	if (_assetDirty) {
		if (AssetLibrary::getInstance()->isLoaded()) {
			updateSprite();
			_assetDirty = false;
		}
	}
	DynamicSprite::visit(r, t, f, zPath);
}

void NetworkSprite::setUrl(const std::string &url, bool force) {
	if (_url != url) {
		_url = url;
		if (force || isCachedTextureUrl(url)) {
			updateSprite(true);
		} else {
			_assetDirty = true;
		}
	}
}

const std::string &NetworkSprite::getUrl() const {
	return _url;
}

void NetworkSprite::updateSprite(bool force) {
	auto lib = AssetLibrary::getInstance();
	if (!_url.empty()) {
		_filePath = getPathForUrl(_url);
		uint64_t assetId = lib->getAssetId(_url, _filePath);
		if (!_asset || _asset->getId() != assetId) {
			retain();
			lib->getAsset([this, force] (Asset *a) {
				onAsset(a, force);
				release();
			}, _url, _filePath, TimeInterval::seconds(10 * 24 * 60 * 60));
		}
	} else {
		onAsset(nullptr, force);
	}
}

void NetworkSprite::onAsset(Asset *a, bool force) {
	_asset = a;
	if (_asset) {
		if (_asset->isDownloadAvailable()) {
			_asset->download();
		}
		if (force) {
			_asset.check();
		}
	} else {
		setTexture(nullptr, Rect::ZERO);
	}
}

void NetworkSprite::onAssetUpdated(data::Subscription::Flags f) {
	if ((f.initial() || f.hasFlag((uint8_t)Asset::Update::FileUpdated)) && _asset->isReadAvailable()) {
		loadTexture();
	}
}

void NetworkSprite::loadTexture() {
	if (_asset && _asset->isReadAvailable()) {
		auto cache = TextureCache::getInstance();

		Asset * asset = _asset;

		retain();
		cache->addTexture(_asset, [this, asset] (cocos2d::Texture2D *tex) {
			if (asset == _asset && tex) {
				setTexture(tex, Rect(0, 0, tex->getContentSize().width, tex->getContentSize().height));
			}
			release();
		}, _texture != nullptr);
	} else {
		setTexture(nullptr, Rect::ZERO);
	}
}

NS_SP_END
