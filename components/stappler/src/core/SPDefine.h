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

#ifndef stappler_core_SPDefine_h
#define stappler_core_SPDefine_h

#include "SPDefault.h"
#include "SPConfig.h"
#include "SPMetastring.h"

#if (SP_NOSTOREKIT)
#define SP_INTERNAL_STOREKIT_ENABLED 0
#elif (SP_STOREKIT)
#define SP_INTERNAL_STOREKIT_ENABLED 1
#else
#define SP_INTERNAL_STOREKIT_ENABLED SP_CONFIG_STOREKIT
#endif

NS_SP_BEGIN

// full localized string
template <typename CharType, CharType ... Chars> auto operator "" _locale() {
	return metastring::merge("@Locale:"_meta, metastring::metastring<Chars ...>());
}

/*inline String operator"" _locale ( const char* str, std::size_t len) {
	String ret;
	ret.reserve(len + "@Locale:"_len);
	ret.append("@Locale:");
	ret.append(str, len);
	return ret;
}*/

// localized token
inline String operator"" _token ( const char* str, std::size_t len) {
	String ret;
	ret.reserve(len + 2);
	ret.append("%");
	ret.append(str, len);
	ret.append("%");
	return ret;
}

inline String localeIndex(size_t idx) {
	String ret; ret.reserve(20);
	ret.append("%=");
	ret.append(toString(idx));
	ret.push_back('%');
	return ret;
}

template <size_t Index>
inline constexpr auto localeIndex() {
	return metastring::merge("%="_meta, metastring::numeric<Index>(), "%"_meta);
}

NS_SP_END

#endif
