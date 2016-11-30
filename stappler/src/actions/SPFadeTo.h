/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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
