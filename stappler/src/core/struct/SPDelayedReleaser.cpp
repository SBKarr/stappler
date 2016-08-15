/*
 * SPDelayedRelease.cpp
 *
 *  Created on: 24 мая 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"

#include "base/CCDirector.h"
#include "base/CCScheduler.h"

NS_SP_BEGIN

struct _DelayedReleaser {
	void update(float dt);

	inline void releaseAfterTime(cocos2d::Ref *, float, const std::string &name);
	inline void releaseAfterFrames(cocos2d::Ref *, uint64_t, const std::string &name);

	inline void registerWithDispatcher();
	inline void unregisterWithDispatcher();

	inline void storeRef(cocos2d::Ref *, const std::string &name);
	inline void removeRef(cocos2d::Ref *);
	inline cocos2d::Ref *getRef(const std::string &name) const;

	bool _registred = false;
	std::map<cocos2d::Ref *, uint64_t> _frames;
	std::map<cocos2d::Ref *, std::string> _names;
	std::map<std::string, cocos2d::Ref *> _refs;
};

static _DelayedReleaser * s_delayedReleaser = nullptr;

void storeForSeconds(cocos2d::Ref *ref, float t, const std::string &name) {
	if (ref) {
		if (!s_delayedReleaser) {
			s_delayedReleaser = new _DelayedReleaser();
		}
		s_delayedReleaser->releaseAfterFrames(ref, (uint64_t)ceilf(fabsf(t) * 60.0f), name);
	}
}
void storeForFrames(cocos2d::Ref *ref, uint64_t f, const std::string &name) {
	if (ref) {
		if (!s_delayedReleaser) {
			s_delayedReleaser = new _DelayedReleaser();
		}
		s_delayedReleaser->releaseAfterFrames(ref, f, name);
	}
}

cocos2d::Ref *getStoredRef(const std::string &name) {
	if (!name.empty() && s_delayedReleaser) {
		return s_delayedReleaser->getRef(name);
	}
	return nullptr;
}

void _DelayedReleaser::update(float dt) {
	std::vector<cocos2d::Ref *> releaseFrames;
	auto itFrames = _frames.begin();
	while (itFrames != _frames.end()) {
		if (itFrames->first->getReferenceCount() == 1) {
			if (itFrames->second == 0) {
				releaseFrames.push_back(itFrames->first);
			} else {
				itFrames->second --;
			}
		}
		itFrames ++;
	}
	for (auto &it : releaseFrames) {
		removeRef(it);
		_frames.erase(it);
		it->release();
	}
	releaseFrames.clear();

	if (_frames.empty()) {
		unregisterWithDispatcher();
	}
}

inline void _DelayedReleaser::releaseAfterFrames(cocos2d::Ref *ref, uint64_t f, const std::string &name) {
	auto it = _frames.find(ref);
	if (it == _frames.end()) {
		ref->retain();
		_frames.insert(std::make_pair(ref, f));
		if (!name.empty()) {
			storeRef(ref, name);
		}
		registerWithDispatcher();
	} else {
		it->second = f;
	}
}

inline void _DelayedReleaser::registerWithDispatcher() {
#if (!SP_RESTRICT)
	if (!_registred) {
		auto dir = cocos2d::Director::getInstance();
		auto sc = dir->getScheduler();
		sc->scheduleUpdate(this, 0, false);
		_registred = true;
	}
#endif
}
inline void _DelayedReleaser::unregisterWithDispatcher() {
#if (!SP_RESTRICT)
	if (_registred) {
		auto dir = cocos2d::Director::getInstance();
		auto sc = dir->getScheduler();
		sc->unscheduleUpdate(this);
		_registred = false;
	}
#endif
}

inline void _DelayedReleaser::storeRef(cocos2d::Ref *ref, const std::string &name) {
	_names.insert(std::make_pair(ref, name));
	_refs.insert(std::make_pair(name, ref));
}
inline void _DelayedReleaser::removeRef(cocos2d::Ref *ref) {
	auto it = _names.find(ref);
	if (it != _names.end()) {
		_refs.erase(it->second);
		_names.erase(it);
	}
}
inline cocos2d::Ref *_DelayedReleaser::getRef(const std::string &name) const {
	auto it = _refs.find(name);
	if (it != _refs.end()) {
		return it->second;
	}
	return nullptr;
}

NS_SP_END
