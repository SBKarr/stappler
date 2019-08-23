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
#include "MMDContent.h"
#include "MMDChars.h"
#include "MMDToken.h"
#include "MMDCore.h"
#include "SPString.h"

NS_MMD_BEGIN

Extensions DefaultExtensions = Extensions::Critic | Extensions::Notes | Extensions::Smart;
Extensions StapplerExtensions = Extensions::Critic | Extensions::Notes | Extensions::Smart | Extensions::StapplerLayout;

static auto clean_inside_pair(const StringView & source, token * t, bool lowercase) -> Content::String {
	auto pairText = text_inside_pair(source, t);
	return Content::cleanString(pairText, lowercase);
}

static auto clean_inside_pair(const StringView & source, token * t, bool lowercase, Content::Footnote::Type type) -> Content::String {
	auto pairText = text_inside_pair(source, t);
	pairText.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	switch (type) {
	case Content::Footnote::Abbreviation:
	case Content::Footnote::Glossary:
		++ pairText;
		pairText.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		break;
	default:
		break;
	}

	return Content::cleanString(pairText, lowercase);
}

/*static auto clean_string_from_token(const StringView & s, token * t, bool lowercase) -> Content::String {
	return clean_string_from_range(s, t->start, t->len, lowercase);
}*/

static void strip_leading_whitespace(const StringView & s, token * chain) {
	while (chain) {
		switch (chain->type) {
			case INDENT_TAB:
			case INDENT_SPACE:
			case NON_INDENT_SPACE:
				chain->type = TEXT_EMPTY;

			case TEXT_EMPTY:
				chain = chain->next;
				break;

			case TEXT_PLAIN:
				sp_mmd_token_trim_leading_whitespace(chain, s.data());

			default:
				return;
		}

		if (chain) {
			chain = chain->next;
		}
	}
}

static void whitespace_accept(token ** remainder) {
	while (sp_mmd_token_chain_accept_multiple(remainder, 3, NON_INDENT_SPACE, INDENT_SPACE, INDENT_TAB));
}

static bool validate_url(const StringView & url) {
	if (url.empty()) {
		return false;
	}
	size_t len = scan_url(url.data());
	return (len && len == url.size()) ? true : false;
}

static auto destination_accept(const StringView & s, token ** remainder, bool validate) -> Content::String {
	const char *source = s.data();
	StringView url;
	Content::String clean;
	token * t = NULL;
	size_t start;
	size_t scan_len;

	if (*remainder == NULL) {
		return url.str<memory::PoolInterface>();
	}

	switch ((*remainder)->type) {
		case PAIR_PAREN:
		case PAIR_ANGLE:
		case PAIR_QUOTE_SINGLE:
		case PAIR_QUOTE_DOUBLE:
			t = sp_mmd_token_chain_accept_multiple(remainder, 2, PAIR_ANGLE, PAIR_PAREN);
			url = text_inside_pair(source, t);
			break;

		default:
			start = (*remainder)->start;

			// Skip any whitespace
			while (chars::isWhitespace(source[start])) {
				start++;
			}

			scan_len = scan_destination(&source[start]);

			// Grab destination string
			url = StringView(&source[start], scan_len);

			// Advance remainder to end of destination
			while ((*remainder)->next &&
			        (*remainder)->next->start < start + scan_len) {
				*remainder = (*remainder)->next;
			}

			t = (*remainder);	// We need to remember this for below
			// Move remainder beyond destination
			*remainder = (*remainder)->next;

			// Is there a space in a URL concatenated with a title or attribute?
			// e.g. [foo]: http://foo.bar/ class="foo"
			// Since only one space between URL and class, they are joined.

			if (t->type == TEXT_PLAIN) {
				// Trim leading whitespace
				sp_mmd_token_trim_leading_whitespace(t, source);
				sp_mmd_token_split_on_char(t, source, ' ');
				*remainder = t->next;
			}

			break;
	}

	// Is this a valid URL?
	clean = Content::cleanString(url, false);

	if (validate && !validate_url(clean)) {
		return Content::String();
	}

	return clean;
}

