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
#include "SPAssetDownload.h"
#include "SPFilesystem.h"
#include "SPScheme.h"
#include "SPData.h"
#include "SPAsset.h"
#include "SPAssetLibrary.h"

NS_SP_BEGIN

bool AssetDownload::init(Asset *a, CacheRequestType) {
	if (!NetworkTask::init(Method::Head, a->getUrl())) {
		return false;
	}

	_cacheRequest = true;
	_asset = a;

	_size = a->getSize();
	if (a->isFileExists()) {
		_mtime = a->getMTime();
		_etag = a->getETag().str();
	}

	if (a->isFileExists()) {
		if (_mtime > 0) {
			_handle.addHeader("If-Modified-Since", Time::microseconds(_mtime).toHttp());
		}
		if (!_etag.empty()) {
			_handle.addHeader("If-None-Match", _etag);
		}
	}

	return true;
}

AssetDownload::~AssetDownload() { }

bool AssetDownload::init(Asset *a, const StringView &file) {
	if (!NetworkDownload::init(a->getUrl(), file)) {
		return false;
	}

	_asset = a;

	setReceiveFile(file, true);

	_size = a->getSize();
	if (a->isFileExists()) {
		_mtime = a->getMTime();
		_etag = a->getETag().str();
	}

	setThreadDownloadProgress([this] (int64_t total, int64_t now) -> int {
		float progress = (float)(now) / (float)(total);
		notifyOnProgress(progress);
		return 0;
	});

	return true;
}

bool AssetDownload::execute() {
	if (NetworkDownload::execute()) {
		if (_handle.getResponseCode() < 300) {
			size_t sizeVal = (size_t)getReceivedHeaderInt("X-FileSize");
			if (sizeVal) {
				_size = sizeVal;
			}
			return true;
		} else {
			// cache request successful when not 304
			return !_cacheRequest;
		}
	}
	return false;
}

void AssetDownload::notifyOnStarted() {
	Thread::onMainThread([this] () {
		_asset->onStarted();
		if (_onStarted) {
			_onStarted(this);
		}
	}, this);
}
void AssetDownload::notifyOnProgress(float progress) {
	Thread::onMainThread([this, progress] () {
		_asset->onProgress(progress);
		if (_onProgress) {
			_onProgress(this, progress);
		}
	}, this);
}
void AssetDownload::notifyOnComplete(bool success) {
	auto file = (_handle.getResponseCode() < 300) ? getReceiveFile() : String();
	Thread::onMainThread([this, success, file, ct = getReceivedHeaderString("Content-Type"), mtime = _mtime, etag = _etag, size = _size] () {
		if (_onCompleted) {
			_onCompleted(this, success);
		}
		_asset->onCompleted(success, _cacheRequest, file, ct, etag, mtime, size);
	}, this);
}

NS_SP_END
