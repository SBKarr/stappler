// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "Define.h"
#include "Networking.h"
#include "SPDataStream.h"
#include "Request.h"

NS_SA_EXT_BEGIN(network)

Handle::Handle(Method method, const String &url) {
	init(method, url);
	setReuse(false);
}

Bytes Handle::performBytesQuery() {
	Buffer stream;
	setReceiveCallback([&] (char *data, size_t size) -> size_t {
		stream.put(data, size);
		return size;
	});
	if (perform()) {
		auto r = stream.get();
		return Bytes(r.data(), r.data() + r.size());
	}
	return Bytes();
}

data::Value Handle::performDataQuery() {
	data::Stream stream;
	setReceiveCallback([&] (char *data, size_t size) -> size_t {
		stream.write(data, size);
		return size;
	});
	if (perform()) {
		return stream.extract();
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
			rctx.setHookErrors(false);
			rctx.setContentType(getReceivedHeaderString("Content-Type").str());
			rctx.setStatus(getResponseCode());
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

Mail::Mail(const data::Value &config) {
	init(Method::Smtp, config.getString("server"));
	setAuthority(config.getString("user"), config.getString("password"));
	setMailFrom(config.getString("from"));
	setReuse(false);
}

bool Mail::send(apr::ostringstream &stream) {
	StringView r(stream.data(), stream.size());
	setSendCallback([&] (char *buf, size_t size) -> size_t {
		auto writeSize = std::min(size, r.size());
		memcpy(buf, r.data(), writeSize);
		r.offset(writeSize);
		return writeSize;
	}, stream.size());
	return perform();
}

NS_SA_EXT_END(network)
