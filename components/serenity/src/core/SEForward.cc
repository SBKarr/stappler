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
#include "Request.h"
#include "WebSocket.h"
#include "Root.h"
#include "Task.h"
#include "InputFilter.h"

#include "SPUrl.h"
#include "SPString.h"

#include "STStorage.h"
#include "STStorageTransaction.h"

NS_DB_BEGIN

namespace messages {

bool isDebugEnabled() {
	return stappler::serenity::messages::isDebugEnabled();
}

void setDebugEnabled(bool v) {
	stappler::serenity::messages::setDebugEnabled(v);
}

void _addErrorMessage(mem::Value &&data) {
	stappler::serenity::messages::_addErrorMessage(std::move(data));
}

void _addDebugMessage(mem::Value &&data) {
	stappler::serenity::messages::_addDebugMessage(std::move(data));
}

void broadcast(const mem::Value &val) {
	stappler::serenity::messages::broadcast(val);
}

void broadcast(const mem::Bytes &val) {
	stappler::serenity::messages::broadcast(val);
}

}

Transaction Transaction::acquire(const Adapter &adapter) {
	if (adapter.interface() == nullptr) {
		return Transaction(nullptr);
	}

	auto key = adapter.getTransactionKey();

	auto makeRequestTransaction = [&] (request_rec *req) -> Transaction {
		if (auto d = stappler::serenity::Request(req).getObject<Data>(key)) {
			auto t = Transaction(d);
			t.retain();
			return t;
		} else {
			stappler::serenity::Request rctx(req);
			d = new (req->pool) Data{adapter, rctx.pool()};
			d->role = AccessRoleId::System;
			rctx.storeObject(d, key);
			d->role = rctx.getAccessRole();
			auto ret = Transaction(d);
			ret.retain();
			if (auto serv = stappler::apr::pool::server()) {
				stappler::serenity::Server(serv).onStorageTransaction(ret);
			}
			return ret;
		}
	};

	auto log = stappler::apr::pool::info();
	if (log.first == uint32_t(stappler::apr::pool::Info::Request)) {
		return makeRequestTransaction((request_rec *)log.second);
	} else if (auto pool = stappler::apr::pool::acquire()) {
		if (auto d = stappler::memory::pool::get<Data>(pool, key)) {
			auto t = Transaction(d);
			t.retain();
			return t;
		} else {
			d = new (pool) Data{adapter, pool};
			d->role = AccessRoleId::System;
			stappler::apr::pool::store(pool, d, key);
			if (auto req = stappler::apr::pool::request()) {
				d->role = stappler::serenity::Request(req).getAccessRole();
			} else {
				d->role = AccessRoleId::Nobody;
			}
			auto ret = Transaction(d);
			ret.retain();
			if (auto serv = stappler::apr::pool::server()) {
				stappler::serenity::Server(serv).onStorageTransaction(ret);
			}
			return ret;
		}
	}
	return Transaction(nullptr);
}

namespace internals {

Adapter getAdapterFromContext() {
	auto log = stappler::apr::pool::info();
	if (log.first == uint32_t(stappler::apr::pool::Info::Request)) {
		return stappler::serenity::Request((request_rec *)log.second).storage();
	} else {
		auto p = mem::pool::acquire();
		stappler::serenity::websocket::Handler *h = nullptr;
		mem::pool::userdata_get((void **)&h, config::getSerenityWebsocketHandleName(), p);
		if (h) {
			return Adapter(h->storage());
		}
	}
	return Adapter(nullptr);
}

void scheduleAyncDbTask(const stappler::Callback<mem::Function<void(const Transaction &)>(stappler::memory::pool_t *)> &setupCb) {
	if (auto serv = stappler::apr::pool::server()) {
		stappler::serenity::Task::perform(stappler::serenity::Server(serv), [&] (stappler::serenity::Task &task) {
			auto cb = setupCb(task.pool());
			task.addExecuteFn([cb = std::move(cb)] (const stappler::serenity::Task &task) -> bool {
				task.performWithStorage([&] (const Transaction &t) {
					t.performAsSystem([&] () -> bool {
						cb(t);
						return true;
					});
				});
				return true;
			});
		});
	}
}

bool isAdministrative() {
	auto req = stappler::serenity::Request(stappler::apr::pool::request());
	return req && req.isAdministrative();
}

mem::String getDocuemntRoot() {
	return stappler::serenity::Server(stappler::apr::pool::server()).getDocumentRoot();
}

const Scheme *getFileScheme() {
	return stappler::serenity::Server(stappler::apr::pool::server()).getFileScheme();
}

const Scheme *getUserScheme() {
	return stappler::serenity::Server(stappler::apr::pool::server()).getUserScheme();
}

db::InputFile *getFileFromContext(int64_t id) {
	return stappler::serenity::InputFilter::getFileFromContext(id);
}

RequestData getRequestData() {
	RequestData ret;

	if (auto req = stappler::apr::pool::request()) {
		ret.exists = true;
		ret.address = mem::StringView(req->useragent_ip, strlen(req->useragent_ip));
		ret.hostname = mem::StringView(req->hostname, strlen(req->hostname));
		ret.uri = mem::StringView(req->uri, strlen(req->hostname));
	}

	return ret;
}

int64_t getUserIdFromContext() {
	if (auto r = stappler::apr::pool::request()) {
		stappler::serenity::Request req(r);
		if (auto user = req.getAuthorizedUser()) {
			return user->getObjectId();
		} else {
			return req.getUserId();
		}
	}
	return 0;
}

}

