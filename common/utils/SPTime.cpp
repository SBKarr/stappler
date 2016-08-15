//
//  SPTime.cpp
//  stappler
//
//  Created by SBKarr on 7/20/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPCore.h"
#include "SPTime.h"

inline time_t _time() {
	return time(NULL);
}

NS_SP_BEGIN

TimeInterval TimeInterval::between(const Time &v1, const Time &v2) {
	if (v1 > v2) {
		return TimeInterval(v1._value - v2._value);
	} else {
		return TimeInterval(v2._value - v1._value);
	}
}

uint64_t TimeInterval::toMicroseconds() const {
	return _value;
}
uint64_t TimeInterval::toMilliseconds() const {
	return _value / 1000ULL;
}
uint64_t TimeInterval::toSeconds() const {
	return _value / 1000000ULL;
}
float TimeInterval::toFloatSeconds() const {
	return _value / 1000000.0f;
}

void TimeInterval::setMicroseconds(uint64_t v) {
	_value = v;
}
void TimeInterval::setMilliseconds(uint64_t v) {
	_value = v * 1000ULL;
}
void TimeInterval::setSeconds(time_t v) {
	_value = v * 1000000ULL;
}

void TimeInterval::clear() {
	_value = 0;
}

TimeInterval::TimeInterval(nullptr_t) {
	_value = 0;
}
TimeInterval & TimeInterval::operator= (nullptr_t) {
	_value = 0;
	return *this;
}


Time Time::now() {
#if (SPAPR)
	return Time(apr_time_now());
#else
	struct timeval t0;
#if (_WINDOWS)
	cocos2d::gettimeofday(&t0, NULL);
#else
	gettimeofday(&t0, NULL);
#endif
	return Time((t0.tv_sec * 1000000LL + t0.tv_usec));
#endif
}

Time Time::microseconds(uint64_t mksec) {
	return Time(mksec);
}
Time Time::milliseconds(uint64_t msec) {
	return Time(msec * 1000ULL);
}
Time Time::seconds(time_t sec) {
	return Time(sec * 1000000ULL);
}

uint64_t Time::toMicroseconds() const {
	return _value;
}
uint64_t Time::toMilliseconds() const {
	return _value / 1000ULL;
}
uint64_t Time::toSeconds() const {
	return _value / 1000000ULL;
}

void Time::setMicroseconds(uint64_t v) {
	_value = v;
}
void Time::setMilliseconds(uint64_t v) {
	_value = v * 1000ULL;
}
void Time::setSeconds(time_t v) {
	_value = v * 1000000ULL;
}

void Time::clear() {
	_value = 0;
}

Time::Time(nullptr_t) {
	_value = 0;
}
Time & Time::operator= (nullptr_t) {
	_value = 0;
	return *this;
}

Time::Time(uint64_t v) {
	_value = v;
}

NS_SP_END
