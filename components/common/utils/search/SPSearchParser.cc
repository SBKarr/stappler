/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPSearchConfiguration.h"
#include "SPSnowballStopwords.cc"

namespace stappler::search {

static const StringView *getLanguageStopwords(Language lang) {
	switch (lang) {
	case Language::Unknown: return nullptr; break;
	case Language::Arabic: return nullptr; break;
	case Language::Danish: return s_danish_stopwords; break;
	case Language::Dutch: return s_dutch_stopwords; break;
	case Language::English: return s_english_stopwords; break;
	case Language::Finnish: return s_finnish_stopwords; break;
	case Language::French: return s_french_stopwords; break;
	case Language::German: return s_german_stopwords; break;
	case Language::Greek: return nullptr; break;
	case Language::Hungarian: return s_hungarian_stopwords; break;
	case Language::Indonesian: return nullptr; break;
	case Language::Irish: return nullptr; break;
	case Language::Italian: return s_italian_stopwords; break;
	case Language::Lithuanian: return nullptr; break;
	case Language::Nepali: return s_nepali_stopwords; break;
	case Language::Norwegian: return s_norwegian_stopwords; break;
	case Language::Portuguese: return s_portuguese_stopwords; break;
	case Language::Romanian: return nullptr; break;
	case Language::Russian: return s_russian_stopwords; break;
	case Language::Spanish: return s_spanish_stopwords; break;
	case Language::Swedish: return s_swedish_stopwords; break;
	case Language::Tamil: return nullptr; break;
	case Language::Turkish: return s_turkish_stopwords; break;
	case Language::Simple: return nullptr; break;
	}
	return nullptr;
}

StringView SearchData::getLanguage() const {
	return getLanguageName(language);
}

bool isStopword(const StringView &word, Language lang) {
	if (lang == Language::Unknown) {
		lang = detectLanguage(word);
		if (lang == Language::Unknown) {
			return false;
		}
	}

	if (auto dict = getLanguageStopwords(lang)) {
		return isStopword(word, dict);
	}

	return false;
}

StringView getLanguageName(Language lang) {
	switch (lang) {
	case Language::Unknown: return StringView(); break;
	case Language::Arabic: return StringView("arabic"); break;
	case Language::Danish: return StringView("danish"); break;
	case Language::Dutch: return StringView("dutch"); break;
	case Language::English: return StringView("english"); break;
	case Language::Finnish: return StringView("finnish"); break;
	case Language::French: return StringView("french"); break;
	case Language::German: return StringView("german"); break;
	case Language::Greek: return StringView("greek"); break;
	case Language::Hungarian: return StringView("hungarian"); break;
	case Language::Indonesian: return StringView("indonesian"); break;
	case Language::Irish: return StringView("irish"); break;
	case Language::Italian: return StringView("italian"); break;
	case Language::Lithuanian: return StringView("lithuanian"); break;
	case Language::Nepali: return StringView("nepali"); break;
	case Language::Norwegian: return StringView("norwegian"); break;
	case Language::Portuguese: return StringView("portuguese"); break;
	case Language::Romanian: return StringView("romanian"); break;
	case Language::Russian: return StringView("russian"); break;
	case Language::Spanish: return StringView("spanish"); break;
	case Language::Swedish: return StringView("swedish"); break;
	case Language::Tamil: return StringView("tamil"); break;
	case Language::Turkish: return StringView("turkish"); break;
	case Language::Simple: return StringView("simple"); break;
	}
	return StringView();
}

Language parseLanguage(const StringView &lang) {
	if (lang == "arabic") { return Language::Arabic; }
	else if (lang == "danish") { return Language::Danish; }
	else if (lang == "dutch") { return Language::Dutch; }
	else if (lang == "english") { return Language::English; }
	else if (lang == "finnish") { return Language::Finnish; }
	else if (lang == "french") { return Language::French; }
	else if (lang == "german") { return Language::German; }
	else if (lang == "greek") { return Language::Greek; }
	else if (lang == "hungarian") { return Language::Hungarian; }
	else if (lang == "indonesian") { return Language::Indonesian; }
	else if (lang == "irish") { return Language::Irish; }
	else if (lang == "italian") { return Language::Italian; }
	else if (lang == "nepali") { return Language::Nepali; }
	else if (lang == "norwegian") { return Language::Norwegian; }
	else if (lang == "portuguese") { return Language::Portuguese; }
	else if (lang == "romanian") { return Language::Romanian; }
	else if (lang == "russian") { return Language::Russian; }
	else if (lang == "spanish") { return Language::Spanish; }
	else if (lang == "swedish") { return Language::Swedish; }
	else if (lang == "tamil") { return Language::Tamil; }
	else if (lang == "turkish") { return Language::Turkish; }
	else if (lang == "simple") { return Language::Simple; }
	return Language::Unknown;
}

Language detectLanguage(const StringView &word) {
	StringView str(word);
	str.skipUntil<StringView::CharGroup<CharGroupId::Numbers>, StringView::Chars<'.'>>();
	if (str.empty()) {
		StringViewUtf8 r(word.data(), word.size());
		while (!r.empty()) {
			r.skipUntil<StringViewUtf8::MatchCharGroup<CharGroupId::Latin>,
				StringViewUtf8::MatchCharGroup<CharGroupId::Cyrillic>,
				StringViewUtf8::MatchCharGroup<CharGroupId::GreekBasic>,
				StringViewUtf8::MatchCharGroup<CharGroupId::Numbers>>();
			if (r.is<StringViewUtf8::MatchCharGroup<CharGroupId::Latin>>()) {
				return Language::English;
			} else if (r.is<StringViewUtf8::MatchCharGroup<CharGroupId::Cyrillic>>()) {
				return Language::Russian;
			} else if (r.is<StringViewUtf8::MatchCharGroup<CharGroupId::GreekBasic>>()) {
				return Language::Greek;
			} else if (r.is<StringViewUtf8::MatchCharGroup<CharGroupId::Numbers>>()) {
				return Language::Simple;
			}
		}
		return Language::Unknown;
	} else {
		return Language::Simple;
	}
}

StringView getParserTokenName(ParserToken tok) {
	switch (tok) {
	case ParserToken::AsciiWord: return "AsciiWord"; break;
	case ParserToken::Word: return "Word"; break;
	case ParserToken::NumWord: return "NumWord"; break;
	case ParserToken::Email: return "Email"; break;
	case ParserToken::Url: return "Url"; break;
	case ParserToken::ScientificFloat: return "ScientificFloat"; break;
	case ParserToken::Version: return "Version"; break;
	case ParserToken::HyphenatedWord_NumPart: return "HyphenatedWord_NumPart"; break;
	case ParserToken::HyphenatedWord_Part: return "HyphenatedWord_Part"; break;
	case ParserToken::HyphenatedWord_AsciiPart: return "HyphenatedWord_AsciiPart"; break;
	case ParserToken::Blank: return "Blank"; break;
	case ParserToken::NumHyphenatedWord: return "NumHyphenatedWord"; break;
	case ParserToken::AsciiHyphenatedWord: return "AsciiHyphenatedWord"; break;
	case ParserToken::HyphenatedWord: return "HyphenatedWord"; break;
	case ParserToken::Path: return "Path"; break;
	case ParserToken::Float: return "Float"; break;
	case ParserToken::Integer: return "Integer"; break;
	case ParserToken::XMLEntity: return "XMLEntity"; break;
	case ParserToken::Custom: return "Custom"; break;
	}
	return StringView();
}

bool isWordPart(ParserToken tok) {
	switch (tok) {
	case ParserToken::HyphenatedWord_NumPart:
	case ParserToken::HyphenatedWord_Part:
	case ParserToken::HyphenatedWord_AsciiPart:
		return true;
		break;
	default:
		break;
	}
	return false;
}

bool isComplexWord(ParserToken tok) {
	switch (tok) {
	case ParserToken::NumHyphenatedWord:
	case ParserToken::AsciiHyphenatedWord:
	case ParserToken::HyphenatedWord:
		return true;
		break;
	default:
		break;
	}
	return false;
}

struct UsedCharGroup : chars::Compose<char16_t,
	chars::CharGroup<char16_t, CharGroupId::Alphanumeric>,
	chars::CharGroup<char16_t, CharGroupId::Cyrillic>,
	chars::CharGroup<char16_t, CharGroupId::LatinSuppl1>,
	chars::CharGroup<char16_t, CharGroupId::GreekBasic>,
	chars::CharGroup<char16_t, CharGroupId::GreekAdvanced>,
	chars::Chars<char16_t, u'-', u'_', u'&', u'/'>
> { };

struct WordCharGroup : chars::Compose<char16_t,
	chars::CharGroup<char16_t, CharGroupId::Alphanumeric>,
	chars::CharGroup<char16_t, CharGroupId::Cyrillic>,
	chars::CharGroup<char16_t, CharGroupId::LatinSuppl1>,
	chars::CharGroup<char16_t, CharGroupId::GreekBasic>,
	chars::CharGroup<char16_t, CharGroupId::GreekAdvanced>,
	chars::Chars<char16_t, char16_t(0xAD)>
> { };

static ParserStatus parseUrlToken(StringView &r, const Callback<ParserStatus(StringView, ParserToken)> &cb) {
	UrlView view;
	if (!view.parse(r)) {
		return ParserStatus::PreventSubdivide;
	}

	if (view.isEmail()) {
		if (cb(view.url, ParserToken::Email) == ParserStatus::Stop) { return ParserStatus::Stop; }
	} else if (view.isPath()) {
		if (cb(view.url, ParserToken::Path) == ParserStatus::Stop) { return ParserStatus::Stop; }
	} else {
		if (cb(view.url, ParserToken::Url) == ParserStatus::Stop) { return ParserStatus::Stop; }
	}

	return ParserStatus::Continue;
}

static ParserStatus tryParseUrl(StringViewUtf8 &tmp2, StringView r, const Callback<ParserStatus(StringView, ParserToken)> &cb) {
	if (tmp2.is('_') || tmp2.is('.') || tmp2.is(':') || tmp2.is('@') || tmp2.is('/') || tmp2.is('?') || tmp2.is('#')) {
		auto tmp3 = tmp2;
		++ tmp3;
		if (tmp3.is<WordCharGroup>() || tmp3.is('/')) {
			StringView rv(r.data(), tmp2.size() + (tmp2.data() - r.data()));
			switch (parseUrlToken(rv, cb)) {
			case ParserStatus::Continue:
				tmp2 = rv;
				return ParserStatus::Continue;
				break;
			case ParserStatus::Stop:
				return ParserStatus::Stop;
				break;
			default:
				break;
			}
		}
	}
	return ParserStatus::PreventSubdivide;
}

static ParserStatus parseDotNumber(StringViewUtf8 &r, StringView tmp, const Callback<ParserStatus(StringView, ParserToken)> &cb, bool allowVersion) {
	if (r.is<chars::CharGroup<char16_t, CharGroupId::Numbers>>()) {
		auto num = r.readChars<chars::CharGroup<char16_t, CharGroupId::Numbers>>();
		if (r.is('.') && allowVersion) {
			while (r.is('.')) {
				++ r;
				num = r.readChars<chars::CharGroup<char16_t, CharGroupId::Alphanumeric>>();
				if (num.empty()) {
					return ParserStatus::PreventSubdivide;
				}
			}
			if (r.is('_') || r.is('@') || r.is(':') || r.is('/') || r.is('?') || r.is('#')) {
				switch (tryParseUrl(r, tmp, cb)) {
				case ParserStatus::PreventSubdivide:
					if (cb(StringView(tmp.data(), r.data() - tmp.data()), ParserToken::Version) == ParserStatus::Stop) { return ParserStatus::Stop; }
					if (cb(r.sub(0, 1), ParserToken::Blank) == ParserStatus::Stop) { return ParserStatus::Stop; }
					++ r;
					break;
				case ParserStatus::Stop:
					return ParserStatus::Stop;
					break;
				default:
					break;
				}
				return ParserStatus::Continue;
			} else if (!r.is<WordCharGroup>()) {
				if (cb(StringView(tmp.data(), r.data() - tmp.data()), ParserToken::Version) == ParserStatus::Stop) { return ParserStatus::Stop; }
				return ParserStatus::Continue;
			}
		} else if (r.is('e') || r.is('E')) {
			++ r;
			num = r.readChars<chars::CharGroup<char16_t, CharGroupId::Numbers>>();
			if (!num.empty()) {
				if (!r.is<WordCharGroup>()) {
					if (cb(StringView(tmp.data(), r.data() - tmp.data()), ParserToken::ScientificFloat) == ParserStatus::Stop) { return ParserStatus::Stop; }
					return ParserStatus::Continue;
				}
			}
		} else if (r.is('@') || r.is(':') || r.is('/') || r.is('?') || r.is('#')) {
			switch (tryParseUrl(r, tmp, cb)) {
			case ParserStatus::PreventSubdivide:
				if (cb(StringView(tmp.data(), r.data() - tmp.data()), ParserToken::Float) == ParserStatus::Stop) { return ParserStatus::Stop; }
				if (cb(r.sub(0, 1), ParserToken::Blank) == ParserStatus::Stop) { return ParserStatus::Stop; }
				++ r;
				break;
			case ParserStatus::Stop:
				return ParserStatus::Stop;
				break;
			default:
				break;
			}
			return ParserStatus::Continue;
		} else if (r.is<WordCharGroup>()) {
			return ParserStatus::PreventSubdivide;
		} else {
			if (cb(StringView(tmp.data(), r.data() - tmp.data()), ParserToken::Float) == ParserStatus::Stop) { return ParserStatus::Stop; }
			return ParserStatus::Continue;
		}
	}
	return ParserStatus::PreventSubdivide;
}

static bool pushWord(StringView word, const Callback<ParserStatus(StringView, ParserToken)> &cb, bool hyph = false) {
	StringView r(word);
	r.readChars<StringView::CharGroup<CharGroupId::Latin>>();
	if (r.empty()) {
		if (cb(word, hyph ? ParserToken::HyphenatedWord_AsciiPart : ParserToken::AsciiWord) == ParserStatus::Stop) { return false; }
	} else if (!r.is<StringView::CharGroup<CharGroupId::Numbers>>()) {
		r.readUntil<StringView::CharGroup<CharGroupId::Numbers>>();
		if (r.empty()) {
			if (cb(word, hyph ? ParserToken::HyphenatedWord_Part : ParserToken::Word) == ParserStatus::Stop) { return false; }
		} else {
			if (cb(word, hyph ? ParserToken::HyphenatedWord_NumPart : ParserToken::NumWord) == ParserStatus::Stop) { return false; }
		}
	} else {
		if (cb(word, hyph ? ParserToken::HyphenatedWord_NumPart : ParserToken::NumWord) == ParserStatus::Stop) { return false; }
	}
	return true;
}

static bool pushHWord(StringView word, const Callback<ParserStatus(StringView, ParserToken)> &cb) {
	ParserStatus stat = ParserStatus::Continue;
	StringView r(word);
	r.readChars<StringView::CharGroup<CharGroupId::Latin>, StringView::Chars<'-'>>();
	if (r.empty()) {
		cb(word, ParserToken::AsciiHyphenatedWord);
	} else if (!r.is<StringView::CharGroup<CharGroupId::Numbers>>()) {
		r.readUntil<StringView::CharGroup<CharGroupId::Numbers>>();
		if (r.empty()) {
			stat = cb(word, ParserToken::HyphenatedWord);
		} else {
			stat = cb(word, ParserToken::NumHyphenatedWord);
		}
	} else {
		stat = cb(word, ParserToken::NumHyphenatedWord);
	}

	if (stat == ParserStatus::Stop) {
		return false;
	} else if (stat == ParserStatus::PreventSubdivide) {
		return true;
	}

	while (!word.empty()) {
		auto sep = word.readChars<StringView::Chars<'-'>>();
		if (!sep.empty()) {
			if (cb(sep, ParserToken::Blank) == ParserStatus::Stop) { return false; }
		}
		auto tmp = word.readUntil<StringView::Chars<'-'>>();
		if (!tmp.empty()) {
			if (!pushWord(tmp, cb, true)) {
				return false;
			}
		}
	}
	return true;
}

static bool parseHyphenatedWord(StringViewUtf8 &tmp, StringView r, const Callback<ParserStatus(StringView, ParserToken)> &cb, size_t depth) {
	auto tmp2 = tmp;
	tmp2.skipChars<WordCharGroup>();

	auto doPushWord = [&] {
		if (depth == 0) {
			return pushWord(StringView(r.data(), tmp2.data() - r.data()), cb);
		} else {
			return pushHWord(StringView(r.data(), tmp2.data() - r.data()), cb);
		}
	};

	if (tmp2.is('-')) {
		tmp2.skipChars<StringViewUtf8::Chars<u'-'>>();
		if (!parseHyphenatedWord(tmp2, StringView(r.data(), tmp.data() - r.data()), cb, depth + 1)) {
			return false;
		}
	} else if (tmp2.is('_') || tmp2.is('.') || tmp2.is(':') || tmp2.is('@') || tmp2.is('/') || tmp2.is('?') || tmp2.is('#')) {
		switch (tryParseUrl(tmp2, r, cb)) {
		case ParserStatus::PreventSubdivide:
			if (!doPushWord()) { return false; }
			if (cb(tmp2.sub(0, 1), ParserToken::Blank) == ParserStatus::Stop) { return false; }
			++ tmp2;
			break;
		case ParserStatus::Stop:
			return false;
			break;
		default:
			break;
		}
	} else {
		if (!doPushWord()) { return false; }
	}
	tmp = tmp2;
	return true;
}

static ParserStatus readCadasterString(StringViewUtf8 &r, StringView tmp, const Callback<ParserStatus(StringView, ParserToken)> &cb) {
	using Numbers = StringViewUtf8::MatchCharGroup<CharGroupId::Numbers>;

	using WhiteSpace = chars::Compose<char16_t,
			chars::Range<char16_t, u'\u2000', u'\u200D'>,
			chars::Chars<char16_t, u'\u0009', u'\u000B', u'\u000C', u'\u0020', u'\u0085', u'\u00A0', u'\u1680', u'\u2028', u'\u2029',
					 u'\u202F', u'\u205F', u'\u2060', u'\u3000', u'\uFEFF', u'\uFFFF'>
	>;

	if (tmp.size() != 2) {
		return ParserStatus::PreventSubdivide;
	}

	if (r.is(':')) {
		StringViewUtf8 rv = r;
		size_t segments = 1;
		while (rv.is(':') || (rv.is<WhiteSpace>())) {
			rv.skipChars<StringViewUtf8::Chars<':'>, WhiteSpace>();
			auto nums = rv.readChars<Numbers>();
			if (nums.empty()) {
				if (segments >= 3) {
					r = rv;
					break;
				} else {
					return ParserStatus::PreventSubdivide;
				}
			} else if (rv.is(':')) {
				++ segments;
			} else if (rv.is<WhiteSpace>()) {
				if (segments >= 3) {
					auto tmp = rv;
					tmp.skipChars<WhiteSpace>();
					nums = rv.readChars<Numbers>();
					if ((nums.size() == 2 && (tmp.is(':') || tmp.is('-') || tmp.is(u'–'))) || nums.empty()) {
						r = rv;
						break;
					}
				} else {
					auto tmp = rv;
					tmp.skipChars<WhiteSpace>();
					if (tmp.is(':')) {
						rv = tmp;
						++ segments;
					}
				}
			} else {
				if (segments >= 3) {
					r = rv;
				}
				break;
			}
		}

		if (segments >= 3) {
			auto code = StringView(tmp.data(), r.data() - tmp.data());
			code.trimUntil<StringView::CharGroup<CharGroupId::Alphanumeric>>();
			if (cb(code, ParserToken::Custom) == ParserStatus::Stop) { return ParserStatus::Stop; }
			r = StringViewUtf8(code.data() + code.size(), (r.data() - code.data() - code.size()) + r.size());
			return ParserStatus::Continue;
		}
	} else if (r.is('-') || r.is(u'–')) {
		StringViewUtf8 rv = r;
		size_t segments = 1;
		size_t nonWsSegments = 0;
		if (!r.is<WhiteSpace>()) {
			++ nonWsSegments;
		}
		while (rv.is('-') || rv.is(u'–') || rv.is<WhiteSpace>() || rv.is('/') || rv.is(':')) {
			rv.skipChars<StringViewUtf8::Chars<'-', u'–', '/', ':'>, WhiteSpace>();
			auto nums = rv.readChars<Numbers>();
			if (nums.empty()) {
				if (segments >= 5) {
					r = rv;
					break;
				} else {
					return ParserStatus::PreventSubdivide;
				}
			} else if (rv.is('-') || rv.is(u'–')) {
				++ segments;
				++ nonWsSegments;
			} else if (rv.is('/') && segments > 1) {
				++ segments;
				++ nonWsSegments;
			} else if (rv.is(':') && segments > 1) {
				++ segments;
				++ nonWsSegments;
			} else if (rv.is<WhiteSpace>()) {
				if (segments >= 5) {
					r = rv;
					break;
				}
				++ segments;
			} else {
				if (segments >= 5) {
					r = rv;
				}
				break;
			}
		}

		if (segments >= 5 && nonWsSegments >= 2) {
			auto code = StringView(tmp.data(), r.data() - tmp.data());
			code.trimUntil<StringView::CharGroup<CharGroupId::Alphanumeric>>();
			if (cb(code, ParserToken::Custom) == ParserStatus::Stop) { return ParserStatus::Stop; }
			r = StringViewUtf8(code.data() + code.size(), (r.data() - code.data() - code.size()) + r.size());
			return ParserStatus::Continue;
		}
	}

	return ParserStatus::PreventSubdivide;
}

static bool parseToken(StringViewUtf8 &r, const Callback<ParserStatus(StringView, ParserToken)> &cb) {
	auto readWord = [&] () {
		auto tmp = r;
		if (!parseHyphenatedWord(tmp, StringView(r.data(), 0), cb, 0)) {
			return false;
		}
		r = tmp;
		return true;
	};

	if (r.is('-')) {
		auto tmp = r;
		++ tmp;
		if (tmp.is<chars::CharGroup<char16_t, CharGroupId::Numbers>>()) {
			tmp.readChars<chars::CharGroup<char16_t, CharGroupId::Numbers>>();
			if (tmp.is('.')) {
				auto tmp2 = tmp;
				++ tmp2;
				switch (parseDotNumber(tmp2, StringView(r.data(), tmp.data() - r.data()), cb, false)) {
				case ParserStatus::Continue:
					r = tmp2;
					break;
				case ParserStatus::PreventSubdivide:
					if (cb(StringView(r.data(), tmp.data() - r.data()), ParserToken::Integer) == ParserStatus::Stop) { return false; }
					r = tmp;
					if (cb(StringView(r.data(), 1), ParserToken::Blank) == ParserStatus::Stop) { return false; }
					++ r;
					break;
				case ParserStatus::Stop:
					return false;
					break;
				}
			} else if (r.is<WordCharGroup>()) {
				if (cb(StringView(r.data(), 1), ParserToken::Blank) == ParserStatus::Stop) { return false; }
				++ r;
				return true;
			} else {
				if (cb(StringView(r.data(), tmp.data() - r.data()), ParserToken::Integer) == ParserStatus::Stop) { return false; }
				r = tmp;
			}
		} else {
			if (cb(StringView(r.data(), 1), ParserToken::Blank) == ParserStatus::Stop) { return false; }
			++ r;
		}
	} else if (r.is('/')) {
		switch (tryParseUrl(r, StringView(r.data(), 0), cb)) {
		case ParserStatus::PreventSubdivide:
			if (cb(StringView(r.data(), 1), ParserToken::Blank) == ParserStatus::Stop) { return false; }
			++ r;
			break;
		case ParserStatus::Stop:
			return false;
			break;
		default:
			break;
		}
	} else if (r.is('&')) {
		StringView tmp(r.data(), std::min(size_t(8), r.size()));
		tmp.readUntil<StringView::Chars<';'>>();
		if (tmp.is(';')) {
			++ tmp;
			if (cb(StringView(r.data(), tmp.data() - r.data()), ParserToken::XMLEntity) == ParserStatus::Stop) { return false; }
			r = StringViewUtf8(tmp.data(), r.size() - (tmp.data() - r.data()));
		} else {
			if (cb(StringView(r.data(), 1), ParserToken::Blank) == ParserStatus::Stop) { return false; }
			++ r;
		}
	} else if (r.is('_')) {
		switch (tryParseUrl(r, StringView(r.data(), 0), cb)) {
		case ParserStatus::PreventSubdivide:
			if (cb(StringView(r.data(), 1), ParserToken::Blank) == ParserStatus::Stop) { return false; }
			++ r;
			break;
		case ParserStatus::Stop:
			return false;
			break;
		default:
			break;
		}
	} else if (r.is<chars::CharGroup<char16_t, CharGroupId::Numbers>>()) {
		auto tmp = r;
		auto num = tmp.readChars<chars::CharGroup<char16_t, CharGroupId::Numbers>>();
		if (tmp.is('.')) {
			auto tmp2 = tmp;
			++ tmp2;
			switch (parseDotNumber(tmp2, StringView(r.data(), tmp.data() - r.data()), cb, true)) {
			case ParserStatus::Continue:
				r = tmp2;
				break;
			case ParserStatus::PreventSubdivide:
				if (cb(StringView(r.data(), tmp.data() - r.data()), ParserToken::Integer) == ParserStatus::Stop) { return false; }
				r = tmp;
				if (cb(StringView(r.data(), 1), ParserToken::Blank) == ParserStatus::Stop) { return false; }
				++ r;
				break;
			case ParserStatus::Stop:
				return false;
				break;
			}
		} else if ((tmp.is(':') || tmp.is('-') || tmp.is(u'–')) && num.size() == 2) {
			switch (readCadasterString(tmp, num, cb)) {
			case ParserStatus::Continue:
				r = tmp;
				break;
			case ParserStatus::Stop:
				return false;
				break;
			default:
				if (cb(num, ParserToken::Integer) == ParserStatus::Stop) { return false; }
				r = tmp;
				if (cb(StringView(r.data(), 1), ParserToken::Blank) == ParserStatus::Stop) { return false; }
				++ r;
				break;
			}
		} else if (tmp.is<StringViewUtf8::CharGroup<CharGroupId::WhiteSpace>>()) {
			auto tmp2 = tmp;
			tmp2.skipChars<StringViewUtf8::CharGroup<CharGroupId::WhiteSpace>>();
			if (tmp2.is<StringViewUtf8::CharGroup<CharGroupId::Numbers>>()) {
				switch (readCadasterString(tmp, num, cb)) {
				case ParserStatus::Continue:
					r = tmp;
					break;
				case ParserStatus::Stop:
					return false;
					break;
				default:
					if (cb(num, ParserToken::Integer) == ParserStatus::Stop) { return false; }
					r = tmp;
					break;
				}
			} else {
				if (cb(StringView(r.data(), tmp.data() - r.data()), ParserToken::Integer) == ParserStatus::Stop) { return false; }
				r = tmp;
			}
		} else if (tmp.is('@')) {
			StringView rv(r.data(), r.size());
			switch (parseUrlToken(rv, cb)) {
			case ParserStatus::Continue:
				r = rv;
				break;
			case ParserStatus::PreventSubdivide:
				if (cb(StringView(r.data(), tmp.data() - r.data()), ParserToken::Integer) == ParserStatus::Stop) { return false; }
				r = tmp;
				break;
			case ParserStatus::Stop:
				return false;
				break;
			}
		} else if (tmp.is<WordCharGroup>()) {
			if (!readWord()) { return false; }
		} else {
			if (cb(StringView(r.data(), tmp.data() - r.data()), ParserToken::Integer) == ParserStatus::Stop) { return false; }
			r = tmp;
		}
	} else if (r.is<WordCharGroup>()) {
		if (!readWord()) { return false; }
	} else {
		++ r;
	}

	return true;
}

struct Stemmer_Reader {
	using String = memory::PoolInterface::StringType;
	using StringStream = memory::PoolInterface::StringStreamType;

