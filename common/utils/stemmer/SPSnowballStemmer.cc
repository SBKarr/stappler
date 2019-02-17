/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPSnowballStemmer.h"

NS_SP_EXT_BEGIN(search)

struct SN_alloc {
	void *(*memalloc)( void *userData, unsigned int size );
	void (*memfree)( void *userData, void *ptr );
	void* userData;	// User data passed to the allocator functions.
};

struct stemmer_modules {
	Stemmer::Language name;
	StemmerData * (*create)(StemmerData *);
	void (*close)(StemmerData *, SN_alloc *);
	int (*stem)(StemmerData *);
};

SP_EXTERN_C struct stemmer_modules * sb_stemmer_get(Stemmer::Language lang);
SP_EXTERN_C const unsigned char * sb_stemmer_stem(StemmerData * z, const unsigned char * word, int size);

static void * staticPoolAlloc(void* userData, unsigned int size) {
	memory::pool_t *pool = (memory::pool_t *)userData;
	size_t s = size;
	auto mem = memory::pool::alloc(pool, s);
	memset(mem,0, s);
	return mem;
}

static void staticPoolFree(void * userData, void * ptr) { }

static const StringView *getLanguageStopwords(Stemmer::Language lang) {
	switch (lang) {
	case Stemmer::Language::Unknown: return nullptr; break;
	case Stemmer::Language::Arabic: return nullptr; break;
	case Stemmer::Language::Danish: return s_danish_stopwords; break;
	case Stemmer::Language::Dutch: return s_dutch_stopwords; break;
	case Stemmer::Language::English: return s_english_stopwords; break;
	case Stemmer::Language::Finnish: return s_finnish_stopwords; break;
	case Stemmer::Language::French: return s_french_stopwords; break;
	case Stemmer::Language::German: return s_german_stopwords; break;
	case Stemmer::Language::Greek: return nullptr; break;
	case Stemmer::Language::Hungarian: return s_hungarian_stopwords; break;
	case Stemmer::Language::Indonesian: return nullptr; break;
	case Stemmer::Language::Irish: return nullptr; break;
	case Stemmer::Language::Italian: return s_italian_stopwords; break;
	case Stemmer::Language::Lithuanian: return nullptr; break;
	case Stemmer::Language::Nepali: return s_nepali_stopwords; break;
	case Stemmer::Language::Norwegian: return s_norwegian_stopwords; break;
	case Stemmer::Language::Portuguese: return s_portuguese_stopwords; break;
	case Stemmer::Language::Romanian: return nullptr; break;
	case Stemmer::Language::Russian: return s_russian_stopwords; break;
	case Stemmer::Language::Spanish: return s_spanish_stopwords; break;
	case Stemmer::Language::Swedish: return s_swedish_stopwords; break;
	case Stemmer::Language::Tamil: return nullptr; break;
	case Stemmer::Language::Turkish: return s_turkish_stopwords; break;
	case Stemmer::Language::Simple: return nullptr; break;
	}
	return nullptr;
}

StringView Stemmer::getLanguageString(Language lang) {
	switch (lang) {
	case Unknown: return StringView(); break;
	case Arabic: return StringView("arabic"); break;
	case Danish: return StringView("danish"); break;
	case Dutch: return StringView("dutch"); break;
	case English: return StringView("english"); break;
	case Finnish: return StringView("finnish"); break;
	case French: return StringView("french"); break;
	case German: return StringView("german"); break;
	case Greek: return StringView("greek"); break;
	case Hungarian: return StringView("hungarian"); break;
	case Indonesian: return StringView("indonesian"); break;
	case Irish: return StringView("irish"); break;
	case Italian: return StringView("italian"); break;
	case Lithuanian: return StringView("lithuanian"); break;
	case Nepali: return StringView("nepali"); break;
	case Norwegian: return StringView("norwegian"); break;
	case Portuguese: return StringView("portuguese"); break;
	case Romanian: return StringView("romanian"); break;
	case Russian: return StringView("russian"); break;
	case Spanish: return StringView("spanish"); break;
	case Swedish: return StringView("swedish"); break;
	case Tamil: return StringView("tamil"); break;
	case Turkish: return StringView("turkish"); break;
	case Simple: return StringView("simple"); break;
	}
	return StringView();
}

