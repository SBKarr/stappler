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

#ifndef STAPPLER_SRC_ACTIONS_SPACTIONS_H_
#define STAPPLER_SRC_ACTIONS_SPACTIONS_H_

#include "2d/CCActionInstant.h"
#include "2d/CCActionInterval.h"
#include "2d/CCActionEase.h"

#include "SPAccelerated.h"
#include "SPProgressAction.h"
#include "SPFadeTo.h"

NS_SP_EXT_BEGIN(action)

using CallbackFunction = Function<void()>;

inline cocos2d::FiniteTimeAction *_sequencable(cocos2d::FiniteTimeAction *a) {
	return a;
}

template <class F>
inline cocos2d::FiniteTimeAction *_sequencable(const Rc<F> &f) {
	return _sequencable(f.get());
}

inline cocos2d::FiniteTimeAction *_sequencable(float d) {
	return cocos2d::DelayTime::create(d);
}

inline cocos2d::FiniteTimeAction *_sequencable(const CallbackFunction &cb) {
	return cocos2d::CallFunc::create(cb);
}

template <class F>
inline cocos2d::FiniteTimeAction *sequence(const F &f) {
	return _sequencable(f);
}

template <class F, class S, class... Args>
inline cocos2d::FiniteTimeAction *sequence(const F &f, const S &s, Args&&... args) {
	return sequence(cocos2d::Sequence::createWithTwoActions(_sequencable(f), _sequencable(s)), args...);
}

template <class F>
inline cocos2d::FiniteTimeAction *spawn(const F &f) {
	return _sequencable(f);
}

template <class F, class S, class... Args>
inline cocos2d::FiniteTimeAction *spawn(const F &f, const S &s, Args&&... args) {
	return spawn(cocos2d::Spawn::createWithTwoActions(_sequencable(f), _sequencable(s)), args...);
}

NS_SP_EXT_END(action)

#endif /* STAPPLER_SRC_ACTIONS_SPACTIONS_H_ */
