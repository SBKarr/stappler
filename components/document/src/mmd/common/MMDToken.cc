/*

	Copyright © 2016 - 2017 Fletcher T. Penney.
	Copyright © 2017 Roman Katuntsev <sbkarr@stappler.org>

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
#include "MMDToken.h"
#include "MMDChars.h"
#include "MMDCore.h"

NS_MMD_BEGIN

Token::Token() { }

Token::Token(Token &&other) : _token(other._token) {
	other._token = nullptr;
}

Token &Token::operator=(Token &&other) {
	_token = other._token;
	other._token = nullptr;
	return *this;
}

/// Forward declaration
static void sp_mmd_print_token(std::ostream &stream, token * t, uint16_t depth, const StringView &source);
static void sp_mmd_print_token_tree(std::ostream &stream, token * t, uint16_t depth, const StringView &source);

/*static void sp_mmd_print_token_node(std::ostream &stream, token * t, const StringView &source) {
	stream << "(" << t->type << ") " << t->start << ":" << t->len;
	if (!source.empty()) {
		stream << " '" << StringView(source.data() + t->start, t->len) << "'";
	}
}*/

/// Print contents of the token based on specified string
static void sp_mmd_print_token(std::ostream &stream, token * t, uint16_t depth, const StringView &source) {
	if (t) {
		for (int i = 0; i < depth; ++i) {
			stream << '\t';
		}

		stream << "* (" << t->type << ") " << t->start << ":" << t->len << " " << t->can_open << ":" << t->can_close << ":" << t->unmatched;
		if (!source.empty()) {
			stream << "\t'" << StringView(source.data() + t->start, t->len) << "'";
		}
		/*if (t->mate) {
			stream << "\t-mate ";
			sp_mmd_print_token_node(stream, t->mate, source);
		}
		if (t->prev) {
			stream << "\t-prev ";
			sp_mmd_print_token_node(stream, t->prev, source);
		}
		if (t->next) {
			stream << "\t-next ";
			sp_mmd_print_token_node(stream, t->next, source);
		}
		if (t->tail) {
			stream << "\t-tail ";
			sp_mmd_print_token_node(stream, t->tail, source);
		}*/
		stream << "\n";

		if (t->child != NULL) {
			sp_mmd_print_token_tree(stream, t->child, depth + 1, source);
		}
	}
}

/// Print contents of the token tree based on specified string
static void sp_mmd_print_token_tree(std::ostream &stream, token * t, uint16_t depth, const StringView &source) {
	while (t != NULL) {
		sp_mmd_print_token(stream, t, depth, source);

		t = t->next;
	}
}


void Token::describe(std::ostream &stream, const StringView &source) {
	sp_mmd_print_token(stream, _token, 0, source);
}

void Token::describeTree(std::ostream &stream, const StringView &source) {
	stream << "=====>\n";
	sp_mmd_print_token_tree(stream, _token, 0, source);
	stream << "<=====\n";
}

Token::Token(token *t) : _token(t) { }

