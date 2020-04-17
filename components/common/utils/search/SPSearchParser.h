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

#ifndef COMPONENTS_COMMON_UTILS_SEARCH_SPSEARCHPARSER_H_
#define COMPONENTS_COMMON_UTILS_SEARCH_SPSEARCHPARSER_H_

#include "SPStringView.h"

namespace stappler::search {

/* API based on postgresql full-text, but parser more correct with urls, emails and paths */

enum class ParserToken {
	AsciiWord,
	Word,
	NumWord,
	Email,
	Url,
	ScientificFloat,
	Version, // or ip-address, or some date
	Blank,
	NumHyphenatedWord,
	AsciiHyphenatedWord,
	HyphenatedWord,
	Path,
	Float,
	Integer,
	XMLEntity,
	Custom,
	HyphenatedWord_NumPart,
	HyphenatedWord_Part,
	HyphenatedWord_AsciiPart,
};

enum class UrlToken {
	Scheme,
	User,
	Password,
	Host,
	Port,
	Path,
	Query,
	Fragment,
	Blank,
};

enum class Language {
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

enum ParserStatus {
	Continue = 0, // just continue parsing
	PreventSubdivide = 1, // do not subdivide complex token (works with isComplexWord(ParserToken))
	Stop = 2, // stop parsing in place
};

struct SearchData {
	using String = memory::PoolInterface::StringType;
	using Language = search::Language;

	enum Rank {
		A,
		B,
		C,
		D,
		Unknown,
	};

	enum Type {
		Parse,
		Cast,
	};

	String buffer;
	Language language = Language::Unknown;
	Rank rank = D;
	Type type = Parse;

	StringView getLanguage() const; // compatibility
};

enum class SearchOp {
	None,
	Not,
	And,
	Or,
	Follow,
};

struct SearchQuery {
	enum Block {
		None,
		Parentesis,
		Quoted,
	};

	enum Format {
		Stappler,
		Postgresql,
	};

	using String = memory::PoolInterface::StringType;

	Block block = None;
	size_t offset = 0;
	SearchOp op = SearchOp::None;
	String value;
	StringView source;
	Vector<SearchQuery> args;

	SearchQuery() = default;
	SearchQuery(StringView value, size_t offset = 1, StringView source = StringView());
	SearchQuery(SearchOp, StringView);

	void clear();
	void encode(const Callback<void(StringView)> &, Format = Stappler) const;

	void describe(std::ostream &stream, size_t depth = 0) const;
	void foreach(const Callback<void(StringView value, StringView source)> &) const;
};

struct StemmerEnv;

bool isStopword(const StringView &word, Language lang = Language::Unknown);
bool isStopword(const StringView &word, StemmerEnv *);
bool isStopword(const StringView &word, const StringView *);

StringView getLanguageName(Language);
Language parseLanguage(const StringView &);
Language detectLanguage(const StringView &);

StringView getParserTokenName(ParserToken);

bool isWordPart(ParserToken);
bool isComplexWord(ParserToken);

void parseHtml(StringView, const Callback<void(StringView)> &);

bool parseUrl(StringView &s, const Callback<void(StringViewUtf8, UrlToken)> &cb);
bool parsePhrase(StringView, const Callback<ParserStatus(StringView, ParserToken)> &);

StemmerEnv *getStemmer(Language lang);

bool stemWord(StringView word, const Callback<void(StringView)> &, StemmerEnv *env);
bool stemWord(StringView word, const Callback<void(StringView)> &, Language lang = Language::Unknown);

// lowercase, remove soft hyphens
mem_pool::String normalizeWord(const StringView &str);

}

#endif /* COMPONENTS_COMMON_UTILS_SEARCH_SPSEARCHPARSER_H_ */
