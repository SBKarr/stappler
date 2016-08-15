//
//  SPNetworkTask.cpp
//  chiertime-federal
//
//  Created by SBKarr on 6/20/13.
//
//

#include "SPDefine.h"
#include "SPNetworkTask.h"
#include "SPTaskManager.h"
#include "SPTaskStack.h"
#include "SPFilesystem.h"
#include "SPDevice.h"
#include "SPData.h"
#include "SPThread.h"
#include "SPThreadManager.h"
#include "SPString.h"

#include "SPThreadLocal.h"

#ifndef SP_NETWORK_THREADS
#define SP_NETWORK_THREADS 3
#endif

NS_SP_BEGIN;

Thread NetworkTask::_networkThread("NetworkingThread", SP_NETWORK_THREADS);

Thread &NetworkTask::thread() {
	return _networkThread;
}

NetworkTask::NetworkTask() { }

NetworkTask::~NetworkTask() { }

bool NetworkTask::init(Method method, const std::string &url) {
	if (!_handle.init(method, url)) {
		return false;
	}

	Device::getInstance();

    return true;
}

bool NetworkTask::execute() {
    bool val = performQuery();
    if (val) {
        return Task::execute();
    } else {
		return false;
    }
}

bool NetworkTask::performQuery() {
	#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	_handle.setRootCertificateFile(filesystem::writablePath("ca_bundle.pem"));
	#endif

	if (_handle.getCookieFile().empty()) {
		std::string cookies("network.cookies");
		if (!ThreadManager::getInstance()->isMainThread()) {
			auto &local = Thread::getThreadLocalStorage();
			if (!local.isNull() && local.getBool("managed_thread")) {
				cookies = filesystem::writablePath(
						toString(cookies, ".", Thread::getThreadName(), ".", Thread::getThreadId(), ".", Thread::getWorkerId()));
			} else {
				cookies = filesystem::writablePath(
						toString(cookies, ".native.", ThreadManager::getInstance()->getNativeThreadId()));
			}
		} else {
			cookies = filesystem::writablePath(std::string(cookies) + ".main");
		}

		_handle.setCookieFile(cookies);
	}

	_handle.setUserAgent(Device::getInstance()->getUserAgent());

	return _handle.perform();
}

void NetworkTask::run() {
	_networkThread.perform(this);
}

void NetworkTask::addHeader(const std::string &header) {
	_handle.addHeader(header);
}

void NetworkTask::addHeader(const std::string &header, const std::string &value) {
	_handle.addHeader(header, value);
}

void NetworkTask::setThreadDownloadProgress(const ThreadProgressCallback &callback) {
	_handle.setDownloadProgress(callback);
}
void NetworkTask::setThreadUploadProgress(const ThreadProgressCallback &callback) {
	_handle.setUploadProgress(callback);
}

void NetworkTask::setReceiveFile(const std::string &str, bool resumeDownload) {
	_handle.setReceiveFile(str, resumeDownload);
}
void NetworkTask::setReceiveCallback(const IOCallback &cb) {
	_handle.setReceiveCallback(cb);
}

void NetworkTask::setSendFile(const std::string &str) {
	_handle.setSendFile(str);
}
void NetworkTask::setSendCallback(const IOCallback &cb, size_t outSize) {
	_handle.setSendCallback(cb, outSize);
}
void NetworkTask::setSendData(const std::string &data) {
	_handle.setSendData(data);
}
void NetworkTask::setSendData(const Bytes &data) {
	_handle.setSendData(data);
}
void NetworkTask::setSendData(Bytes &&data) {
	_handle.setSendData(std::move(data));
}
void NetworkTask::setSendData(const uint8_t *data, size_t size) {
	_handle.setSendData(data, size);
}
void NetworkTask::setSendData(const data::Value &data, data::EncodeFormat fmt) {
	_handle.setSendData(data, fmt);
}

std::string NetworkTask::getReceivedHeaderString(const std::string &h) {
	return _handle.getReceivedHeaderString(h);
}

int64_t NetworkTask::getReceivedHeaderInt(const std::string &h) {
	return _handle.getReceivedHeaderInt(h);
}

NS_SP_END