Stemmer::Language Stemmer::parseLanguage(const StringView &lang) {
	if (lang == "arabic") { return Arabic; }
	else if (lang == "danish") { return Danish; }
	else if (lang == "dutch") { return Dutch; }
	else if (lang == "english") { return English; }
	else if (lang == "finnish") { return Finnish; }
	else if (lang == "french") { return French; }
	else if (lang == "german") { return German; }
	else if (lang == "greek") { return Greek; }
	else if (lang == "hungarian") { return Hungarian; }
	else if (lang == "indonesian") { return Indonesian; }
	else if (lang == "irish") { return Irish; }
	else if (lang == "italian") { return Italian; }
	else if (lang == "nepali") { return Nepali; }
	else if (lang == "norwegian") { return Norwegian; }
	else if (lang == "portuguese") { return Portuguese; }
	else if (lang == "romanian") { return Romanian; }
	else if (lang == "russian") { return Russian; }
	else if (lang == "spanish") { return Spanish; }
	else if (lang == "swedish") { return Swedish; }
	else if (lang == "tamil") { return Tamil; }
	else if (lang == "turkish") { return Turkish; }
	else if (lang == "simple") { return Simple; }
	return Unknown;
}

bool Stemmer::isStopword(const StringView &word, Language lang) const {
	if (lang == Unknown) {
		lang = detectWordLanguage(word);
		if (lang == Unknown) {
			return false;
		}
	}

	if (_stopwordsEnabled) {
		if (auto dict = getLanguageStopwords(lang)) {
			while (dict && !dict->empty()) {
				if (word == *dict) {
					return true;
				} else {
					++ dict;
				}
			}
		}
	}

	if (_filterCallback) {
		return _filterCallback(word, lang);
	}
	return false;
}

void Stemmer::setHighlightParams(const StringView &start, const StringView &stop) {
	_startToken = start.str<memory::PoolInterface>();
	_stopToken = stop.str<memory::PoolInterface>();
}

StringView Stemmer::getHighlightStart() const {
	return _startToken;
}

StringView Stemmer::getHighlightStop() const {
	return _stopToken;
}

void Stemmer::setFragmentParams(const StringView &start, const StringView &stop) {
	_startFragment = start.str<memory::PoolInterface>();
	_stopFragment = stop.str<memory::PoolInterface>();
}

StringView Stemmer::getFragmentStart() const {
	return _startFragment;
}
StringView Stemmer::getFragmentStop() const {
	return _stopToken;
}

void Stemmer::setFragmentMaxWords(size_t value) {
	_maxWords = value;
}
size_t Stemmer::getFragmentMaxWords() const {
	return _maxWords;
}

void Stemmer::setFragmentMinWords(size_t value) {
	_minWords = value;
}
size_t Stemmer::getFragmentMinWords() const {
	return _minWords;
}

void Stemmer::setFragmentShortWord(size_t value) {
	_shortWord = value;
}
size_t Stemmer::getFragmentShortWord() const {
	return _shortWord;
}

void Stemmer::setFragmentCallback(const Function<void(const StringView &, const StringView &)> &cb) {
	_fragmentCallback = cb;
}

Stemmer::Vector<Stemmer::String> Stemmer::stemQuery(const StringView &query, Language lang) {
	Vector<String> queryList; queryList.reserve(1_KiB / sizeof(String));
	stemPhrase(query, [&] (const StringView &word, const StringView &str, Language) {
		auto it = std::lower_bound(queryList.begin(), queryList.end(), str);
		if (it == queryList.end()) {
			queryList.emplace_back(str.str<memory::PoolInterface>());
		} else if (*it != str) {
			queryList.emplace(it, str.str<memory::PoolInterface>());
		}
	}, lang);
	return queryList;
}

Stemmer::Vector<Stemmer::String> Stemmer::stemQuery(const Vector<SearchData> &query) {
	Vector<String> queryList; queryList.reserve(1_KiB / sizeof(String));
	for (auto &it : query) {
		stemPhrase(it.buffer, [&] (const StringView &word, const StringView &str, Language) {
			auto it = std::lower_bound(queryList.begin(), queryList.end(), str);
			if (it == queryList.end()) {
				queryList.emplace_back(str.str<memory::PoolInterface>());
			} else if (*it != str) {
				queryList.emplace(it, str.str<memory::PoolInterface>());
			}
		}, it.language);
	}
	return queryList;
}

Stemmer::Stemmer() : _alloc{&staticPoolAlloc, &staticPoolFree, memory::pool::acquire()} { }

void Stemmer::setLanguageCallback(const LanguageCallback &cb) {
	_languageCallback = cb;
}
const Stemmer::LanguageCallback & Stemmer::getLanguageCallback() const {
	return _languageCallback;
}