StringView text_inside_pair(const StringView & s, token * pair) {
	const char * source = s.data();

	if (source && pair) {
		if (pair->child && pair->child->mate) {
			// [foo], [^foo], [#foo] should give different strings -- use closer len
			return StringView(&source[pair->start + pair->child->mate->len], pair->len - (pair->child->mate->len * 2));
		} else {
			if (pair->child) {
				return StringView(&source[pair->start + pair->child->len], pair->len - (pair->child->len + 1));
			}
		}
	}

	return StringView();
}

auto url_accept(const char * source, size_t start, size_t max_len, size_t * end_pos, bool validate) -> Content::String {
	StringView url;
	Content::String clean;
	size_t scan_len;

	scan_len = scan_destination(&source[start]);

	if (scan_len) {
		if (scan_len > max_len) {
			scan_len = max_len;
		}

		if (end_pos) {
			*end_pos = start + scan_len;
		}

		// Is this <foo>?
		if ((source[start] == '<') &&
		        (source[start + scan_len - 1] == '>')) {
			// Strip '<' and '>'
			start++;
			scan_len -= 2;
		}

		url = StringView(&source[start], scan_len);

		clean = Content::cleanString(url, false);

		if (validate && !validate_url(clean)) {
			return Content::String();
		}
	}

	return clean;
}

auto label_from_token(const StringView & s, token * t) -> Content::String {
	const char * source = s.data();
	return Content::labelFromString(StringView(&source[t->start], t->len));
}

auto clean_string_from_range(const StringView & s, size_t start, size_t len, bool lowercase) -> Content::String {
	const char * source = s.data();
	return Content::cleanString(StringView(&source[start], len), lowercase);
}

token * manual_label_from_header(token * h, const char * source) {
	if (!h || !h->child) {
		return NULL;
	}

	token * walker = h->child->tail;
	token * label = NULL;
	short count = 0;

	while (walker) {
		switch (walker->type) {
			case MANUAL_LABEL:
				// Already identified
				label = walker;
				walker = NULL;
				break;

			case INDENT_TAB:
			case INDENT_SPACE:
			case NON_INDENT_SPACE:
			case TEXT_NL:
			case TEXT_LINEBREAK:
			case TEXT_EMPTY:
			case MARKER_H1:
			case MARKER_H2:
			case MARKER_H3:
			case MARKER_H4:
			case MARKER_H5:
			case MARKER_H6:
				walker = walker->prev;
				break;

			case TEXT_PLAIN:
				if (walker->len == 1) {
					if (source[walker->start] == ' ') {
						walker = walker->prev;
						break;
					}
				}

				walker = NULL;
				break;

			case PAIR_BRACKET:
				label = walker;

				while (walker && walker->type == PAIR_BRACKET) {
					walker = walker->prev;
					count++;
				}

				if (count % 2 == 0) {
					// Even count
					label = NULL;
				} else {
					// Odd count
					label->type = MANUAL_LABEL;
				}

			default:
				walker = NULL;
		}
	}

	return label;
}

bool table_has_caption(token * t) {
	if (t->next && t->next->type == BLOCK_PARA) {
		t = t->next->child;

		if (t->type == PAIR_BRACKET) {
			t = t->next;

			if (t && t->next &&
			        t->next->type == PAIR_BRACKET) {
				t = t->next;
			}

			if (t && t->next &&
			        ((t->next->type == TEXT_NL) ||
			         (t->next->type == TEXT_LINEBREAK))) {
				t = t->next;
			}

			if (t && t->next == NULL) {
				return true;
			}
		}
	}

	return false;
}


Content::Link::Link(const StringView &source, Token && l, String && u, const StringView & t, const StringView & attr, bool clearUrl)
: label(move(l)) {
	if (label) {
		clean_text = clean_inside_pair(source, label, true);
		label_text = label_from_token(source, label);
	}

	url = clearUrl ? Content::cleanString(StringView(u), false) : move(u);

	StringView r(t);
	StringStream stream;
	while (!r.empty()) {
		stream << r.readUntil<StringView::Chars<'"'>>();
		if (r.is('"')) {
			stream << "&quot;";
			++ r;
		}
	}

	title = stream.str();
	parseAttributes(attributes, attr);
}

