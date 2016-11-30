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
#include "SPTimeout.h"

NS_SP_EXT_BEGIN(action)

template <class F>
inline cocos2d::FiniteTimeAction *sequence(F *f) {
    static_assert(std::is_convertible<F *, cocos2d::FiniteTimeAction *>::value, "Invalid Type for sequence<VA>!");

	return f;
}

template <class F, class S, class... Args>
inline cocos2d::FiniteTimeAction *sequence(F *f, S *s, Args&&... args) {
    static_assert(std::is_convertible<F *, cocos2d::FiniteTimeAction *>::value, "Invalid Type for sequence<VA>!");
    static_assert(std::is_convertible<S *, cocos2d::FiniteTimeAction *>::value, "Invalid Type for sequence<VA>!");

	auto seq = cocos2d::Sequence::createWithTwoActions(f, s);

	return sequence(seq, args...);
}

template <class F>
inline cocos2d::FiniteTimeAction *spawn(F *f) {
    static_assert(std::is_convertible<F *, cocos2d::FiniteTimeAction *>::value, "Invalid Type for spawn<VA>!");

	return f;
}

template <class F, class S, class... Args>
inline cocos2d::FiniteTimeAction *spawn(F *f, S *s, Args&&... args) {
    static_assert(std::is_convertible<F *, cocos2d::FiniteTimeAction *>::value, "Invalid Type for spawn<VA>!");
    static_assert(std::is_convertible<S *, cocos2d::FiniteTimeAction *>::value, "Invalid Type for spawn<VA>!");

	auto seq = cocos2d::Spawn::createWithTwoActions(f, s);

	return spawn(seq, args...);
}

template <class... Args>
inline cocos2d::FiniteTimeAction *callback(float duration, Args&&... args) {
	return construct<Timeout>(duration, args...);
}

template <class... Args>
inline cocos2d::FiniteTimeAction *callback(cocos2d::FiniteTimeAction *a, Args&&... args) {
	return construct<Timeout>(a, args...);
}

NS_SP_EXT_END(action)

#endif /* STAPPLER_SRC_ACTIONS_SPACTIONS_H_ */
