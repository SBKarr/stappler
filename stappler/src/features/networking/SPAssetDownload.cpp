//
//  SPAssetDownload.cpp
//  stappler
//
//  Created by SBKarr on 7/27/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPAssetDownload.h"
#include "SPFilesystem.h"
#include "SPScheme.h"
#include "SPData.h"
#include "SPAsset.h"
#include "SPAssetLibrary.h"

NS_SP_BEGIN

bool AssetDownload::init(Asset *a, CacheRequestType) {
	if (!NetworkDownload::init(a->getUrl(), "")) {
		return false;
	}

	_cacheRequest = true;
	_asset = a;

	_size = a->getSize();
	_mtime = a->getMTime();
	_etag = a->getETag();

	return true;
}

AssetDownload::~AssetDownload() { }

bool AssetDownload::init(Asset *h, const std::string &file) {
	if (!NetworkDownload::init(h->getUrl(), file)) {
		return false;
	}

	_asset = h;

	setReceiveFile(file, true);

	_size = h->getSize();
	_mtime = h->getMTime();
	_etag = h->getETag();

	setThreadDownloadProgress([this] (int64_t total, int64_t now) -> int {
		float progress = (float)(now) / (float)(total);
		notifyOnProgress(progress);
		return 0;
	});

	return true;
}

bool AssetDownload::execute() {
	return executeLoop();
}

AssetDownload::Validation AssetDownload::validateDownload(bool fileOnly, bool bind) {
	auto savedMethod = _handle.getMethod();
	auto ret = Validation::Unavailable;
	_handle.init(Method::Head, getUrl());
	if (performQuery()) {
		ret = Validation::Invalid;
		uint64_t mtimeVal = (uint64_t)getReceivedHeaderInt("X-FileModificationTime");
		size_t sizeVal = (size_t)getReceivedHeaderInt("X-FileSize");
		std::string etag = getReceivedHeaderString("ETag");

		if ((mtimeVal == 0 || sizeVal == 0) && etag.empty()) {
			//logTag("AssetDownload", "server does not support preflight requests for url: %s", _url.c_str());
			_handle.init(savedMethod, getUrl());
			return Validation::NotSupported;
		}

		if (((!_etag.empty() && !etag.empty() && _etag != etag) || _size != sizeVal || _mtime != mtimeVal) && !fileOnly) {
			//logTag("AssetDownload", "saved file is not valid");
			_handle.setResumeDownload(false);

			_size = sizeVal;
			_mtime = mtimeVal;
			if (!etag.empty()) {
				_etag = etag;
			}

			notifyOnCacheData(_mtime, _size, _etag, getContentType(), bind);
		} else {
			if (fileOnly) {
				_size = sizeVal;
				_mtime = mtimeVal;
				if (!etag.empty()) {
					_etag = etag;
				}

				notifyOnCacheData(_mtime, _size, _etag, getContentType(), bind);
			}
			if (_size != 0) {
				auto size = filesystem::size(getReceiveFile());
				if (size == _size) {
					_handle.init(savedMethod, getUrl());
					return Validation::Valid; // download completed and valid
				}
			} else {
				_handle.init(savedMethod, getUrl());
				return Validation::Valid; // download completed and valid
			}
		}
	}
	_handle.init(savedMethod, getUrl());
	return ret;
}

void AssetDownload::updateCache(bool bind) {
	auto task = new NetworkTask;
	task->init(NetworkTask::Method::Head, getUrl());
	task->performQuery();

	auto mtime = (uint64_t)task->getReceivedHeaderInt("X-FileModificationTime");
	auto size = (size_t)task->getReceivedHeaderInt("X-FileSize");
	auto etag = task->getReceivedHeaderString("ETag");
	auto ct = task->getReceivedHeaderString("Content-Type");

	notifyOnCacheData(mtime, size, etag, ct, bind);
	task->release();
}

bool AssetDownload::executeLoop() {
	_handle.init(Method::Get, getUrl());
	auto validate = validateDownload();
	if (validate == Validation::Unavailable) {
		return false;
	}
	if (!_cacheRequest) {
		if (validate == Validation::NotSupported) {
			notifyOnStarted();
			return performQuery();
		} else if (validate == Validation::Invalid) {
			notifyOnStarted();
			if (performQuery()) {
				if (filesystem::exists(getReceiveFile())) {
					return executeLoop();
				}
			}
		} else if (validate == Validation::Valid) {
			return true;
		}

		return false;
	} else {
		return true;
	}
}

void AssetDownload::notifyOnCacheData(uint64_t id, size_t size, const std::string &etag, const std::string &ct, bool bind) {
	Thread::onMainThread([this, id, size, etag, ct] () {
		_asset->onCacheData(id, size, etag, ct);
	}, bind ? this : nullptr);
}

void AssetDownload::notifyOnStarted(bool bind) {
	Thread::onMainThread([this] () {
		_asset->onStarted();
		if (_onStarted) {
			_onStarted(this);
		}
	}, bind ? this : nullptr);
}
void AssetDownload::notifyOnProgress(float progress, bool bind) {
	Thread::onMainThread([this, progress] () {
		_asset->onProgress(progress);
		if (_onProgress) {
			_onProgress(this, progress);
		}
	}, bind ? this : nullptr);
}
void AssetDownload::notifyOnComplete(bool success, bool bind) {
	auto ct = getReceivedHeaderString("Content-Type");
	Thread::onMainThread([this, success, ct] () {
		if (_onCompleted) {
			_onCompleted(this, success);
		}
		_asset->onCompleted(success, getReceiveFile(), ct, _cacheRequest);
	}, bind ? this : nullptr);
}

bool AssetDownload::performQuery() {
	// bypass NetworkDownload's behavior
	return NetworkTask::performQuery();
}

NS_SP_END
