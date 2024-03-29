/**
Copyright (c) 2016-2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPDataSubscription.h"

namespace stappler::data {

Subscription::Flags Subscription::Initial(1);

Subscription::Id Subscription::getNextId() {
	static std::atomic<Subscription::Id::Type> nextId(0);
	return Id(nextId.fetch_add(1));
}

Subscription::~Subscription() {
	setForwardedSubscription(nullptr);
}

void Subscription::setDirty(Flags flags, bool forwardedOnly) {
	if (_forwardedFlags) {
		for (auto &it : (*_forwardedFlags)) {
			if (forwardedOnly) {
				if (_flags.find(it.first) != _flags.end()) {
					it.second |= flags;
				}
			} else {
				it.second |= flags;
			}
		}
	} else {
		for (auto &it : _flags) {
			it.second |= flags;
		}
	}
}

bool Subscription::subscribe(Id id) {
	if (_forwardedFlags) {
		auto it = _forwardedFlags->find(id);
		if (it == _forwardedFlags->end()) {
			_forwardedFlags->insert(std::make_pair(id, Initial));
			_flags.insert(std::make_pair(id, Initial));
			return true;
		}
	} else {
		auto it = _flags.find(id);
		if (it == _flags.end()) {
			_flags.insert(std::make_pair(id, Initial));
			return true;
		}
	}
	return false;
}

bool Subscription::unsubscribe(Id id) {
	if (_forwardedFlags) {
		auto it = _forwardedFlags->find(id);
		if (it == _forwardedFlags->end()) {
			_forwardedFlags->erase(id);
			_flags.erase(id);
			return true;
		}
	} else {
		auto it = _flags.find(id);
		if (it != _flags.end()) {
			_flags.erase(id);
			return true;
		}
	}
	return false;
}

Subscription::Flags Subscription::check(Id id) {
	if (_forwardedFlags) {
		auto it = _forwardedFlags->find(id);
		if (it != _forwardedFlags->end()) {
			auto val = it->second;
			it->second = Flags(0);
			return val;
		}
	} else {
		auto it = _flags.find(id);
		if (it != _flags.end()) {
			auto val = it->second;
			it->second = Flags(0);
			return val;
		}
	}
	return Flags(0);
}

void Subscription::setForwardedSubscription(Subscription *sub) {
	if (_forwardedFlags) {
		// erase forwarded items from sub
		for (auto &it : _flags) {
			_forwardedFlags->erase(it.first);
		}
	}
	_forwardedFlags = nullptr;
	_forwarded = sub;
	_flags.clear();
	if (sub) {
		_forwardedFlags = sub->_forwardedFlags ? sub->_forwardedFlags : &sub->_flags;
	}
}

}
