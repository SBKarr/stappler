//
//  SPNetworkDownloadTask.cpp
//  chiertime-federal
//
//  Created by SBKarr on 6/24/13.
//
//

#include "SPDefine.h"
#include "SPNetworkDownload.h"
#include "SPTaskStack.h"
#include "SPFilesystem.h"
#include "SPThread.h"

#include "platform/CCFileUtils.h"

USING_NS_SP;

NetworkDownload::NetworkDownload() { }

bool NetworkDownload::init(const std::string &url, const std::string &fileName) {
    if (!NetworkTask::init(Method::Get, url)) {
        return false;
    }

    if (!fileName.empty()) {
    	setReceiveFile(fileName, false);
    }
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

void NetworkDownload::onComplete() {
	notifyOnComplete(isSuccessful());
	NetworkTask::onComplete();
}

void NetworkDownload::notifyOnStarted(bool bind) {
	Thread::onMainThread([this] () {
		if (_onStarted) {
			_onStarted(this);
		}
	}, bind ? this : nullptr);
}
void NetworkDownload::notifyOnProgress(float progress, bool bind) {
	Thread::onMainThread([this, progress] () {
		if (_onProgress) {
			_onProgress(this, progress);
		}
	}, bind ? this : nullptr);
}

void NetworkDownload::notifyOnComplete(bool success, bool bind) {
	Thread::onMainThread([this, success] () {
		if (_onCompleted) {
			_onCompleted(this, success);
		}
	}, bind ? this : nullptr);
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
