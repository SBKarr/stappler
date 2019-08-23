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

#ifndef MMD_COMMON_MMDAHOCORASICK_H_
#define MMD_COMMON_MMDAHOCORASICK_H_

#include "MMDCommon.h"

NS_MMD_BEGIN

class Trie : public memory::PoolInterface {
public:
	struct Node {
		char c; // Character for this node
		uint8_t match_type; // 0 = no match, otherwise what have we matched?
		uint16_t len; // Length of string matched
		size_t child[256]; // Where should we go next?
		size_t ac_fail; // Where should we go if we fail?

		Node();
	};

	struct Match {
		size_t start = 0; // Starting offset for this match
		size_t len = 0; // Length for this match
		uint8_t match_type = 0; // Match type
	};

	using Result = VectorType<Match>;

	static constexpr size_t kTrieStartingSize = 256;

	template <typename V>
	using Vector = VectorType<V>;

	Trie(size_t startingSize = kTrieStartingSize);

	bool insert(const char *key, uint8_t match_type);
	void toGraphviz() const;

	void prepare();

	uint8_t searchMatchType(const char * query);

	Result search(const char * source, size_t start, size_t len) const;
	Result searchLeftmostLongest(const char * source, size_t start, size_t len) const;

protected:
	bool insertNode(size_t s, const unsigned char * key, uint8_t match_type, uint16_t depth);
	void prepareNode(size_t s, char * buffer, uint16_t depth, size_t last_match_state);

	size_t doSearchNode(size_t s, const char * query) const;
	size_t doSearch(const char * query) const;

	void matchLeftmostLongest(Result &);

	Vector<Node> nodes;
};

NS_MMD_END

#endif /* MMD_COMMON_MMDAHOCORASICK_H_ */
