/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "RTSourceAsset.h"
#include "SPAssetLibrary.h"
#include "SPThread.h"

NS_RT_BEGIN

bool SourceMemoryAsset::init(const StringDocument &str, StringView type) {
	_data = Bytes((const uint8_t *)str.get().data(), (const uint8_t *)(str.get().data() + str.get().size()));
	_type = type.str();
	return true;
}

bool SourceMemoryAsset::init(const StringView &str, StringView type) {
	_data = Bytes((const uint8_t *)str.data(), (const uint8_t *)(str.data() + str.size()));
	_type = type.str();
	return true;
}

bool SourceMemoryAsset::init(const BytesView &data, StringView type) {
	_data = Bytes(data.data(), data.data() + data.size());
	_type = type.str();
	return true;
}

Rc<Document> SourceMemoryAsset::openDocument() {
	if (!_data.empty()) {
		Rc<Document> ret = Document::openDocument(_data, _type);
		if (ret && ret->prepare()) {
			return ret;
		}
	}

	return Rc<Document>();
}

Bytes SourceMemoryAsset::getData() const {
	return _data;
}

bool SourceMemoryAsset::getImageSize(size_t &w, size_t &h) const {
	CoderSource coder(_data);
	return Bitmap::getImageSize(coder, w, h);
}


bool SourceFileAsset::init(const FilePath &file, StringView type) {
	_file = file.get().str();
	_type = type.str();
	return true;
}

bool SourceFileAsset::init(const StringView &str, StringView type) {
	_file = str.str();
	_type = type.str();
	return true;
}

Rc<Document> SourceFileAsset::openDocument() {
	if (!_file.empty()) {
		Rc<Document> ret = Document::openDocument(_file, _type);
		if (ret && ret->prepare()) {
			return ret;
		}
	}

	return Rc<Document>();
}

Bytes SourceFileAsset::getData() const {
	return filesystem::readFile(_file);
}

bool SourceFileAsset::getImageSize(size_t &w, size_t &h) const {
	return Bitmap::getImageSize(StringView(_file), w, h);
}


bool SourceNetworkAsset::init(const String &url, const String &path, TimeInterval ttl,
		const String &cacheDir, const Asset::DownloadCallback &dcb) {
	retain();
	AssetLibrary::getInstance()->getAsset([this] (Asset *a) {
		onAsset(a);
		release();
	}, url, path, ttl, cacheDir, dcb);

	return true;
}

bool SourceNetworkAsset::init(Asset *a) {
	onAsset(a);
	return true;
}

bool SourceNetworkAsset::init(const AssetCallback &cb) {
	retain();
	cb([this] (Asset *a) {
		onAsset(a);
		release();
	});
	return true;
}

void SourceNetworkAsset::setAsset(Asset *a) {
	onAsset(a);
}

void SourceNetworkAsset::setAsset(const AssetCallback &cb) {
	retain();
	cb([this] (Asset *a) {
		onAsset(a);
		release();
	});
}

Asset *SourceNetworkAsset::getAsset() const {
	return _asset.get();
}

Rc<Document> SourceNetworkAsset::openDocument() {
	if (!_savedFilePath.empty()) {
		Rc<Document> ret = Document::openDocument(_savedFilePath, _savedType);
		if (ret && ret->prepare()) {
			return ret;
		}
	}

	return Rc<Document>();
}

SyncRWLock *SourceNetworkAsset::getRWLock() const {
	return _asset.get();
}

int64_t SourceNetworkAsset::getMTime() const {
	return _asset->getMTime();
}

bool SourceNetworkAsset::tryLockDocument(int64_t mtime) {
	if (_asset && _asset->isReadAvailable() &&
			((int64_t(_asset->getMTime()) > mtime && !_asset->isDownloadInProgress()) || mtime == 0)) {
		if (_asset->tryReadLock(this)) {
			retain();

			_savedFilePath = _asset->getFilePath().str();
			_savedType = _asset->getContentType().str();

			return true;
		}
	}
	return false;
}

void SourceNetworkAsset::releaseDocument() {
	_asset->releaseReadLock(this);
	_savedFilePath.clear();
	_savedType.clear();
	release();
}

bool SourceNetworkAsset::tryReadLock() {
	if (_asset->tryReadLock(this)) {
		retain();
		return true;
	}
	return false;
}

void SourceNetworkAsset::releaseReadLock() {
	_asset->releaseReadLock(this);
	release();
}

bool SourceNetworkAsset::download() { return _asset->download(); }

bool SourceNetworkAsset::isDownloadAvailable() const { return _asset->isDownloadAvailable(); }
bool SourceNetworkAsset::isDownloadInProgress() const { return _asset->isDownloadInProgress(); }
bool SourceNetworkAsset::isUpdateAvailable() const { return _asset->isUpdateAvailable(); }
bool SourceNetworkAsset::isReadAvailable() const { return _asset->isReadAvailable(); }

StringView SourceNetworkAsset::getContentType() const {
	return _asset->getContentType();
}

Bytes SourceNetworkAsset::getData() const {
	return filesystem::readFile(_asset->getFilePath());
}

bool SourceNetworkAsset::getImageSize(size_t &w, size_t &h) const {
	return Bitmap::getImageSize(_asset->getFilePath(), w, h);
}

void SourceNetworkAsset::setExtraData(data::Value &&data) {
	if (_asset) {
		_asset->setData(move(data));
		_asset->save();
	}
}

data::Value SourceNetworkAsset::getExtraData() const {
	if (_asset) {
		return _asset->getData();
	}
	return data::Value();
}

void SourceNetworkAsset::onAsset(Asset *a) {
	setForwardedSubscription(a);
	_asset = a;
	if (_asset) {
		setDirty(Subscription::Flags(Asset::FileUpdated), true);
	}
}

NS_RT_END
