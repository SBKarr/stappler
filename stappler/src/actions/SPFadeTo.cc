// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPFadeTo.h"
#include "2d/CCNode.h"

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
