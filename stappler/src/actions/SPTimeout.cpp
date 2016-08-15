/*
 * SPCallback.cpp
 *
 *  Created on: 19 окт. 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPTimeout.h"
#include "2d/CCActionInstant.h"

NS_SP_BEGIN

bool Timeout::init(float duration, const std::function<void()> &cb) {
	return initWithTwoActions(cocos2d::DelayTime::create(duration), cocos2d::CallFunc::create(cb));
}

bool Timeout::init(cocos2d::FiniteTimeAction *a, const std::function<void()> &cb) {
	return initWithTwoActions(a, cocos2d::CallFunc::create(cb));
}

NS_SP_END