	enum Type {
		None,
		Content,
		Inline,
		Drop,
	};

	struct Tag : html::Tag<StringView> {
		using html::Tag<StringView>::Tag;

		Type type = None;
		bool init = false;
	};

	using Parser = html::Parser<Stemmer_Reader, StringView, Stemmer_Reader::Tag>;
	using StringReader = Parser::StringReader;

	void write(const StringView &d) {
		switch (type) {
		case Type::None: break;
		case Type::Content: buffer << d; break;
		case Type::Inline: buffer << d; break;
		case Type::Drop: break;
		}
	}

	void processData(Parser &p, const StringView &buf) {
		StringView r(buf);
		r.trimChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		if (!r.empty()) {
			if (callback) {
				callback(p, r);
			}
		}
	}

	Type getTypeByName(const StringView &r) const {
		if (r == "a" || r == "abbr" || r == "acronym" || r == "b"
			|| r == "br" || r == "code" || r == "em" || r == "font"
			|| r == "i" || r == "img" || r == "ins" || r == "kbd"
			|| r == "map" || r == "samp" || r == "small" || r == "span"
			|| r == "strong") {
			return Inline;
		} else if (r == "sub" || r == "sup") {
			return Drop;
		} else if (r == "p" || r == "h1" || r == "h2" || r == "h3"
			|| r == "h4" || r == "h5" || r == "h6") {
			return Content;
		}
		return None;
	}

