
#ifndef MMD_COMMON_MMDCORE_H_
#define MMD_COMMON_MMDCORE_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _sp_mmd_token {
	uint16_t type;			//!< Type for the token
	bool can_open;		//!< Can token open a matched pair?
	bool can_close;		//!< Can token close a matched pair?
	bool unmatched;		//!< Has token been matched yet?

	uint32_t start;			//!< Starting offset in the source string
	uint32_t len;			//!< Length of the token in the source string

	struct _sp_mmd_token * next;			//!< Pointer to next token in the chain
	struct _sp_mmd_token * prev;			//!< Pointer to previous marker in the chain
	struct _sp_mmd_token * child;			//!< Pointer to child chain

	struct _sp_mmd_token * tail;			//!< Pointer to last token in the chain

	struct _sp_mmd_token * mate;			//!< Pointer to other token in matched pair
} _sp_mmd_token;

/// Get pointer to a new token
_sp_mmd_token * sp_mmd_token_new( unsigned short type, uint32_t start, uint32_t len );

/// Duplicate an existing token
_sp_mmd_token * sp_mmd_token_copy( _sp_mmd_token * original );

/// Create a parent for a chain of tokens
_sp_mmd_token * sp_mmd_token_new_parent( _sp_mmd_token * child, unsigned short type );

/// Add a new token to the end of a token chain.  The new token
/// may or may not also be the start of a chain
void sp_mmd_token_chain_append( _sp_mmd_token * chain_start, _sp_mmd_token * t	);

/// Add a new token to the end of a parent's child
/// token chain.  The new token may or may not be
/// the start of a chain.
void sp_mmd_token_append_child( _sp_mmd_token * parent, _sp_mmd_token * t );

/// Remove the first child of a token
void sp_mmd_token_remove_first_child( _sp_mmd_token * parent );

/// Remove the last child of a token
void sp_mmd_token_remove_last_child( _sp_mmd_token * parent );

/// Remove the last token in a chain
void sp_mmd_token_remove_tail(_sp_mmd_token * head);

/// Pop token out of it's chain, connecting head and tail of chain back together.
/// Token must be freed if it is no longer needed.
void sp_mmd_token_pop_link_from_chain( _sp_mmd_token * t );

/// Remove one or more tokens from chain
void tokens_prune( _sp_mmd_token * first, _sp_mmd_token * last );

/// Given a start/stop point in token chain, create a new parent token.
/// Reinsert the new parent in place of the removed segment.
/// Return pointer to new container token.
_sp_mmd_token * sp_mmd_token_prune_graft( _sp_mmd_token * first, _sp_mmd_token * last, unsigned short container_type );

/// Free token
void sp_mmd_token_free( _sp_mmd_token * t );

/// Free token tree
void sp_mmd_token_tree_free( _sp_mmd_token * t );

/// Find the child node of a given parent that contains the specified
/// offset position.
_sp_mmd_token * sp_mmd_token_child_for_offset( _sp_mmd_token * parent,	uint32_t offset );

/// Find first child node of a given parent that intersects the specified
/// offset range.
_sp_mmd_token * sp_mmd_token_first_child_in_range( _sp_mmd_token * parent, uint32_t start, uint32_t len );

/// Find last child node of a given parent that intersects the specified
/// offset range.
_sp_mmd_token * sp_mmd_token_last_child_in_range( _sp_mmd_token * parent, uint32_t start, uint32_t len );

void sp_mmd_token_trim_leading_whitespace(_sp_mmd_token * t, const char * string);
void sp_mmd_token_trim_trailing_whitespace(_sp_mmd_token * t, const char * string);
void sp_mmd_token_trim_whitespace(_sp_mmd_token * t, const char * string);

///
_sp_mmd_token * sp_mmd_token_chain_accept(_sp_mmd_token ** t, short type);
_sp_mmd_token * sp_mmd_token_chain_accept_multiple(_sp_mmd_token ** t, int n, ...);

void sp_mmd_token_skip_until_type(_sp_mmd_token ** t, short type);
void sp_mmd_token_skip_until_type_multiple(_sp_mmd_token ** t, int n, ...);
void sp_mmd_token_split_on_char(_sp_mmd_token * t, const char * source, const char c);
void sp_mmd_token_split(_sp_mmd_token * t, uint32_t start, uint32_t len, unsigned short new_type);


