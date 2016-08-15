/*
 * SPDataObserver.cpp
 *
 *  Created on: 12 янв. 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPDataSubscription.h"

NS_SP_EXT_BEGIN(data)

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

NS_SP_EXT_END(data)
