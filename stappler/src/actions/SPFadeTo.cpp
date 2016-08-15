//
//  SPFadeTo.cpp
//  chieftime-federal
//
//  Created by SBKarr on 11.07.13.
//
//

#include "SPDefine.h"
#include "SPFadeTo.h"

NS_SP_BEGIN

bool FadeIn::init(float duration) {
	return FadeTo::initWithDuration(duration, 255);
}
    
void FadeIn::startWithTarget(cocos2d::Node *pTarget) {
	cocos2d::FadeTo::startWithTarget(pTarget);
    if (pTarget) {
        _duration = (1.0f - ((float)pTarget->getOpacity() / 255.0f)) * _duration;
    }
}

bool FadeOut::init(float duration) {
    return FadeTo::initWithDuration(duration, 0);
}

void FadeOut::startWithTarget(cocos2d::Node *pTarget) {
    FadeTo::startWithTarget(pTarget);
    if (pTarget) {
        _duration = ((float)pTarget->getOpacity() / 255.0f) * _duration;
    }
}

NS_SP_END
