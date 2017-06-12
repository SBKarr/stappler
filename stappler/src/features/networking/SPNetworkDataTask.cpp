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
#include "SPNetworkDataTask.h"
#include "SPDataStream.h"

NS_SP_BEGIN

NetworkDataTask::NetworkDataTask() { }
NetworkDataTask::~NetworkDataTask() { }

bool NetworkDataTask::init(Method method, const std::string &url, const data::Value &data, data::EncodeFormat f) {
	if (!NetworkTask::init(method, url)) {
		return false;
	}

	addHeader("Accept", "application/cbor, application/json");

	if ((method == Method::Post || method == Method::Put) && !data.isNull()) {
		setSendData(data, f);
	}

	return true;
}

bool NetworkDataTask::execute() {
	data::StreamBuffer buffer;
	auto ret = false;

	setReceiveCallback([&] (char *data, size_t size) -> size_t {
		buffer.sputn(data, size);
		return size;
	});

	ret = NetworkTask::execute();

	setReceiveCallback(nullptr);

	_data = buffer.extract();

	return ret;
}

const data::Value &NetworkDataTask::getData() const {
	return _data;
}
data::Value &NetworkDataTask::getData() {
	return _data;
}

NS_SP_END