typedef void _sp_mmd_stack;

/// Add a new pointer to the stack
void sp_mmd_stack_push( _sp_mmd_stack * s, void * element );


typedef struct {
	const char *str;
	uint32_t len;
	uint32_t extensions;
	uint16_t recurse_depth;
	bool allow_meta;

	_sp_mmd_token * root;

	//_sp_mmd_stack * abbreviation_stack;
	//_sp_mmd_stack * citation_stack;
	//_sp_mmd_stack * footnote_stack;
	//_sp_mmd_stack * glossary_stack;
	//_sp_mmd_stack * link_stack;
	//_sp_mmd_stack * metadata_stack;

	_sp_mmd_stack * definition_stack;
	_sp_mmd_stack * header_stack;
	_sp_mmd_stack * table_stack;

	void * content;
} _sp_mmd_engine;

void sp_mmd_recursive_parse_indent(_sp_mmd_engine * e, _sp_mmd_token * block);
void sp_mmd_recursive_parse_list_item(_sp_mmd_engine * e, _sp_mmd_token * block);
void sp_mmd_recursive_parse_blockquote(_sp_mmd_engine * e, _sp_mmd_token * block);
void sp_mmd_strip_line_tokens_from_block(_sp_mmd_engine * e, _sp_mmd_token * block);
void sp_mmd_is_para_html(_sp_mmd_engine * e, _sp_mmd_token * block);
void sp_mmd_is_list_loose(_sp_mmd_token * list);

void sp_mmd_parse_token_chain(_sp_mmd_engine * e, _sp_mmd_token * chain);
void sp_mmd_assign_ambidextrous_tokens_in_block(_sp_mmd_engine * e, _sp_mmd_token * block, size_t start_offset);
_sp_mmd_token * sp_mmd_mmd_tokenize_string(_sp_mmd_engine * e, size_t start, size_t len, bool stop_on_empty_line);

/// Token types for parse tree
enum token_types {
	DOC_START_TOKEN = 0,	//!< DOC_START_TOKEN must be type 0

	BLOCK_BLOCKQUOTE = 50,		//!< This must start *after* the largest number in parser.h
	BLOCK_CODE_FENCED,
	BLOCK_CODE_INDENTED,
	BLOCK_DEFLIST,
	BLOCK_DEFINITION,
	BLOCK_DEF_ABBREVIATION,
	BLOCK_DEF_CITATION,
	BLOCK_DEF_GLOSSARY,
	BLOCK_DEF_FOOTNOTE,
	BLOCK_DEF_LINK,
	BLOCK_EMPTY,
	BLOCK_HEADING,				//!< Placeholder for theme cascading
	BLOCK_H1,					//!< Leave H1, H2, etc. in order
	BLOCK_H2,
	BLOCK_H3,
	BLOCK_H4,
	BLOCK_H5,
	BLOCK_H6,
	BLOCK_HR,
	BLOCK_HTML,
	BLOCK_LIST_BULLETED,
	BLOCK_LIST_BULLETED_LOOSE,
	BLOCK_LIST_ENUMERATED,
	BLOCK_LIST_ENUMERATED_LOOSE,
	BLOCK_LIST_ITEM,
	BLOCK_LIST_ITEM_TIGHT,
	BLOCK_META,
	BLOCK_PARA,
	BLOCK_SETEXT_1,
	BLOCK_SETEXT_2,
	BLOCK_TABLE,
	BLOCK_TABLE_HEADER,
	BLOCK_TABLE_SECTION,
	BLOCK_TERM,
	BLOCK_TOC,

	CRITIC_ADD_OPEN,
	CRITIC_ADD_CLOSE,
	CRITIC_DEL_OPEN,
	CRITIC_DEL_CLOSE,
	CRITIC_COM_OPEN,
	CRITIC_COM_CLOSE,
	CRITIC_SUB_OPEN,
	CRITIC_SUB_DIV,
	CRITIC_SUB_DIV_A,
	CRITIC_SUB_DIV_B,
	CRITIC_SUB_CLOSE,
	CRITIC_HI_OPEN,
	CRITIC_HI_CLOSE,

