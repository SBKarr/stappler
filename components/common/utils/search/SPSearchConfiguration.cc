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

namespace stappler::search {

static bool stemWordDefault(Language lang, StemmerEnv *env, ParserToken tok, StringView word, const Callback<void(StringView)> &cb, const StringView *stopwords) {
	switch (tok) {
	case ParserToken::AsciiWord:
	case ParserToken::AsciiHyphenatedWord:
	case ParserToken::HyphenatedWord_AsciiPart:
	case ParserToken::Word:
	case ParserToken::HyphenatedWord:
	case ParserToken::HyphenatedWord_Part:
		switch (lang) {
		case Language::Simple: {
			auto str = normalizeWord(word);
			if (stopwords && isStopword(str, stopwords)) {
				return false;
			}
			cb(str);
			break;
		}
		default: {
			auto str = normalizeWord(word);
			if (stopwords && isStopword(str, stopwords)) {
				return false;
			}
			return stemWord(str, cb, env);
			break;
		}
		}
		break;

	case ParserToken::NumWord:
	case ParserToken::NumHyphenatedWord:
	case ParserToken::HyphenatedWord_NumPart: {
		auto str = normalizeWord(word);
		if (stopwords && isStopword(str, stopwords)) {
			return false;
		}
		cb(str);
		break;
	}

	case ParserToken::Email: {
		auto str = normalizeWord(word);
		valid::validateEmail(str);
		if (stopwords && isStopword(str, stopwords)) {
			return false;
		}
		cb(str);
		break;
	}

	case ParserToken::Url: {
		auto str = normalizeWord(word);
		valid::validateUrl(str);
		if (stopwords && isStopword(str, stopwords)) {
			return false;
		}
		cb(str);
		break;
	}

	case ParserToken::Version:
	case ParserToken::Path:
	case ParserToken::ScientificFloat: {
		auto str = normalizeWord(word);
		if (stopwords && isStopword(str, stopwords)) {
			return false;
		}
		cb(str);
		break;
	}

	case ParserToken::Float:
	case ParserToken::Integer: {
		cb(word);
		break;
	}

	case ParserToken::Custom: {
		StringViewUtf8 tmp(word);
		auto num = tmp.readChars<StringViewUtf8::CharGroup<CharGroupId::Numbers>>();
		if (num.size() == 2) {
			if (tmp.is(':') || tmp.is('-') || tmp.is(u'–')) {
				bool cond = tmp.is('-') || tmp.is(u'–');
				mem_pool::String str; str.reserve(word.size());
				if (cond) {
					StringViewUtf8 word2(word);
					while (!word2.empty()) {
						auto r = word2.readUntil<StringViewUtf8::CharGroup<CharGroupId::WhiteSpace>, StringViewUtf8::Chars<u'–'>, StringViewUtf8::Chars<':'>>();
						if (!r.empty()) {
							str.append(r.data(), r.size());
						}
						if (word2.is(u'–')) {
							str.emplace_back('-');
							++ word2;
						} else if (word2.is(':')) {
							str.emplace_back('/');
							++ word2;
						} else {
							auto space = word2.readChars<StringViewUtf8::CharGroup<CharGroupId::WhiteSpace>>();
							if (cond && !space.empty() && !r.empty()) {
								str.emplace_back('/');
							}
						}
					}
				} else {
					while (!word.empty()) {
						auto r = word.readUntil<StringViewUtf8::CharGroup<CharGroupId::WhiteSpace>>();
						if (!r.empty()) {
							str.append(r.data(), r.size());
						}
						auto space = word.readChars<StringViewUtf8::CharGroup<CharGroupId::WhiteSpace>>();
						if (cond && !space.empty() && !r.empty()) {
							str.emplace_back('/');
						}
					}
				}
				string::tolower(str);
				cb(str);
				return true;
			}
		}
		auto str = normalizeWord(word);
		if (stopwords && isStopword(str, stopwords)) {
			return false;
		}
		cb(str);
		break;
	}
	case ParserToken::XMLEntity:
	case ParserToken::Blank:
		return false;
		break;
	}
	return true;
}

Configuration::Configuration() : Configuration(Language::English) { }

Configuration::Configuration(Language lang)
: _language(lang), _primary(search::getStemmer(_language)), _secondary(search::getStemmer(Language::English)) { }

void Configuration::setLanguage(Language lang) {
	_language = lang;
	_primary = search::getStemmer(_language);
	if (!_secondary) {
		_secondary = search::getStemmer(Language::English);
	}
}

Language Configuration::getLanguage() const {
	return _language;
}

void Configuration::setStemmer(ParserToken tok, StemmerCallback &&cb) {
	_stemmers.emplace(tok, move(cb));
}

Configuration::StemmerCallback Configuration::getStemmer(ParserToken tok) const {
	auto it = _stemmers.find(tok);
	if (it != _stemmers.end()) {
		return it->second;
	}

	return StemmerCallback([&, lang = _language, env = getEnvForToken(tok), stopwords = _customStopwords] (StringView word, const Callback<void(StringView)> &cb) -> bool {
		return stemWordDefault(lang, env, tok, word, cb, stopwords);
	});
}

void Configuration::setCustomStopwords(const StringView *w) {
	_customStopwords = w;
}

const StringView *Configuration::getCustomStopwords() const {
	return _customStopwords;
}

void Configuration::stemPhrase(const StringView &str, const StemWordCallback &cb) const {
	parsePhrase(str, [&] (StringView word, ParserToken tok) {
		stemWord(word, tok, cb);
		return ParserStatus::Continue;
	});
}

size_t Configuration::makeSearchVector(SearchVector &vec, StringView str, SearchData::Rank rank, size_t counter) const {
	if (str.empty()) {
		return counter;
	}

	parsePhrase(str, [&] (StringView word, ParserToken tok) {
		stemWord(word, tok, [&] (StringView w, StringView s, ParserToken tok) {
			if (!s.empty()) {
				auto it = vec.find(s);
				if (it == vec.end()) {
					vec.emplace(s.str(), Vector<Pair<size_t, SearchData::Rank>>({pair(counter + 1, rank)}));
				} else {
					it->second.emplace_back(pair(counter + 1, rank));
				}
			}
		});
		if (tok != ParserToken::Blank) {
			++ counter;
		}
		return ParserStatus::Continue;
	});

	return counter;
}

String Configuration::encodeSearchVector(const SearchVector &vec, SearchData::Rank rank) const {
	StringStream ret;
	for (auto &it : vec) {
		if (!ret.empty()) {
			ret << " ";
		}

		ret << "'" << it.first << "':";
		for (auto &v : it.second) {
			if (ret.weak().back() != ':') { ret << ","; }
			ret << v.first;
			auto r = v.second;
			if (r == SearchData::Unknown) {
				r = rank;
			}
			switch (r) {
			case SearchData::A: ret << 'A'; break;
			case SearchData::B: ret << 'B'; break;
			case SearchData::C: ret << 'C'; break;
			case SearchData::D:
			case SearchData::Unknown:
				break;
			}
		}
	}
	return ret.str();
}

void Configuration::stemHtml(const StringView &str, const StemWordCallback &cb) const {
	parseHtml(str, [&] (StringView str) {
		stemPhrase(str, cb);
	});
}

bool Configuration::stemWord(const StringView &word, ParserToken tok, const StemWordCallback &cb) const {
	auto it = _stemmers.find(tok);
	if (it != _stemmers.end()) {
		return it->second(word, [&] (StringView stem) {
			cb(word, stem, tok);
		});
	} else {
		return stemWordDefault(_language, getEnvForToken(tok), tok, word, [&] (StringView stem) {
			cb(word, stem, tok);
		}, _customStopwords);
	}
}

StemmerEnv *Configuration::getEnvForToken(ParserToken tok) const {
	switch (tok) {
	case ParserToken::AsciiWord:
	case ParserToken::AsciiHyphenatedWord:
	case ParserToken::HyphenatedWord_AsciiPart:
		return _secondary;
		break;
	case ParserToken::Word:
	case ParserToken::HyphenatedWord:
	case ParserToken::HyphenatedWord_Part:
		return _primary;
		break;

	case ParserToken::NumWord:
	case ParserToken::NumHyphenatedWord:
	case ParserToken::HyphenatedWord_NumPart:
	case ParserToken::Email:
	case ParserToken::Url:
	case ParserToken::Version:
	case ParserToken::Path:
	case ParserToken::Integer:
	case ParserToken::Float:
	case ParserToken::ScientificFloat:
	case ParserToken::XMLEntity:
	case ParserToken::Custom:
	case ParserToken::Blank:
		return nullptr;
	}
	return nullptr;
}

Configuration::String Configuration::makeHeadline(const HeadlineConfig &cfg, const StringView &origin, const Vector<String> &stemList) const {
	memory::PoolInterface::StringStreamType result;
	result.reserve(origin.size() + (cfg.startToken.size() + cfg.stopToken.size()) * stemList.size());

	bool isOpen = false;
	StringViewUtf8 r(origin);
	StringView dropSep;

	parsePhrase(origin, [&] (StringView word, ParserToken tok) {
		auto status = ParserStatus::Continue;
		if (tok == ParserToken::Blank || !stemWord(word, tok, [&] (StringView word, StringView stem, ParserToken tok) {
			auto it = std::lower_bound(stemList.begin(), stemList.end(), stem);
			if (it != stemList.end() && *it == stem) {
				if (!isOpen) {
					result << cfg.startToken;
					isOpen = true;
				} else if (!dropSep.empty()) {
					result << dropSep;
					dropSep.clear();
				}
				if (isComplexWord(tok)) {
					status = ParserStatus::PreventSubdivide;
				}
			} else {
				if (isOpen) {
					result << cfg.stopToken;
					isOpen = false;
					if (!dropSep.empty()) {
						result << dropSep;
						dropSep.clear();
					}
				}
				if (isComplexWord(tok)) {
					return;
				}
			}
			result << word;
		})) {
			if (isOpen) {
				if (!dropSep.empty()) {
					dropSep = StringView(dropSep.data(), word.data() + word.size() - dropSep.data());
				} else {
					dropSep = word;
				}
			} else {
				result << word;
			}
		}
		return status;
	});

	if (isOpen) {
		result << cfg.stopToken;
		isOpen = false;
	}

	return result.str();
}

Configuration::String Configuration::makeHtmlHeadlines(const HeadlineConfig &cfg, const StringView &origin, const Vector<String> &stemList, size_t count) const {
	return makeHeadlines(cfg, [&] (const Function<bool(const StringView &, const StringView &)> &cb) {
		Stemmer_Reader r;
		r.callback = [&] (Stemmer_Reader::Parser &p, const StringView &str) {
			if (!cb(str, StringView())) {
				p.cancel();
			}
		};
		html::parse(r, origin, false);
	}, stemList, count);
}

Configuration::String Configuration::makeHeadlines(const HeadlineConfig &cfg, const Callback<void(const Function<bool(const StringView &frag, const StringView &tag)>)> &cb,
		const Vector<String> &stemList, size_t count) const {

	using SplitTokens = StringViewUtf8::MatchCompose<
		StringViewUtf8::MatchCharGroup<CharGroupId::WhiteSpace>,
		StringViewUtf8::MatchChars<'-', u'—', u'\'', u'«', u'»', u'’', u'“', '(', ')', '"', ',', '*', ':', ';', '/', '\\'>>;

	using TrimToken = StringView::Chars<'.'>;

	struct WordIndex {
		StringView word;
		uint16_t index;
		uint16_t selectedCount; // selected words in block
		uint16_t allWordsCount; // all words in block;
		const WordIndex *end;
	};

	WordIndex *topIndex = nullptr;
	StringStream ret; ret.reserve(1_KiB);

	auto rateWord = [&] (WordIndex &index, WordIndex *list, size_t listCount) {
		index.end = &index;
		index.selectedCount = 1;
		index.allWordsCount = 1;
		while (listCount > 0) {
			uint16_t offset = list->index - index.index;
			if (offset < cfg.maxWords) {
				++ index.selectedCount;
				index.allWordsCount = offset;
				index.end = list;
			} else {
				break;
			}

			++ list;
			-- listCount;
		}

		if (!topIndex || index.selectedCount > topIndex->selectedCount
				|| (index.selectedCount == topIndex->selectedCount && index.allWordsCount < topIndex->allWordsCount)) {
			topIndex = &index;
		}
	};

	auto writeFragmentWords = [&] (StringStream &out, const StringView &str, const std::array<WordIndex, 32> &words, const WordIndex *word) {
		bool isOpen = false;
		for (auto it = word; it < word->end; ++ it) {
			if (!isOpen) {
				out << cfg.startToken;
			}
			out << it->word;
			auto next = it + 1;
			if (next <= word->end && it->index + 1 == next->index) {
				isOpen = true;
			} else {
				isOpen = false;
				out << cfg.stopToken;
			}

			if (next <= word->end) {
				out << StringView(it->word.data() + it->word.size(), next->word.data() - (it->word.data() + it->word.size()));
			}
		}

		if (!isOpen) {
			out << cfg.startToken;
		}
		out << word->end->word;
		out << cfg.stopToken;
		isOpen = false;
	};

	auto makeFragmentPrefix = [&] (StringStream &out, const StringView &str, size_t numWords, size_t allWords) {
		if (numWords == allWords) {
			out << str;
			return;
		} else if (numWords == 0) {
			return;
		}

		StringViewUtf8 r(str);
		while (!r.empty() && numWords > 0) {
			r.backwardSkipChars<SplitTokens>();
			auto tmp = StringViewUtf8(r.backwardReadUntil<SplitTokens>());

			auto tmpR = tmp;
			tmpR.trimChars<TrimToken>();

			if (string::getUtf16Length(tmpR) > cfg.shortWord) {
				-- numWords;
			}
		}

		if (!r.empty()) {
			out << cfg.separator << " ";
		}

		out << StringView(r.data() + r.size(), (str.data() + str.size()) - (r.data() + r.size()));
	};

	auto makeFragmentSuffix = [&] (StringStream &out, const StringView &str, size_t numWords, size_t allWords) {
		if (numWords == allWords) {
			out << str;
			return;
		} else if (numWords == 0) {
			return;
		}

		StringViewUtf8 r(str);
		while (!r.empty() && numWords > 0) {
			auto sep = StringViewUtf8(r.readChars<SplitTokens>());
			auto tmp = StringViewUtf8(r.readUntil<SplitTokens>());

			auto tmpR = tmp;
			tmpR.trimChars<TrimToken>();

			out << sep << tmp;

			if (string::getUtf16Length(tmpR) > cfg.shortWord) {
				-- numWords;
			}
		}

		if (!r.empty()) {
			out << " " << cfg.separator;
		}
	};

	auto makeFragment = [&] (StringStream &out, const StringView &str, const StringView &tagId, const std::array<WordIndex, 32> &words, const WordIndex *word, size_t idx) {
		out << cfg.startFragment;
		auto prefixView = StringView(str.data(), word->word.data() - str.data());
		auto suffixView = StringView(word->end->word.data() + word->end->word.size(), str.data() + str.size() - (word->end->word.data() + word->end->word.size()));
		if (idx < cfg.maxWords) {
			// write whole block
			out << prefixView;
			writeFragmentWords(out, str, words, word);
			out << suffixView;
		} else if (word->allWordsCount < cfg.minWords) {
			// make offsets
			size_t availStart = word->index;
			size_t availEnd = idx - word->end->index - 1;
			size_t diff = (cfg.minWords - word->allWordsCount) + 1;

			if (availStart >= diff / 2 && availEnd >= diff / 2) {
				makeFragmentPrefix(out, prefixView, diff / 2, word->index);
				writeFragmentWords(out, str, words, word);
				makeFragmentSuffix(out, suffixView, diff / 2, idx - word->end->index - 1);
			} else if (availStart < diff / 2 && availEnd < diff / 2) {
				out << prefixView;
				writeFragmentWords(out, str, words, word);
				out << suffixView;
			} else if (availStart < diff / 2) {
				out << prefixView;
				writeFragmentWords(out, str, words, word);
				makeFragmentSuffix(out, suffixView, diff - availStart - 1, idx - word->end->index - 1);
			} else if (availEnd < diff / 2) {
				makeFragmentPrefix(out, prefixView, diff - availEnd - 1, word->index);
				writeFragmentWords(out, str, words, word);
				out << suffixView;
			}
		} else {
			// try minimal offsets
			makeFragmentPrefix(out, prefixView, 1, word->index);
			writeFragmentWords(out, str, words, word);
			makeFragmentSuffix(out, suffixView, 1, idx - word->end->index - 1);
		}
		out << cfg.stopFragment;

		if (cfg.fragmentCallback) {
			cfg.fragmentCallback(out.weak(), tagId);
		}
	};

	cb([&] (const StringView &str, const StringView &fragmentTag) -> bool {
		std::array<WordIndex, 32> wordsMatch;
		uint16_t wordCount = 0;
		uint16_t idx = 0;

		bool enabledComplex = false;
		parsePhrase(str, [&] (StringView word, ParserToken tok) {
			auto status = ParserStatus::Continue;
			if (tok != ParserToken::Blank && string::getUtf16Length(word) > cfg.shortWord && wordCount < 32) {
				if (enabledComplex) {
					if (isWordPart(tok)) {
						wordsMatch[wordCount] = WordIndex{word, idx};
						++ wordCount;
						++ idx;
						return status;
					} else {
						enabledComplex = false;
					}
				}
				stemWord(word, tok, [&] (StringView word, StringView stem, ParserToken tok) {
					auto it = std::lower_bound(stemList.begin(), stemList.end(), stem);
					if (it != stemList.end() && string::compareCaseInsensivive(*it, stem) == 0) {
						if (isComplexWord(tok)) {
							enabledComplex = true;
						} else {
							wordsMatch[wordCount] = WordIndex{word, idx};
							++ wordCount;
						}
					}
				});
				++ idx;
			}
			return status;
		});

		if (wordCount == 0) {
			return true;
		}

		for (size_t i = 0; i < wordCount; ++ i) {
			rateWord(wordsMatch[i], &wordsMatch[i + 1], wordCount - 1 - i);
		}

		if (topIndex && count > 0) {
			if (cfg.fragmentCallback) {
				StringStream out;
				makeFragment(out, str, fragmentTag, wordsMatch, topIndex, idx);
				ret << out.weak();
			} else {
				makeFragment(ret, str, fragmentTag, wordsMatch, topIndex, idx);
				-- count;
			}
		}

		if (count == 0) {
			return false;
		}

		topIndex = nullptr;
		return true;
	});
	return ret.str();
}

Configuration::Vector<Configuration::String> Configuration::stemQuery(const Vector<SearchData> &query) const {
	Vector<String> queryList;
	for (auto &it : query) {
		stemPhrase(it.buffer, [&] (StringView word, StringView stem, ParserToken tok) {
			auto it = std::lower_bound(queryList.begin(), queryList.end(), stem);
			if (it == queryList.end()) {
				queryList.emplace_back(stem.str<memory::PoolInterface>());
			} else if (*it != stem) {
				queryList.emplace(it, stem.str<memory::PoolInterface>());
			}
		});
	}
	return queryList;
}

}
