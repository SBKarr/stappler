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

#ifndef COMMON_UTILS_STEMMER_SPSNOWBALLSTEMMER_H_
#define COMMON_UTILS_STEMMER_SPSNOWBALLSTEMMER_H_

#include "SPStringView.h"

NS_SP_EXT_BEGIN(search)

struct SN_alloc;

struct StemmerData {
	using symbol = unsigned char;

	SN_alloc *alloc;
	int (*stem)(StemmerData *);

	symbol * p;
	int c; int l; int lb; int bra; int ket;
	symbol * * S;
	int * I;
	unsigned char * B;
};

struct SearchData;

class Stemmer : public memory::AllocPool {
public:
	enum Language : int {
		Unknown = 0,
		Arabic,
		Danish,
		Dutch,
		English,
		Finnish,
		French,
		German,
		Greek,
		Hungarian,
		Indonesian,
		Irish,
		Italian,
		Lithuanian,
		Nepali,
		Norwegian,
		Portuguese,
		Romanian,
		Russian,
		Spanish,
		Swedish,
		Tamil,
		Turkish,
		Simple
	};

	using LanguageCallback = Function<Language(const StringView &)>;
	using FilterCallback = Function<bool(const StringView &, Language)>;

	template <typename K, typename V>
	using Map = memory::PoolInterface::MapType<K, V>;

	template <typename T>
	using Vector = memory::PoolInterface::VectorType<T>;

	template <typename T>
	using Set = memory::PoolInterface::SetType<T>;

	using String = memory::PoolInterface::StringType;
	using StringStream = memory::PoolInterface::StringStreamType;

	using SplitTokens = StringViewUtf8::MatchCompose<
		StringViewUtf8::MatchCharGroup<CharGroupId::WhiteSpace>,
		StringViewUtf8::MatchChars<'-', u'—', u'\'', u'«', u'»', u'’', u'“', '(', ')', '"', ',', '*', ':', ';', '/', '\\'>>;

	using TrimToken = StringView::Chars<'.'>;

public:
	static constexpr size_t DefaultMaxWords = 24;
	static constexpr size_t DefaultMinWords = 12;
	static constexpr size_t DefaultShortWord = 3;

	static StringView getLanguageString(Language);
	static Language parseLanguage(const StringView &);

	// split phrase
	template <typename Callback>
	static void splitPhrase(const StringView &, const Callback &);

	static void splitHtmlText(const StringView &, const Function<void(const StringView &)> &);

	Stemmer();

	// custom language detection used, when language specified as Unknown
	void setLanguageCallback(const LanguageCallback &);
	const LanguageCallback & getLanguageCallback() const;

	void setFilterCallback(const FilterCallback &);
	const FilterCallback & getFilterCallback() const;

	// control embedded stopwords dictionaries
	void setStopwordsEnabled(bool);
	bool isStopwordsEnabled() const;

	template <typename Callback>
	void stemPhrase(const StringView &, const Callback &, Language = Unknown);

	StringView stemWord(const StringView &, Language = Unknown);
	Language detectWordLanguage(const StringView &) const;

	bool isStopword(const StringView &, Language = Unknown) const;

	void setHighlightParams(const StringView &start, const StringView &stop);
	StringView getHighlightStart() const;
	StringView getHighlightStop() const;

	void setFragmentParams(const StringView &start, const StringView &stop);
	StringView getFragmentStart() const;
	StringView getFragmentStop() const;

	void setFragmentMaxWords(size_t);
	size_t getFragmentMaxWords() const;

	void setFragmentMinWords(size_t);
	size_t getFragmentMinWords() const;

	void setFragmentShortWord(size_t);
	size_t getFragmentShortWord() const;

	Vector<String> stemQuery(const StringView &query, Language = Unknown);
	Vector<String> stemQuery(const Vector<SearchData> &query);

	String makeHighlight(const StringView &origin, const Vector<String> &query, Language = Unknown);
	String makeHtmlHeadlines(const StringView &origin, const Vector<String> &query, size_t count = 1, Language = Unknown);

protected:
	struct Alloc {
		void *(*memalloc)( void *userData, unsigned int size );
		void (*memfree)( void *userData, void *ptr );
		memory::pool_t * userData;
	};

	StemmerData *getStemmer(Language);

	Alloc _alloc;
	Map<Language, StemmerData> _stemmers;

	bool _stopwordsEnabled = true;
	LanguageCallback _languageCallback;
	FilterCallback _filterCallback;

	String _startToken;
	String _stopToken;

	String _startFragment;
	String _stopFragment;

	size_t _maxWords = DefaultMaxWords;
	size_t _minWords = DefaultMinWords;
	size_t _shortWord = DefaultShortWord;
};

struct SearchData {
	enum Rank {
		A,
		B,
		C,
		D
	};

	using Language = Stemmer::Language;

	String buffer;
	Language language = Language::Unknown;
	Rank rank = D;

	static StringView getLanguageString(Language);
	StringView getLanguageString() const;
};

template <typename Callback>
void Stemmer::splitPhrase(const StringView &words, const Callback &cb) {
	StringViewUtf8(words).split<SplitTokens>([&] (StringViewUtf8 word) {
		StringView r(word);
		r.trimChars<TrimToken>();
		if (!r.empty()) {
			cb(r);
		}
	});
}

template <typename Callback>
void Stemmer::stemPhrase(const StringView &words, const Callback &cb, Language ilang) {
	splitPhrase(words, [&] (const StringView &word) {
		String w = string::tolower<memory::PoolInterface>(word);

		auto lang = ilang;
		if (lang == Unknown) {
			lang = detectWordLanguage(w);
			if (lang == Unknown) {
				return;
			}
		}

		auto ret = stemWord(w, lang);
		if (!ret.empty()) {
			cb(word, ret, lang);
		}
	});
}

NS_SP_EXT_END(search)

#endif /* COMMON_UTILS_STEMMER_SPSNOWBALLSTEMMER_H_ */
