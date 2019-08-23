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

#include "SPCommon.h"
#include "MMDTokenPair.h"
#include "MMDToken.h"
#include "MMDCore.h"

NS_MMD_BEGIN

TokenPair::TokenPair() {
	memset(this, 0, sizeof(TokenPair));
}

void TokenPair::add(uint8_t open_type, uint8_t close_type, uint8_t type, Options options) {
	if (open_type > 255 || close_type > 255 || type > 255) {
		std::cout << "Warn - too large\n";
	}
	// \todo: This needs to be more sophisticated
	can_open_pair[open_type] = 1;
	can_close_pair[close_type] = 1;
	(pair_type)[open_type][close_type] = type;

	if ((options & Options::AllowEmpty) != Options::None) {
		empty_allowed[type] = true;
	}

	if ((options & Options::MatchLength) != Options::None) {
		match_len[type] = true;
	}

	if ((options & Options::PruneMatch) != Options::None) {
		should_prune[type] = true;
	}
}

static void token_pair_mate(token * a, token * b) {
	if (a == NULL || b == NULL) {
		return;
	}

	a->mate = b;
	a->unmatched = false;

	b->mate = a;
	b->unmatched = false;
}
/// Mate opener and closer together

void TokenPair::match(const Token &tparent, Vector<Token> &stack, uint16_t depth) const {
	// Avoid stack overflow in "pathologic" input
	if (depth == kMaxPairRecursiveDepth) {
		return;
	}

	// Walk the child chain
	token * parent = tparent.getToken();
	token * walker = parent->child;

	// Counter
	size_t start_counter = stack.size();
	size_t i;				// We're sharing one stack, so any opener earlier than this belongs to a parent

	token * peek;
	uint8_t type;

	unsigned int opener_count[kMaxTokenTypes] = {0};	// Keep track of which token types are on the stack
	while (walker != NULL) {
		if (walker->child) {
			match(Token(walker), stack, depth + 1);
		}

		// Is this a closer?
		if (walker->can_close && can_close_pair[walker->type] && walker->unmatched ) {
			i = stack.size();

			// Do we even have a valid opener in the stack?
			// It's only worth checking if the stack is beyond a certain size
			if (i > start_counter + kLargeStackThreshold) {
				for (int j = 0; j < kMaxTokenTypes; ++j) {
					if (opener_count[j]) {
						if (pair_type[j][walker->type]) {
							goto close;
						}
					}
				}

				// No opener available for this as closer
				goto open;
			}

close:
			// Find matching opener for this closer
			while (i > start_counter) {
				peek = stack.at(i - 1).getToken();

				type = pair_type[peek->type][walker->type];

				if (type) {
					if (!empty_allowed[type]) {
						// Make sure they aren't consecutive tokens
						if ((peek->next == walker) &&
						        (peek->start + peek->len == walker->start)) {
							// i--;
							i = start_counter;	// In this situation, we can't use this token as a closer
							continue;
						}
					}

					if (match_len[type]) {
						// Lengths must match
						if (peek->len != walker->len) {
							i--;
							continue;
						}
					}

					token_pair_mate(peek, walker);

					// Clear portion of stack between opener and closer as they are now unavailable for mating
					while (stack.size() > (i - 1)) {
						peek = stack.back().getToken();
						stack.pop_back();
						opener_count[peek->type]--;
					}

					#ifndef NDEBUG
					//fprintf(stderr, "stack now sized %lu\n", stack.size());
					#endif
					// Prune matched section

					if (should_prune[type]) {
						if (peek->prev == NULL) {
							walker = sp_mmd_token_prune_graft(peek, walker, pair_type[peek->type][walker->type]);
							parent->child = walker;
						} else {
							walker = sp_mmd_token_prune_graft(peek, walker, pair_type[peek->type][walker->type]);
						}
					}

					break;
				}

				#ifndef NDEBUG
				else {
					//fprintf(stderr, "token type %d failed to match stack element\n", walker->type);
				}

				#endif
				i--;
			}
		}

open:

		// Is this an opener?
		if (walker->can_open && can_open_pair[walker->type] && walker->unmatched) {
			stack.push_back(walker);
			opener_count[walker->type]++;
			#ifndef NDEBUG
			//fprintf(stderr, "push token type %d to stack (%lu elements)\n", walker->type, stack.size());
			#endif
		}

		walker = walker->next;
	}

	#ifndef NDEBUG
	//fprintf(stderr, "token stack has %lu elements (of %lu)\n", stack.size(), stack.capacity());
	#endif

	// Remove unused tokens from stack and return to parent
	stack.resize(start_counter);
}

struct TokenPairEngineNode {
	Extensions ext;
	TokenPairEngine engine;
};

std::mutex s_engineMutex;
std::map<Extensions::Value, TokenPairEngine> s_engineMap;

TokenPairEngine *TokenPairEngine::engineForExtensions(Extensions::Value extensions) {
	extensions &= (Extensions::Critic | Extensions::Notes | Extensions::Compatibility);

	s_engineMutex.lock();

	auto it = s_engineMap.find(extensions);
	if (it == s_engineMap.end()) {
		it = s_engineMap.emplace(extensions, extensions).first;
	}

	auto ret = &(it->second);
	s_engineMutex.unlock();
	return ret;
}

