/*

	Copyright © 2015-2017 Fletcher T. Penney.
	Copyright © 2017 Roman Katuntsev <sbkarr@stappler.org>

	The `c-template` project is released under the MIT License.

	GLibFacade.c and GLibFacade.h are from the MultiMarkdown v4 project:

		https://github.com/fletcher/MultiMarkdown-4/

	MMD 4 is released under both the MIT License and GPL.


	CuTest is released under the zlib/libpng license. See CuTest.c for the text
	of the license.


	## The MIT License ##

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

*/

#include "SPCommon.h"
#include "MMDAhoCorasick.h"

NS_MMD_BEGIN

Trie::Node::Node() {
	memset(this, 0, sizeof(Node));
}

Trie::Trie(size_t startingSize) {
	if (startingSize <= 1) {
		startingSize = kTrieStartingSize;
	}

	nodes.reserve(startingSize);
	nodes.resize(1);
}

bool Trie::insert(const char *key, uint8_t match_type) {
	if (key && (key[0] != '\0')) {
		return insertNode(0, (const unsigned char *)key, match_type, 0);
	}

	return false;
}

void Trie::toGraphviz() const {
	fprintf(stderr, "digraph dfa {\n");

	size_t s = 0;
	for (auto &it : nodes) {
		if (it.match_type) {
			fprintf(stderr, "\"%zu\" [shape=doublecircle]\n", s);
		}

		for (int i = 0; i < 256; ++i) {
			if (it.child[i]) {
				switch (i) {
					default:
						fprintf(stderr, "\"%zu\" -> \"%zu\" [label=\"%c\"]\n", s, it.child[i], (char)i);
				}
			}
		}

		if (it.ac_fail) {
			fprintf(stderr, "\"%zu\" -> \"%zu\" [label=\"fail\"]\n", s, it.ac_fail);
		}
		++ s;
	}

	fprintf(stderr, "}\n");
}

/// Prepare trie for Aho-Corasick search algorithm by mapping failure connections
void Trie::prepare() {
	for (auto &it : nodes) {
		it.ac_fail = 0;
	}

	// Create a buffer to use
	char buffer[nodes.capacity()];

	prepareNode(0, buffer, 0, 0);
}

uint8_t Trie::searchMatchType(const char * query) {
	size_t s = doSearch(query);
	if (s == maxOf<size_t>()) {
		return maxOf<uint8_t>();
	}

	return nodes[s].match_type;
}

bool Trie::insertNode(size_t s, const unsigned char * key, uint8_t match_type, uint16_t depth) {
	// Get node for state s
	auto n = &nodes.at(s);

	size_t i;

	if (key[0] == '\0') {
		// We've hit end of key
		n->match_type = match_type;
		n->len = depth;
		return true;		// Success
	}

	if (n->child[key[0]] != 0) {
		// First character already in trie, advance forward
		return insertNode(n->child[key[0]], key + 1, match_type, ++depth);
	} else {
		i = nodes.size();
		n->child[key[0]] = i;
		nodes.emplace_back(Node());
		nodes.back().c = key[0];

		// Advance forward
		return insertNode(i, key + 1, match_type, ++depth);
	}
}

void Trie::prepareNode(size_t s, char * buffer, unsigned short depth, size_t last_match_state) {
	buffer[depth] = '\0';
	buffer[depth + 1] = '\0';

	// Current node
	Node * n = &(nodes[s]);

	char * suffix = buffer;

	// Longest match seen so far??
	suffix += 1;

	// Find valid suffixes for failure path
	while ((suffix[0] != '\0') && (n->ac_fail == 0)) {
		n->ac_fail = doSearch(suffix);

		if (n->ac_fail == maxOf<size_t>()) {
			n->ac_fail = 0;
		}

		if (n->ac_fail == s) {
			// Something went wrong
			fprintf(stderr, "Recursive trie fallback detected at state %zu('%c') - suffix:'%s'!\n", s, n->c, suffix);
			n->ac_fail = 0;
		}

		suffix++;
	}

	// Prepare children
	for (int i = 0; i < 256; ++i) {
		if ((n->child[i] != 0) &&
				(n->child[i] != s)) {
			buffer[depth] = i;
			prepareNode(n->child[i], buffer, depth + 1, last_match_state);
		}
	}
}

