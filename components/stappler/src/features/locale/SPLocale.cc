// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPLocale.h"

NS_SP_EXT_BEGIN(locale)

class LocaleManager {
public:
	using StringMap = memory::dict<memory::u16string, memory::u16string>;
	using LocaleMap = memory::map<memory::string, StringMap>;

	using StringIndexMap = memory::dict<size_t, memory::u16string>;
	using LocaleIndexMap = memory::map<memory::string, StringIndexMap>;

	using Interface = memory::PoolInterface;

	static LocaleManager *s_sharedInstance;

	static LocaleManager *getInstance() {
		if (!s_sharedInstance) {
			auto p = memory::pool::createTagged(nullptr, "LocaleManager");
			memory::pool::push(p);
			s_sharedInstance = new LocaleManager(p);
			memory::pool::pop();
		}
		return s_sharedInstance;
	}

	LocaleManager(memory::pool_t *p) : _defaultTime{
		"today",
		"yesterday",
		"jan", "feb", "mar", "apr", "may", "jun",
		"jul", "aug", "sep", "oct", "nov", "dec"
	}, _pool(p) { }

	void define(const StringView &locale, LocaleInitList &&init) {
		memory::pool::push(_pool);
		auto it = _strings.find(locale);
		if (it == _strings.end()) {
			it = _strings.emplace(locale.str<Interface>(), StringMap()).first;
		}
		for (auto &iit : init) {
			it->second.emplace(string::toUtf16<Interface>(iit.first), string::toUtf16<Interface>(iit.second));
		}
		memory::pool::pop();
	}

	void define(const StringView &locale, LocaleIndexList &&init) {
		memory::pool::push(_pool);
		auto it = _indexes.find(locale);
		if (it == _indexes.end()) {
			it = _indexes.emplace(locale.str<Interface>(), StringIndexMap()).first;
		}
		for (auto &iit : init) {
			it->second.emplace(iit.first, string::toUtf16<Interface>(iit.second));
		}
		memory::pool::pop();
	}

	void define(const StringView &locale, const std::array<StringView, toInt(TimeTokens::Max)> &arr) {
		memory::pool::push(_pool);
		auto it = _timeTokens.find(locale);
		if (it == _timeTokens.end()) {
			it = _timeTokens.emplace(locale.str<Interface>(), std::array<memory::string, toInt(TimeTokens::Max)>()).first;
		}

		size_t i = 0;
		for (auto &arr_it : arr) {
			it->second[i] = arr_it.str<Interface>();
			++ i;
		}
		memory::pool::pop();
	}

