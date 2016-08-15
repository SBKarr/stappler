/*
 * SPLocaleManager.h
 *
 *  Created on: 16 июн. 2016 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_FEATURES_LOCALE_SPLOCALE_H_
#define STAPPLER_SRC_FEATURES_LOCALE_SPLOCALE_H_

#include "SPEventHeader.h"

NS_SP_EXT_BEGIN(locale)

using LocaleInitList = std::initializer_list<std::pair<std::string, std::string>>;
using StringMap = std::unordered_map<std::string, std::string>;
using LocaleMap = std::unordered_map<std::string, StringMap>;
using NumRule = std::function<uint8_t(int64_t)>;

extern EventHeader onLocale;

void define(const std::string &locale, LocaleInitList &&);
std::string string(const std::string &);
std::string numeric(const std::string &, size_t);

void setDefault(const std::string &);
const std::string &getDefault();

void setLocale(const std::string &);
const std::string &getLocale();

void setNumRule(const std::string &, const NumRule &);

bool hasLocaleTags(const char16_t *, size_t);
std::u16string resolveLocaleTags(const char16_t *, size_t);

std::string language(const std::string &locale);

// convert locale name to common form ('en-us', 'ru-ru', 'fr-fr')
std::string common(const std::string &locale);

NS_SP_EXT_END(locale)

#endif /* STAPPLER_SRC_FEATURES_LOCALE_SPLOCALE_H_ */
