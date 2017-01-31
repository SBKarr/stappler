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
#include "SPAsset.h"
#include "SPThreadManager.h"
#include "SPTaskManager.h"
#include "SPAssetDownload.h"
#include "SPAssetLibrary.h"

#include "SPFilesystem.h"
#include "SPEvent.h"
#include "SPStorage.h"

NS_SP_BEGIN

bool AssetFile::init(Asset *a) {
	if (!a->tryReadLock(this)) {
		return false;
	}

	_mtime = a->getMTime();
	_id = a->getId();
	_size = a->getSize();
	_url = a->getUrl();
	_contentType = a->getContentType();
	_etag = a->getETag();

	_path = toString(a->getFilePath(), ".", _ctime);
	filesystem::copy(a->getFilePath(), _path);
	a->releaseReadLock(this);

	_exists = filesystem::exists(_path);
	if (_exists) {
		_ctime = filesystem::ctime(_path);
		AssetLibrary::getInstance()->addAssetFile(_path, _url, _id, _ctime);
		return true;
	}

	return false;
}

AssetFile::~AssetFile() {
	remove();
}

Bytes AssetFile::readFile() const {
	if (_exists) {
		return filesystem::readFile(_path);
	}
	return Bytes();
}
filesystem::ifile AssetFile::open() const {
	if (_exists) {
		return filesystem::openForReading(_path);
	}
	return filesystem::ifile();
}

void AssetFile::remove() {
	if (_exists) {
		filesystem::remove(_path);
		AssetLibrary::getInstance()->removeAssetFile(_path);
		_exists = false;
	}
}

bool AssetFile::match(const Asset *a) const {
	if (a->getId() == _id && a->getMTime() == _mtime && a->getETag() == _etag && a->getContentType() == _contentType && a->getSize() == _size) {
		return true;
	}
	return false;
}

Asset::Asset(const data::Value &val, const DownloadCallback &dcb) {
	for (auto &it : val.asDict()) {
		if (it.first == "id") {
			_id = reinterpretValue<uint64_t>(it.second.getInteger());
		} else if (it.first == "size") {
			_size = size_t(it.second.getInteger());
		} else if (it.first == "mtime") {
			_mtime = it.second.getInteger();
		} else if (it.first == "touch") {
			_touch.setMicroseconds(it.second.getInteger());
		} else if (it.first == "ttl") {
			_ttl.setMicroseconds(it.second.getInteger());
		} else if (it.first == "download") {
			_downloadInProgress = it.second.getBool();
		} else if (it.first == "url") {
			_url = it.second.getString();
		} else if (it.first == "path") {
			_path = filepath::absolute(it.second.getString());
		} else if (it.first == "cache") {
			_cachePath = filepath::absolute(it.second.getString());
		} else if (it.first == "contentType") {
			_contentType = it.second.getString();
		} else if (it.first == "etag") {
			_etag = it.second.getString();
		} else if (it.first == "data") {
			_data = data::read(it.second.getString());
		}
	}

	_fileExisted = filesystem::exists(_path);
	if (_fileExisted) {
		auto size = filesystem::size(_path);
		if (size == 0) {
			filesystem::remove(_path);
			_fileExisted = false;
		}
	}

	_downloadFunction = dcb;

	retain();
	Thread::onMainThread([this] {
		if (_fileExisted) {
			syncWithNetwork();
		}
		release();
	});
}

Asset::~Asset() {
	AssetLibrary::getInstance()->removeAsset(this);
}

bool Asset::download() {
	auto tmpPath = AssetLibrary::getInstance()->getTempPath(_path);
	filesystem::mkdir_recursive(filepath::root(tmpPath));
	if (_downloadFunction) {
		if (_downloadFunction(this)) {
			_downloadInProgress = true;
			_unupdated = true;
			_progress = 0;
			touch();
			update(Update::DownloadStarted);
			return true;
		}
	} else {
		if (AssetLibrary::getInstance()->downloadAsset(this)) {
			_downloadInProgress = true;
			_unupdated = true;
			_progress = 0;
			touch();
			return true;
		}
	}
	return false;
}

