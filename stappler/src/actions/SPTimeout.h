/*
 * SPCallback.h
 *
 *  Created on: 19 окт. 2015 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_ACTIONS_SPTIMEOUT_H_
#define STAPPLER_SRC_ACTIONS_SPTIMEOUT_H_

#include "SPDefine.h"
#include "2d/CCActionInterval.h"

NS_SP_BEGIN

class Timeout : public cocos2d::Sequence {
public:
	virtual bool init(float duration, const std::function<void()> &);
	virtual bool init(cocos2d::FiniteTimeAction *a, const std::function<void()> &);
};

NS_SP_END

#endif /* STAPPLER_SRC_ACTIONS_SPTIMEOUT_H_ */
