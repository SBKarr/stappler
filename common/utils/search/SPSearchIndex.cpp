// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPCommon.h"
#include "SPSearchIndex.h"
#include "SPString.h"

NS_SP_EXT_BEGIN(search)

bool SearchIndex::init(const TokenizerCallback &tcb) {
	_tokenizer = tcb;
	return true;
}

void SearchIndex::reserve(size_t s) {
	_nodes.reserve(s);
}

void SearchIndex::add(const StringView &v, int64_t id, int64_t tag) {
	String origin(string::tolower(v));

	_nodes.emplace_back(Node{id, tag});

	Node &node = _nodes.back();
	uint32_t idx = uint32_t(_nodes.size() - 1);
	auto &canonical = node.canonical;

	auto tokenFn = [&] (const StringView &str) {
		if (!str.empty()) {
			if (!canonical.empty()) {
				canonical.append(" ");
			}
			auto s = canonical.size();
			canonical.append(str.str());
			onToken(_tokens, str, idx, Slice{ uint16_t(s), uint16_t(str.size()) });
		}
	};

	if (_tokenizer) {
		_tokenizer(origin, tokenFn);
	} else {
		StringView r(origin);
		r.split<DefaultSep>(tokenFn);
	}

	if (canonical.empty()) {
		_nodes.pop_back();
	} else if (canonical != origin) {
		node.alignment = Distance(origin, canonical);
	}

	std::cout << v << "\n" << node.canonical << "\n" << node.alignment.info() << "\n";
}

SearchIndex::Result SearchIndex::performSearch(const StringView &v, size_t minMatch, const HeuristicCallback &cb) {
	String origin(string::tolower(v));

	SearchIndex::Result res{this};

	uint32_t wordIndex = 0;

	auto tokenFn = [&] (const StringView &str) {
		//std::cout << "Token: " << str << "\n";
		auto lb = std::lower_bound(_tokens.begin(), _tokens.end(), str, [&] (const Token &l, const StringView &r) {
			return string::compare(makeStringView(l.index, l.slice), r) < 0;
		});

		if (lb != _tokens.end()) {
			auto node = &_nodes.at(lb->index);
			StringView value = makeStringView(*lb);
			//std::cout << "Found: '" << value << "' from '" << _nodes.at(lb->index).canonical << "'\n";

			while (lb != _tokens.end() && value.size() >= str.size() && String::traits_type::compare(value.data(), str.data(), str.size()) == 0) {
				auto ret_it = std::lower_bound(res.nodes.begin(), res.nodes.end(), node,
						[&] (const ResultNode &l, const Node *r) {
					return l.node < r;
				});
				if (ret_it == res.nodes.end() || ret_it->node != node) {
					res.nodes.emplace(ret_it, ResultNode{ 0.0f, node, {ResultToken{wordIndex, uint16_t(str.size()), lb->slice}} });
				} else {
					ret_it->matches.emplace_back(ResultToken{wordIndex, uint16_t(str.size()), lb->slice});
				}
				++ lb;
				if (lb != _tokens.end()) {
					value = makeStringView(*lb);
					node = &_nodes.at(lb->index);
					//std::cout << "Next: '" << value << "' from '" << _nodes.at(lb->index).canonical << "'\n";
				}
			}
		}
		wordIndex ++;
	};

	if (_tokenizer) {
		_tokenizer(origin, tokenFn);
	} else {
		StringView r(origin);
		r.split<DefaultSep>(tokenFn);
	}

	if (cb) {
		for (auto &it : res.nodes) {
			it.score = cb(*this, it);
		}

		std::sort(res.nodes.begin(), res.nodes.end(), [] (const ResultNode &l, const ResultNode &r) {
			return l.score > r.score;
		});
	}

	return res;
}

StringView SearchIndex::resolveToken(const Node &node, const ResultToken &token) const {
	return StringView(node.canonical.data() + token.slice.start, token.match);
}

SearchIndex::Slice SearchIndex::convertToken(const Node &node, const ResultToken &ret) const {
	if (node.alignment.empty()) {
		return Slice{ret.slice.start, ret.match};
	} else {
		auto start = ret.slice.start + node.alignment.diff_original(ret.slice.start);
		auto end = ret.slice.start + ret.match;
		end += node.alignment.diff_original(end, true);
		return Slice{uint16_t(start), uint16_t(end - start)};
	}
}

void SearchIndex::print() const {
	for (auto &it : _tokens) {
		std::cout << it.index << " " << makeStringView(it) << " " << _nodes.at(it.index).id << "\n";
	}
}

StringView SearchIndex::makeStringView(const Token &t) const {
	return makeStringView(t.index, t.slice);
}

StringView SearchIndex::makeStringView(uint32_t idx, const Slice &sl) const {
	const Node &node = _nodes.at(idx);
	return StringView(node.canonical.data() + sl.start, sl.size);
}

void SearchIndex::onToken(Vector<Token> &vec, const StringView &rep, uint32_t idx, const Slice &sl) {
	auto insert_it = std::lower_bound(vec.begin(), vec.end(), rep, [&] (const Token &l, const StringView &r) {
		return string::compare(makeStringView(l.index, l.slice), r) < 0;
	});
	if (insert_it ==  vec.end()) {
		vec.emplace_back(Token{idx, sl});
	} else {
		vec.emplace(insert_it, Token{idx, sl});
	}
}

float SearchIndex::Heuristic::operator () (const SearchIndex &index, const SearchIndex::ResultNode &node) {
	float score = 0.0f;
	uint32_t idx = maxOf<uint32_t>();

	Vector<StringView> matches;

	float mod = tagScore(node.node->tag);

	size_t fullMatches = 0;
	float fullMatchCost = fullMatchScore / float(node.matches.size());
	for (const SearchIndex::ResultToken &token_it : node.matches) {
		if (excludeEqualMatches) {
			auto t = index.resolveToken(*node.node, token_it);
			if (std::find(matches.begin(), matches.end(), t) == matches.end()) {
				matches.push_back(t);
			} else {
				continue;
			}
		}

		if (token_it.match == token_it.slice.size) {
			score += mod * fullMatchCost;
			++ fullMatches;
		}

		score += mod * wordScore(token_it.match, token_it.slice.size);
		score += mod * positionScore(idx, token_it.word);
		idx = token_it.word;
	}
	return score;
}

NS_SP_EXT_END(search)