void Stemmer::setFilterCallback(const FilterCallback &cb) {
	_filterCallback = cb;
}
const Stemmer::FilterCallback & Stemmer::getFilterCallback() const {
	return _filterCallback;
}

void Stemmer::setStopwordsEnabled(bool value) {
	_stopwordsEnabled = value;
}

bool Stemmer::isStopwordsEnabled() const {
	return _stopwordsEnabled;
}

StringView Stemmer::stemWord(const StringView &word, Language lang) {
	if (lang == Unknown) {
		lang = detectWordLanguage(word);
		if (lang == Unknown) {
			return StringView();
		}
	}

	if (lang == Simple) {
		return word;
	}

	if (auto stemmer = getStemmer(lang)) {
		if (!isStopword(word, lang)) {
			auto w = sb_stemmer_stem(stemmer, (const unsigned char *)word.data(), int(word.size()));
			return StringView((const char *)w,  size_t(stemmer->l));
		}
	}

	return StringView();
}

Stemmer::Language Stemmer::detectWordLanguage(const StringView &word) const {
	if (_languageCallback) {
		return _languageCallback(word);
	}

	return detectWordDefaultLanguage(word);
}

StemmerData *Stemmer::getStemmer(Language lang) {
	auto it = _stemmers.find(lang);
	if (it == _stemmers.end()) {
		auto mod = sb_stemmer_get(lang);

		auto it = _stemmers.emplace(lang, StemmerData{}).first;
		it->second.alloc = (SN_alloc *)&_alloc;
		if (auto env = mod->create(&it->second)) {
			env->stem = mod->stem;
			return env;
		} else {
			_stemmers.erase(it);
		}
		return nullptr;
	} else {
		return &it->second;
	}
}

