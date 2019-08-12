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

#include "STTask.h"
#include "STRoot.h"
#include "STPqHandle.h"

namespace stellator {

void Task::addExecuteFn(const ExecuteCallback &cb) {
	mem::perform([&] {
		_execute.push_back(cb);
	}, _pool);
}

void Task::addExecuteFn(ExecuteCallback &&cb) {
	mem::perform([&] {
		_execute.push_back(std::move(cb));
	}, _pool);
}

void Task::addCompleteFn(const CompleteCallback &cb) {
	mem::perform([&] {
		_complete.push_back(cb);
	}, _pool);
}
void Task::addCompleteFn(CompleteCallback &&cb) {
	mem::perform([&] {
		_complete.push_back(std::move(cb));
	}, _pool);
}

void Task::performWithStorage(const mem::Callback<void(const db::Transaction &)> &cb) const {
	auto root = Root::getInstance();
	auto dbd = root->dbdOpen(pool(), _server);
	if (dbd.get()) {
		db::pq::Handle h(root->getDbDriver(), dbd);
		if (auto t = db::Transaction::acquire(&h)) {
			cb(t);
		}
		root->dbdClose(_server, dbd);
	}
}

bool Task::execute() {
	bool success = true;
	for (auto &it : _execute) {
		if (!it(*this)) {
			success = false;
		}
	}
	return success;
}
void Task::onComplete() {
	for (auto &it : _complete) {
		it(*this, _isSuccessful);
	}
}

Task::Task(mem::pool_t *p) : MemPool(p, MemPool::WrapTag()) { }

}
