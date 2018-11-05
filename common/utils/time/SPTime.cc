// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

uint64_t TimeStorage::toMicroseconds() const {
	return _value;
}
uint64_t TimeStorage::toMilliseconds() const {
	return _value / 1000ULL;
}
uint64_t TimeStorage::toSeconds() const {
	return _value / 1000000ULL;
}
float TimeStorage::toFloatSeconds() const {
	return _value / 1000000.0f;
}

struct tm TimeStorage::asLocal() const {
	auto sec = time_t(toSeconds());
	struct tm tm;
	localtime_r(&sec, &tm);
	return tm;
}

struct tm TimeStorage::asGmt() const {
	auto sec = time_t(toSeconds());
	struct tm tm;
	gmtime_r(&sec, &tm);
	return tm;
}

void TimeStorage::setMicroseconds(uint64_t v) {
	_value = v;
}
void TimeStorage::setMilliseconds(uint64_t v) {
	_value = v * 1000ULL;
}
void TimeStorage::setSeconds(time_t v) {
	_value = v * 1000000ULL;
}

void TimeStorage::clear() {
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
	return Time(t0.tv_sec * 1000000LL + t0.tv_usec);
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
Time Time::floatSeconds(float sec) {
	return Time(uint64_t(sec * 1000000.0f));
}

Time::Time(nullptr_t) : TimeStorage(0) { }
Time & Time::operator= (nullptr_t) {
	_value = 0;
	return *this;
}

NS_SP_END
