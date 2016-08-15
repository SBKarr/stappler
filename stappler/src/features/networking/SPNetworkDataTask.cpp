/*
 * SPNetworkDataTask.cpp
 *
 *  Created on: 16 марта 2016 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPNetworkDataTask.h"

NS_SP_BEGIN

NetworkDataTask::NetworkDataTask() { }
NetworkDataTask::~NetworkDataTask() { }

bool NetworkDataTask::init(Method method, const std::string &url, const data::Value &data, data::EncodeFormat f) {
	if (!NetworkTask::init(method, url)) {
		return false;
	}

	setReceiveCallback([this] (char *data, size_t size) -> size_t {
		_buffer.sputn(data, size);
		return size;
	});

	addHeader("Accept", "application/cbor, application/json");

	if ((method == Method::Post || method == Method::Put) && !data.isNull()) {
		setSendData(data, f);
	}

	return true;
}

const data::Value &NetworkDataTask::getData() const {
	return _buffer.data();
}
data::Value &NetworkDataTask::getData() {
	return _buffer.data();
}

NS_SP_END