	PAIR_CRITIC_ADD,
	PAIR_CRITIC_DEL,
	PAIR_CRITIC_COM,
	PAIR_CRITIC_SUB_ADD,
	PAIR_CRITIC_SUB_DEL,
	PAIR_CRITIC_HI,

	PAIRS,			//!< Placeholder for theme cascading
	PAIR_ANGLE,
	PAIR_BACKTICK,
	PAIR_BRACKET,
	PAIR_BRACKET_ABBREVIATION,
	PAIR_BRACKET_FOOTNOTE,
	PAIR_BRACKET_GLOSSARY,
	PAIR_BRACKET_CITATION,
	PAIR_BRACKET_IMAGE,
	PAIR_BRACKET_VARIABLE,
	PAIR_BRACE,
	PAIR_EMPH,
	PAIR_MATH,
	PAIR_PAREN,
	PAIR_QUOTE_SINGLE,
	PAIR_QUOTE_DOUBLE,
	PAIR_QUOTE_ALT,
	PAIR_RAW_FILTER,
	PAIR_SUBSCRIPT,
	PAIR_SUPERSCRIPT,
	PAIR_STAR,
	PAIR_STRONG,
	PAIR_UL,
	PAIR_BRACES,

	STAR,
	UL,
	EMPH_START,
	EMPH_STOP,
	STRONG_START,
	STRONG_STOP,

	BRACKET_LEFT,
	BRACKET_RIGHT,
	BRACKET_ABBREVIATION_LEFT,
	BRACKET_FOOTNOTE_LEFT,
	BRACKET_GLOSSARY_LEFT,
	BRACKET_CITATION_LEFT,
	BRACKET_IMAGE_LEFT,
	BRACKET_VARIABLE_LEFT,

	PAREN_LEFT,
	PAREN_RIGHT,

	ANGLE_LEFT,
	ANGLE_RIGHT,

	BRACE_DOUBLE_LEFT,
	BRACE_DOUBLE_RIGHT,

	AMPERSAND,
	AMPERSAND_LONG,
	APOSTROPHE,
	BACKTICK,
	CODE_FENCE,
	COLON,
	DASH_M,
	DASH_N,
	ELLIPSIS,
	QUOTE_SINGLE,
	QUOTE_DOUBLE,
	QUOTE_LEFT_SINGLE,
	QUOTE_RIGHT_SINGLE,
	QUOTE_LEFT_DOUBLE,
	QUOTE_RIGHT_DOUBLE,
	QUOTE_RIGHT_ALT,

	ESCAPED_CHARACTER,

	HTML_ENTITY,
	HTML_COMMENT_START,
	HTML_COMMENT_STOP,
	PAIR_HTML_COMMENT,

	MATH_PAREN_OPEN,
	MATH_PAREN_CLOSE,
	MATH_BRACKET_OPEN,
	MATH_BRACKET_CLOSE,
	MATH_DOLLAR_SINGLE,
	MATH_DOLLAR_DOUBLE,

	EQUAL,
	PIPE,
	PLUS,
	SLASH,

	SUPERSCRIPT,
	SUBSCRIPT,

	INDENT_TAB,
	INDENT_SPACE,
	NON_INDENT_SPACE,

	HASH1,							//!< Leave HASH1, HASH2, etc. in order
	HASH2,
	HASH3,
	HASH4,
	HASH5,
	HASH6,
	MARKER_BLOCKQUOTE,
	MARKER_H1,						//!< Leave MARKER_H1, MARKER_H2, etc. in order
	MARKER_H2,
	MARKER_H3,
	MARKER_H4,
	MARKER_H5,
	MARKER_H6,
	MARKER_LIST_BULLET,
	MARKER_LIST_ENUMERATOR,

	TABLE_ROW,
	TABLE_CELL,
	TABLE_DIVIDER,

	TOC,

