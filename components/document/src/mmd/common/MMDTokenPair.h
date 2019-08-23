/*

	Copyright © 2016 - 2017 Fletcher T. Penney.
	Copyright © 2017 Roman Katuntsev <sbkarr@stappler.org>


	The `MultiMarkdown 6` project is released under the MIT License..

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
#ifndef MMD_COMMON_MMDTOKENPAIR_H_
#define MMD_COMMON_MMDTOKENPAIR_H_

#include "MMDCommon.h"

NS_MMD_BEGIN

struct TokenPair {
	template <typename T>
	using Vector = memory::PoolInterface::VectorType<T>;

	constexpr static int kMaxTokenTypes	= 230;			//!< This needs to be larger than the largest token type being used
	constexpr static int kLargeStackThreshold = 1000;	//!< Avoid unnecessary searches of large stacks
	constexpr static int kMaxPairRecursiveDepth = 100;	//!< Maximum recursion depth to traverse when pairing tokens -- to prevent stack overflow with "pathologic" input

	enum Options {
		None = 0,
		AllowEmpty = 1 << 0,		//!< Allow consecutive tokens to match with each other
		MatchLength	= 1 << 1,		//!< Require that opening/closing tokens be same length
		PruneMatch		= 1 << 2,		//!< Move the matched sub-chain into a child chain
	};

	bool empty_allowed[kMaxTokenTypes];				//!< Is this pair type allowed to be empty?
	bool match_len[kMaxTokenTypes];					//!< Does this pair type require matched lengths of openers/closers?
	bool should_prune[kMaxTokenTypes];				//!< Does this pair type need to be pruned to a child token chain?

	uint8_t can_open_pair[kMaxTokenTypes];				//!< Can token type open a pair?
	uint8_t can_close_pair[kMaxTokenTypes];				//!< Can token type close a pair?
	uint8_t pair_type[kMaxTokenTypes][kMaxTokenTypes];	//!< Which pair are we forming?

	TokenPair();

	void add(uint8_t open_type, uint8_t close_type, uint8_t pair_type, Options options);

	void match(const Token &parent, Vector<Token> &stack, uint16_t depth) const;
};

SP_DEFINE_ENUM_AS_MASK(TokenPair::Options);

struct TokenPairEngine {
	static TokenPairEngine *engineForExtensions(Extensions::Value);

	TokenPairEngine(Extensions::Value);

	TokenPair pairings1;
	TokenPair pairings2;
	TokenPair pairings3;
	TokenPair pairings4;
};

NS_MMD_END

#endif /* MMD_COMMON_MMDTOKENPAIR_H_ */
