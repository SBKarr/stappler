/*
 * SPLocaleManager.cpp
 *
 *  Created on: 16 июн. 2016 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPLocale.h"

NS_SP_EXT_BEGIN(locale)

std::string _default;
std::string _locale;

LocaleMap _strings;
std::unordered_map<std::string, NumRule> _numRules;

EventHeader onLocale("Locale", "onLocale");

void define(const std::string &locale, LocaleInitList &&init) {
	auto it = _strings.find(locale);
	if (it != _strings.end()) {
		for (auto &iit : init) {
			it->second.emplace(iit.first, iit.second);
		}
	} else {
		auto it = _strings.emplace(locale, StringMap()).first;
		for (auto &iit : init) {
			it->second.emplace(iit.first, iit.second);
		}
	}
}

std::string string(const std::string &str) {
	auto it = _strings.find(_locale);
	if (it == _strings.end()) {
		it = _strings.find(_default);
	}
	if (it == _strings.end()) {
		it = _strings.begin();
	}

	if (it != _strings.end()) {
		auto sit = it->second.find(str);
		if (sit != it->second.end()) {
			return sit->second;
		}
	}

	return std::string();
}

std::string numeric(const std::string &str, int64_t num) {
	auto ruleIt = _numRules.find(_locale);
	if (ruleIt == _numRules.end()) {
		auto fmt = string(str);
		CharReaderBase r(fmt);
		return r.readUntil<CharReaderBase::Chars<':'>>().str();
	} else {
		uint8_t numEq = ruleIt->second(num);
		auto fmt = string(str);
		CharReaderBase r(fmt);
		CharReaderBase def = r.readUntil<CharReaderBase::Chars<':'>>();
		if (r.empty() || numEq == 0) {
			return def.str();
		}

		while (!r.empty()) {
			if (r.is(':')) {
				++ r;
			}

			CharReaderBase res = r.readUntil<CharReaderBase::Chars<':'>>();
			if (numEq == 1) {
				return res.str();
			} else {
				-- numEq;
			}
		}

		return def.str();
	}
}

void setDefault(const std::string &def) {
	_default = def;
}
const std::string &getDefault() {
	return _default;
}

void setLocale(const std::string &loc) {
	if (_locale != loc) {
		_locale = loc;
		onLocale(nullptr, loc);
	}
}
const std::string &getLocale() {
	return _locale;
}

void setNumRule(const std::string &locale, const NumRule &rule) {
	_numRules.emplace(locale, rule);
}

bool hasLocaleTags(const char16_t *str, size_t len) {
	if (len == 0) {
		return false;
	}

	CharReaderUcs2 r(str, len);
	if (r.is(u"@Locale:")) { // raw locale string
		return true;
	} else {
		r.skipUntil<CharReaderUcs2::Chars<'%'>>();
		if (r.is('%')) {
			++ r;
			r.skipChars<
				CharReaderUcs2::CharGroup<chars::CharGroupId::Alphanumeric>,
				CharReaderUcs2::Chars<':', '.', '-', '_', '[', ']', '+'>>();
			if (r.is('%')) {
				return true;
			}
		}
	}

	return false;
}

std::u16string resolveLocaleTags(const char16_t *str, size_t len) {
	CharReaderUcs2 r(str, len);
	if (r.is(u"@Locale:")) { // raw locale string
		auto name = string::toUtf8(std::u16string(str + "@Locale:"_len, len - "@Locale:"_len));
		return string::toUtf16(locale::string(name));
	} else {
		std::u16string ret; ret.reserve(len);
		while (!r.empty()) {
			auto tmp = r.readUntil<CharReaderUcs2::Chars<'%'>>();
			ret.append(tmp.data(), tmp.size());
			if (r.is('%')) {
				++ r;
				auto token = r.readChars<
					CharReaderUcs2::CharGroup<chars::CharGroupId::Alphanumeric>,
					CharReaderUcs2::Chars<':', '.', '-', '_', '[', ']', '+'>>();
				if (r.is('%')) {
					++ r;

					std::u16string replacement;
					if (token.is(u"Num:")) {
						CharReaderUcs2 splitMaster(token);
						CharReaderUcs2 num;
						while (!splitMaster.empty()) {
							num = splitMaster.readUntil<CharReaderUcs2::Chars<':'>>();
							if (splitMaster.is(':')) {
								++ splitMaster;
							}
						}

						if (!num.empty()) {
							CharReaderUcs2 validate(num);
							if (validate.is('-')) {
								++ validate;
							}
							validate.skipChars<CharReaderUcs2::CharGroup<chars::CharGroupId::Numbers>>();
							if (validate.empty()) {
								CharReaderUcs2 vtoken(token.data(), token.size() - num.size() - 1);
								replacement = string::toUtf16(locale::numeric(string::toUtf8(vtoken.str()), num.readInteger()));
							}
						}
					}

					if (replacement.empty()) {
						replacement = string::toUtf16(locale::string(string::toUtf8(token.str())));
					}

					if (replacement.empty()) {
						ret.push_back(u'%');
						ret.append(token.data(), token.size());
						ret.push_back(u'%');
					} else {
						ret.append(replacement);
					}
				} else {
					ret.push_back(u'%');
					ret.append(token.data(), token.size());
				}
			}
		}
		return ret;
	}
	return std::u16string();
}

std::string language(const std::string &locale) {
	if (locale == "ru-ru") {
		return "Русский";
	} else if (locale.compare(0, 3, "en-") == 0) {
		return "English";
	}
	return std::string();
}

std::string common(const std::string &locale) {
	std::string ret = string::tolower(locale);
	if (ret.size() == 2) {
		if (ret == "en") {
			ret.reserve(5);
			ret.append("-gb");
		} else {
			ret.reserve(5);
			ret.append("-").append(string::tolower(locale));
		}
	}

	return ret;
}

NS_SP_EXT_END(locale)
