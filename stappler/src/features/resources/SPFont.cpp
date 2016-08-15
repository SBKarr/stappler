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