Content::Link::Link(const StringView &source, Token && l)
: label(move(l)) {
	clean_text = source.str<memory::PoolInterface>();
	label_text = source.str<memory::PoolInterface>();
	url = String("#") + source.str<memory::PoolInterface>();
}

void Content::Link::parseAttributes(AttrVec &attr, const StringView &str) {
	StringView key;
	StringView value;
	size_t scan_len;

	StringView r(str);

	while (!r.empty() && scan_attr(r.data())) {
		r += scan_spnl(r.data());

		// Get key
		scan_len = scan_key(r.data());
		key = StringView(r.data(), scan_len);
		r += scan_len + 1; // Skip '='

		// Get value
		scan_len = scan_value(r.data());
		value = StringView(r.data(), scan_len);
		r += scan_len;

		value.trimChars<StringView::Chars<'"'>>();
		attr.emplace_back(key, value);
	}
}

Content::Footnote::Footnote(const StringView &source, Token && l, Token && c, bool lowercase, Type type)
: label(move(l)) {
	if (label) {
		clean_text = clean_inside_pair(source, label, lowercase, type);
		label_text = label_from_token(source, label);
	}

	if (c) {
		switch (c.getToken()->type) {
			case BLOCK_PARA:
				content = move(c);
				break;

			case TEXT_PLAIN:
				sp_mmd_token_trim_leading_whitespace(c, source.data());

			default:
				content = Token(sp_mmd_token_new_parent(c, BLOCK_PARA));
				free_para = true;
				break;
		}
	}
}

auto Content::labelFromString(const StringView &str) -> String {
	StringView r(str);
	String ret; ret.reserve(str.size());
	while (!r.empty()) {
		auto tmp = r.readChars<
				StringView::CharGroup<CharGroupId::Alphanumeric>,
				StringView::Chars<'.', '_', ':', '-'>,
				stappler::chars::UniChar>();

		ret.append(tmp.data(), tmp.size());
		r.skipUntil<
				StringView::CharGroup<CharGroupId::Alphanumeric>,
				StringView::Chars<'.', '_', ':', '-'>,
				stappler::chars::UniChar>();
	}

	string::tolower(ret);
	return ret;
}

auto Content::cleanString(const StringView &str, bool lowercase) -> String {
	if (str.empty()) {
		return String();
	}

	StringStream stream; stream.reserve(str.size());
	StringView r(str);
	r.skipChars<StringView::Chars<'\t', ' ', '\n', '\r'>>();
	while (!r.empty()) {
		StringView tmp = r.readUntil<StringView::Chars<'\t', ' ', '\n', '\r'>>();
		if (!tmp.empty()) {
			stream << tmp;
		}
		tmp = r.readChars<StringView::Chars<'\t', ' ', '\n', '\r'>>();
		if (!r.empty() && !tmp.empty()) {
			stream << ' ';
		}
	}

	String ret(stream.str());

	if (lowercase) {
		string::tolower(ret);
	}

	return ret;
}

auto Content::explicitLink(const StringView &s, const Extensions &ext, token * bracket, token * paren) -> Link * {
	const char *source = s.data();
	const char *end = &source[paren->start + paren->len];
	if (*end == ')') { -- end; }
	size_t pos = paren->child->next->start;

	size_t attr_len;

	String url;
	StringView title;
	StringView attributes;

	// Skip whitespace
	while (chars::isWhitespace(source[pos])) {
		pos++;
	}

	// Grab URL
	url = url_accept(source, pos, paren->start + paren->len - 1 - pos, &pos, false);

	// Skip whitespace
	while (chars::isWhitespace(source[pos])) {
		pos++;
	}

	StringView r(&source[pos], end - &source[pos]);
	if (r.is('"')) {
		size_t d = 0;
		++ r;

		title = r;
		while (!r.empty()) {
			r.skipUntil<StringView::Chars<'"'>>();
			if (r.is('"')) {
				++ r;
				if (r.empty() || chars::isWhitespace(r[0]) || r.is('"')) {
					if (d == 0) {
						break;
					} else {
						-- d;
					}
				} else {
					++ d;
				}
			}
		}

		title = StringView(title.data(), title.size() - r.size() - 1);
		pos += title.size() + 2;
	}

	// Skip whitespace
	while (chars::isWhitespace(source[pos])) {
		pos++;
	}

	// Grab attributes, if present
	attr_len = scan_attributes(&source[pos]);

	if (attr_len) {
		attributes = StringView(&source[pos], attr_len);
	}

	Link *l = nullptr;
	if (!attributes.empty()) {
		if (!ext.hasFlag(Extensions::Compatibility)) {
			l = new Link(source, Token(), move(url), title, attributes, false);
		}
	} else {
		l = new Link(source, Token(), move(url), title, attributes, false);
	}

	return l;
}

