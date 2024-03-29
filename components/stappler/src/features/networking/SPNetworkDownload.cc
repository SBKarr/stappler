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
#include "SPNetworkDownload.h"
#include "SPFilesystem.h"
#include "SPThread.h"

#include "platform/CCFileUtils.h"

NS_SP_BEGIN

NetworkDownload::NetworkDownload() { }

bool NetworkDownload::init(const StringView &url, const StringView &fileName) {
    if (!NetworkTask::init(Method::Get, url)) {
        return false;
    }

    if (!fileName.empty()) {
    	setReceiveFile(fileName, false);
    }

    addCompleteCallback([this] (const Task &, bool success) {
    	notifyOnComplete(success);
    });

    return true;
}

bool NetworkDownload::execute() {
    notifyOnStarted();
    return NetworkTask::execute();
}

bool NetworkDownload::performQuery() {
	bool ret = NetworkTask::performQuery();
    if (!ret) {
		filesystem::remove(getReceiveFile());
    }
	return ret;
}

void NetworkDownload::notifyOnStarted() {
	Thread::onMainThread([this] () {
		if (_onStarted) {
			_onStarted(this);
		}
	}, this);
}
void NetworkDownload::notifyOnProgress(float progress) {
	Thread::onMainThread([this, progress] () {
		if (_onProgress) {
			_onProgress(this, progress);
		}
	}, this);
}

void NetworkDownload::notifyOnComplete(bool success) {
	Thread::onMainThread([this, success] () {
		if (_onCompleted) {
			_onCompleted(this, success);
		}
	}, this);
}

void NetworkDownload::setStartedCallback(StartedCallback func) {
	_onStarted = func;
}

void NetworkDownload::setProgressCallback(ProgressCallback func) {
	_onProgress = func;
	if (_onProgress) {
		setThreadDownloadProgress([this] (int64_t total, int64_t now) -> int {
			float progress = (float)(now) / (float)(total);
			notifyOnProgress(progress);
			return 0;
		});
	} else {
		setThreadDownloadProgress(nullptr);
	}
}

void NetworkDownload::setCompletedCallback(CompletedCallback func) {
	_onCompleted = func;
}

NS_SP_END