	WideStringView string(const WideStringView &str) {
		auto it = _strings.find(StringView(_locale));
		if (it == _strings.end()) {
			it = _strings.find(StringView(_default));
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

		return WideStringView();
	}

	WideStringView string(size_t index) {
		auto it = _indexes.find(StringView(_locale));
		if (it == _indexes.end()) {
			it = _indexes.find(StringView(_default));
		}
		if (it == _indexes.end()) {
			it = _indexes.begin();
		}

		if (it != _indexes.end()) {
			auto sit = it->second.find(index);
			if (sit != it->second.end()) {
				return sit->second;
			}
		}

		return WideStringView();
	}

	WideStringView numeric(const WideStringView &str, int64_t num) {
		auto ruleIt = _numRules.find(StringView(_locale));
		if (ruleIt == _numRules.end()) {
			return string(str);
		} else {
			uint8_t numEq = ruleIt->second(num);
			auto fmt = string(str);
			WideStringView r(fmt);
			WideStringView def = r.readUntil<WideStringView::Chars<':'>>();
			if (r.empty() || numEq == 0) {
				return def;
			}

			while (!r.empty()) {
				if (r.is(':')) {
					++ r;
				}

				WideStringView res = r.readUntil<WideStringView::Chars<':'>>();
				if (numEq == 1) {
					return res;
				} else {
					-- numEq;
				}
			}

			return def;
		}
	}

	void setDefault(const String &def) {
		_default = def;
	}
	const String &getDefault() {
		return _default;
	}

	void setLocale(const String &loc) {
		if (_locale != loc) {
			_locale = loc;
			onLocale(nullptr, loc);
		}
	}
	const String &getLocale() {
		return _locale;
	}

	void setNumRule(const StringView &locale, const NumRule &rule) {
		_numRules.emplace(locale.str<Interface>(), rule);
	}

	bool hasLocaleTagsFast(WideStringView r) {
		if (r.empty()) {
			return false;
		}

		if (r.is(u"@Locale:")) { // raw locale string
			return true;
		} else if (r.is(u"%=")) {
			r += 2;
			auto tmp = r.readChars<WideStringView::CharGroup<CharGroupId::Numbers>>();
			if (!tmp.empty() && r.is('%')) {
				return true;
			}
		} else {
			constexpr size_t maxChars = config::maxFastLocaleChars;
			WideStringView shortView(r.data(), std::min(r.size(), maxChars));
			shortView.skipUntil<WideStringView::Chars<'%'>>();
			if (shortView.is('%')) {
				++ shortView;
				if (shortView.is('=')) {
					++ shortView;
					shortView = WideStringView(shortView.data(), std::min(maxChars, r.size() - (shortView.data() - r.data())));
					shortView.skipChars<WideStringView::CharGroup<CharGroupId::Numbers>>();
					if (shortView.is('%')) {
						return true;
					}
				} else {
					shortView = WideStringView(shortView.data(), std::min(maxChars, r.size() - (shortView.data() - r.data())));
					shortView.skipChars<
						WideStringView::CharGroup<CharGroupId::Alphanumeric>,
						WideStringView::Chars<':', '.', '-', '_', '[', ']', '+', '='>>();
					if (shortView.is('%')) {
						return true;
					}
				}
			}
		}
		return false;
	}

	bool hasLocaleTags(WideStringView r) {
		if (r.empty()) {
			return false;
		}

		if (r.is(u"@Locale:")) { // raw locale string
			return true;
		} else if (r.is(u"%=")) {
			r += 2;
			auto tmp = r.readChars<WideStringView::CharGroup<CharGroupId::Numbers>>();
			if (!tmp.empty() && r.is('%')) {
				return true;
			}
		} else {
			while (!r.empty()) {
				r.skipUntil<WideStringView::Chars<'%'>>();
				if (r.is('%')) {
					++ r;
					if (r.is('=')) {
						++ r;
						r.skipChars<WideStringView::CharGroup<CharGroupId::Numbers>>();
						if (r.is('%')) {
							return true;
						}
					} else {
						r.skipChars<
							WideStringView::CharGroup<CharGroupId::Alphanumeric>,
							WideStringView::Chars<':', '.', '-', '_', '[', ']', '+', '='>>();
						if (r.is('%')) {
							return true;
						}
					}
				}
			}
		}

		return false;
	}

	WideString resolveLocaleTags(WideStringView r) {
		if (r.is(u"@Locale:")) { // raw locale string
			r += "@Locale:"_len;
			return string(r).str();
		} else {
			WideString ret; ret.reserve(r.size());
			while (!r.empty()) {
				auto tmp = r.readUntil<WideStringView::Chars<'%'>>();
				ret.append(tmp.data(), tmp.size());
				if (r.is('%')) {
					++ r;
					auto token = r.readChars<
						WideStringView::CharGroup<CharGroupId::Alphanumeric>,
						WideStringView::Chars<':', '.', '-', '_', '[', ']', '+', '='>>();
					if (r.is('%')) {
						++ r;
						WideStringView replacement;
						if (token.is('=')) {
							auto numToken = token;
							++ numToken;
							if (numToken.is<WideStringView::CharGroup<CharGroupId::Numbers>>()) {
								numToken.readInteger().unwrap([&] (int64_t id) {
									if (numToken.empty()) {
										replacement = string(size_t(id));
									}
								});
							}
						} else if (token.is(u"Num:")) {
							WideStringView splitMaster(token);
							WideStringView num;
							while (!splitMaster.empty()) {
								num = splitMaster.readUntil<WideStringView::Chars<':'>>();
								if (splitMaster.is(':')) {
									++ splitMaster;
								}
							}

							if (!num.empty()) {
								WideStringView validate(num);
								if (validate.is('-')) {
									++ validate;
								}
								validate.skipChars<WideStringView::CharGroup<CharGroupId::Numbers>>();
								if (validate.empty()) {
									WideStringView vtoken(token.data(), token.size() - num.size() - 1);
									num.readInteger().unwrap([&] (int64_t id) {
										replacement = numeric(vtoken, id);
									});
								}
							}
						}

						if (replacement.empty() && !token.is('=')) {
							replacement = string(token);
						}

						if (replacement.empty()) {
							ret.push_back(u'%');
							ret.append(token.data(), token.size());
							ret.push_back(u'%');
						} else {
							ret.append(replacement.str());
						}
					} else {
						ret.push_back(u'%');
						ret.append(token.data(), token.size());
					}
				}
			}
			return ret;
		}
		return WideString();
	}

	String language(const String &locale) {
		if (locale == "ru-ru") {
			return "Русский";
		} else if (locale.compare(0, 3, "en-") == 0) {
			return "English";
		}
		return String();
	}

	String common(const String &locale) {
		String ret = string::tolower(locale);
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

	StringView timeToken(TimeTokens tok) {
		auto it = _timeTokens.find(StringView(_locale));
		if (it == _timeTokens.end()) {
			it = _timeTokens.find(StringView(_default));
		}

		auto &table = it == _timeTokens.end()?_defaultTime:it->second;
		return table[toInt(tok)];
	}

	const std::array<memory::string, toInt(TimeTokens::Max)> &timeTokenTable() {
		auto it = _timeTokens.find(StringView(_locale));
		if (it == _timeTokens.end()) {
			it = _timeTokens.find(StringView(_default));
		}

		return it == _timeTokens.end()?_defaultTime:it->second;
	}

protected:
	String _default;
	String _locale;

	LocaleMap _strings;
	LocaleIndexMap _indexes;
	memory::map<memory::string, NumRule> _numRules;
	memory::map<memory::string, std::array<memory::string, toInt(TimeTokens::Max)>> _timeTokens;
	std::array<memory::string, toInt(TimeTokens::Max)> _defaultTime;

	memory::pool_t *_pool;
};

LocaleManager *LocaleManager::s_sharedInstance = nullptr;

EventHeader onLocale("Locale", "onLocale");

Initializer::Initializer(const String &locale, LocaleInitList && list) {
	LocaleManager::getInstance()->define(locale, move(list));
}

void define(const StringView &locale, LocaleInitList &&init) {
	LocaleManager::getInstance()->define(locale, move(init));
}

void define(const StringView &locale, LocaleIndexList &&init) {
	LocaleManager::getInstance()->define(locale, move(init));
}

void define(const StringView &locale, const std::array<StringView, toInt(TimeTokens::Max)> &arr) {
	LocaleManager::getInstance()->define(locale, arr);
}

WideStringView string(const WideStringView &str) {
	return LocaleManager::getInstance()->string(str);
}

WideStringView string(size_t idx) {
	return LocaleManager::getInstance()->string(idx);
}

WideStringView numeric(const WideStringView &str, int64_t num) {
	return LocaleManager::getInstance()->numeric(str, num);
}

void setDefault(const String &def) {
	LocaleManager::getInstance()->setDefault(def);
}
const String &getDefault() {
	return LocaleManager::getInstance()->getDefault();
}

void setLocale(const String &loc) {
	LocaleManager::getInstance()->setLocale(loc);
}
const String &getLocale() {
	return LocaleManager::getInstance()->getLocale();
}

void setNumRule(const String &locale, const NumRule &rule) {
	LocaleManager::getInstance()->setNumRule(locale, rule);
}

bool hasLocaleTagsFast(const WideStringView &r) {
	return LocaleManager::getInstance()->hasLocaleTagsFast(r);
}

bool hasLocaleTags(const WideStringView &r) {
	return LocaleManager::getInstance()->hasLocaleTags(r);
}

WideString resolveLocaleTags(const WideStringView &r) {
	return LocaleManager::getInstance()->resolveLocaleTags(r);
}

String language(const String &locale) {
	return LocaleManager::getInstance()->language(locale);
}

String common(const String &locale) {
	return LocaleManager::getInstance()->common(locale);
}

StringView timeToken(TimeTokens tok) {
	return LocaleManager::getInstance()->timeToken(tok);
}

static bool isToday(struct tm &tm, struct tm &now) {
	return tm.tm_year == now.tm_year && tm.tm_yday == now.tm_yday;
}

static bool isYesterday(struct tm &tm, struct tm &now) {
	if (now.tm_yday > 0) {
		return tm.tm_year == now.tm_year && tm.tm_yday == now.tm_yday - 1;
	} else if (now.tm_year > 0) {
		if ((tm.tm_year & 3) || (((tm.tm_year % 100) == 0) && (((tm.tm_year % 400) != 100)))) {
			return tm.tm_year == now.tm_year - 1 && tm.tm_yday == 354;
		} else {
			return tm.tm_year == now.tm_year - 1 && tm.tm_yday == 355;
		}
	}
	return false;
}

template <typename T>
static String localDate_impl(const std::array<T, toInt(TimeTokens::Max)> &table, Time t) {
	auto sec_now = time_t(Time::now().toSeconds());
	struct tm tm_now;
	localtime_r(&sec_now, &tm_now);

	auto sec_time = time_t(t.toSeconds());
	struct tm tm_time;
	localtime_r(&sec_time, &tm_time);

	if (isToday(tm_time, tm_now)) {
		return String(table[toInt(TimeTokens::Today)].data(), table[toInt(TimeTokens::Today)].size());
	} else if (isYesterday(tm_time, tm_now)) {
		return String(table[toInt(TimeTokens::Yesterday)].data(), table[toInt(TimeTokens::Yesterday)].size());
	}
	if (tm_time.tm_year == tm_now.tm_year) {
		return toString(tm_time.tm_mday, " ", table[tm_time.tm_mon + 2]);
	} else {
		return toString(tm_time.tm_mday, " ", table[tm_time.tm_mon + 2], " ", 1900 + tm_time.tm_year);
	}
}

String localDate(Time t) {
	return localDate_impl(LocaleManager::getInstance()->timeTokenTable(), t);
}

String localDate(const std::array<StringView, toInt(TimeTokens::Max)> &table, Time t) {
	return localDate_impl(table, t);
}

NS_SP_EXT_END(locale)