Content::Content(const Extensions &ext) : extensions(ext) {
	headers.reserve(64);
	definitions.reserve(64);
	tables.reserve(64);
}

const Extensions & Content::getExtensions() const {
	return extensions;
}

void Content::addExtension(Extensions::Value val) const {
	const_cast<Extensions &>(extensions).flags |= val;
}

void Content::setQuotesLanguage(QuotesLanguage l) {
	quotes = l;
}
QuotesLanguage Content::getQuotesLanguage() const {
	return quotes;
}

void Content::reset() {
	headers.clear(); headers.reserve(64);
	definitions.clear(); definitions.reserve(64);
	tables.clear(); tables.reserve(64);

	abbreviation.clear(); abbreviation.reserve(32);
	citation.clear(); citation.reserve(32);
	glossary.clear(); glossary.reserve(32);
	footnotes.clear(); footnotes.reserve(32);
	links.clear(); links.reserve(32);

	meta.clear(); meta.reserve(8);

	linksView.clear();
	citationView.clear();
	footnotesView.clear();
	abbreviationView.clear();
	glossaryView.clear();
}

auto Content::getHeaders() const -> const Vector<Token> & {
	return headers;
}

auto Content::getDefinitions() const -> const Vector<Token> & {
	return definitions;
}

auto Content::getTables() const -> const Vector<Token> & {
	return tables;
}

auto Content::getAbbreviations() const -> const Vector<Footnote *> & {
	return abbreviation;
}

auto Content::getCitations() const -> const Vector<Footnote *> & {
	return citation;
}

auto Content::getGlossary() const -> const Vector<Footnote *> & {
	return glossary;
}

auto Content::getFootnotes() const -> const Vector<Footnote *> & {
	return footnotes;
}

auto Content::getLinks() const -> const Vector<Link *> & {
	return links;
}

auto Content::getMetaDict() const -> const Dict<String> & {
	return meta;
}

template <typename T>
static auto getFromDictView(const Content::DictView<T *> &dict, const StringView &target) -> T * {
	auto key = Content::cleanString(target, false);

	auto it = dict.find(key);
	if (it != dict.end()) {
		return it->second;
	}

	key = Content::labelFromString(target);
	it = dict.find(key);
	if (it != dict.end()) {
		return it->second;
	}

	// None found
	return nullptr;
}

Content::Link *Content::getLink(const StringView &key) const {
	return getFromDictView(linksView, key);
}

Content::Footnote *Content::getAbbreviation(const StringView &target) const {
	return getFromDictView(abbreviationView, target);
}

Content::Footnote *Content::getCitation(const StringView &target) const {
	return getFromDictView(citationView, target);
}

Content::Footnote *Content::getFootnote(const StringView &target) const {
	return getFromDictView(footnotesView, target);
}

Content::Footnote *Content::getGlossary(const StringView &target) const {
	return getFromDictView(glossaryView, target);
}

StringView Content::getMeta(const StringView &target) const {
	auto key = labelFromString(target);
	auto it = meta.find(key);
	if (it != meta.end()) {
		return StringView(it->second);
	}
	return StringView();
}