Stemmer::String Stemmer::makeHighlight(const StringView &origin, const Vector<String> &queryList, Language lang) {
	memory::PoolInterface::StringStreamType result;
	result.reserve(origin.size() + (_startToken.size() + _stopToken.size()) * queryList.size());

	bool isOpen = false;
	StringViewUtf8 r(origin);
	StringView dropSep;
	while (!r.empty()) {
		auto sep = StringView(r.readChars<SplitTokens>());
		if (!dropSep.empty()) {
			sep = StringView(dropSep.data(), dropSep.size() + sep.size());
			dropSep = StringView();
		}

		auto tmp = StringView(r.readUntil<SplitTokens>());

		auto tmpR = tmp;
		tmpR.trimChars<TrimToken>();

		if (!tmpR.empty()) {
			if (tmpR.data() != tmp.data()) {
				sep = StringView(sep.data(), sep.size() + (tmpR.data() - tmp.data()));
			}

			if (tmpR.data() + tmpR.size() != tmp.data() + tmp.size()) {
				dropSep = StringView(tmpR.data() + tmpR.size(), tmp.data() + tmp.size() - (tmpR.data() + tmpR.size()));
			}

			auto stem = stemWord(tmp, lang);
			auto it = std::lower_bound(queryList.begin(), queryList.end(), stem, [] (const String &l, const StringView &r) {
				return string::compareCaseInsensivive(l, r) < 0;
			});
			if (it != queryList.end() && string::compareCaseInsensivive(*it, stem) == 0) {
				result << sep;
				if (!isOpen) {
					result << _startToken;
					isOpen = true;
				}
			} else {
				if (isOpen) {
					result << _stopToken;
					isOpen = false;
				}
				result << sep;
			}
			result << tmpR;
		} else if (!tmp.empty()) {
			dropSep = StringView(sep.data(), sep.size() + (tmp.size()));
		} else {
			if (isOpen) {
				result << _stopToken;
				isOpen = false;
			}
			result << sep;
		}
	}

	if (isOpen) {
		result << _stopToken;
		isOpen = false;
	}

	return result.str();
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

Stemmer::String Stemmer::makeHtmlHeadlines(const StringView &origin, const Vector<String> &queryList, size_t count, Language lang) {
	return makeProducerHeadlines([&] (const Function<bool(const StringView &, const StringView &)> &cb) {
		Stemmer_Reader r;
		r.callback = [&] (Stemmer_Reader::Parser &p, const StringView &str) {
			if (!cb(str, StringView())) {
				p.cancel();
			}
		};
		html::parse(r, origin, false);
	}, queryList, lang);
}

String Stemmer::makeProducerHeadlines(const Callback<void(const Function<bool(const StringView &frag, const StringView &tag)>)> &cb,
		const Vector<String> &queryList, size_t count, Language lang) {

	struct WordIndex {
		StringView word;
		uint16_t index;
		uint16_t selectedCount; // selected words i block
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
			if (offset < _maxWords) {
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
				out << _startToken;
			}
			out << it->word;
			auto next = it + 1;
			if (next <= word->end && it->index + 1 == next->index) {
				isOpen = true;
			} else {
				isOpen = false;
				out << _stopToken;
			}

			if (next <= word->end) {
				out << StringView(it->word.data() + it->word.size(), next->word.data() - (it->word.data() + it->word.size()));
			}
		}

		if (!isOpen) {
			out << _startToken;
		}
		out << word->end->word;
		out << _stopToken;
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

			if (string::getUtf16Length(tmpR) > _shortWord) {
				-- numWords;
			}
		}

		if (!r.empty()) {
			out << "… ";
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

			if (string::getUtf16Length(tmpR) > _shortWord) {
				-- numWords;
			}
		}

		if (!r.empty()) {
			out << " …";
		}
	};

	auto makeFragment = [&] (StringStream &out, const StringView &str, const StringView &tagId, const std::array<WordIndex, 32> &words, const WordIndex *word, size_t idx) {
		out << _startFragment;
		auto prefixView = StringView(str.data(), word->word.data() - str.data());
		auto suffixView = StringView(word->end->word.data() + word->end->word.size(), str.data() + str.size() - (word->end->word.data() + word->end->word.size()));
		if (idx < _maxWords) {
			// write whole block
			out << prefixView;
			writeFragmentWords(out, str, words, word);
			out << suffixView;
		} else if (word->allWordsCount < _minWords) {
			// make offsets
			size_t availStart = word->index;
			size_t availEnd = idx - word->end->index - 1;
			size_t diff = (_minWords - word->allWordsCount) + 1;

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
		out << _stopFragment;

		if (_fragmentCallback) {
			_fragmentCallback(out.weak(), tagId);
		}
	};

	cb([&] (const StringView &str, const StringView &fragmentTag) -> bool {
		std::array<WordIndex, 32> wordsMatch;
		uint16_t wordCount = 0;
		uint16_t idx = 0;

		stemPhrase(str, [&] (const StringView &word, const StringView &stem, Language lang) {
			if (string::getUtf16Length(word) > _shortWord) {
				if (wordCount < 32) {
					auto it = std::lower_bound(queryList.begin(), queryList.end(), stem, [] (const String &l, const StringView &r) {
						return string::compareCaseInsensivive(l, r) < 0;
					});
					if (it != queryList.end() && string::compareCaseInsensivive(*it, stem) == 0) {
						wordsMatch[wordCount] = WordIndex{word, idx};
						++ wordCount;
					}
				}
				++ idx;
			}
		}, lang);

		if (wordCount == 0) {
			return true;
		}

		for (size_t i = 0; i < wordCount; ++ i) {
			rateWord(wordsMatch[i], &wordsMatch[i + 1], wordCount - 1 - i);
		}

		if (topIndex && count > 0) {
			makeFragment(ret, str, fragmentTag, wordsMatch, topIndex, idx);
			-- count;
		}

		if (count == 0) {
			return false;
		}

		topIndex = nullptr;
		return true;
	});
	return ret.str();
}

void Stemmer::splitHtmlText(const StringView &data, const Function<void(const StringView &)> &cb) {
	Stemmer_Reader r;
	r.callback = [&] (Stemmer_Reader::Parser &p, const StringView &str) {
		cb(str);
	};
	html::parse(r, data, false);
}

Stemmer::Language Stemmer::detectWordDefaultLanguage(const StringView &word) {
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
				return English;
			} else if (r.is<StringViewUtf8::MatchCharGroup<CharGroupId::Cyrillic>>()) {
				return Russian;
			} else if (r.is<StringViewUtf8::MatchCharGroup<CharGroupId::GreekBasic>>()) {
				return Greek;
			} else if (r.is<StringViewUtf8::MatchCharGroup<CharGroupId::Numbers>>()) {
				return Simple;
			}
		}
		return Unknown;
	} else {
		return Simple;
	}
}

StringView SearchData::getLanguageString(Language lang) {
	return search::Stemmer::getLanguageString(lang);
}

StringView SearchData::getLanguageString() const {
	return search::Stemmer::getLanguageString(language);
}

NS_SP_EXT_END(search)
