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
#include "MMDEngine.h"
#include "MMDCore.h"
#include "MMDContent.h"
#include "MMDTokenPair.h"
#include "MMDAhoCorasick.h"

#include "SPLog.h"

NS_MMD_BEGIN

class MemPoolHolder {
public:
	MemPoolHolder() { memory::pool::initialize(); }
	~MemPoolHolder() {
		if (pool) { memory::pool::destroy(pool); }
		memory::pool::terminate();
	}

	memory::pool_t * getPool() {
		if (!pool) { pool = memory::pool::create(nullptr); }
		return pool;
	}

private:
	memory::pool_t *pool = nullptr;
};

thread_local MemPoolHolder tl_pool;

struct Engine::Internal : memory::PoolInterface {
	using mmd_engine = _sp_mmd_engine;
	using token = _sp_mmd_token;

	Internal(memory::pool_t *p, const StringView &v, const Extensions &ext);
	~Internal();

	token * parse(const StringView &);
	bool prepare();
	void reset();

	void process(const ProcessCallback &);

	StringView source;
	Content content;
	mmd_engine engine;
	memory::pool_t *pool = nullptr;

	TokenPairEngine * pairs = nullptr;

	bool isDebug = false;
	StringStreamType debug;
};

Engine::Internal::Internal(memory::pool_t *p, const StringView &v, const Extensions & ext)
: content(ext) {

#ifdef DEBUG
	//isDebug = true;
#endif

	source = v;
	engine.str = v.data();
	engine.len = v.size();
	engine.extensions = toInt(ext.flags);
	engine.recurse_depth = 0;
	engine.allow_meta = false;

	engine.root = nullptr;

	engine.definition_stack = (void *)&content.getDefinitions();
	engine.header_stack = (void *)&content.getHeaders();
	engine.table_stack = (void *)&content.getTables();

	pool = p;
	engine.content = &content;

	pairs = TokenPairEngine::engineForExtensions(ext.flags);
}

Engine::Internal::~Internal() {
	if (!debug.empty()) {
		memory::pool::push(pool);
		std::cout << StringView(debug.weak());
		memory::pool::pop();
	}
}

using TokenVec = Content::Vector<Token>;
using token = _sp_mmd_token;

static void mmd_pair_tokens_in_block(token * block, const TokenPair & e, TokenVec & s);

static void mmd_pair_tokens_in_chain(token * head, const TokenPair & e, TokenVec & s) {
	while (head != nullptr) {
		mmd_pair_tokens_in_block(head, e, s);
		head = head->next;
	}
}

/// Match token pairs inside block
static void mmd_pair_tokens_in_block(token * block, const TokenPair & e, TokenVec & s) {
	if (block == nullptr) {
		return;
	}

	switch (block->type) {
		case BLOCK_BLOCKQUOTE:
		case BLOCK_DEFLIST:
		case BLOCK_DEFINITION:
		case BLOCK_DEF_ABBREVIATION:
		case BLOCK_DEF_CITATION:
		case BLOCK_DEF_FOOTNOTE:
		case BLOCK_DEF_GLOSSARY:
		case BLOCK_DEF_LINK:
		case BLOCK_H1:
		case BLOCK_H2:
		case BLOCK_H3:
		case BLOCK_H4:
		case BLOCK_H5:
		case BLOCK_H6:
		case BLOCK_PARA:
		case BLOCK_SETEXT_1:
		case BLOCK_SETEXT_2:
		case BLOCK_TERM:
			e.match(Token(block), s, 0);
			break;

		case DOC_START_TOKEN:
		case BLOCK_LIST_BULLETED:
		case BLOCK_LIST_BULLETED_LOOSE:
		case BLOCK_LIST_ENUMERATED:
		case BLOCK_LIST_ENUMERATED_LOOSE:
			mmd_pair_tokens_in_chain(block->child, e, s);
			break;

		case BLOCK_LIST_ITEM:
		case BLOCK_LIST_ITEM_TIGHT:
			e.match(Token(block), s, 0);
			mmd_pair_tokens_in_chain(block->child, e, s);
			break;

		case LINE_TABLE:
		case BLOCK_TABLE:
			// TODO: Need to parse into cells first
			e.match(Token(block), s, 0);
			mmd_pair_tokens_in_chain(block->child, e, s);
			break;

		case BLOCK_EMPTY:
		case BLOCK_CODE_INDENTED:
		case BLOCK_CODE_FENCED:
		default:
			// Nothing to do here
			return;
	}
}

