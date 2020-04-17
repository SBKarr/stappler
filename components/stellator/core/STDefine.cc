/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "STDefine.h"

#include "STServer.h"
#include "STMemory.h"
#include "STTask.h"
#include "STRoot.h"

namespace stellator::mem {

static stellator::Server::Config *getServerFromContext(pool_t *p, uint32_t tag, const void *ptr) {
	switch (tag) {
	case uint32_t(Info::Server): return (stellator::Server::Config *)ptr; break;
	//case uint32_t(Connection): return ((conn_rec *)ptr)->base_server; break;
	//case uint32_t(Request): return ((request_rec *)ptr)->server; break;
	case uint32_t(Info::Pool): return mem::pool::get<stellator::Server::Config>(p, "Apr.Server"); break;
	}
	return nullptr;
}

stellator::Server server() {
	stellator::Server::Config *ret = nullptr;
	pool::foreach_info(&ret, [] (void *ud, pool_t *p, uint32_t tag, const void *data) -> bool {
		auto ptr = getServerFromContext(p, tag, data);
		if (ptr) {
			*((stellator::Server::Config **)ud) = ptr;
			return false;
		}
		return true;
	});

	return stellator::Server(ret);
}

stellator::Request request() {
	stellator::Request::Config *ret = nullptr;
	pool::foreach_info(&ret, [] (void *ud, pool_t *p, uint32_t tag, const void *data) -> bool {
		if (tag == uint32_t(Info::Request)) {
			*((stellator::Request::Config **)ud) = (stellator::Request::Config *)data;
			return false;
		}

		return true;
	});

	return ret;
}

}


NS_DB_BEGIN

namespace messages {

static std::mutex s_debugMutex;

bool isDebugEnabled() {
	return stellator::Root::getInstance()->isDebugEnabled();
}

void setDebugEnabled(bool v) {
	stellator::Root::getInstance()->setDebugEnabled(v);
}

void _addErrorMessage(mem::Value &&data) {
	s_debugMutex.lock();
	std::cout << "[Error]: " << stappler::data::EncodeFormat::Pretty << data << "\n";
	s_debugMutex.unlock();
	// not implemented
}

void _addDebugMessage(mem::Value &&data) {
	s_debugMutex.lock();
	std::cout << "[Debug]: " << stappler::data::EncodeFormat::Pretty << data << "\n";
	s_debugMutex.unlock();
	// not implemented
}

void broadcast(const mem::Value &val) {
	if (val.getBool("local")) {
		stellator::Root::getInstance()->onBroadcast(val);
	} else {
		if (auto a = db::Adapter::FromContext()) {
			a.broadcast(val);
		}
	}
}

void broadcast(const mem::Bytes &val) {
	if (auto a = db::Adapter::FromContext()) {
		a.broadcast(val);
	}
}

}

Transaction Transaction::acquire(const Adapter &adapter) {
	if (auto pool = stappler::memory::pool::acquire()) {
		if (auto d = stappler::memory::pool::get<Data>(pool, "current_transaction")) {
			return Transaction(d);
		} else {
			d = new (pool) Data{adapter};
			d->role = AccessRoleId::System;
			stappler::memory::pool::store(pool, d, "current_transaction");
			return Transaction(d);
		}
	}
	return Transaction(nullptr);
}

namespace internals {

Adapter getAdapterFromContext() {
	if (auto p = mem::pool::acquire()) {
		Interface *h = nullptr;
		stappler::memory::pool::userdata_get((void **)&h, config::getStorageInterfaceKey(), p);
		if (h) {
			return Adapter(h);
		}
	}
	return Adapter(nullptr);
}

void scheduleAyncDbTask(const stappler::Callback<mem::Function<void(const Transaction &)>(stappler::memory::pool_t *)> &setupCb) {
	if (auto serv = stellator::mem::server()) {
		stellator::Task::perform(serv, [&] (stellator::Task &task) {
			auto cb = setupCb(task.pool());
			task.addExecuteFn([cb = std::move(cb)] (const stellator::Task &task) -> bool {
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
	return true;
}

mem::String getDocuemntRoot() {
	return stellator::mem::server().getDocumentRoot().str<mem::Interface>();
}

const Scheme *getFileScheme() {
	return stellator::mem::server().getFileScheme();
}

const Scheme *getUserScheme() {
	return stellator::mem::server().getUserScheme();
}

InputFile *getFileFromContext(int64_t) {
	return nullptr;
}

RequestData getRequestData() {
	return RequestData();
}

int64_t getUserIdFromContext() {
	return 0;
}

}

NS_DB_END
