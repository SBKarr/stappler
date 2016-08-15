/*
 * StorageAdapter.cpp
 *
 *  Created on: 29 февр. 2016 г.
 *      Author: sbkarr
 */

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
	auto log = AllocStack::get().log();
	if (log.target == apr::LogContext::Target::Request) {
		return Request(log.request).storage();
	} else if (log.target == apr::LogContext::Target::Pool) {
		websocket::Handler *h = nullptr;
		apr_pool_userdata_get((void **)h, config::getSerenityWebsocketHandleName(), log.pool);
		if (h) {
			return h->storage();
		}
	}
	return nullptr;
}

NS_SA_EXT_END(storage)