/// Strong/emph parsing is done using single `*` and `_` characters, which are
/// then combined in a separate routine here to determine when
/// consecutive characters should be interpreted as STRONG instead of EMPH
/// \todo: Perhaps combining this with the routine when they are paired
/// would improve performance?
static void pair_emphasis_tokens(const StringView &source, token * t) {
	token * closer;

	while (t != NULL) {
		if (t->mate != NULL) {
			switch (t->type) {
				case STAR:
				case UL:
					closer = t->mate;

					if (t->next && (t->next->mate == closer->prev) && (t->type == t->next->type)
							&& (t->next->mate != t) && (t->start + t->len == t->next->start)
							&& (closer->start == closer->prev->start + closer->prev->len)) {

						// We have a strong pair
						t->type = STRONG_START;
						t->len = 2;
						closer->type = STRONG_STOP;
						closer->len = 2;
						closer->start--;

						tokens_prune(t->next, t->next);
						tokens_prune(closer->prev, closer->prev);

						sp_mmd_token_prune_graft(t, closer, PAIR_STRONG);
					} else {
						t->type = EMPH_START;
						closer->type = EMPH_STOP;

						sp_mmd_token_prune_graft(t, closer, PAIR_EMPH);
					}

					break;

				default:
					break;
			}

		}

		if (t->child != NULL) {
			switch (t->type) {
				case PAIR_BACKTICK:
				case PAIR_MATH:
					break;

				default:
					pair_emphasis_tokens(source, t->child);
					break;
			}
		}

		t = t->next;
	}
}

static void automatic_search_text(const StringView &str, token * t, const Trie & ac) {
	Trie::Result res = ac.searchLeftmostLongest(str.data(), t->start, t->len);
	Trie::Result::iterator walker;
	token * tok = t;

	for (auto &it : res) {
		sp_mmd_token_split(tok, uint32_t(it.start), uint32_t(it.len), it.match_type);

		// Advance token to next token
		while (tok->start < it.start + it.len) {
			tok = tok->next;
		}
	}
}

/// Determine which nodes to descend into to search for abbreviations
static void automatic_search(const StringView &str, token * t, const Trie & ac) {
	while (t) {
		switch (t->type) {
			case TEXT_PLAIN:
				automatic_search_text(str, t, ac);
				break;

			case DOC_START_TOKEN:
			case BLOCK_BLOCKQUOTE:
			case BLOCK_DEFINITION:
			case BLOCK_DEFLIST:
			case BLOCK_LIST_BULLETED:
			case BLOCK_LIST_BULLETED_LOOSE:
			case BLOCK_LIST_ENUMERATED:
			case BLOCK_LIST_ENUMERATED_LOOSE:
			case BLOCK_LIST_ITEM_TIGHT:
			case BLOCK_LIST_ITEM:
			case BLOCK_PARA:
			case BLOCK_TABLE:
			case BLOCK_TABLE_HEADER:
			case BLOCK_TABLE_SECTION:
			case BLOCK_TERM:
			case LINE_LIST_BULLETED:
			case LINE_LIST_ENUMERATED:
			case PAIR_BRACKET:
			case PAIR_BRACKET_FOOTNOTE:
			case PAIR_BRACKET_GLOSSARY:
			case PAIR_BRACKET_IMAGE:
			case PAIR_QUOTE_DOUBLE:
			case PAIR_QUOTE_SINGLE:
			case PAIR_STAR:
			case PAIR_UL:
			case TABLE_CELL:
			case TABLE_ROW:
				automatic_search(str, t->child, ac);
				break;

//			case PAIR_PAREN:
			default:
				break;
		}

		t = t->next;
	}
}

