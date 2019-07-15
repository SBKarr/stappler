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

NS_DB_BEGIN

namespace messages {

void _addErrorMessage(mem::Value &&data) {
	// not implemented
}

void _addDebugMessage(mem::Value &&data) {
	// not implemented
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
	if (auto p = stappler::memory::pool::acquire()) {
		Interface *h = nullptr;
		stappler::memory::pool::userdata_get((void **)&h, config::getStorageInterfaceKey(), p);
		if (h) {
			return Adapter(h);
		}
	}
	return Adapter(nullptr);
}

void scheduleAyncDbTask(const stappler::Callback<mem::Function<void(const Transaction &)>(stappler::memory::pool_t *)> &setupCb) {

}

bool isAdministrative() {
	return false;
}

mem::String getDocuemntRoot() {
	auto p = stappler::filesystem::writablePath();
	return mem::String(p.data(), p.size());
}

const Scheme *getFileScheme() {
	return nullptr;
}

const Scheme *getUserScheme() {
	return nullptr;
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
