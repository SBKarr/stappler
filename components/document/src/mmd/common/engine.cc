
#include "SPCommon.h"
#include "MMDEngine.h"
#include "MMDContent.h"
#include "MMDCore.h"
#include "MMDToken.h"
#include "MMDChars.h"
#include "SPHtmlParser.h"

#define kMaxParseRecursiveDepth 1000		//!< Maximum recursion depth when parsing -- to prevent stack overflow with "pathologic" input

NS_MMD_BEGIN

static Content *acquireContent(void *enginePtr) {
	using mmd_engine = _sp_mmd_engine;

	if (mmd_engine *engine = (mmd_engine *)enginePtr) {
		return (Content *)engine->content;
	}
	return nullptr;
}

extern "C" {

using mmd_engine = _sp_mmd_engine;
using token = _sp_mmd_token;

// Basic parser function declarations
void * ParseAlloc( void *(*mallocProc)(size_t) );
void Parse(void *yyp, int yymajor, token * yyminor, mmd_engine *engine);
void ParseFree(void *p, void (*freeProc)(void *, size_t));
void ParseTrace(FILE *TraceFILE, const char *zTracePrompt);

static void check_html_id(mmd_engine * e, token *t, const char *start) {
	Content *c = (Content *)e->content;

	StringView current(start, start - e->str);
	if (current.is('<')) {
		++ current;
		if (!current.is<StringView::CharGroup<CharGroupId::Latin>>()) {
			return;
		}

		html::Tag_readName(current, true);

		StringView attrName;
		StringView attrValue;
		while (!current.empty() && !current.is('>') && !current.is('/')) {
			attrName.clear();
			attrValue.clear();

			attrName = html::Tag_readAttrName(current, true);
			if (attrName.empty()) {
				continue;
			}

			attrValue = html::Tag_readAttrValue(current, true);
			if (attrName == "id") {
				if (!attrValue.empty()) {
					c->emplaceHtmlId(t, attrValue);
				}
				break;
			}
		}
	}
}

static bool line_is_empty(token * t) {
	while (t) {
		switch (t->type) {
			case NON_INDENT_SPACE:
			case INDENT_TAB:
			case INDENT_SPACE:
				t = t->next;
				break;

			case TEXT_LINEBREAK:
			case TEXT_NL:
				return true;

			default:
				return false;
		}
	}

	return true;
}

static void mmd_assign_line_type(mmd_engine * e, token * line) {
	if (!line) {
		return;
	}

	if (!line->child) {
		line->type = LINE_EMPTY;
		return;
	}

	const char * source = e->str;

	token * t = NULL;
	token * first_child = line->child;

	short temp_short;
	size_t scan_len;

	// Skip non-indenting space
	if (first_child->type == NON_INDENT_SPACE) {
		//token_remove_first_child(line);
		first_child = first_child->next;
	} else if (first_child->type == TEXT_PLAIN && first_child->len == 1) {
		if (source[first_child->start] == ' ') {
			//token_remove_first_child(line);
			first_child = first_child->next;
		}
	}

	if (first_child == NULL) {
		line->type = LINE_EMPTY;
		return;
	}

	switch (first_child->type) {
		case INDENT_TAB:
			if (line_is_empty(first_child)) {
				line->type = LINE_EMPTY;
				e->allow_meta = false;
			} else {
				line->type = LINE_INDENTED_TAB;
			}

			break;

		case INDENT_SPACE:
			if (line_is_empty(first_child)) {
				line->type = LINE_EMPTY;
				e->allow_meta = false;
			} else {
				line->type = LINE_INDENTED_SPACE;
			}

			break;

		case ANGLE_LEFT:
			if (scan_html_block(&source[line->start])) {
				line->type = LINE_HTML;
			} else {
				line->type = LINE_PLAIN;
			}

			break;

		case ANGLE_RIGHT:
			line->type = LINE_BLOCKQUOTE;
			first_child->type = MARKER_BLOCKQUOTE;
			break;

		case BACKTICK:
			if (e->extensions & toInt(Extensions::Compatibility)) {
				line->type = LINE_PLAIN;
				break;
			}

			scan_len = scan_fence_end(&source[first_child->start]);

			if (scan_len) {
				switch (first_child->len) {
					case 3:
						line->type = LINE_FENCE_BACKTICK_3;
						break;

					case 4:
						line->type = LINE_FENCE_BACKTICK_4;
						break;

					default:
						line->type = LINE_FENCE_BACKTICK_5;
						break;
				}

				break;
			} else {
				scan_len = scan_fence_start(&source[first_child->start]);

				if (scan_len) {
					switch (first_child->len) {
						case 3:
							line->type = LINE_FENCE_BACKTICK_START_3;
							break;

						case 4:
							line->type = LINE_FENCE_BACKTICK_START_4;
							break;

						default:
							line->type = LINE_FENCE_BACKTICK_START_5;
							break;
					}

					break;
				}
			}

			line->type = LINE_PLAIN;
			break;

		case COLON:
			line->type = LINE_PLAIN;

			if (e->extensions & toInt(Extensions::Compatibility)) {
				break;
			}

			if (scan_definition(&source[first_child->start])) {
				line->type = LINE_DEFINITION;
			}

			break;

		case HASH1:
		case HASH2:
		case HASH3:
		case HASH4:
		case HASH5:
		case HASH6:
			if (scan_atx(&source[first_child->start])) {
				line->type = (first_child->type - HASH1) + LINE_ATX_1;
				first_child->type = (line->type - LINE_ATX_1) + MARKER_H1;

				// Strip trailing whitespace from '#' sequence
				first_child->len = first_child->type - MARKER_H1 + 1;

				// Strip trailing '#' sequence if present
				if (line->child->tail->type == TEXT_NL) {
					if ((line->child->tail->prev->type >= HASH1) &&
					        (line->child->tail->prev->type <= HASH6)) {
						line->child->tail->prev->type -= HASH1;
						line->child->tail->prev->type += MARKER_H1;
					}
				} else {
					if ((line->child->tail->type >= HASH1) &&
					        (line->child->tail->type <= HASH6)) {
						line->child->tail->type -= TEXT_EMPTY;
						line->child->tail->type += MARKER_H1;
					}
				}
			} else {
				line->type = LINE_PLAIN;
			}

			break;

		case HTML_COMMENT_START:
			if (!first_child->next || !first_child->next->next) {
				line->type = LINE_START_COMMENT;
			} else {
				line->type = LINE_PLAIN;
			}

			break;

		case HTML_COMMENT_STOP:
			if (!first_child->next || !first_child->next->next) {
				line->type = LINE_STOP_COMMENT;
			} else {
				line->type = LINE_PLAIN;
			}

			break;

		case TEXT_NUMBER_POSS_LIST:
			if (first_child->next) {
				switch (source[first_child->next->start]) {
					case ' ':
					case '\t':
						line->type = LINE_LIST_ENUMERATED;
						first_child->type = MARKER_LIST_ENUMERATOR;

						switch (first_child->next->type) {
							case TEXT_PLAIN:

								// Strip whitespace between bullet and text
								while (chars::isWhitespace(source[first_child->next->start])) {
									first_child->next->start++;
									first_child->next->len--;
								}

								break;

							case INDENT_SPACE:
							case INDENT_TAB:
							case NON_INDENT_SPACE:
								t = first_child;

								while (t->next && ((t->next->type == INDENT_SPACE) ||
								                   (t->next->type == INDENT_TAB) ||
								                   (t->next->type == NON_INDENT_SPACE))) {
									tokens_prune(t->next, t->next);
								}

								break;
						}

						break;

					default:
						line->type = LINE_PLAIN;
						first_child->type = TEXT_PLAIN;
						break;
				}
			} else {
				line->type = LINE_LIST_ENUMERATED;
				first_child->type = MARKER_LIST_ENUMERATOR;
			}

			break;

		case EQUAL:

			// Could this be a setext heading marker?
			if (scan_setext(&source[first_child->start])) {
				line->type = LINE_SETEXT_1;
			} else {
				line->type = LINE_PLAIN;
			}

			break;

		case DASH_N:
		case DASH_M:
			if (scan_setext(&source[first_child->start])) {
				line->type = LINE_SETEXT_2;
				break;
			}

		case STAR:
		case UL:
			// Could this be a horizontal rule?
			t = first_child->next;
			temp_short = first_child->len;

			while (t) {
				switch (t->type) {
					case DASH_N:
					case DASH_M:
						if (t->type == first_child->type) {
							t = t->next;

							if (t) {
								temp_short += t->len;
							}
						} else {
							temp_short = 0;
							t = NULL;
						}

						break;

					case STAR:
					case UL:
						if (t->type == first_child->type) {
							t = t->next;
							temp_short++;
						} else {
							temp_short = 0;
							t = NULL;
						}

						break;

					case NON_INDENT_SPACE:
					case INDENT_TAB:
					case INDENT_SPACE:
						t = t->next;
						break;

					case TEXT_PLAIN:
						if ((t->len == 1) && (source[t->start] == ' ')) {
							t = t->next;
							break;
						}

						temp_short = 0;
						t = NULL;
						break;

					case TEXT_NL:
					case TEXT_LINEBREAK:
						t = NULL;
						break;

					default:
						temp_short = 0;
						t = NULL;
						break;
				}
			}

			if (temp_short > 2) {
				// This is a horizontal rule, not a list item
				line->type = LINE_HR;
				break;
			}

			if (first_child->type == UL) {
				// Revert to plain for this type
				line->type = LINE_PLAIN;
				break;
			}

			// If longer than 1 character, then it can't be a list marker, so it's a
			// plain line
			if (first_child->len > 1) {
				line->type = LINE_PLAIN;
				break;
			}

		case PLUS:
			if (!first_child->next) {
				// TODO: Should this be an empty list item instead??
				line->type = LINE_PLAIN;
			} else {
				switch (source[first_child->next->start]) {
					case ' ':
					case '\t':
						line->type = LINE_LIST_BULLETED;
						first_child->type = MARKER_LIST_BULLET;

						switch (first_child->next->type) {
							case TEXT_PLAIN:

								// Strip whitespace between bullet and text
								while (chars::isWhitespace(source[first_child->next->start])) {
									first_child->next->start++;
									first_child->next->len--;
								}

								break;

							case INDENT_SPACE:
							case INDENT_TAB:
							case NON_INDENT_SPACE:
								t = first_child;

								while (t->next && ((t->next->type == INDENT_SPACE) ||
								                   (t->next->type == INDENT_TAB) ||
								                   (t->next->type == NON_INDENT_SPACE))) {
									tokens_prune(t->next, t->next);
								}

								break;
						}

						break;

					default:
						line->type = LINE_PLAIN;
						break;
				}
			}

			break;

		case TEXT_LINEBREAK:
		case TEXT_NL:
			e->allow_meta = false;
			line->type = LINE_EMPTY;
			break;

		case TOC:
			line->type = (e->extensions & toInt(Extensions::Compatibility)) ? LINE_PLAIN : LINE_TOC;
			break;

		case BRACKET_LEFT:
			if (e->extensions & toInt(Extensions::Compatibility)) {
				scan_len = scan_ref_link_no_attributes(&source[line->start]);
				line->type = (scan_len) ? LINE_DEF_LINK : LINE_PLAIN;
			} else {
				scan_len = scan_ref_link(&source[line->start]);
				line->type = (scan_len) ? LINE_DEF_LINK : LINE_PLAIN;
			}

			break;

		case BRACKET_ABBREVIATION_LEFT:
			if (e->extensions & toInt(Extensions::Notes)) {
				scan_len = scan_ref_abbreviation(&source[line->start]);
				line->type = (scan_len) ? LINE_DEF_ABBREVIATION : LINE_PLAIN;
			} else {
				scan_len = scan_ref_link_no_attributes(&source[line->start]);
				line->type = (scan_len) ? LINE_DEF_LINK : LINE_PLAIN;
			}

			break;

		case BRACKET_CITATION_LEFT:
			if (e->extensions & toInt(Extensions::Notes)) {
				scan_len = scan_ref_citation(&source[line->start]);
				line->type = (scan_len) ? LINE_DEF_CITATION : LINE_PLAIN;
			} else {
				scan_len = scan_ref_link_no_attributes(&source[line->start]);
				line->type = (scan_len) ? LINE_DEF_LINK : LINE_PLAIN;
			}

			break;

		case BRACKET_FOOTNOTE_LEFT:
			if (e->extensions & toInt(Extensions::Notes)) {
				scan_len = scan_ref_foot(&source[line->start]);
				line->type = (scan_len) ? LINE_DEF_FOOTNOTE : LINE_PLAIN;
			} else {
				scan_len = scan_ref_link_no_attributes(&source[line->start]);
				line->type = (scan_len) ? LINE_DEF_LINK : LINE_PLAIN;
			}

			break;

		case BRACKET_GLOSSARY_LEFT:
			if (e->extensions & toInt(Extensions::Notes)) {
				scan_len = scan_ref_glossary(&source[line->start]);
				line->type = (scan_len) ? LINE_DEF_GLOSSARY : LINE_PLAIN;
			} else {
				scan_len = scan_ref_link_no_attributes(&source[line->start]);
				line->type = (scan_len) ? LINE_DEF_LINK : LINE_PLAIN;
			}

			break;

		case PIPE:

			// If PIPE is first, save checking later and assign LINE_TABLE now
			if (!(e->extensions & toInt(Extensions::Compatibility))) {
				scan_len = scan_table_separator(&source[line->start]);
				line->type = (scan_len) ? LINE_TABLE_SEPARATOR : LINE_TABLE;

				break;
			}

		case TEXT_PLAIN:
			if (e->allow_meta && !(e->extensions & toInt(Extensions::Compatibility))) {
				scan_len = scan_url(&source[line->start]);

				if (scan_len == 0) {
					scan_len = scan_meta_line(&source[line->start]);
					line->type = (scan_len) ? LINE_META : LINE_PLAIN;
					break;
				}
			}

		default:
			line->type = LINE_PLAIN;
			break;
	}

	if ((line->type == LINE_PLAIN) &&
	        !(e->extensions & toInt(Extensions::Compatibility))) {
		// Check if this is a potential table line
		token * walker = first_child;

		while (walker != NULL) {
			if (walker->type == PIPE) {
				scan_len = scan_table_separator(&source[line->start]);
				line->type = (scan_len) ? LINE_TABLE_SEPARATOR : LINE_TABLE;

				return;
			}

			walker = walker->next;
		}
	}
}

static void strip_quote_markers_from_line(token * line, const char * source) {
	if (!line || !line->child) {
		return;
	}

	token * t;

	switch (line->child->type) {
		case MARKER_BLOCKQUOTE:
		case NON_INDENT_SPACE:
			t = line->child;
			line->child = t->next;
			t->next = NULL;

			if (line->child) {
				line->child->prev = NULL;
				line->child->tail = t->tail;
			}

			sp_mmd_token_free(t);
			break;
	}

	if (line->child && (line->child->type == TEXT_PLAIN)) {
		// Strip leading whitespace from first text token
		t = line->child;

		while (t->len && chars::isWhitespace(source[t->start])) {
			t->start++;
			t->len--;
		}

		if (t->len == 0) {
			line->child = t->next;
			t->next = NULL;

			if (line->child) {
				line->child->prev = NULL;
				line->child->tail = t->tail;
			}

			sp_mmd_token_free(t);
		}
	}
}

static void strip_quote_markers_from_block(mmd_engine * e, token * block) {
	if (!block || !block->child) {
		return;
	}

	token * t = block->child;

	while (t != NULL) {
		strip_quote_markers_from_line(t, e->str);
		mmd_assign_line_type(e, t);

		t = t->next;
	}
}

/// Strip leading indenting space from line (if present)
static void deindent_line(token  * line) {
	if (!line || !line->child) {
		return;
	}

	token * t;

	switch (line->child->type) {
		case INDENT_TAB:
		case INDENT_SPACE:
			t = line->child;
			line->child = t->next;
			t->next = NULL;

			if (line->child) {
				line->child->prev = NULL;
				line->child->tail = t->tail;
			}

			sp_mmd_token_free(t);
			break;
	}
}


/// Strip leading indenting space from block
/// (for recursively parsing nested lists)
static void deindent_block(mmd_engine * e, token * block) {
	if (!block || !block->child) {
		return;
	}

	token * t = block->child;

	while (t != NULL) {
		deindent_line(t);
		mmd_assign_line_type(e, t);

		t = t->next;
	}
}

static void *sp_mmd_malloc(size_t s) {
	return memory::pool::palloc(memory::pool::acquire(), s);
}

static void sp_mmd_free(void *ptr, size_t size) {
	memory::pool::free(memory::pool::acquire(), ptr, size);
}

/// Parse token tree
void sp_mmd_parse_token_chain(mmd_engine * e, token * chain) {
	if (e->recurse_depth == kMaxParseRecursiveDepth) {
		return;
	}

	e->recurse_depth++;

	void* pParser = ParseAlloc (sp_mmd_malloc);		// Create a parser (for lemon)
	token * walker = chain->child;				// Walk the existing tree
	token * remainder;							// Hold unparsed tail of chain

#ifndef NDEBUG
	// ParseTrace(stderr, "parser >>");
#endif

	// Remove existing token tree
	e->root = NULL;

	while (walker != NULL) {
		remainder = walker->next;

		// Snip token from remainder
		walker->next = NULL;
		walker->tail = walker;

		if (remainder) {
			remainder->prev = NULL;
		}

		#ifndef NDEBUG
		// fprintf(stderr, "\nNew line\n");
		#endif

		Parse(pParser, walker->type, walker, e);

		walker = remainder;
	}

	// Signal finish to parser
	Parse(pParser, 0, NULL, e);

	// Disconnect of (now empty) root
	chain->child = NULL;
	sp_mmd_token_append_child(chain, e->root);
	e->root = NULL;

	ParseFree(pParser, sp_mmd_free);

	e->recurse_depth--;
}

void sp_mmd_recursive_parse_indent(mmd_engine * e, token * block) {
	// Remove one indent level from all lines to allow recursive parsing
	deindent_block(e, block);

	// First line is now plain text
	block->child->type = LINE_PLAIN;

	// Strip tokens?
	switch (block->type) {
		case BLOCK_DEFINITION:
			// Strip leading ':' from definition
			sp_mmd_token_remove_first_child(block->child);
			break;
	}

	sp_mmd_parse_token_chain(e, block);
}

void sp_mmd_recursive_parse_list_item(mmd_engine * e, token * block) {
	token * marker = sp_mmd_token_copy(block->child->child);

	// Strip list marker from first line
	sp_mmd_token_remove_first_child(block->child);

	// Remove one indent level from all lines to allow recursive parsing
	deindent_block(e, block);

	sp_mmd_parse_token_chain(e, block);

	// Insert marker back in place
	marker->next = block->child->child;

	if (block->child->child) {
		block->child->child->prev = marker;
	}

	block->child->child = marker;
}

void sp_mmd_recursive_parse_blockquote(mmd_engine * e, token * block) {
	// Strip blockquote markers (if present)
	strip_quote_markers_from_block(e, block);

	sp_mmd_parse_token_chain(e, block);
}

void sp_mmd_is_list_loose(token * list) {
	bool loose = false;

	token * walker = list->child;

	if (walker == NULL) {
		return;
	}

	while (walker->next != NULL) {
		if (walker->type == BLOCK_LIST_ITEM) {
			if (walker->child->type == BLOCK_PARA) {
				loose = true;
			} else {
				walker->type = BLOCK_LIST_ITEM_TIGHT;
			}
		}

		walker = walker->next;
	}

	if (loose) {
		switch (list->type) {
			case BLOCK_LIST_BULLETED:
				list->type = BLOCK_LIST_BULLETED_LOOSE;
				break;

			case BLOCK_LIST_ENUMERATED:
				list->type = BLOCK_LIST_ENUMERATED_LOOSE;
				break;
		}
	}
}

/// Is this actually an HTML block?
void sp_mmd_is_para_html(mmd_engine * e, token * block) {
	if ((block == NULL) ||
	        (block->child == NULL) ||
	        (block->child->type != LINE_PLAIN)) {
		return;
	}

	token * t = block->child->child;

	if (t->type == ANGLE_LEFT || t->type == HTML_COMMENT_START) {
		if (scan_html_block(&(e->str[t->start]))) {
			block->type = BLOCK_HTML;
			return;
		}

		if (scan_html_line(&(e->str[t->start]))) {
			block->type = BLOCK_HTML;
			return;
		}
	}
}

static void strip_line_tokens_from_metadata(mmd_engine * e, token * metadata) {
	token * l = metadata->child;
	const char * source = e->str;

	auto content = acquireContent(e);

	Content::String metaKey;
	Content::StringStream stream;

	size_t start, len;
	while (l) {
		switch (l->type) {
			case LINE_META:
meta:
				if (!metaKey.empty()) {
					content->emplaceMeta(move(metaKey), stream.str());
					metaKey.clear();
					stream.clear();
				}

				len = scan_meta_key(&source[l->start]);
				metaKey = Content::labelFromString(StringView(source + l->start, len));
				start = l->start + len + 1;
				len = l->start + l->len - start - 1;
				stream << StringView(&source[start], len);
				break;

			case LINE_INDENTED_TAB:
			case LINE_INDENTED_SPACE:
				while (l->len && chars::isWhitespace(source[l->start])) {
					l->start++;
					l->len--;
				}

			case LINE_PLAIN:
plain:
				stream << '\n' << StringView(&source[l->start], l->len);
				break;

			case LINE_SETEXT_2:
			case LINE_YAML:
				break;

			case LINE_TABLE:
				if (scan_meta_line(&source[l->start])) {
					goto meta;
				} else {
					goto plain;
				}

			default:
				fprintf(stderr, "ERROR!\n");
				Token(l).describe(std::cerr);
				break;
		}

		l = l->next;
	}

	if (!metaKey.empty()) {
		content->emplaceMeta(move(metaKey), stream.str());
		metaKey.clear();
		stream.clear();
	}
}

static void strip_line_tokens_from_deflist(mmd_engine * e, token * deflist) {
	token * walker = deflist->child;

	while (walker) {
		switch (walker->type) {
			case LINE_EMPTY:
				walker->type = TEXT_EMPTY;
				break;

			case LINE_PLAIN:
				walker->type = BLOCK_TERM;
				break;

			case BLOCK_TERM:
				break;

			case BLOCK_DEFINITION:
				sp_mmd_strip_line_tokens_from_block(e, walker);
				break;
		}

		walker = walker->next;
	}
}

static void strip_line_tokens_from_table(mmd_engine * e, token * table) {
	token * walker = table->child;

	while (walker) {
		switch (walker->type) {
			case BLOCK_TABLE_SECTION:
				sp_mmd_strip_line_tokens_from_block(e, walker);
				break;

			case BLOCK_TABLE_HEADER:
				sp_mmd_strip_line_tokens_from_block(e, walker);
				break;

			case LINE_EMPTY:
				walker->type = TEXT_EMPTY;
				break;
		}

		walker = walker->next;
	}
}

void parse_table_row_into_cells(token * row) {
	token * first = NULL;
	token * last = NULL;

	token * walker = row->child;

	if (walker) {
		if (walker->type == PIPE) {
			walker->type = TABLE_DIVIDER;
			first = walker->next;
		} else {
			first = walker;
			last = first;
		}

		walker = walker->next;
	}

	while (walker) {
		switch (walker->type) {
			case PIPE:
				sp_mmd_token_prune_graft(first, last, TABLE_CELL);
				first = NULL;
				last = NULL;
				walker->type = TABLE_DIVIDER;
				break;

			case TEXT_NL:
			case TEXT_LINEBREAK:
				break;

			default:
				if (!first) {
					first = walker;
				}

				last = walker;
		}

		walker = walker->next;
	}

	if (first) {
		sp_mmd_token_prune_graft(first, last, TABLE_CELL);
	}
}

void sp_mmd_strip_line_tokens_from_block(mmd_engine * e, token * block) {
	if ((block == NULL) || (block->child == NULL)) {
		return;
	}

	#ifndef NDEBUG
	// fprintf(stderr, "Strip line tokens from %d (%lu:%lu) (child %d)\n", block->type, block->start, block->len, block->child->type);
	// token_tree_describe(block, e->dstr->str);
	#endif

	token * l = block->child;

	// Custom actions
	switch (block->type) {
		case BLOCK_META:
			// Handle metadata differently
			return strip_line_tokens_from_metadata(e, block);

		case BLOCK_CODE_INDENTED:

			// Strip trailing empty lines from indented code blocks
			while (l->tail->type == LINE_EMPTY) {
				sp_mmd_token_remove_last_child(block);
			}

			break;

		case BLOCK_DEFLIST:
			// Handle definition lists
			return strip_line_tokens_from_deflist(e, block);

		case BLOCK_TABLE:
			// Handle tables
			return strip_line_tokens_from_table(e, block);

	}

	token * children = NULL;
	block->child = NULL;

	token * temp;

	// Move contents of line directly into the parent block
	while (l != NULL) {
		// Remove leading non-indent space from line
		if (block->type != BLOCK_CODE_FENCED && l->child && l->child->type == NON_INDENT_SPACE) {
			sp_mmd_token_remove_first_child(l);
		}

		switch (l->type) {
			case LINE_SETEXT_1:
			case LINE_SETEXT_2:
				if ((block->type == BLOCK_SETEXT_1) ||
				        (block->type == BLOCK_SETEXT_2)) {
					temp = l->next;
					tokens_prune(l, l);
					l = temp;
					break;
				}

			case LINE_DEFINITION:
				if (block->type == BLOCK_DEFINITION) {
					// Remove leading colon
					sp_mmd_token_remove_first_child(l);
				}

			case LINE_ATX_1:
			case LINE_ATX_2:
			case LINE_ATX_3:
			case LINE_ATX_4:
			case LINE_ATX_5:
			case LINE_ATX_6:
			case LINE_BLOCKQUOTE:
			case LINE_CONTINUATION:
			case LINE_DEF_ABBREVIATION:
			case LINE_DEF_CITATION:
			case LINE_DEF_FOOTNOTE:
			case LINE_DEF_GLOSSARY:
			case LINE_DEF_LINK:
			case LINE_EMPTY:
			case LINE_LIST_BULLETED:
			case LINE_LIST_ENUMERATED:
			case LINE_META:
			case LINE_PLAIN:
			case LINE_START_COMMENT:
			case LINE_STOP_COMMENT:
handle_line:

			case LINE_INDENTED_TAB:
			case LINE_INDENTED_SPACE:

				// Strip leading indent (Only the first one)
				if (block->type != BLOCK_CODE_FENCED && l->child && ((l->child->type == INDENT_SPACE) || (l->child->type == INDENT_TAB))) {
					sp_mmd_token_remove_first_child(l);
				}

				// If we're not a code block, strip additional indents
				if ((block->type != BLOCK_CODE_INDENTED) &&
				        (block->type != BLOCK_CODE_FENCED)) {
					while (l->child && ((l->child->type == INDENT_SPACE) || (l->child->type == INDENT_TAB))) {
						sp_mmd_token_remove_first_child(l);
					}
				}

				// Add contents of line to parent block
				sp_mmd_token_append_child(block, l->child);

				// Disconnect line from it's contents
				l->child = NULL;

				// Need to remember first line we strip
				if (children == NULL) {
					children = l;
				}

				// Advance to next line
				l = l->next;
				break;

			case BLOCK_DEFINITION:
				// Sometimes these get created unintentionally inside other blocks
				// Process inside it, then treat it like a line to be stripped

				// Change to plain line
				l->child->type = LINE_PLAIN;
				sp_mmd_strip_line_tokens_from_block(e, l);

				// Move children to parent
				// Add ':' back
				if (e->str[l->child->start - 1] == ':') {
					temp = sp_mmd_token_new(COLON, l->child->start - 1, 1);
					sp_mmd_token_append_child(block, temp);
				}

				sp_mmd_token_append_child(block, l->child);
				l->child = NULL;

				if (children == NULL) {
					children = l;
				}

				l = l->next;
				break;

			case LINE_TABLE_SEPARATOR:
			case LINE_TABLE:
				if (block->type == BLOCK_TABLE_HEADER) {
					l->type = (l->type == LINE_TABLE) ? TABLE_ROW : LINE_TABLE_SEPARATOR;
					parse_table_row_into_cells(l);
				} else if (block->type == BLOCK_TABLE_SECTION) {
					l->type =  TABLE_ROW;
					parse_table_row_into_cells(l);
				} else {
					goto handle_line;
				}

			default:
				// token_describe(block, e->dstr->str);
				// fprintf(stderr, "Unspecified line type %d inside block type %d\n", l->type, block->type);
				// This is a block, need to remove it from chain and
				// Add to parent
				temp = l->next;

				sp_mmd_token_pop_link_from_chain(l);
				sp_mmd_token_append_child(block, l);

				// Advance to next line
				l = temp;
				break;
		}
	}

	// Free token chain of line types
	sp_mmd_token_tree_free(children);
}

/// Create a token chain from source string
/// stop_on_empty_line allows us to stop parsing part of the way through
token * sp_mmd_mmd_tokenize_string(mmd_engine * e, size_t start, size_t len, bool stop_on_empty_line) {
	// Reset metadata flag
	e->allow_meta = ((e->extensions & toInt(Extensions::Compatibility)) != 0) ? false : true;


	// Create a scanner (for re2c)
	Scanner s;
	s.start = &e->str[start];
	s.cur = s.start;

	// Strip trailing whitespace
//	while (len && char_is_whitespace_or_line_ending(str[len - 1]))
//		len--;

	// Where do we stop parsing?
	const char * stop = &e->str[start] + len;

	int type;								// TOKEN type
	token * t;								// Create tokens for incorporation

	token * root = sp_mmd_token_new(0, uint32_t(start), 0);		// Store the final parse tree here
	token * line = sp_mmd_token_new(0, uint32_t(start), 0);		// Store current line here

	const char * last_stop = &e->str[start];	// Remember where last token ended

	do {
		// Scan for next token (type of 0 means there is nothing left);
		type = scan(&s, stop);

		//if (type && s.start != last_stop) {
		if (s.start != last_stop) {
			// We skipped characters between tokens

			if (type) {
				// Create a default token type for the skipped characters
				t = sp_mmd_token_new(TEXT_PLAIN, (uint32_t)(last_stop - e->str), uint32_t(s.start - last_stop));

				sp_mmd_token_append_child(line, t);
			} else {
				if (stop > last_stop) {
					// Source text ends without newline
					t = sp_mmd_token_new(TEXT_PLAIN, (uint32_t)(last_stop - e->str), uint32_t(stop - last_stop));

					sp_mmd_token_append_child(line, t);
				}
			}
		} else if (type == 0 && stop > last_stop) {
			// Source text ends without newline
			t = sp_mmd_token_new(TEXT_PLAIN, (uint32_t)(last_stop - e->str), uint32_t(stop - last_stop));
			sp_mmd_token_append_child(line, t);
		}


		switch (type) {
			case 0:
				// 0 means we finished with input
				// Add current line to root

				// What sort of line is this?
				mmd_assign_line_type(e, line);

				sp_mmd_token_append_child(root, line);
				break;

			case TEXT_NL_SP:
			case TEXT_LINEBREAK_SP:
			case TEXT_LINEBREAK:
			case TEXT_NL:
				if (s.start <= s.cur) {
					// We hit the end of a line
					switch (type) {
						case TEXT_NL_SP:
							t = sp_mmd_token_new(TEXT_NL, (uint32_t)(s.start - e->str), (uint32_t)(s.cur - s.start) - 1);
							break;

						case TEXT_LINEBREAK_SP:
							t = sp_mmd_token_new(TEXT_LINEBREAK, (uint32_t)(s.start - e->str), (uint32_t)(s.cur - s.start) - 1);
							break;

						default:
							t = sp_mmd_token_new(type, (uint32_t)(s.start - e->str), (uint32_t)(s.cur - s.start));
							break;
					}
				} else {
					type = 0;
					continue;
				}

				sp_mmd_token_append_child(line, t);

				// What sort of line is this?
				mmd_assign_line_type(e, line);

				sp_mmd_token_append_child(root, line);

				// If this is first line, do we have proper metadata?
				if (e->allow_meta && root->child == line) {
					if (line->type == LINE_SETEXT_2) {
						line->type = LINE_YAML;
					} else if (line->type != LINE_META) {
						e->allow_meta = false;
					}
				}

				if (stop_on_empty_line) {
					if (line->type == LINE_EMPTY) {
						return root;
					}
				}

				switch (type) {
					case TEXT_NL_SP:
						line = sp_mmd_token_new(0, uint32_t(s.cur - e->str - 1), 0);
						t = sp_mmd_token_new(NON_INDENT_SPACE, uint32_t(s.cur - e->str - 1), 1);
						sp_mmd_token_append_child(line, t);
						break;

					case TEXT_LINEBREAK_SP:
						line = sp_mmd_token_new(0, uint32_t(s.cur - e->str - 1), 0);
						t = sp_mmd_token_new(NON_INDENT_SPACE, uint32_t(s.cur - e->str - 1), 1);
						sp_mmd_token_append_child(line, t);
						break;

					default:
						line = sp_mmd_token_new(0, uint32_t(s.cur - e->str), 0);
						break;
				}

				break;
			case ANGLE_LEFT:
				// scan for html ids
				t = sp_mmd_token_new(type, uint32_t(s.start - e->str), uint32_t(s.cur - s.start));
				if (scan_html(s.start)) {
					check_html_id(e, t, s.start);
				}
				sp_mmd_token_append_child(line, t);
				break;

			default:
				t = sp_mmd_token_new(type, uint32_t(s.start - e->str), uint32_t(s.cur - s.start));
				sp_mmd_token_append_child(line, t);
				break;
		}

		// Remember where token ends to detect skipped characters
		last_stop = s.cur;
	} while (type != 0);

	return root;
}

/// Ambidextrous tokens can open OR close a pair.  This routine gives the opportunity
/// to change this behavior on case-by-case basis.  For example, in `foo **bar** foo`, the
/// first set of asterisks can open, but not close a pair.  The second set can close, but not
/// open a pair.  This allows for complex behavior without having to bog down the tokenizer
/// with figuring out which type of asterisk we have.  Default behavior is that open and close
/// are enabled, so we just have to figure out when to turn it off.
void sp_mmd_assign_ambidextrous_tokens_in_block(mmd_engine * e, token * block, size_t start_offset) {
	if (block == NULL || block->child == NULL) {
		return;
	}

	size_t offset;		// Temp variable for use below
	size_t lead_count, lag_count, pre_count, post_count;

	token * t = block->child;

	const char * str = e->str;

	while (t != NULL) {
		switch (t->type) {
			case BLOCK_META:

				// Do we treat this like metadata?
				if ((e->extensions & toInt(Extensions::Compatibility)) == 0 &&
						(e->extensions & toInt(Extensions::NoMetadata)) == 0) {
					break;
				}

				// This is not metadata
				t->type = BLOCK_PARA;

			case DOC_START_TOKEN:
			case BLOCK_BLOCKQUOTE:
			case BLOCK_DEF_ABBREVIATION:
			case BLOCK_DEFLIST:
			case BLOCK_DEFINITION:
			case BLOCK_H1:
			case BLOCK_H2:
			case BLOCK_H3:
			case BLOCK_H4:
			case BLOCK_H5:
			case BLOCK_H6:
			case BLOCK_LIST_BULLETED:
			case BLOCK_LIST_BULLETED_LOOSE:
			case BLOCK_LIST_ENUMERATED:
			case BLOCK_LIST_ENUMERATED_LOOSE:
			case BLOCK_LIST_ITEM:
			case BLOCK_LIST_ITEM_TIGHT:
			case BLOCK_PARA:
			case BLOCK_SETEXT_1:
			case BLOCK_SETEXT_2:
			case BLOCK_TABLE:
			case BLOCK_TERM:
			case LINE_LIST_BULLETED:
			case LINE_LIST_ENUMERATED:
				// Assign child tokens of blocks
				sp_mmd_assign_ambidextrous_tokens_in_block(e, t, start_offset);
				break;

			case CRITIC_SUB_DIV:
				// Divide this into two tokens
				t->child = sp_mmd_token_new(CRITIC_SUB_DIV_B, t->start + 1, 1);
				t->child->next = t->next;
				t->next = t->child;
				t->child = NULL;
				t->len = 1;
				t->type = CRITIC_SUB_DIV_A;
				break;

			case STAR:
				// Look left and skip over neighboring '*' characters
				offset = t->start;

				while ((offset != 0) && ((str[offset] == '*') || (str[offset] == '_'))) {
					offset--;
				}

				// We can only close if there is something to left besides whitespace
				if ((offset == 0) || (chars::isWhitespaceOrLineEnding(str[offset]))) {
					// Whitespace or punctuation to left, so can't close
					t->can_close = 0;
				}

				// Look right and skip over neighboring '*' characters
				offset = t->start + 1;

				while ((str[offset] == '*') || (str[offset] == '_')) {
					offset++;
				}

				// We can only open if there is something to right besides whitespace/punctuation
				if (chars::isWhitespaceOrLineEnding(str[offset])) {
					// Whitespace to right, so can't open
					t->can_open = 0;
				}

				// If we're in the middle of a word, then we need to be more precise
				if (t->can_open && t->can_close) {
					lead_count = 0;			//!< '*' in run before current
					lag_count = 0;			//!< '*' in run after current
					pre_count = 0;			//!< '*' before word
					post_count = 0;			//!< '*' after word

					offset = t->start - 1;

					// How many '*' in this run before current token?
					while (offset && (str[offset] == '*')) {
						lead_count++;
						offset--;
					}

					// Skip over letters/numbers
					// TODO: Need to fix this to actually get run at beginning of word, not in middle,
					// e.g. **foo*bar*foo*bar**
					while (offset && (!chars::isWhitespaceOrLineEndingOrPunctuation(str[offset]))) {
						offset--;
					}

					// Are there '*' at the beginning of this word?
					while ((offset != maxOf<size_t>()) && (str[offset] == '*')) {
						pre_count++;
						offset--;
					}

					offset = t->start + 1;

					// How many '*' in this run after current token?
					while (str[offset] == '*') {
						lag_count++;
						offset++;
					}

					// Skip over letters/numbers
					// TODO: Same as above
					while (!chars::isWhitespaceOrLineEndingOrPunctuation(str[offset])) {
						offset++;
					}

					// Are there '*' at the end of this word?
					while (offset && (str[offset] == '*')) {
						post_count++;
						offset++;
					}

					// Are there '*' before/after word?
					if (pre_count + post_count > 0) {
						if (pre_count + post_count == lead_count + lag_count + 1) {
							// Same number outside as in the current run
							// **foo****bar**
							if (pre_count == post_count) {
								// **foo****bar**
								// We want to wrap the word, since
								// <strong>foo</strong><strong>bar</strong> doesn't make sense
								t->can_open = 0;
								t->can_close = 0;
							} else if (pre_count == 0) {
								// foo**bar**
								// Open only so we don't close outside the word
								t->can_close = 0;
							} else if (post_count == 0) {
								// **foo**bar
								// Close only so we don't close outside the word
								t->can_open = 0;
							}
						} else if (pre_count == lead_count + lag_count + 1 + post_count) {
							// ***foo**bar*
							// We want to close what's open
							t->can_open = 0;
						} else if (post_count == pre_count + lead_count + lag_count + 1) {
							// *foo**bar***
							// We want to open a set to close at the end
							t->can_close = 0;
						} else {
							if (pre_count != lead_count + lag_count + 1) {
								// **foo**bar -> close, otherwise don't
								t->can_close = 0;
							}

							if (post_count != lead_count + lag_count + 1) {
								// foo**bar** -> open, otherwise don't
								t->can_open = 0;
							}
						}
					}
				}

				break;

			case UL:
				// Look left and skip over neighboring '_' characters
				offset = t->start;

				while ((offset != 0) && ((str[offset] == '_') || (str[offset] == '*'))) {
					offset--;
				}

				if ((offset == 0) || (chars::isWhitespaceOrLineEnding(str[offset]))) {
					// Whitespace to left, so can't close
					t->can_close = 0;
				}

				// We don't allow intraword underscores (e.g.  `foo_bar_foo`)
				if ((offset > 0) && (chars::isAlphanumeric(str[offset]))) {
					// Letters to left, so can't open
					t->can_open = 0;
				}

				// Look right and skip over neighboring '_' characters
				offset = t->start + 1;

				while ((str[offset] == '*') || (str[offset] == '_')) {
					offset++;
				}

				if (chars::isWhitespaceOrLineEnding(str[offset])) {
					// Whitespace to right, so can't open
					t->can_open = 0;
				}

				if (chars::isAlphanumeric(str[offset])) {
					// Letters to right, so can't close
					t->can_close = 0;
				}

				break;

			case BACKTICK:
				// Backticks are used for code spans, but also for ``foo'' double quote syntax.
				// We care only about the quote syntax.
				offset = t->start;

				// TODO: This does potentially prevent ``foo `` from closing due to space before closer?
				// Bug or feature??
				if (t->len != 2) {
					break;
				}

				if ((offset == 0) || (str[offset] != '`' && chars::isWhitespaceOrLineEndingOrPunctuation(str[offset - 1]))) {
					// Whitespace or punctuation to left, so can't close
					t->can_close = 0;
				}

				break;

			case QUOTE_SINGLE:
				// Some of these are actually APOSTROPHE's and should not be paired
				offset = t->start;

				if (!((offset == 0) ||
				        (chars::isWhitespaceOrLineEndingOrPunctuation(str[offset - 1])) ||
				        (chars::isWhitespaceOrLineEndingOrPunctuation(str[offset + 1])))) {
					t->type = APOSTROPHE;
					break;
				}

				if (offset && (chars::isPunctuation(str[offset - 1])) &&
				        (chars::isAlphanumeric(str[offset + 1]))) {
					// If possessive apostrophe, e.g. `x`'s
					if (str[offset + 1] == 's' || str[offset + 1] == 'S') {
						if (chars::isWhitespaceOrLineEndingOrPunctuation(str[offset + 2])) {
							t->type = APOSTROPHE;
							break;
						}
					}
				}

			case QUOTE_DOUBLE:
				offset = t->start;

				if ((offset == 0) || (chars::isWhitespaceOrLineEnding(str[offset - 1]))) {
					t->can_close = 0;
				}

				if (chars::isWhitespaceOrLineEnding(str[offset + 1])) {
					t->can_open = 0;
				}

				break;

			case DASH_N:
				if ((e->extensions & toInt(Extensions::Smart)) == 0) {
					break;
				}

				// We want `1-2` to trigger a DASH_N, but regular hyphen otherwise (`a-b`)
				// This doesn't apply to `--` or `---`
				offset = t->start;

				if (t->len == 1) {
					// Check whether we have '1-2'
					if ((offset == 0) || (!chars::isDigit(str[offset - 1])) ||
					        (!chars::isDigit(str[offset + 1]))) {
						t->type = TEXT_PLAIN;
					}
				}

				break;

			case MATH_DOLLAR_SINGLE:
			case MATH_DOLLAR_DOUBLE:
				if ((e->extensions & toInt(Extensions::Compatibility)) != 0) {
					break;
				}

				offset = t->start;

				// Look left
				if ((offset == 0) || (chars::isWhitespaceOrLineEnding(str[offset - 1]))) {
					// Whitespace to left, so can't close
					t->can_close = 0;
				} else if ((offset != 0) && (!chars::isWhitespaceOrLineEndingOrPunctuation(str[offset - 1]))) {
					// No whitespace or punctuation to left, can't open
					t->can_open = 0;
				}

				// Look right
				offset = t->start + t->len;

				if (chars::isWhitespaceOrLineEnding(str[offset])) {
					// Whitespace to right, so can't open
					t->can_open = 0;
				} else if (!chars::isWhitespaceOrLineEndingOrPunctuation(str[offset])) {
					// No whitespace or punctuation to right, can't close
					t->can_close = 0;
				}

				break;

			case SUPERSCRIPT:
			case SUBSCRIPT:
				if ((e->extensions & toInt(Extensions::Compatibility)) != 0) {
					t->type = TEXT_PLAIN;
					break;
				}

				offset = t->start;

				// Look left -- no whitespace to left
				if ((offset == 0) || (chars::isWhitespaceOrLineEndingOrPunctuation(str[offset - 1]))) {
					t->can_open = 0;
				}

				if ((offset != 0) && (chars::isWhitespaceOrLineEnding(str[offset - 1]))) {
					t->can_close = 0;
				}

				offset = t->start + t->len;

				if (chars::isWhitespaceOrLineEndingOrPunctuation(str[offset])) {
					t->can_open = 0;
				}

				// We need to be contiguous in order to match
				if (t->can_close) {
					offset = t->start;
					t->can_close = 0;

					while ((offset > 0) && !(chars::isWhitespaceOrLineEnding(str[offset - 1]))) {
						if (str[offset - 1] == str[t->start]) {
							t->can_close = 1;
							break;
						}

						offset--;
					}
				}

				// We need to be contiguous in order to match
				if (t->can_open) {
					offset = t->start + t->len;
					t->can_open = 0;

					while (!(chars::isWhitespaceOrLineEnding(str[offset]))) {
						if (str[offset] == str[t->start]) {
							t->can_open = 1;
							break;
						}

						offset++;
					}

					// Are we a standalone, e.g x^2
					if (!t->can_close && !t->can_open) {
						offset = t->start + t->len;

						while (!chars::isWhitespaceOrLineEndingOrPunctuation(str[offset])) {
							offset++;
						}

						t->len = uint32_t(offset - t->start);
						t->can_close = 0;

						// Shift next token right and move those characters as child node
						// It's possible that one (or more?) tokens are entirely subsumed.
						while (t->next && t->next->start + t->next->len < offset) {
							tokens_prune(t->next, t->next);
						}

						if ((t->next != NULL) && ((t->next->type == TEXT_PLAIN) || (t->next->type == TEXT_NUMBER_POSS_LIST))) {
							t->next->len = uint32_t(t->next->start + t->next->len - offset);
							t->next->start = uint32_t(offset);
						}

						t->child = sp_mmd_token_new(TEXT_PLAIN, t->start + 1, t->len - 1);
					}
				}

				break;
		}

		t = t->next;
	}
}

}

NS_MMD_END