	TEXT_BACKSLASH,
	RAW_FILTER_LEFT,
	TEXT_BRACE_LEFT,
	TEXT_BRACE_RIGHT,
	TEXT_EMPTY,
	TEXT_HASH,
	TEXT_LINEBREAK,
	TEXT_LINEBREAK_SP,
	TEXT_NL,
	TEXT_NL_SP,
	TEXT_NUMBER_POSS_LIST,
	TEXT_PERCENT,
	TEXT_PERIOD,
	TEXT_PLAIN,

	MANUAL_LABEL,
};

#define LINE_HR                          1
#define LINE_SETEXT_1                    2
#define LINE_SETEXT_2                    3
#define LINE_YAML                        4
#define LINE_CONTINUATION                5
#define LINE_PLAIN                       6
#define LINE_INDENTED_TAB                7
#define LINE_INDENTED_SPACE              8
#define LINE_TABLE                       9
#define LINE_TABLE_SEPARATOR            10
#define LINE_FALLBACK                   11
#define LINE_HTML                       12
#define LINE_ATX_1                      13
#define LINE_ATX_2                      14
#define LINE_ATX_3                      15
#define LINE_ATX_4                      16
#define LINE_ATX_5                      17
#define LINE_ATX_6                      18
#define LINE_BLOCKQUOTE                 19
#define LINE_LIST_BULLETED              20
#define LINE_LIST_ENUMERATED            21
#define LINE_DEF_ABBREVIATION           22
#define LINE_DEF_CITATION               23
#define LINE_DEF_FOOTNOTE               24
#define LINE_DEF_GLOSSARY               25
#define LINE_DEF_LINK                   26
#define LINE_TOC                        27
#define LINE_DEFINITION                 28
#define LINE_META                       29
#define LINE_BACKTICK                   30
#define LINE_FENCE_BACKTICK_3           31
#define LINE_FENCE_BACKTICK_4           32
#define LINE_FENCE_BACKTICK_5           33
#define LINE_FENCE_BACKTICK_START_3     34
#define LINE_FENCE_BACKTICK_START_4     35
#define LINE_FENCE_BACKTICK_START_5     36
#define LINE_STOP_COMMENT               37
#define LINE_EMPTY                      38
#define LINE_START_COMMENT              39


/// Re2c scanner data -- this structure is used by the re2c
/// lexer to track progress and offsets within the source
/// string.  They can be used to create "tokens" that match
/// sections of the text with an abstract syntax tree.
struct Scanner {
	const char *	start;		//!< Start of current token
	const char *	cur;		//!< Character currently being matched
	const char *	ptr;		//!< Used for backtracking by re2c
	const char *	ctx;
};

typedef struct Scanner Scanner;

/// Scan for the next token
int scan(
    Scanner * s,			//!< Pointer to Scanner state structure
    const char * stop		//!< Pointer to position in string at which to stop parsing
);

enum alignments {
	ALIGN_LEFT      = 1 << 0,
	ALIGN_RIGHT     = 1 << 1,
	ALIGN_CENTER    = 1 << 2,
	ALIGN_WRAP      = 1 << 3
};

size_t scan_alignment_string(const char * c);
size_t scan_attr(const char * c);
size_t scan_attributes(const char * c);
size_t scan_atx(const char * c);
size_t scan_definition(const char * c);
size_t scan_destination(const char * c);
size_t scan_email(const char * c);
size_t scan_fence_start(const char * c);
size_t scan_fence_end(const char * c);
size_t scan_html(const char * c);
size_t scan_html_block(const char * c);
size_t scan_html_comment(const char * c);
size_t scan_html_line(const char * c);
size_t scan_key(const char * c);
size_t scan_meta_key(const char * c);
size_t scan_meta_line(const char * c);
size_t scan_ref_abbreviation(const char * c);
size_t scan_ref_citation(const char * c);
size_t scan_ref_foot(const char * c);
size_t scan_ref_glossary(const char * c);
size_t scan_ref_link(const char * c);
size_t scan_ref_link_no_attributes(const char * c);
size_t scan_setext(const char * c);
size_t scan_spnl(const char * c);
size_t scan_table_separator(const char * c);
size_t scan_title(const char * c);
size_t scan_url(const char * c);
size_t scan_value(const char * c);

#ifdef __cplusplus
} // extern c
#endif

#endif /* MMD_COMMON_MMDCORE_H_ */