auto Engine::Internal::parse(const StringView &str) -> token * {
	reset();

	// Tokenize the string
	token * doc = sp_mmd_mmd_tokenize_string(&engine, str.data() - source.data(), str.size(), false);

	// Parse tokens into blocks
	sp_mmd_parse_token_chain(&engine, doc);

	if (doc) {
		// Parse blocks for pairs
		sp_mmd_assign_ambidextrous_tokens_in_block(&engine, doc, 0);

		// Prepare stack to be used for token pairing
		// This avoids allocating/freeing one for each iteration.
		TokenVec pair_stack; pair_stack.reserve(64);

		mmd_pair_tokens_in_block(doc, pairs->pairings1, pair_stack);
		mmd_pair_tokens_in_block(doc, pairs->pairings2, pair_stack);
		mmd_pair_tokens_in_block(doc, pairs->pairings3, pair_stack);
		mmd_pair_tokens_in_block(doc, pairs->pairings4, pair_stack);

		pair_emphasis_tokens(source, doc);

		if (isDebug) {
			Token(doc).describeTree(debug, source);
		}
	}

	return doc;
}

bool Engine::Internal::prepare() {
	if (!engine.root) {
		memory::pool::push(pool);

		engine.root = parse(source);
		content.process(source);

		// Process abbreviations, glossary, etc.
		if (!content.getExtensions().hasFlag(Extensions::Compatibility)) {
			// Only search if we have a target
			auto count = content.getAbbreviations().size() + content.getGlossary().size();
			if (count > 0) {
				Trie ac;
				for (auto &it : content.getAbbreviations()) {
					ac.insert(it->label_text.data(), PAIR_BRACKET_ABBREVIATION);
				}

				for (auto &it : content.getGlossary()) {
					ac.insert(it->clean_text.data(), PAIR_BRACKET_GLOSSARY);
				}

				ac.prepare();
				automatic_search(source.data(), engine.root, ac);
			}
		}

		memory::pool::pop();

		if (isDebug) {
			debug << "Allocated on parsing: " << memory::pool::get_allocated_bytes(pool) << "\n";
		}
		return engine.root != nullptr;
	}

	return true;
}

void Engine::Internal::reset() {
	if (engine.root) {
		engine.root = nullptr;
	}

	content.reset();
}

void Engine::Internal::process(const ProcessCallback &cb) {
	if (!prepare()) {
		log::text("MMD", "Fail to parse multimarkdown text");
		return;
	}

	auto p = memory::pool::create(pool);
	memory::pool::push(p);

	cb(content, source, engine.root);

	memory::pool::pop();

	if (isDebug) {
		debug << "Allocated on processing: " << memory::pool::get_allocated_bytes(p) << "\n";
	}
	memory::pool::destroy(p);
}

Engine::~Engine() {
	clear();
}

bool Engine::init(memory::pool_t *p, const StringView &source, const Extensions & ext) {
	clear();

	auto pool = memory::pool::create(p);
	memory::pool::push(pool);
	_internal = new (pool) Internal(pool, source, ext);
	memory::pool::pop();
	return true;
}

bool Engine::init(const StringView &source, const Extensions & ext) {
	clear();

	auto pool = memory::pool::create(tl_pool.getPool());
	memory::pool::push(pool);
	_internal = new (pool) Internal(pool, source, ext);
	memory::pool::pop();
	return true;
}

void Engine::clear() {
	if (_internal) {
		auto pool = _internal->pool;
		_internal->~Internal();
		_internal = nullptr;
		memory::pool::destroy(pool);
	}
}

void Engine::prepare() {
	if (_internal) {
		_internal->prepare();
	}
}

void Engine::setQuotesLanguage(QuotesLanguage l) {
	if (_internal) {
		_internal->content.setQuotesLanguage(l);
	}
}
QuotesLanguage Engine::getQuotesLanguage() const {
	return (_internal)?_internal->content.getQuotesLanguage():QuotesLanguage::English;
}

void Engine::process(const ProcessCallback &cb) {
	if (_internal) {
		_internal->process(cb);
	}
}

NS_MMD_END