	inline void onBeginTag(Parser &p, Tag &tag) { tag.type = getTypeByName(tag.name); }
	inline void onEndTag(Parser &p, Tag &tag, bool isClosed) { }
	inline void onTagAttribute(Parser &p, Tag &tag, StringReader &name, StringReader &value) { }
	inline void onInlineTag(Parser &p, Tag &tag) { }

	inline void onPushTag(Parser &p, Tag &tag) {
		if (type == None && tag.type == Content) {
			buffer.clear();
			type = Content;
			tag.init = true;
		} else if (type == Content && tag.type == Drop) {
			type = Drop;
		}
	}

	inline void onPopTag(Parser &p, Tag &tag) {
		if (tag.init) {
			processData(p, buffer.weak());
			buffer.clear();
			type = None;
		} else if (type == Drop && tag.type == Drop) {
			if (p.tagStack.size() > 1) {
				type = p.tagStack.at(p.tagStack.size() - 2).type;
			} else {
				type = None;
			}
		}
	}

	inline void onTagContent(Parser &p, Tag &tag, StringReader &s) { write(s); }

	Type type = Type::None;
	StringStream buffer;
	Function<void(Parser &, const StringView &)> callback;
};

void parseHtml(StringView str, const Callback<void(StringView)> &cb) {
	if (str.empty()) {
		return;
	}

	Stemmer_Reader r;
	r.callback = [&] (Stemmer_Reader::Parser &p, const StringView &str) {
		cb(str);
	};
	html::parse(r, str, false);
}

bool parsePhrase(StringView str, const Callback<ParserStatus(StringView, ParserToken)> &cb) {
	StringViewUtf8 r(str);

	while (!r.empty()) {
		auto tmp = r.readUntil<UsedCharGroup>();
		if (!tmp.empty()) {
			if (cb(StringView(tmp.data(), tmp.size()), ParserToken::Blank) == ParserStatus::Stop) {
				return false;
			}
		}

		if (!r.empty()) {
			auto control = r.data();
			if (!parseToken(r, cb)) {
				return false;
			}
			if (r.data() == control) {
				std::cout << "Parsing is stalled\n";
			}
		}
	}
	return true;
}


struct StemmerEnv {
	using symbol = unsigned char;

