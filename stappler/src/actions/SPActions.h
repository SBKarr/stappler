
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
