/*
 * SPDefault.h
 *
 *  Created on: 19 марта 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_CORE_SPDEFAULT_H_
#define LIBS_STAPPLER_CORE_SPDEFAULT_H_

/* this file should be included by SPDefine.h */
#include "SPForward.h"

NS_SP_BEGIN

using Opacity = uint8_t;

template <class T, class... Args>
inline T *construct(Args&&... args) {
	auto pRet = new T();
    if (pRet->init(std::forward<Args>(args)...)) {
    	pRet->autorelease();
		return pRet;
	} else { \
		delete pRet;
		return nullptr;
	}
}

template <class T, class... Args>
inline T *create(Args&&... args) {
	return construct<T, Args...>(std::forward<Args>(args)...);
}

namespace screen {
	float density();
	int dpi();
	float statusBarHeight();
}

namespace node {
	bool isTouched(cocos2d::Node *node, const cocos2d::Vec2 &point, float padding = 0);
	void dump(cocos2d::Node *node, int depth = -1);
}

inline uint8_t getIntensivityFromColor(const cocos2d::Color4B &color) {
	float r = color.r / 255.0;
	float g = color.g / 255.0;
	float b = color.b / 255.0;

	return 0.2989f * r + 0.5870f * g + 0.1140f * b;
}

inline uint8_t getIntensivityFromColor(const cocos2d::Color3B &color) {
	float r = color.r / 255.0;
	float g = color.g / 255.0;
	float b = color.b / 255.0;

	return 0.2989f * r + 0.5870f * g + 0.1140f * b;
}

void storeForSeconds(cocos2d::Ref *, float, const std::string & = "");
void storeForFrames(cocos2d::Ref *, uint64_t, const std::string & = "");
cocos2d::Ref *getStoredRef(const std::string &);

void PrintBacktrace(int len, cocos2d::Ref * = nullptr);

template <typename T>
inline void safe_retain(T *t) {
	if (t) {
		t->retain();
	}
}

template <typename T>
inline void safe_release(T *t) {
	if (t) {
		t->release();
	}
}

NS_SP_END

#endif /* LIBS_STAPPLER_CORE_SPDEFAULT_H_ */
