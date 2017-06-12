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

#include "Define.h"
#include "StorageAdapter.h"
#include "StorageResolver.h"
#include "StorageScheme.h"
#include "Request.h"
#include "WebSocket.h"

NS_SA_EXT_BEGIN(storage)

const Field *Resolver::getSchemeField(const String &name) const {
	auto scheme = getScheme();
	auto ret = scheme->getField(name);
	if (ret) {
		return ret;
	}
	if (_transform) {
		auto t = _transform->find(scheme->getName());
		if (t != _transform->end()) {
			auto &in = t->second.input;
			auto newKey = in.transformKey(name);
			if (!newKey.empty()) {
				return scheme->getField(newKey);
			}
		}
	}
	return nullptr;
}
Scheme *Resolver::getScheme() const {
	return _scheme;
}

Adapter *Adapter::FromContext() {
	auto log = apr::pool::info();
	if (log.first == uint32_t(apr::pool::Info::Request)) {
		return Request((request_rec *)log.second).storage();
	} else if (log.first == uint32_t(apr::pool::Info::Pool)) {
		websocket::Handler *h = nullptr;
		apr_pool_userdata_get((void **)h, config::getSerenityWebsocketHandleName(), (apr_pool_t *)log.second);
		if (h) {
			return h->storage();
		}
	}
	return nullptr;
}

NS_SA_EXT_END(storage)
