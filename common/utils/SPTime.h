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

#ifndef __stappler__SPTime__
#define __stappler__SPTime__

#include "SPCore.h"

NS_SP_BEGIN

class Time;

class TimeInterval {
public:
	static TimeInterval between(const Time &, const Time &);

	constexpr static TimeInterval microseconds(uint64_t mksec) {
		return TimeInterval(mksec);
	}
	constexpr static TimeInterval milliseconds(uint64_t msec) {
		return TimeInterval(msec * 1000ULL);
	}
	constexpr static TimeInterval seconds(time_t sec) {
		return TimeInterval(sec * 1000000ULL);
	}

	uint64_t toMicroseconds() const;
	uint64_t toMilliseconds() const;
	uint64_t toSeconds() const;
	float toFloatSeconds() const;

	void setMicroseconds(uint64_t);
	void setMilliseconds(uint64_t);
	void setSeconds(time_t);

	void clear();

	inline operator bool() const;

    inline const Time operator+(const Time& v) const;

    inline const TimeInterval operator+(const TimeInterval& v) const;
    inline TimeInterval& operator+=(const TimeInterval& v);

    inline const TimeInterval operator-(const TimeInterval& v) const;
    inline TimeInterval& operator-=(const TimeInterval& v);

    inline const TimeInterval operator*(float s) const;
    inline TimeInterval& operator*=(float s);

    inline const TimeInterval operator/(float s) const;
    inline TimeInterval& operator/=(float s);

    inline bool operator<(const TimeInterval& v) const;
    inline bool operator>(const TimeInterval& v) const;
    inline bool operator<=(const TimeInterval& v) const;
    inline bool operator>=(const TimeInterval& v) const;
    inline bool operator==(const TimeInterval& v) const;
    inline bool operator!=(const TimeInterval& v) const;

    TimeInterval(nullptr_t);
    TimeInterval & operator= (nullptr_t);

    TimeInterval() = default;
    TimeInterval(const TimeInterval &) = default;
    TimeInterval(TimeInterval &&) = default;
    TimeInterval & operator= (const TimeInterval &) = default;
    TimeInterval & operator= (TimeInterval &&) = default;

protected:
    friend class Time;

    constexpr TimeInterval(uint64_t v) {
    	_value = v;
    }

	uint64_t _value = 0;
};

class Time {
public:
	static Time now();

	static Time microseconds(uint64_t mksec);
	static Time milliseconds(uint64_t msec);
	static Time seconds(time_t sec);

	uint64_t toMicroseconds() const;
	uint64_t toMilliseconds() const;
	uint64_t toSeconds() const;

	void setMicroseconds(uint64_t);
	void setMilliseconds(uint64_t);
	void setSeconds(time_t);

	void clear();

	inline operator bool() const;

    inline const Time operator+(const TimeInterval& v) const;
    inline Time& operator+=(const TimeInterval& v);

    inline const TimeInterval operator-(const Time& v) const;
    inline const Time operator-(const TimeInterval& v) const;
    inline Time& operator-=(const TimeInterval& v);

    inline bool operator<(const Time& v) const;
    inline bool operator>(const Time& v) const;
    inline bool operator<=(const Time& v) const;
    inline bool operator>=(const Time& v) const;
    inline bool operator==(const Time& v) const;
    inline bool operator!=(const Time& v) const;

    Time(nullptr_t);
    Time & operator= (nullptr_t);

    Time() = default;
    Time(const Time &) = default;
    Time(Time &&) = default;
    Time & operator= (const Time &) = default;
    Time & operator= (Time &&) = default;

protected:
    friend class TimeInterval;

	explicit Time(uint64_t);

	uint64_t _value = 0;
};

constexpr TimeInterval operator"" _sec ( unsigned long long int val ) { return TimeInterval::seconds((time_t)val); }
constexpr TimeInterval operator"" _msec ( unsigned long long int val ) { return TimeInterval::milliseconds(val); }
constexpr TimeInterval operator"" _mksec ( unsigned long long int val ) { return TimeInterval::microseconds(val); }

inline TimeInterval::operator bool() const {
	return _value != 0;
}

inline const Time TimeInterval::operator+(const Time& v) const {
	return v + *this;
}

inline const TimeInterval TimeInterval::operator+(const TimeInterval& v) const {
	return TimeInterval(_value + v._value);
}
inline TimeInterval& TimeInterval::operator+=(const TimeInterval& v) {
	_value += v._value;
	return *this;
}

inline const TimeInterval TimeInterval::operator-(const TimeInterval& v) const {
	if (_value < v._value) {
		return TimeInterval();
	} else {
		return TimeInterval(_value - v._value);
	}
}
inline TimeInterval& TimeInterval::operator-=(const TimeInterval& v) {
	if (_value < v._value) {
		_value = 0;
	} else {
		_value -= v._value;
	}
	return *this;
}

inline const TimeInterval TimeInterval::operator*(float s) const {
	return TimeInterval(_value * fabsf(s));
}
inline TimeInterval& TimeInterval::operator*=(float s) {
	_value *= fabsf(s);
	return *this;
}

inline const TimeInterval TimeInterval::operator/(float s) const {
	return TimeInterval(_value / fabsf(s));
}
inline TimeInterval& TimeInterval::operator/=(float s) {
	_value /= fabsf(s);
	return *this;
}

inline bool TimeInterval::operator<(const TimeInterval& v) const { return _value < v._value; }
inline bool TimeInterval::operator>(const TimeInterval& v) const { return _value > v._value; }
inline bool TimeInterval::operator<=(const TimeInterval& v) const { return _value <= v._value; }
inline bool TimeInterval::operator>=(const TimeInterval& v) const { return _value >= v._value; }
inline bool TimeInterval::operator==(const TimeInterval& v) const { return _value == v._value; }
inline bool TimeInterval::operator!=(const TimeInterval& v) const { return _value != v._value; }


inline Time::operator bool() const {
	return _value != 0;
}

inline const Time Time::operator+(const TimeInterval& v) const {
	return Time(_value + v._value);
}
inline Time& Time::operator+=(const TimeInterval& v) {
	_value += v._value;
	return *this;
}

inline const TimeInterval Time::operator-(const Time& v) const {
	return TimeInterval::between(*this, v);
}
inline const Time Time::operator-(const TimeInterval& v) const {
	if (_value < v._value) {
		return Time();
	}
	return Time(_value - v._value);
}
inline Time& Time::operator-=(const TimeInterval& v) {
	if (_value < v._value) {
		_value = 0;
	}
	_value -= v._value;
	return *this;
}

inline bool Time::operator<(const Time& v) const { return _value < v._value; }
inline bool Time::operator>(const Time& v) const { return _value > v._value; }
inline bool Time::operator<=(const Time& v) const { return _value <= v._value; }
inline bool Time::operator>=(const Time& v) const { return _value >= v._value; }
inline bool Time::operator==(const Time& v) const { return _value == v._value; }
inline bool Time::operator!=(const Time& v) const { return _value != v._value; }

NS_SP_END

#endif /* defined(__stappler__SPTime__) */
