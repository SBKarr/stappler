/*
 * SPToString.h
 *
 *  Created on: 20 июля 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_CORE_STRING_SPTOSTRING_H_
#define LIBS_STAPPLER_CORE_STRING_SPTOSTRING_H_

#include "SPCommon.h"

NS_SP_BEGIN

/*
 * toString functionality, based on StringStream
 * usage: string::toString(value) - single value, to_string (if available) or StringStream
 * usage: string::toString(arg1, arg2, ...) - variable length, StringStream on all platform (stream modifiers allowed)
 */

inline float stof(const String &str, size_t *idx = nullptr) {
	const char *src = str.c_str();
	char *ptr = nullptr;
	auto ret = strtod(str.c_str(), &ptr);
	if (ptr && idx) {
		*idx = (ptr - src);
	}
	return (float)ret;
}

#if (ANDROID || SPAPR)

template <class T>
inline String toString(T value) {
	StringStream stream;
	stream << value;
    return stream.str();
}

#else

template <class T> inline String toString(T value) { return std::to_string(value); }

#endif

inline String toString(const String &value) { return value; }
inline String toString(const char *value) { return value; }

template <class T>
inline void toStringStream(StringStream &stream, T value) {
	stream << value;
}

template <class T, typename... Args>
inline void toStringStream(StringStream &stream, T value, Args && ... args) {
	stream << value;
	toStringStream(stream, std::forward<Args>(args)...);
}

template<typename T, typename... Args>
inline String toString(T t, Args && ... args) {
	StringStream stream;
	toStringStream(stream, t);
	toStringStream(stream, std::forward<Args>(args)...);
    return stream.str();
}

NS_SP_END

#endif /* LIBS_STAPPLER_CORE_STRING_SPTOSTRING_H_ */
