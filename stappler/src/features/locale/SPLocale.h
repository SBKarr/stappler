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

#ifndef STAPPLER_SRC_FEATURES_LOCALE_SPLOCALE_H_
#define STAPPLER_SRC_FEATURES_LOCALE_SPLOCALE_H_

#include "SPEventHeader.h"

NS_SP_EXT_BEGIN(locale)

using LocaleInitList = std::initializer_list<Pair<StringView, StringView>>;
using LocaleIndexList = std::initializer_list<Pair<size_t, StringView>>;
using NumRule = Function<uint8_t(int64_t)>;

enum class TimeTokens {
	Today = 0,
	Yesterday,
	Jan,
	Feb,
	Mar,
	Apr,
	Nay,
	Jun,
	jul,
	Aug,
	Sep,
	Oct,
	Nov,
	Dec,
	Max
};

extern EventHeader onLocale;

struct Initializer {
	Initializer(const String &locale, LocaleInitList &&);
};

void define(const StringView &locale, LocaleInitList &&);
void define(const StringView &locale, LocaleIndexList &&);
void define(const StringView &locale, const std::array<StringView, toInt(TimeTokens::Max)> &);

void setDefault(const String &);
const String &getDefault();

void setLocale(const String &);
const String &getLocale();

void setNumRule(const String &, const NumRule &);

WideStringView string(const WideStringView &);
WideStringView string(size_t);
WideStringView numeric(const WideStringView &, size_t);

bool hasLocaleTagsFast(const WideStringView &);
bool hasLocaleTags(const WideStringView &);
WideString resolveLocaleTags(const WideStringView &);

String language(const String &locale);

// convert locale name to common form ('en-us', 'ru-ru', 'fr-fr')
String common(const String &locale);

StringView timeToken(TimeTokens);

String localDate(Time);
String localDate(const std::array<StringView, toInt(TimeTokens::Max)> &, Time);

NS_SP_EXT_END(locale)

#endif /* STAPPLER_SRC_FEATURES_LOCALE_SPLOCALE_H_ */
