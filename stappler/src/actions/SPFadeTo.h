//
//  SPFadeTo.h
//  chieftime-federal
//
//  Created by SBKarr on 11.07.13.
//
//

#ifndef __chieftime_federal__SPFadeTo__
#define __chieftime_federal__SPFadeTo__

#include "SPDefine.h"
#include "2d/CCActionInterval.h"

NS_SP_BEGIN

class FadeIn : public cocos2d::FadeTo {
public:
    virtual bool init(float duration);
    virtual void startWithTarget(cocos2d::Node *pTarget) override;
};

class FadeOut : public cocos2d::FadeTo {
public:
    virtual bool init(float duration);
    virtual void startWithTarget(cocos2d::Node *pTarget) override;
};

NS_SP_END

#endif /* defined(__chieftime_federal__SPFadeTo__) */