NS_DB_END

NS_SA_EXT_BEGIN(messages)

struct ErrorNotificator {
	using Callback = Function<void(data::Value &&)>;

	Callback error;
	Callback debug;
};

bool isDebugEnabled() {
	return Root::getInstance()->isDebugEnabled();
}

void setDebugEnabled(bool v) {
	Root::getInstance()->setDebugEnabled(v);
}

void _addErrorMessage(data::Value &&data) {
	auto root = Root::getInstance();
	if (root->isDebugEnabled()) {
		data::Value bcast{
			std::make_pair("message", data::Value(true)),
			std::make_pair("level", data::Value("error")),
			std::make_pair("data", data::Value(data)),
		};
		if (auto a = storage::Adapter::FromContext()) {
			a.broadcast(bcast);
		}
	}

	if (auto serv = Server(apr::pool::server())) {
		serv.reportError(data);
	}

	Request rctx(apr::pool::request());
	if (rctx) {
		rctx.addErrorMessage(std::move(data));
	} else {
		auto pool = apr::pool::acquire();
		ErrorNotificator *err = nullptr;
		mem::pool::userdata_get((void **)&err, "Serenity.ErrorNotificator", pool);
		if (err && err->error) {
			err->error(std::move(data));
		} else {
			log::text("Error", data::toString(data, false));
		}
	}
}

void _addDebugMessage(data::Value &&data) {
	auto root = Root::getInstance();
	if (root->isDebugEnabled()) {
		data::Value bcast{
			std::make_pair("message", data::Value(true)),
			std::make_pair("level", data::Value("debug")),
			std::make_pair("data", data::Value(data)),
		};
		if (auto a = storage::Adapter::FromContext()) {
			a.broadcast(bcast);
		}
	}

	if (auto serv = Server(apr::pool::server())) {
		serv.reportError(data);
	}

	if (isDebugEnabled()) {
		Request rctx(apr::pool::request());
		if (rctx) {
			rctx.addDebugMessage(std::move(data));
		} else {
			auto pool = apr::pool::acquire();
			ErrorNotificator *err = nullptr;
			mem::pool::userdata_get((void **)&err, (const char *)config::getSerenityErrorNotificatorName(), pool);
			if (err && err->debug) {
				err->debug(std::move(data));
			} else {
				log::text("Debug", data::toString(data, false));
			}
		}
	}
#if DEBUG
	log::text("Debug", data::toString(data, false));
#endif
}

void broadcast(const data::Value &val) {
	if (val.getBool("local")) {
		Root::getInstance()->onBroadcast(val);
	} else {
		if (auto a = storage::Adapter::FromContext()) {
			a.broadcast(val);
		}
	}
}
void broadcast(const Bytes &val) {
	if (auto a = storage::Adapter::FromContext()) {
		a.broadcast(val);
	}
}

void broadcastAsync(const data::Value &val) {
	Task::perform([&] (Task &task) {
		auto v = new data::Value(val);
		task.addExecuteFn([v] (const Task &task) -> bool {
			task.performWithStorage([&] (const storage::Transaction &t) {
				db::Adapter(t.getAdapter()).broadcast(*v);
			});
			return true;
		});
	});
}

void setNotifications(apr_pool_t *pool,
		const Function<void(data::Value &&)> &error,
		const Function<void(data::Value &&)> &debug) {
	ErrorNotificator *err = (ErrorNotificator *)apr_pcalloc(pool, sizeof(ErrorNotificator));
	new (err) ErrorNotificator();
	err->error = error;
	err->debug = debug;

	mem::pool::userdata_set(err, (const char *)config::getSerenityErrorNotificatorName(), nullptr, pool);
}

NS_SA_EXT_END(messages)
