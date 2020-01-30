/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_UTILS_SPTIME_H_
#define COMMON_UTILS_SPTIME_H_

#include "SPStringView.h"

#define SP_COMPILE_TIME (Time::fromCompileTime(__DATE__, __TIME__))

NS_SP_BEGIN

class Time;

class TimeStorage {
public:
	uint64_t toMicroseconds() const;
	uint64_t toMilliseconds() const;
	uint64_t toSeconds() const;
	float toFloatSeconds() const;

	uint64_t toMicros() const { return toMicroseconds(); }
	uint64_t toMillis() const { return toMilliseconds(); }

	uint64_t mksec() const { return toMicroseconds(); }
	uint64_t msec() const { return toMilliseconds(); }
	uint64_t sec() const { return toSeconds(); }
	float fsec() const { return toFloatSeconds(); }

	struct tm asLocal() const;
	struct tm asGmt() const;

	void setMicros(uint64_t value) { setMicroseconds(value); }
	void setMillis(uint64_t value) { setMilliseconds(value); }

	void setMicroseconds(uint64_t);
	void setMilliseconds(uint64_t);
	void setSeconds(time_t);

	void clear();

	operator bool() const { return _value != 0; }

	TimeStorage() = default;
	TimeStorage(const TimeStorage &) = default;
	TimeStorage(TimeStorage &&) = default;
	TimeStorage & operator= (const TimeStorage &) = default;
	TimeStorage & operator= (TimeStorage &&) = default;

protected:
    constexpr TimeStorage(uint64_t v) : _value(v) { }

	uint64_t _value = 0;
};

class TimeInterval : public TimeStorage {
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
	constexpr static TimeInterval floatSeconds(float sec) {
		return TimeInterval(uint64_t(sec * 1000000.0f));
	}

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

	constexpr TimeInterval() : TimeStorage(0) { }
	constexpr TimeInterval(const TimeInterval &other) : TimeStorage(other._value) { }
	constexpr TimeInterval(TimeInterval &&other) : TimeStorage(other._value) { }
	constexpr TimeInterval & operator= (const TimeInterval &other) { _value = other._value; return *this; }
	constexpr TimeInterval & operator= (TimeInterval &&other) { _value = other._value; return *this; }

protected:
    friend class Time;

    using TimeStorage::TimeStorage;
};

class Time : public TimeStorage {
public:
	static Time now();

	static Time fromCompileTime(const char *, const char *);

	/*
	 * Parses an HTTP date in one of three standard forms:
	 *
	 *     Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
	 *     Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
	 *     Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format
	 *     2011-04-28T06:34:00+09:00      ; Atom time format
	 */
	static Time fromHttp(const StringView &);

	/*
	 * Parses a string resembling an RFC 822 date.  This is meant to be
	 * leinent in its parsing of dates.  Hence, this will parse a wider
	 * range of dates than Time::fromHttp.
	 *
	 *     Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
	 *     Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
	 *     Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format
	 *     Sun, 6 Nov 1994 08:49:37 GMT   ; RFC 822, updated by RFC 1123
	 *     Sun, 06 Nov 94 08:49:37 GMT    ; RFC 822
	 *     Sun, 6 Nov 94 08:49:37 GMT     ; RFC 822
	 *     Sun, 06 Nov 94 08:49 GMT       ; Unknown [drtr@ast.cam.ac.uk]
	 *     Sun, 6 Nov 94 08:49 GMT        ; Unknown [drtr@ast.cam.ac.uk]
	 *     Sun, 06 Nov 94 8:49:37 GMT     ; Unknown [Elm 70.85]
	 *     Sun, 6 Nov 94 8:49:37 GMT      ; Unknown [Elm 70.85]
	 *     Mon,  7 Jan 2002 07:21:22 GMT  ; Unknown [Postfix]
	 *     Sun, 06-Nov-1994 08:49:37 GMT  ; RFC 850 with four digit years
	 */
	static Time fromRfc(const StringView &);

	static Time microseconds(uint64_t mksec);
	static Time milliseconds(uint64_t msec);
	static Time seconds(time_t sec);
	static Time floatSeconds(float sec);

	template <typename Interface = memory::DefaultInterface>
	auto toHttp() -> typename Interface::StringType {
		return toRfc822<Interface>();
	}

	template <typename Interface = memory::DefaultInterface>
	auto toRfc822() -> typename Interface::StringType {
		using StringType = typename Interface::StringType;
		char buf[30] = { 0 };
		encodeRfc822(buf);
		return StringType(buf, 29);
	}

	template <typename Interface = memory::DefaultInterface>
	auto toCTime() -> typename Interface::StringType {
		using StringType = typename Interface::StringType;
		char buf[25] = { 0 };
		encodeCTime(buf);
		return StringType(buf, 24);
	}

	// XML dateTime format
	template <typename Interface = memory::DefaultInterface>
	auto toIso8601() -> typename Interface::StringType {
		using StringType = typename Interface::StringType;
		char buf[20] = { 0 };
		encodeIso8601(buf);
		return StringType(buf);
	}

	template <typename Interface = memory::DefaultInterface>
	auto toFormat(const char *fmt) -> typename Interface::StringType {
		using StringType = typename Interface::StringType;
		char buf[1_KiB] = { 0 }; // should be enough
		return StringType(buf, encodeToFormat(buf, 1_KiB, fmt));
	}

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

	Time() : TimeStorage(0) { }
	Time(const Time &other) : TimeStorage(other._value) { }
	Time(Time &&other) : TimeStorage(other._value) { }
	Time & operator= (const Time &other) { _value = other._value; return *this; }
	Time & operator= (Time &&other) { _value = other._value; return *this; }

protected:
    friend class TimeInterval;

    using TimeStorage::TimeStorage;

	void encodeRfc822(char *);
	void encodeCTime(char *);
	void encodeIso8601(char *);

	size_t encodeToFormat(char *, size_t, const char *fmt);
};

struct sp_time_exp_t {
	/** microseconds past tm_sec */
	int32_t tm_usec;
	/** (0-61) seconds past tm_min */
	int32_t tm_sec;
	/** (0-59) minutes past tm_hour */
	int32_t tm_min;
	/** (0-23) hours past midnight */
	int32_t tm_hour;
	/** (1-31) day of the month */
	int32_t tm_mday;
	/** (0-11) month of the year */
	int32_t tm_mon;
	/** year since 1900 */
	int32_t tm_year;
	/** (0-6) days since Sunday */
	int32_t tm_wday;
	/** (0-365) days since January 1 */
	int32_t tm_yday;
	/** daylight saving time */
	int32_t tm_isdst;
	/** seconds east of UTC */
	int32_t tm_gmtoff;

	sp_time_exp_t();
	sp_time_exp_t(Time t, int32_t offset, bool use_localtime);
	sp_time_exp_t(Time t, int32_t offs);
	sp_time_exp_t(Time t);
	sp_time_exp_t(Time t, bool use_localtime);

	Time get() const;
	Time gmt_get() const;
	Time ltz_get() const;
};

constexpr TimeInterval operator"" _sec ( unsigned long long int val ) { return TimeInterval::seconds((time_t)val); }
constexpr TimeInterval operator"" _msec ( unsigned long long int val ) { return TimeInterval::milliseconds(val); }
constexpr TimeInterval operator"" _mksec ( unsigned long long int val ) { return TimeInterval::microseconds(val); }


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

#endif /* COMMON_UTILS_SPTIME_H_ */