size_t Trie::doSearchNode(size_t s, const char * query) const {
	if (query[0] == '\0') {
		return s;
	}

	if (nodes[s].child[(unsigned char)query[0]] == 0) {
		return maxOf<size_t>();
	}

	// Partial match, keep going
	return doSearchNode(nodes[s].child[(unsigned char)query[0]], query + 1);
}

size_t Trie::doSearch(const char * query) const {
	if (query) {
		return doSearchNode(0, query);
	}

	return 0;
}

auto Trie::search(const char * source, size_t start, size_t len) const -> Result {
	// Store results in a linked list
	// match * result = match_new(0, 0, 0);
	Result result;

	// Keep track of our state
	size_t state = 0;
	size_t temp_state;

	// Character being compared
	unsigned char test_value;
	size_t counter = start;
	size_t stop = start + len;

	while ((counter < stop) && (source[counter] != '\0')) {
		// Read next character
		test_value = (unsigned char)source[counter++];

		// Check for path that allows us to match next character
		while (state != 0 && nodes[state].child[test_value] == 0) {
			state = nodes[state].ac_fail;
		}

		// Advance state for the next character
		state = nodes[state].child[test_value];

		// Check for partial matches
		temp_state = state;

		while (temp_state != 0) {
			if (nodes[temp_state].match_type) {
				result.emplace_back(Match{counter - nodes[temp_state].len, nodes[temp_state].len, nodes[temp_state].match_type});
			}

			// Iterate to find shorter matches
			temp_state = nodes[temp_state].ac_fail;
		}
	}

	return result;
}

inline Trie::Result::iterator next(Trie::Result::iterator it) {
	return it + 1;
}

inline Trie::Result::iterator prev(Trie::Result::iterator it) {
	return it - 1;
}

inline bool hasNext(Trie::Result & header, Trie::Result::iterator it) {
	return (it + 1) != header.end();
}

inline bool hasPrev(Trie::Result & header, Trie::Result::iterator it) {
	return it > header.begin();
}

inline bool isLefter(Trie::Result & header, Trie::Result::iterator it) {
	if (it != header.end()) {
		auto n = it + 1;
		if (n != header.end()) {
			return n->start > it->start && n->start < it->start + it->len;
		}
	}
	return false;
}

inline bool isRighter(Trie::Result & header, Trie::Result::iterator it) {
	if (it != header.end()) {
		auto n = it + 1;
		if (n != header.end()) {
			return n->start < it->start;
		}
	}
	return false;
}

inline bool isPrevRighter(Trie::Result & header, Trie::Result::iterator it) {
	if (it > header.begin()) {
		auto p = it - 1;
		return p->len && p->start >= it->start;
	}
	return false;
}

void match_set_filter_leftmost_longest(Trie::Result & header) {
	// Filter results to include only leftmost/longest results
	Trie::Result::iterator matchIt = header.begin();
	Trie::Result::iterator nextIt;

	while (matchIt != header.end()) {
		if (hasNext(header, matchIt)) {
			if (matchIt->start == next(matchIt)->start) {
				// The next match is longer than this one
				matchIt = header.erase(matchIt);
				continue;
			}

			while (isLefter(header, matchIt)) {
				// This match is "lefter" than next
				header.erase(next(matchIt));
			}

			while (isRighter(header, matchIt)) {
				// Next match is "lefter" than us
				nextIt = matchIt;
				matchIt = prev(matchIt);
				header.erase(nextIt);
			}
		}

		while (isPrevRighter(header, matchIt)) {
			// We are "lefter" than previous
			nextIt = prev(matchIt);
			matchIt = header.erase(nextIt);
		}

		++ matchIt;
	}
}

auto Trie::searchLeftmostLongest(const char * source, size_t start, size_t len) const -> Result {
	auto result = search(source, start, len);
	if (!result.empty()) {
		match_set_filter_leftmost_longest(result);
	}
	return result;
}

NS_MMD_END
