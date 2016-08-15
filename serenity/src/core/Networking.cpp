/*
 * Networking.cpp
 *
 *  Created on: 12 мая 2016 г.
 *      Author: sbkarr
 */

#include "Define.h"
#include "Networking.h"
#include "SPDataStream.h"
#include "Request.h"

NS_SA_EXT_BEGIN(network)

Handle::Handle(Method method, const String &url) {
	init(method, url);
	setReuse(false);
}

data::Value Handle::performDataQuery() {
	data::Stream stream;
	setReceiveCallback([&] (char *data, size_t size) -> size_t {
		stream.write(data, size);
		return size;
	});
	if (perform()) {
		return stream.data();
	}
	return data::Value();
}
data::Value Handle::performDataQuery(const data::Value &val, data::EncodeFormat fmt) {
	apr::ostringstream stream;
	stream << fmt << val;
	DataReader<ByteOrder::Network> r((const uint8_t *)stream.data(), stream.size());
	setSendCallback([&] (char *data, size_t size) -> size_t {
		auto writeSize = std::min(size, r.size());
		memcpy(data, r.data(), writeSize);
		r.offset(writeSize);
		return writeSize;
	}, r.size());
	return performDataQuery();
}
bool Handle::performCallbackQuery(const IOCallback &cb) {
	setReceiveCallback(cb);
	return perform();
}
bool Handle::performProxyQuery(Request &rctx) {
	bool init = false;
	setReceiveCallback([&] (char *data, size_t size) -> size_t {
		if (!init) {
			rctx.setContentType(getReceivedHeaderString("Content-Type"));
			init = true;
		}
		rctx.write(data, size);
		return size;
	});
	if (perform()) {
		rctx.flush();
		return true;
	}
	return false;
}

Mail::Mail(const String &url, const String &user, const String &passwd) {
	init(Method::Smtp, url);
	setAuthority(user, passwd);
	setReuse(false);
}

bool Mail::send(apr::ostringstream &stream) {
	CharReaderBase r(stream.data(), stream.size());
	setSendCallback([&] (char *buf, size_t size) -> size_t {
		auto writeSize = std::min(size, r.size());
		memcpy(buf, r.data(), writeSize);
		r.offset(writeSize);
		return writeSize;
	}, stream.size());
	return perform();
}

NS_SA_EXT_END(network)