	void *(*memalloc)( void *userData, unsigned int size );
	void (*memfree)( void *userData, void *ptr );
	void* userData;	// User data passed to the allocator functions.

	int (*stem)(StemmerEnv *);

	symbol * p;
	int c; int l; int lb; int bra; int ket;
	symbol * * S;
	int * I;
	unsigned char * B;

	const StringView *stopwords;
};

struct stemmer_modules {
	Language name;
	StemmerEnv * (*create)(StemmerEnv *);
	void (*close)(StemmerEnv *);
	int (*stem)(StemmerEnv *);
};

SP_EXTERN_C struct stemmer_modules * sb_stemmer_get(Language lang);
SP_EXTERN_C const unsigned char * sb_stemmer_stem(StemmerEnv * z, const unsigned char * word, int size);

static void * staticPoolAlloc(void* userData, unsigned int size) {
	memory::pool_t *pool = (memory::pool_t *)userData;
	size_t s = size;
	auto mem = memory::pool::alloc(pool, s);
	memset(mem,0, s);
	return mem;
}

static void staticPoolFree(void * userData, void * ptr) { }

StemmerEnv *getStemmer(Language lang) {
	auto pool = memory::pool::acquire();

	auto key = toString("SP.Stemmer.", getLanguageName(lang));

	StemmerEnv *data = nullptr;
	memory::pool::userdata_get((void **)&data, key.data(), pool);
	if (data) {
		return data;
	}

	auto mod = sb_stemmer_get(lang);
	if (!mod->create) {
		return nullptr;
	}

	data = (StemmerEnv *)memory::pool::palloc(pool, sizeof(StemmerEnv));
	memset(data, 0, sizeof(StemmerEnv));
	data->memalloc = &staticPoolAlloc;
	data->memfree = &staticPoolFree;
	data->userData = pool;

	if (auto env = mod->create(data)) {
		env->stem = mod->stem;
		env->stopwords = getLanguageStopwords(lang);
		memory::pool::userdata_set(data, key.data(), nullptr, pool);
		return env;
	}
	return nullptr;
}

bool isStopword(const StringView &word, StemmerEnv *env) {
	if (env) {
		return isStopword(word, env->stopwords);
	}
	return false;
}

bool isStopword(const StringView &word, const StringView *stopwords) {
	if (stopwords) {
		while (stopwords && !stopwords->empty()) {
			if (word == *stopwords) {
				return true;
			} else {
				++ stopwords;
			}
		}
	}
	return false;
}

bool stemWord(StringView word, const Callback<void(StringView)> &cb, StemmerEnv *env) {
	if (isStopword(word, env)) {
		return false;
	}
	auto w = sb_stemmer_stem(env, (const unsigned char *)word.data(), int(word.size()));
	cb(StringView((const char *)w,  size_t(env->l)));
	return true;
}

bool stemWord(StringView word, const Callback<void(StringView)> &cb, Language lang) {
	if (lang == Language::Unknown) {
		lang = detectLanguage(word);
		if (lang == Language::Unknown) {
			return false;
		}
	}

	if (lang == Language::Simple) {
		cb(word);
	}

	if (auto stemmer = getStemmer(lang)) {
		return stemWord(word, cb, stemmer);
	}

	return false;
}

mem_pool::String normalizeWord(const StringView &str) {
	mem_pool::String ret; ret.reserve(str.size());
	auto ptr = str.data();
	while (ptr < str.data() + str.size()) {
		if (*ptr & (0x80)) {
			uint8_t len = unicode::utf8_length_data[((const uint8_t *)ptr)[0]];
			for (uint8_t i = 0; i < len; ++ i) {
				ret.emplace_back(*ptr);
				++ ptr;
			}

			if (len == 2) {
				if (ret[ret.size() - 2] == char(0xC2) && ret[ret.size() - 1] == char(0xAD)) {
					ret.pop_back();
					ret.pop_back();
				} else {
					string::tolower(ret[ret.size() - 2], ret[ret.size() - 1]);
				}
			}
		} else {
			ret.emplace_back(::tolower(*ptr));
			++ ptr;
		}
	}
	return ret;
}

SearchQuery::SearchQuery(StringView w, size_t off, StringView source) : offset(off), value(w.str<memory::PoolInterface>()), source(source) { }

SearchQuery::SearchQuery(SearchOp op, StringView w) : op(op), value(w.str<memory::PoolInterface>()) { }

void SearchQuery::clear() {
	block = None;
	offset = 0;
	op = SearchOp::None;
	value.clear();
	args.clear();
}

static void SearchQuery_encode_Stappler(const Callback<void(StringView)> &cb, const SearchQuery * t) {
	if (t->args.empty()) {
		if (!t->value.empty()) {
			switch (t->block) {
			case SearchQuery::None: break;
			case SearchQuery::Parentesis: cb << "("; break;
			case SearchQuery::Quoted: cb << "("; break;
			}

			if (t->op == SearchOp::Not) {
				cb << "!";
			}
			cb << t->value;
		}
	} else {
		if (!t->args.empty()) {
			switch (t->block) {
			case SearchQuery::None: break;
			case SearchQuery::Parentesis: cb << "("; break;
			case SearchQuery::Quoted: cb << "\""; break;
			}

			auto it = t->args.begin();
			if (t->op == SearchOp::Not) {
				cb << "!";
			}
			SearchQuery_encode_Stappler(cb, &(*it));
			++ it;

			for (;it != t->args.end(); ++ it) {
				cb << " ";
				switch (t->op) {
				case SearchOp::None:
				case SearchOp::And:
					break;
				case SearchOp::Not: cb << " !"; break;
				case SearchOp::Or: cb << " | "; break;
				case SearchOp::Follow:
					if (t->offset > 1 && t->offset <= 5) {
						for (size_t i = 1; i < t->offset; ++ i) {
							cb << "a ";
						}
					}
					break;
				}
				SearchQuery_encode_Stappler(cb, &(*it));
			}

			switch (t->block) {
			case SearchQuery::None: break;
			case SearchQuery::Parentesis: cb << ")"; break;
			case SearchQuery::Quoted: cb << "\""; break;
			}
		}
	}
}

static void SearchQuery_encode_Postgresql(const Callback<void(StringView)> &cb, const SearchQuery * t) {
	if (t->args.empty()) {
		if (!t->value.empty()) {
			switch (t->block) {
			case SearchQuery::None: break;
			case SearchQuery::Parentesis: cb << "("; break;
			case SearchQuery::Quoted: cb << "("; break;
			}

			if (t->op == SearchOp::Not) {
				cb << "!";
			}
			cb << t->value;
		}
	} else {
		if (!t->args.empty()) {
			switch (t->block) {
			case SearchQuery::None: break;
			case SearchQuery::Parentesis: cb << "("; break;
			case SearchQuery::Quoted: cb << "("; break;
			}

			auto it = t->args.begin();
			if (t->op == SearchOp::Not) {
				cb << "!";
			}
			SearchQuery_encode_Postgresql(cb, &(*it));
			++ it;

			for (;it != t->args.end(); ++ it) {
				switch (t->op) {
				case SearchOp::None: cb << " "; break;
				case SearchOp::Not: cb << " !"; break;
				case SearchOp::And: cb << " & "; break;
				case SearchOp::Or: cb << " | "; break;
				case SearchOp::Follow:
					if (it->offset > 1 && it->offset <= 5) {
						cb << " <" << it->offset << "> ";
					} else {
						cb << " <-> ";
					}
					break;
				}
				SearchQuery_encode_Postgresql(cb, &(*it));
			}

			switch (t->block) {
			case SearchQuery::None: break;
			case SearchQuery::Parentesis: cb << ")"; break;
			case SearchQuery::Quoted: cb << ")"; break;
			}
		}
	}
}

void SearchQuery::encode(const Callback<void(StringView)> &cb, Format fmt) const {
	switch (fmt) {
	case Stappler: SearchQuery_encode_Stappler(cb, this); break;
	case Postgresql: SearchQuery_encode_Postgresql(cb, this); break;
	}
}

static void SearchQuery_print(std::ostream &stream, const SearchQuery * t, uint16_t depth) {
	if (t->args.empty()) {
		for (size_t i = 0; i < depth; ++ i) { stream << "  "; }

		switch (t->block) {
		case SearchQuery::None: break;
		case SearchQuery::Parentesis: stream << "(parentesis) "; break;
		case SearchQuery::Quoted: stream << "(quoted) "; break;
		}

		if (t->op == SearchOp::Not) {
			stream << "(not) ";
		}

		if (t->offset > 1) {
			stream << "<" << t->offset << "> ";
		}

		if (!t->value.empty()) {
			stream << "'" << t->value << "'";
		}
		stream << "\n";

	} else {
		for (size_t i = 0; i < depth; ++ i) { stream << "  "; }
		stream << "-> ";

		switch (t->block) {
		case SearchQuery::None: break;
		case SearchQuery::Parentesis: stream << "(parentesis)"; break;
		case SearchQuery::Quoted: stream << "(quoted)"; break;
		}

		switch (t->op) {
		case SearchOp::None: stream << " (none)"; break;
		case SearchOp::Not: stream << " (not)"; break;
		case SearchOp::And: stream << " (and)"; break;
		case SearchOp::Or: stream << " (or)"; break;
		case SearchOp::Follow: stream << " (follow)"; break;
		}
		stream << "\n";

		for (auto &it : t->args) {
			SearchQuery_print(stream, &it, depth + 1);
		}
	}
}

void SearchQuery::describe(std::ostream &stream, size_t depth) const {
	SearchQuery_print(stream, this, depth);
}

static void SearchQuery_foreach(const SearchQuery * t, const Callback<void(StringView value, StringView source)> &cb) {
	if (t->args.empty()) {
		if (!t->value.empty()) {
			cb(t->value, t->source);
		}
	} else {
		for (auto &it : t->args) {
			SearchQuery_foreach(&it, cb);
		}
	}
}

void SearchQuery::foreach(const Callback<void(StringView value, StringView source)> &cb) const {
	SearchQuery_foreach(this, cb);
}

}
