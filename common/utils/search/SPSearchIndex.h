/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_UTILS_SEARCH_SPSEARCHINDEX_H_
#define COMMON_UTILS_SEARCH_SPSEARCHINDEX_H_

#include "SPStringView.h"
#include "SPSearchDistance.h"
#include "SPRef.h"

NS_SP_EXT_BEGIN(search)

class SearchIndex : public Ref {
public:
	using DefaultSep = StringView::Compose<
			StringView::CharGroup<CharGroupId::WhiteSpace>,
			StringView::Chars<'=', '/', '(', ')', '.', ',', '-', '\'', '"', ':', ';', '?', '!', '@', '#', '$', '%', '^', '*', '\\', '+', '[', ']'>
	>;

	struct Slice;
	struct Node;
	struct Token;
	struct ResultToken;
	struct ResultNode;
	struct Result;

	enum TokenType {
		SearchNode,
		SearchRequest
	};

	using TokenCallback = Function<void(const StringView &)>;
	using TokenizerCallback = Function<void(const StringView &, const TokenCallback &, TokenType)>;
	using HeuristicCallback = Function<float(const SearchIndex &index, const ResultNode &result)>;
	using FilterCallback = Function<bool(const Node *)>;

	struct Slice {
		uint16_t start = 0; // start position in node's canonical string
		uint16_t size = 0; // length in node's canonical string
	};

	struct Node {
		int64_t id = 0;
		int64_t tag = 0;
		String canonical;
		Distance alignment;
	};

	struct Token {
		uint32_t index = 0; // node index
		Slice slice; // slice from canonical
	};

	struct ResultToken {
		uint32_t word = 0; // node index
		uint16_t match = 0; // node index
		Slice slice; // slice from canonical
	};

	struct ResultNode {
		float score = 0.0f;
		const Node *node;
		Vector<ResultToken> matches;
	};

	struct Result {
		Rc<SearchIndex> ref;
		Vector<ResultNode> nodes;
	};

	struct Heuristic {
		using TagCallback = Function<float(int64_t)>;
		using SizeCallback = Function<float(uint32_t, uint32_t)>;

		Heuristic() { }
		Heuristic(const TagCallback &cb, bool exclude = true) : excludeEqualMatches(exclude), tagScore(cb) { }

		float operator ()(const SearchIndex &index, const SearchIndex::ResultNode &result);

		bool excludeEqualMatches = true;

		// for every full token match score is incremented to fullMatchScore / countOfMatches
		float fullMatchScore = 2.0f;

		// all token score is multiplied by score of it's node tag
		TagCallback tagScore = [] (int64_t tag) -> float {
			return 1.0f;
		};

		// token score is better for longer word
		SizeCallback wordScore = [] (uint32_t match, uint32_t size) -> float {
			return (1.0f + log2f(size)) * (float(match) / float(size));
		};

		// token score is better if tokens match sequentially
		SizeCallback positionScore = [] (uint32_t prev, uint32_t current) -> float {
			if (prev != maxOf<uint32_t>()) {
				return (prev + 1 == current)?1.0f:((prev + 2 == current)?0.5f:0.0f);
			}
			return 0.0f;
		};
	};

	bool init(const TokenizerCallback & = nullptr);

	void reserve(size_t);
	void add(const StringView &, int64_t id, int64_t tag);

	Result performSearch(const StringView &, size_t minMatch, const HeuristicCallback & = Heuristic(),
			const FilterCallback & filter = nullptr);

	StringView resolveToken(const Node &, const ResultToken &) const;
	Slice convertToken(const Node &, const ResultToken &) const;

	void print() const;

protected:
	StringView makeStringView(const Token &) const;
	StringView makeStringView(uint32_t idx, const Slice &) const;

	void onToken(Vector<Token> &vec, const StringView &, uint32_t, const Slice &);

	Vector<Node> _nodes;
	Vector<Token> _tokens;
	TokenizerCallback _tokenizer;
};

NS_SP_EXT_END(search)

#endif /* COMMON_UTILS_SEARCH_SPSEARCHINDEX_H_ */
