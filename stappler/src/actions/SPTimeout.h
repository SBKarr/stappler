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

#ifndef STAPPLER_SRC_ACTIONS_SPTIMEOUT_H_
#define STAPPLER_SRC_ACTIONS_SPTIMEOUT_H_

#include "SPDefine.h"
#include "2d/CCActionInterval.h"

NS_SP_BEGIN

class Timeout : public cocos2d::Sequence {
public:
	virtual bool init(float duration, const std::function<void()> &);
	virtual bool init(float duration, cocos2d::FiniteTimeAction *a);
	virtual bool init(cocos2d::FiniteTimeAction *a, const std::function<void()> &);
};

NS_SP_END

#endif /* STAPPLER_SRC_ACTIONS_SPTIMEOUT_H_ */