TokenPairEngine::TokenPairEngine(Extensions::Value extensions) {
	// CriticMarkup
	if ((extensions & Extensions::Critic) != Extensions::None) {
		pairings1.add(CRITIC_ADD_OPEN, CRITIC_ADD_CLOSE, PAIR_CRITIC_ADD, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
		pairings1.add(CRITIC_DEL_OPEN, CRITIC_DEL_CLOSE, PAIR_CRITIC_DEL, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
		pairings1.add(CRITIC_COM_OPEN, CRITIC_COM_CLOSE, PAIR_CRITIC_COM, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
		pairings1.add(CRITIC_SUB_OPEN, CRITIC_SUB_DIV_A, PAIR_CRITIC_SUB_DEL, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
		pairings1.add(CRITIC_SUB_DIV_B, CRITIC_SUB_CLOSE, PAIR_CRITIC_SUB_ADD, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
		pairings1.add(CRITIC_HI_OPEN, CRITIC_HI_CLOSE, PAIR_CRITIC_HI, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
	}

	// HTML Comments
	pairings2.add(HTML_COMMENT_START, HTML_COMMENT_STOP, PAIR_HTML_COMMENT, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);

	// Brackets, Parentheses, Angles
	pairings3.add(BRACKET_LEFT, BRACKET_RIGHT, PAIR_BRACKET, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);

	if ((extensions & Extensions::Notes) != Extensions::None) {
		pairings3.add(BRACKET_CITATION_LEFT, BRACKET_RIGHT, PAIR_BRACKET_CITATION, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
		pairings3.add(BRACKET_FOOTNOTE_LEFT, BRACKET_RIGHT, PAIR_BRACKET_FOOTNOTE, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
		pairings3.add(BRACKET_GLOSSARY_LEFT, BRACKET_RIGHT, PAIR_BRACKET_GLOSSARY, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
		pairings3.add(BRACKET_ABBREVIATION_LEFT, BRACKET_RIGHT, PAIR_BRACKET_ABBREVIATION, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
	} else {
		pairings3.add(BRACKET_CITATION_LEFT, BRACKET_RIGHT, PAIR_BRACKET, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
		pairings3.add(BRACKET_FOOTNOTE_LEFT, BRACKET_RIGHT, PAIR_BRACKET, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
		pairings3.add(BRACKET_GLOSSARY_LEFT, BRACKET_RIGHT, PAIR_BRACKET, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
		pairings3.add(BRACKET_ABBREVIATION_LEFT, BRACKET_RIGHT, PAIR_BRACKET, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
	}

	pairings3.add(BRACKET_VARIABLE_LEFT, BRACKET_RIGHT, PAIR_BRACKET_VARIABLE, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);

	pairings3.add(BRACKET_IMAGE_LEFT, BRACKET_RIGHT, PAIR_BRACKET_IMAGE, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
	pairings3.add(PAREN_LEFT, PAREN_RIGHT, PAIR_PAREN, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
	pairings3.add(ANGLE_LEFT, ANGLE_RIGHT, PAIR_ANGLE, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
	pairings3.add(BRACE_DOUBLE_LEFT, BRACE_DOUBLE_RIGHT, PAIR_BRACES, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);

	// Strong/Emph
	pairings4.add(STAR, STAR, PAIR_STAR, TokenPair::Options::None);
	pairings4.add(UL, UL, PAIR_UL, TokenPair::Options::None);

	// Quotes and Backticks
	pairings3.add(BACKTICK, BACKTICK, PAIR_BACKTICK, TokenPair::Options::PruneMatch | TokenPair::Options::MatchLength);

	pairings4.add(BACKTICK,   QUOTE_RIGHT_ALT,   PAIR_QUOTE_ALT, TokenPair::Options::AllowEmpty | TokenPair::Options::MatchLength);
	pairings4.add(QUOTE_SINGLE, QUOTE_SINGLE, PAIR_QUOTE_SINGLE, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
	pairings4.add(QUOTE_DOUBLE, QUOTE_DOUBLE, PAIR_QUOTE_DOUBLE, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);

	// Math
	if ((extensions & Extensions::Compatibility) == Extensions::None) {
		pairings3.add(MATH_PAREN_OPEN, MATH_PAREN_CLOSE, PAIR_MATH, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
		pairings3.add(MATH_BRACKET_OPEN, MATH_BRACKET_CLOSE, PAIR_MATH, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
		pairings3.add(MATH_DOLLAR_SINGLE, MATH_DOLLAR_SINGLE, PAIR_MATH, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
		pairings3.add(MATH_DOLLAR_DOUBLE, MATH_DOLLAR_DOUBLE, PAIR_MATH, TokenPair::Options::AllowEmpty | TokenPair::Options::PruneMatch);
	}

	// Superscript/Subscript
	if ((extensions & Extensions::Compatibility) == Extensions::None) {
		pairings4.add(SUPERSCRIPT, SUPERSCRIPT, PAIR_SUPERSCRIPT, TokenPair::Options::PruneMatch);
		pairings4.add(SUBSCRIPT, SUBSCRIPT, PAIR_SUBSCRIPT, TokenPair::Options::PruneMatch);
	}

	// Text Braces -- for raw text syntax
	if ((extensions & Extensions::Compatibility) == Extensions::None) {
		pairings4.add(TEXT_BRACE_LEFT, TEXT_BRACE_RIGHT, PAIR_BRACE, TokenPair::Options::PruneMatch);
		pairings4.add(RAW_FILTER_LEFT, TEXT_BRACE_RIGHT, PAIR_RAW_FILTER, TokenPair::Options::PruneMatch);
	}
}

NS_MMD_END
