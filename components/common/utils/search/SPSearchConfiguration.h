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

#ifndef COMPONENTS_COMMON_UTILS_SEARCH_SPSEARCHCONFIGURATION_H_
#define COMPONENTS_COMMON_UTILS_SEARCH_SPSEARCHCONFIGURATION_H_

#include "SPSearchParser.h"

namespace stappler::search {

class Configuration : public memory::AllocPool {
public:
	template <typename K, typename V>
	using Map = memory::PoolInterface::MapType<K, V>;

	template <typename T>
	using Vector = memory::PoolInterface::VectorType<T>;

	template <typename T>
	using Set = memory::PoolInterface::SetType<T>;

	using String = memory::PoolInterface::StringType;
	using StringStream = memory::PoolInterface::StringStreamType;

	using StemmerCallback = Function<bool(StringView, const Callback<void(StringView)> &)>;
	using StemWordCallback = Callback<void(StringView, StringView, ParserToken)>;

	struct HeadlineConfig {
		static constexpr size_t DefaultMaxWords = 24;
		static constexpr size_t DefaultMinWords = 12;
		static constexpr size_t DefaultShortWord = 3;

		StringView startToken = StringView("<b>");
		StringView stopToken = StringView("</b>");

		StringView startFragment = StringView("<div>");;
		StringView stopFragment = StringView("</div>");
		StringView separator = StringView("…");

		size_t maxWords = DefaultMaxWords;
		size_t minWords = DefaultMinWords;
		size_t shortWord = DefaultShortWord;

		Function<void(StringView, StringView)> fragmentCallback;
	};

public:
	Configuration();
	Configuration(Language);

	void setLanguage(Language);
	Language getLanguage() const;

	void setStemmer(ParserToken, StemmerCallback &&);
	StemmerCallback getStemmer(ParserToken) const;

	void stemPhrase(const StringView &, const StemWordCallback &) const;
	void stemHtml(const StringView &, const StemWordCallback &) const;

	bool stemWord(const StringView &, ParserToken, const StemWordCallback &) const;

	String makeHeadline(const HeadlineConfig &, const StringView &origin, const Vector<String> &stemList) const;

	String makeHtmlHeadlines(const HeadlineConfig &, const StringView &origin, const Vector<String> &stemList, size_t count = 1) const;

	String makeHeadlines(const HeadlineConfig &, const Callback<void(const Function<bool(const StringView &frag, const StringView &tag)>)> &producer,
			const Vector<String> &stemList, size_t count = 1) const;

	Vector<String> stemQuery(const Vector<SearchData> &query) const;

protected:
	StemmerEnv *getEnvForToken(ParserToken) const;

	Language _language = Language::Simple;
	StemmerEnv *_primary = nullptr;
	StemmerEnv *_secondary = nullptr;

	Map<ParserToken, StemmerCallback> _stemmers;
};

}

#endif /* COMPONENTS_COMMON_UTILS_SEARCH_SPSEARCHCONFIGURATION_H_ */