extern "C" {

using token = _sp_mmd_token;

/// Get pointer to a new token
token * sp_mmd_token_new(unsigned short type, uint32_t start, uint32_t len) {
	token * t = (token *)memory::pool::palloc(memory::pool::acquire(), sizeof(token));

	if (t) {
		t->type = type;
		t->start = start;
		t->len = len;

		t->next = nullptr;
		t->prev = nullptr;
		t->child = nullptr;

		t->tail = t;

		t->can_open = true;			//!< Default to true -- we assume openers can open and closers can close
		t->can_close = true;		//!< unless specified otherwise (e.g. for ambidextrous tokens)
		t->unmatched = true;

		t->mate = nullptr;
	}

	return t;
}


/// Duplicate an existing token
token * sp_mmd_token_copy(token * original) {
	token * t = (token *)memory::pool::palloc(memory::pool::acquire(), sizeof(token));

	if (t) {
		* t = * original;
	}

	return t;
}


/// Create a parent for a chain of tokens
token * sp_mmd_token_new_parent(token * child, unsigned short type) {
	if (child == NULL) {
		return sp_mmd_token_new(type, 0, 0);
	}

	token * t = sp_mmd_token_new(type, child->start, 0);
	t->child = child;
	child->prev = NULL;

	// Ensure that parent length correctly includes children
	if (child == NULL) {
		t->len = 0;
	} else if (child->next == NULL) {
		t->len = child->len;
	} else {
		while (child->next != NULL) {
			child = child->next;
		}

		t->len = child->start + child->len - t->start;
	}

	return t;
}


/// Add a new token to the end of a token chain.  The new token
/// may or may not also be the start of a chain
void sp_mmd_token_chain_append(token * chain_start, token * t) {
	if ((chain_start == NULL) ||
	        (t == NULL)) {
		return;
	}

	switch (t->type) {
	case BLOCK_HTML:
		break;
	default:
		break;
	}

	// Append t
	chain_start->tail->next = t;
	t->prev = chain_start->tail;

	// Adjust tail marker
	chain_start->tail = t->tail;
}


/// Add a new token to the end of a parent's child
/// token chain.  The new token may or may not be
/// the start of a chain.
void sp_mmd_token_append_child(token * parent, token * t) {
	if ((parent == NULL) || (t == NULL)) {
		return;
	}

	if (parent->child == NULL) {
		// Parent has no children
		parent->child = t;
	} else {
		// Append to to existing child chain
		sp_mmd_token_chain_append(parent->child, t);
	}

	// Set len on parent
	parent->len = parent->child->tail->start + parent->child->tail->len - parent->start;
}


/// Remove the first child of a token
void sp_mmd_token_remove_first_child(token * parent) {
	if ((parent == NULL) || (parent->child == NULL)) {
		return;
	}

	token * t = parent->child;
	parent->child = t->next;

	if (parent->child) {
		parent->child->prev = NULL;
		parent->child->tail = t->tail;
	}

	sp_mmd_token_free(t);
}


/// Remove the last child of a token
void sp_mmd_token_remove_last_child(token * parent) {
	if ((parent == NULL) || (parent->child == NULL)) {
		return;
	}

	token * t = parent->child->tail;

	if (t->prev) {
		t->prev->next = NULL;
		parent->child->tail = t->prev;
	}

	sp_mmd_token_free(t);
}


/// Remove the last token in a chain
void sp_mmd_token_remove_tail(token * head) {
	if ((head == NULL) || (head->tail == head)) {
		return;
	}

	token * t = head->tail;

	if (t->prev) {
		t->prev->next = NULL;
		head->tail = t->prev;
	}

	sp_mmd_token_free(t);
}


/// Pop token out of it's chain, connecting head and tail of chain back together.
/// Token must be freed if it is no longer needed.
/// \todo: If t is the tail token of a chain, the tail is no longer correct on the start of chain.
void sp_mmd_token_pop_link_from_chain(token * t) {
	if (t == NULL) {
		return;
	}

	token * prev = t->prev;
	token * next = t->next;

	t->next = NULL;
	t->prev = NULL;
	t->tail = t;

	if (prev) {
		prev->next = next;
	}

	if (next) {
		next->prev = prev;
	}
}


/// Remove one or more tokens from chain
void tokens_prune(token * first, token * last) {
	if (first == NULL || last == NULL) {
		return;
	}

	token * prev = first->prev;
	token * next = last->next;

	if (prev != NULL) {
		prev->next = next;
	}

	if (next != NULL) {
		next->prev = prev;
	}

	first->prev = NULL;
	last->next = NULL;

	sp_mmd_token_tree_free(first);
}


/// Given a start/stop point in token chain, create a new container token.
/// Return pointer to new container token.
token * sp_mmd_token_prune_graft(token * first, token * last, unsigned short container_type) {
	if (first == NULL || last == NULL) {
		return first;
	}

	token * next = last->next;

	// Duplicate first token -- this will be child of new container
	token * new_child = sp_mmd_token_copy(first);
	new_child->prev = NULL;
	new_child->tail = last;

	if (new_child->next) {
		new_child->next->prev = new_child;
	}

	// Swap last (if necessary)
	if (first == last) {
		last = new_child;
	}

	// Existing first token will be new container
	first->child = new_child;
	first->type = container_type;
	first->len = last->start + last->len - first->start;
	first->next = next;
	first->can_close = 0;
	first->can_open = 0;

	// Fix mating
	if (first->mate) {
		first->mate = NULL;
		new_child->mate->mate = new_child;
	}

	// Disconnect last token
	last->next = NULL;

	if (next) {
		next->prev = first;
	}

	return first;
}


/// Free token
void sp_mmd_token_free(token * t) {
	return;
}

/// Free token chain
void sp_mmd_token_tree_free(token * t) {
	return;
}

/// Find the child node of a given parent that contains the specified
/// offset position.
token * sp_mmd_token_child_for_offset(
    token * parent,						//!< Pointer to parent token
	uint32_t offset						//!< Search position
) {
	if (parent == NULL) {
		return NULL;
	}

	if ((parent->start > offset) ||
	        (parent->start + parent->len < offset)) {
		return NULL;
	}

	token * walker = parent->child;

	while (walker != NULL) {
		if (walker->start <= offset) {
			if (walker->start + walker->len > offset) {
				return walker;
			}
		}

		if (walker->start > offset) {
			return NULL;
		}

		walker = walker->next;
	}

	return NULL;
}


/// Given two character ranges, see if they intersect (touching doesn't count)
static bool ranges_intersect(size_t start1, size_t len1, size_t start2, size_t len2) {
	return ((start1 < start2 + len2) && (start2 < start1 + len1)) ? true : false;
}

/// Find first child node of a given parent that intersects the specified
/// offset range.
token * sp_mmd_token_first_child_in_range(
    token * parent,						//!< Pointer to parent token
	uint32_t start,						//!< Start search position
	uint32_t len							//!< Search length
) {
	if (parent == NULL) {
		return NULL;
	}

	if ((parent->start > start + len) ||
	        (parent->start + parent->len < start)) {
		return NULL;
	}

	token * walker = parent->child;

	while (walker != NULL) {
		if (ranges_intersect(start, len, walker->start, walker->len)) {
			return walker;
		}

		if (walker->start > start) {
			return NULL;
		}

		walker = walker->next;
	}

	return NULL;
}


/// Find last child node of a given parent that intersects the specified
/// offset range.
token * sp_mmd_token_last_child_in_range(
    token * parent,						//!< Pointer to parent token
	uint32_t start,						//!< Start search position
	uint32_t len							//!< Search length
) {
	if (parent == NULL) {
		return NULL;
	}

	if ((parent->start > start + len) ||
	        (parent->start + parent->len < start)) {
		return NULL;
	}

	token * walker = parent->child;
	token * last = NULL;

	while (walker != NULL) {
		if (ranges_intersect(start, len, walker->start, walker->len)) {
			last = walker;
		}

		if (walker->start > start + len) {
			return last;
		}

		walker = walker->next;
	}

	return last;
}


void sp_mmd_token_trim_leading_whitespace(token * t, const char * string) {
	while (t->len && chars::isWhitespace(string[t->start])) {
		t->start++;
		t->len--;
	}
}


void sp_mmd_token_trim_trailing_whitespace(token * t, const char * string) {
	while (t->len && chars::isWhitespace(string[t->start + t->len - 1])) {
		t->len--;
	}
}


void sp_mmd_token_trim_whitespace(token * t, const char * string) {
	sp_mmd_token_trim_leading_whitespace(t, string);
	sp_mmd_token_trim_trailing_whitespace(t, string);
}


/// Check whether first token in the chain matches the given type.
/// If so, return and advance the chain.
token * sp_mmd_token_chain_accept(token ** t, short type) {
	token * result = NULL;

	if (t && *t && ((*t)->type == type)) {
		result = *t;
		*t = (*t)->next;
	}

	return result;
}


/// Allow checking for multiple token types
token * sp_mmd_token_chain_accept_multiple(token ** t, int n, ...) {
	token * result = NULL;
	va_list valist;

	va_start(valist, n);

	for (int i = 0; i < n; ++i) {
		result = sp_mmd_token_chain_accept(t, va_arg(valist, int));

		if (result) {
			break;
		}
	}

	va_end(valist);

	return result;
}


void sp_mmd_token_skip_until_type(token ** t, short type) {
	while ((*t) && ((*t)->type != type)) {
		*t = (*t)->next;
	}
}


/// Allow checking for multiple token types
void sp_mmd_token_skip_until_type_multiple(token ** t, int n, ...) {
	va_list valist;
	int type[n];

	va_start(valist, n);

	// Load target types
	for (int i = 0; i < n; ++i) {
		type[i] = va_arg(valist, int);
	}

	//
	while (*t) {
		for (int i = 0; i < n; ++i) {
			if ((*t)->type == type[i]) {
				return;
			}
		}

		*t = (*t)->next;
	}

	va_end(valist);
}


void sp_mmd_token_split_on_char(token * t, const char * source, const char c) {
	if (!t) {
		return;
	}

	size_t start = t->start;
	uint32_t pos = 0;
	uint32_t stop = t->len;
	token * n = NULL;

	while (pos + 1 < stop) {
		if (source[start + pos] == c) {
			n = sp_mmd_token_new(t->type, uint32_t(start + pos + 1), stop - (pos + 1));
			n->next = t->next;
			t->next = n;

			t->len = pos;

			t = t->next;
		}

		pos++;
	}
}


// Split a token and create new ones as needed
void sp_mmd_token_split(token * t, uint32_t start, uint32_t len, unsigned short new_type) {
	if (!t) {
		return;
	}

	size_t stop = start + len;

	if (start < t->start) {
		return;
	}

	if (stop > t->start + t->len) {
		return;
	}

	token * A;		// This will be new token
	bool inset_start = false;
	bool inset_stop = false;

	// Will we need a leading token?
	if (start > t->start) {
		inset_start = true;
	}

	// Will we need a lagging token?
	if (stop < t->start + t->len) {
		inset_stop = true;
	}


	if (inset_start) {
		A = sp_mmd_token_new(new_type, start, len);

		if (inset_stop) {
			// We will end up with t->A->T2

			// Create T2
			token * T2 = sp_mmd_token_new(t->type, uint32_t(stop), uint32_t(t->start + t->len - stop));
			T2->next = t->next;

			if (t->next) {
				t->next->prev = T2;
			}

			A->next = T2;
			T2->prev = A;
		} else {
			// We will end up with T->A
			A->next = t->next;

			if (t->next) {
				t->next->prev = A;
			}
		}

		t->next = A;
		A->prev = t;

		t->len = start - t->start;
	} else {
		if (inset_stop) {
			// We will end up with A->T
			// But we swap the tokens to ensure we don't
			// cause difficulty pointing to this chain,
			// resulting in T->A, where T is the new type
			A = sp_mmd_token_new(t->type, uint32_t(stop), uint32_t(t->start + t->len - stop));
			A->prev = t;
			A->next = t->next;
			t->next = A;

			if (A->next) {
				A->next->prev = A;
			}

			t->len = uint32_t(stop - t->start);
			t->type = new_type;
		} else {
			// We will end up with A
			t->type = new_type;
		}
	}
}

}

NS_MMD_END
