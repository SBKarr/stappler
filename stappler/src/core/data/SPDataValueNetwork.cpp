/*
 * SPDataValueNetwork.cpp
 *
 *  Created on: 6 дек. 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPData.h"
#include "SPFilesystem.h"
#include "SPString.h"
#include "SPNetworkDataTask.h"
#include "SPDataJsonBuffer.h"

NS_SP_EXT_BEGIN(data)

std::atomic<uint32_t> s_networkCounter(0);

struct NetworkDataReader {
	NetworkDataReader(const String &url, const std::function<void(data::Value &)> &cb, Thread &t, const String &key)
	: _url(url), _key(key), _callback(cb) {
		auto task = Rc<NetworkDataTask>::create(NetworkTask::Method::Get, _url);
		task->addCompleteCallback(std::bind(&NetworkDataReader::onDownloadCompleted, this, task, std::placeholders::_2));
		t.perform(task);
	}

	void onDownloadCompleted(NetworkDataTask *task, bool success) {
		auto &data = task->getData();
		_callback(data);
		delete this;
	}

	String _url, _key;
	std::function<void(Value &)> _callback = nullptr;
};

void readUrl(const String &url, const std::function<void(Value &)> &cb, const std::string &key) {
	new NetworkDataReader(url, cb, NetworkTask::thread(), key);
}

void readUrl(const String &url, const std::function<void(data::Value &)> &cb, Thread &t, const std::string &key) {
	new NetworkDataReader(url, cb, t, key);
}

NS_SP_EXT_END(data)