void Content::process(const StringView &str) {
	processDefinitions(str);
	processHeaders(str);
	processTables(str);

	linksView.reserve(links.size());
	for (auto &it : links) {
		linksView.try_emplace(it->clean_text, it);
		linksView.try_emplace(it->label_text, it);
	}

	citationView.reserve(citation.size());
	for (auto &it : citation) {
		citationView.try_emplace(it->clean_text, it);
		citationView.try_emplace(it->label_text, it);
	}

	footnotesView.reserve(footnotes.size());
	for (auto &it : footnotes) {
		footnotesView.try_emplace(it->clean_text, it);
		footnotesView.try_emplace(it->label_text, it);
	}

	glossaryView.reserve(glossary.size());
	for (auto &it : glossary) {
		glossaryView.try_emplace(it->clean_text, it);
		glossaryView.try_emplace(it->label_text, it);
	}

	abbreviationView.reserve(abbreviation.size());
	for (auto &it : abbreviation) {
		abbreviationView.try_emplace(it->clean_text, it);
		abbreviationView.try_emplace(it->label_text, it);
	}
}

void Content::emplaceMeta(String && key, String && value) {
	string::trim(key);
	string::trim(value);
	meta.emplace(move(key), cleanString(value, false));
}

void Content::emplaceHtmlId(Token && token, const StringView &v) {
	Link * l = new Link(v, move(token));
	links.emplace_back(l);
}

void Content::processDefinitions(const StringView &str) {
	for (auto &it : definitions) {
		processDefinition(str, it);
	}
}

void Content::processDefinition(const StringView &str, const Token &t) {
	auto block = t.getToken();

	Footnote *f;

	token * label = block->child;

	if (label->type == BLOCK_PARA) {
		label = label->child;
	}

	switch (block->type) {
		case BLOCK_DEF_ABBREVIATION:
		case BLOCK_DEF_CITATION:
		case BLOCK_DEF_FOOTNOTE:
		case BLOCK_DEF_GLOSSARY:
			switch (block->type) {
				case BLOCK_DEF_ABBREVIATION:
					f = new Footnote(str, label, block->child, false, Footnote::Abbreviation);

					// Adjust the properties
					f->label_text = f->clean_text;

					if (f->content.getToken()->child &&
					        f->content.getToken()->child->next &&
					        f->content.getToken()->child->next->next) {
						f->clean_text = clean_string_from_range(str, f->content.getToken()->child->next->next->start,
								block->start + block->len - f->content.getToken()->child->next->next->start, false);
					} else {
						f->clean_text.clear();
					}

					abbreviation.push_back(f);
					break;

				case BLOCK_DEF_CITATION:
					f = new Footnote(str, label, block->child, true, Footnote::Citation);
					citation.push_back(f);
					break;

				case BLOCK_DEF_FOOTNOTE:
					f = new Footnote(str, label, block->child, true, Footnote::Note);
					footnotes.push_back(f);
					break;

				case BLOCK_DEF_GLOSSARY:
					// Strip leading '?' from term
					f = new Footnote(str, label, block->child, false, Footnote::Glossary);
					glossary.push_back(f);
					break;
			}

			label->type = TEXT_EMPTY;

			if (label->next) {
				label->next->type = TEXT_EMPTY;
			}

			strip_leading_whitespace(str, label);
			break;

		case BLOCK_DEF_LINK:
			extractDefinition(str, &(label));
			break;

		default:
			fprintf(stderr, "process %d\n", block->type);
	}

	block->type = BLOCK_EMPTY;
}

