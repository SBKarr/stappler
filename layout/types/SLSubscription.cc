// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPLayout.h"
#include "SLSubscription.h"

NS_LAYOUT_BEGIN

Subscription::Flags Subscription::Initial(1);

Subscription::Id Subscription::getNextId() {
	static std::atomic<Subscription::Id::Type> nextId(0);
	return Id(nextId.fetch_add(1));
}

void Subscription::setDirty(Flags flags) {
	for (auto &it : _flags) {
		it.second |= flags;
	}
}

bool Subscription::subscribe(Id id) {
	auto it = _flags.find(id);
	if (it == _flags.end()) {
		_flags.insert(std::make_pair(id, Initial));
		return true;
	}
	return false;
}

bool Subscription::unsubscribe(Id id) {
	auto it = _flags.find(id);
	if (it != _flags.end()) {
		_flags.erase(id);
		return true;
	}
	return false;
}

Subscription::Flags Subscription::check(Id id) {
	auto it = _flags.find(id);
	if (it != _flags.end()) {
		auto val = it->second;
		it->second = Flags(0);
		return val;
	}
	return Flags(0);
}

NS_LAYOUT_END