void Asset::clear() {
	retainWriteLock((LockPtr)this, [this] {
		filesystem::remove(_path);
		if (!_cachePath.empty()) {
			filesystem::remove(_cachePath, true, true);
		}
		_fileExisted = false;
		update(Update::FileUpdated);
		releaseWriteLock((LockPtr)this);
	});
}

void Asset::checkFile() {
	auto e = filesystem::exists(_path);
	if (_fileExisted != e) {
		_fileExisted = e;
		update(Update::FileUpdated);
	}
}

bool Asset::isReadAvailable() const {
	return _fileExisted && !_fileUpdate;
}
bool Asset::isDownloadAvailable() const {
	return (!_fileExisted || _unupdated) && !_fileUpdate && !_waitFileSwap;
}
bool Asset::isUpdateAvailable() const {
	return _unupdated;
}

void Asset::setDownloadCallback(const DownloadCallback &cb) {
	_downloadFunction = cb;
}

void Asset::onCacheData(uint64_t mtime, size_t size, const std::string &etag, const std::string &ct) {
	if (mtime != _mtime || _size != size || (!etag.empty() && etag != _etag)) {
		_unupdated = true;
	}
	_mtime = mtime;
	_size = size;
	if (!etag.empty()) {
		_etag = etag;
	}
	_storageDirty = true;
	save();
}
void Asset::onStarted() {
	_downloadInProgress = true;
	update(Update::DownloadStarted);
}
void Asset::onProgress(float progress) {
	_progress = progress;
	update(Update::DownloadProgress);
}
void Asset::onCompleted(bool success, const std::string &str, const std::string &ct, bool cacheRequest) {
	if (!cacheRequest) {
		_downloadInProgress = false;
		if (success) {
			_tempPath = str;
			_contentType = ct;
			swapFiles();
			update(Update::DownloadSuccessful);
		} else {
			filesystem::remove(str);
			update(Update::DownloadFailed);
			save();
		}
		update(Update::DownloadCompleted);
	} else {
		if (success && _unupdated && _fileExisted) {
			download();
		}
	}
}
void Asset::onFile() {
	_fileExisted = filesystem::exists(_path);
	_fileUpdate = false;
	update(Update::FileUpdated);
}

void Asset::update(Update u) {
	setDirty(Subscription::Flag((uint8_t)u));
}

bool Asset::swapFiles() {
	if (!_tempPath.empty() && filesystem::exists(_tempPath)) {
		std::string tmp = _tempPath;
		_tempPath.clear();
		_waitFileSwap = true;
		retainWriteLock((LockPtr)this, [this, tmp] {
			_fileUpdate = true;
			auto &thread = storage::thread();
			thread.perform([this, tmp] (cocos2d::Ref *) -> bool {
				filesystem::remove(_path);
				if (!_cachePath.empty()) {
					filesystem::remove(_cachePath, true, true);
				}
				return filesystem::move(tmp, _path);
			}, [this] (cocos2d::Ref *, bool) {
				_fileUpdate = false;
				_unupdated = false;
				_waitFileSwap = false;
				_fileExisted = filesystem::exists(_path);
				touch();
				releaseWriteLock(this);
				onFile();
			}, this);
		});

		return true;
	}
	return false;
}

void Asset::onLocked(Lock lock) {
	if (lock == Lock::None) {
		update(Update::Unlocked);
	} else if (lock == Lock::Write) {
		update(Update::WriteLocked);
	} else if (lock == Lock::Read) {
		update(Update::ReadLocked);
	}
}

void Asset::touchWithTime(Time t) {
	_touch = t;
	_storageDirty = true;
}

void Asset::save() {
	AssetLibrary::getInstance()->saveAsset(this);
}

void Asset::touch() {
	AssetLibrary::getInstance()->touchAsset(this);
}

void Asset::syncWithNetwork() {
	auto d = Rc<AssetDownload>::create(this, AssetDownload::CacheRequest);
	d->run();
}

Rc<AssetFile> Asset::cloneFile() {
	if (_lock == Lock::Read || _lock == Lock::None) {
		return Rc<AssetFile>::create(this);
	}
	return Rc<AssetFile>();
}

NS_SP_END