bool Content::extractDefinition(const StringView &str, token ** remainder) {
	const char * source = str.data();
	token * label = nullptr;
	token * title = nullptr;
	String url_char;
	StringView title_char;
	StringView attr_char;
	token * temp = NULL;
	size_t attr_len;

	Link * l = NULL;
	Footnote * f = NULL;

	// Store label
	label = *remainder;

	*remainder = (*remainder)->next;

	// Prepare for parsing

	// Account for settings

	switch (label->type) {
		case PAIR_BRACKET_CITATION:
		case PAIR_BRACKET_FOOTNOTE:
		case PAIR_BRACKET_GLOSSARY:
			if (extensions.hasFlag(Extensions::Notes)) {
				if (!sp_mmd_token_chain_accept(remainder, COLON)) {
					return false;
				}

				title = *remainder;		// Track first token of content in 'title'

				// Store for later use
				switch (label->type) {
					case PAIR_BRACKET_CITATION:
						f = new Footnote(str, label, title, true, Footnote::Note);
						citation.push_back(f);
						break;

					case PAIR_BRACKET_FOOTNOTE:
						f = new Footnote(str, label, title, true, Footnote::Note);
						footnotes.push_back(f);
						break;

					case PAIR_BRACKET_GLOSSARY:
						f = new Footnote(str, label, title, false, Footnote::Note);
						glossary.push_back(f);
						break;
				}

				break;
			}

		case PAIR_BRACKET:
			// Reference Link Definition

			if (!sp_mmd_token_chain_accept(remainder, COLON)) {
				return false;
			}

			// Skip space
			whitespace_accept(remainder);
			url_char = destination_accept(str, remainder, false);
			whitespace_accept(remainder);

			// Grab title, if present
			temp = *remainder;

			title = sp_mmd_token_chain_accept_multiple(remainder, 2, PAIR_QUOTE_DOUBLE, PAIR_QUOTE_SINGLE);

			if (!title) {
				// See if there's a title on next line
				whitespace_accept(remainder);
				sp_mmd_token_chain_accept_multiple(remainder, 2, TEXT_NL, TEXT_LINEBREAK);
				whitespace_accept(remainder);

				title = sp_mmd_token_chain_accept_multiple(remainder, 2, PAIR_QUOTE_DOUBLE, PAIR_QUOTE_SINGLE);

				if (!title) {
					*remainder = temp;
				}
			}

			title_char = text_inside_pair(str, title);

			// Get attributes
			if ((*remainder) && (((*remainder)->type != TEXT_NL) && ((*remainder)->type != TEXT_LINEBREAK))) {
				if (!extensions.hasFlag(Extensions::Compatibility)) {
					attr_len = scan_attributes(&source[(*remainder)->start]);

					if (attr_len) {
						attr_char = StringView(&source[(*remainder)->start], attr_len);

						// Skip forward
						attr_len += (*remainder)->start;

						while ((*remainder) && (*remainder)->start < attr_len) {
							*remainder = (*remainder)->next;
						}
					}

					l = new Link(str, label, move(url_char), title_char, attr_char, false);
				} else {
					// Not valid match
				}
			} else {
				l = new Link(str, label, move(url_char), title_char, attr_char, false);
			}

			// Store link for later use
			if (l) {
				links.push_back(l);
			}

			break;

		case PAIR_BRACKET_VARIABLE:
			fprintf(stderr, "Process variable:\n");
			Token(label).describe(std::cerr, str);
			break;

		default:
			// Rest of block is not definitions (or has already been processed)
			return false;
	}

	// Advance to next line
	sp_mmd_token_skip_until_type_multiple(remainder, 2, TEXT_NL, TEXT_LINEBREAK);

	if (*remainder) {
		*remainder = (*remainder)->next;
	}

	return true;
}

void Content::processHeaders(const StringView &str) {
	// NTD in compatibility mode or if disabled
	if (extensions.hasFlag(Extensions::NoLabels)) {
		return;
	}

	for (auto &it : headers) {
		processHeader(str, it);
	}
}

void Content::processHeader(const StringView &str, const Token &t) {
	token *h = t;

	// See if we have a manual label
	token * manual = manual_label_from_header(t, str.data());

	String url("#");
	if (manual) {
		url.append(label_from_token(str, manual));
		h = manual;
	} else {
		url.append(label_from_token(str, h));
	}

	Link * l = new Link(str, h, move(url), StringView(), StringView(), true);
	links.emplace_back(l);
}

void Content::processTables(const StringView &str) {
	for (auto &it : tables) {
		processTable(str, it);
	}
}

void Content::processTable(const StringView &str, const Token &tok) {
	auto t = tok.getToken();
	// Is there a caption
	if (table_has_caption(t)) {
		token * temp_token = t->next->child;

		if (temp_token->next &&
		        temp_token->next->type == PAIR_BRACKET) {
			temp_token = temp_token->next;
		}

		String url("#");
		url.append(label_from_token(str, temp_token));

		Link * l = new Link(str, temp_token, move(url), StringView(), StringView(), true);
		links.emplace_back(l);
	}
}

NS_MMD_END

extern "C" {

void sp_mmd_stack_push( _sp_mmd_stack * s,	void * element ) {
	((stappler::mmd::Content::Vector<stappler::mmd::Token> *)s)->emplace_back(stappler::mmd::Token((stappler::mmd::token *)element));
}

}
