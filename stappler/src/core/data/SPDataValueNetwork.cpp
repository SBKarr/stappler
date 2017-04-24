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
#include "SPData.h"
#include "SPFilesystem.h"
#include "SPString.h"
#include "SPNetworkDataTask.h"
#include "SPDataJsonBuffer.h"
#include "SPDevice.h"

NS_SP_EXT_BEGIN(data)

std::atomic<uint32_t> s_networkCounter(0);

struct NetworkDataReader {
	NetworkDataReader(const String &url, const Function<void(Value &)> &cb,
			const Function<void(const NetworkDataTask &, Value &)> &ncb, Thread &t, const String &key)
	: _url(url), _key(key), _callback(cb), _nwcallback(ncb) {
		auto task = Rc<NetworkDataTask>::create(NetworkTask::Method::Get, _url);
		task->addCompleteCallback(std::bind(&NetworkDataReader::onDownloadCompleted, this, task, std::placeholders::_2));
		t.perform(task);
	}

	void onDownloadCompleted(NetworkDataTask *task, bool success) {
		auto &data = task->getData();
		if (_callback) {
			_callback(data);
		}
		if (_nwcallback) {
			_nwcallback(*task, data);
		}
		delete this;
	}

	String _url, _key;
	Function<void(Value &)> _callback = nullptr;
	Function<void(const NetworkDataTask &, Value &)> _nwcallback = nullptr;
};

void readUrl(const String &url, const Function<void(Value &)> &cb, const String &key) {
	new NetworkDataReader(url, cb, nullptr, NetworkTask::thread(), key);
}

void readUrl(const String &url, const Function<void(Value &)> &cb, Thread &t, const String &key) {
	new NetworkDataReader(url, cb, nullptr, t, key);
}

void readUrl(const String &url, const Function<void(const NetworkDataTask &, Value &)> &cb, const String &key) {
	new NetworkDataReader(url, nullptr, cb, NetworkTask::thread(), key);
}

void readUrl(const String &url, const Function<void(const NetworkDataTask &, Value &)> &cb, Thread &t, const String &key) {
	new NetworkDataReader(url, nullptr, cb, t, key);
}

NS_SP_EXT_END(data)
