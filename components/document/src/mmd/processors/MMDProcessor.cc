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
#include "MMDProcessor.h"
#include "MMDEngine.h"
#include "MMDContent.h"
#include "MMDCore.h"
#include "MMDChars.h"

NS_MMD_BEGIN

void Processor::process(const Content &c, const StringView &, const Token &) {
	content = &c;
	quotes_lang = c.getQuotesLanguage();
	odf_para_type = BLOCK_PARA;
	//p->language = e->language;

	if (content->getExtensions().hasFlag(Extensions::RandomFoot)) {
		random_seed_base = rand() % 32000;
	}

	used_abbreviations.reserve(content->getAbbreviations().size() + 2);
	used_citations.reserve(content->getCitations().size() + 2);
	used_footnotes.reserve(content->getFootnotes().size() + 2);
	used_glossaries.reserve(content->getGlossary().size() + 2);

	if (!content->getExtensions().hasFlag(Extensions::NoMetadata) && !content->getExtensions().hasFlag(Extensions::Compatibility)) {
		processMetaDict(c.getMetaDict());
	}
}

void Processor::processMetaDict(const Content::Dict<Content::String> &m) {
	for (auto &it : m) {
		processMeta(it.first, it.second);
	}
}

void Processor::processMeta(const StringView &key, const StringView &value) {
	if (key == "baseheaderlevel") {
		auto n = StringToNumber<int>(value.data());
		if (base_header_level < n) {
			base_header_level = n;
		}
	} else if (key == "language") {
		auto temp_char = Content::labelFromString(value);
		if (temp_char == "de") {
			// scratch->language = LC_DE;
			quotes_lang = QuotesLanguage::German;
		} else if (temp_char == "es") {
			// scratch->language = LC_ES;
			quotes_lang = QuotesLanguage::English;
		} else if (temp_char == "fr") {
			// scratch->language = LC_FR;
			quotes_lang = QuotesLanguage::French;
		} else if (temp_char == "nl") {
			// scratch->language = LC_NL;
			quotes_lang = QuotesLanguage::Dutch;
		} else if (temp_char == "sv") {
			// scratch->language = LC_SV;
			quotes_lang = QuotesLanguage::Swedish;
		} else {
			// scratch->language = LC_EN;
			quotes_lang = QuotesLanguage::English;
		}
	} else if (key == "quoteslanguage") {
		auto temp_char = Content::labelFromString(value);
		if (temp_char == "dutch" || temp_char == "nl") {
			quotes_lang = QuotesLanguage::Dutch;
		} else if (temp_char == "french" || temp_char == "fr") {
			quotes_lang = QuotesLanguage::French;
		} else if (temp_char == "german" || temp_char == "de") {
			quotes_lang = QuotesLanguage::German;
		} else if (temp_char == "germanguillemets") {
			quotes_lang = QuotesLanguage::GermanGuill;
		} else if (temp_char == "swedish" || temp_char == "sv") {
			quotes_lang = QuotesLanguage::Swedish;
		} else {
			quotes_lang = QuotesLanguage::English;
		}
	} else {
		// Any other key triggers complete document
		if (!content->getExtensions().hasFlag(Extensions::Snippet)) {
			content->addExtension(Extensions::Complete);
		}
	}
}

void Processor::printTokenRaw(std::ostream & out, token * t) {
	if (t) {
		switch (t->type) {
			case EMPH_START:
			case EMPH_STOP:
			case STRONG_START:
			case STRONG_STOP:
			case TEXT_EMPTY:
				break;

			case PAIR_EMPH:
			case PAIR_STRONG:
				printTokenTreeRaw(out, t->child);
				break;

			default:
				out << StringView(&source[t->start], t->len);
				break;
		}
	}
}

void Processor::printTokenTreeRaw(std::ostream & out, token * t) {
	while (t) {
		printTokenRaw(out, t);
		t = t->next;
	}
}


void Processor::readTableColumnAlignments(token * table) {
	token * walker = table->child->child;

	table_alignment[0] = '\0';
	table_column_count = 0;

	if (walker == NULL) {
		return;
	}

	// Find the separator line
	while (walker->next) {
		walker = walker->next;
	}

	walker->type = TEXT_EMPTY;

	// Iterate through cells to create alignment string
	short counter = 0;
	short align = 0;

	walker = walker->child;

	while (walker) {
		switch (walker->type) {
			case TABLE_CELL:
				align = scan_alignment_string(&source[walker->start]);

				switch (align) {
					case ALIGN_LEFT: table_alignment[counter] = 'l'; break;
					case ALIGN_RIGHT: table_alignment[counter] = 'r'; break;
					case ALIGN_CENTER: table_alignment[counter] = 'c'; break;
					case ALIGN_LEFT | ALIGN_WRAP: table_alignment[counter] = 'L'; break;
					case ALIGN_RIGHT | ALIGN_WRAP: table_alignment[counter] = 'R'; break;
					case ALIGN_CENTER | ALIGN_WRAP: table_alignment[counter] = 'C'; break;
					case ALIGN_WRAP: table_alignment[counter] = 'N'; break;
					default: table_alignment[counter] = 'n';
				}

				counter++;
				break;
		}

		walker = walker->next;
	}

	table_alignment[counter] = '\0';
	table_column_count = counter;
}

Content::Link * Processor::parseBrackets(token * bracket, int16_t * skip_token) {
	Content::Link * temp_link = nullptr;
	StringView temp_char;
	int16_t temp_short = 0;

	// What is next?
	token * next = bracket->next;

	if (next) {
		temp_short = 1;
	}

	if (next && next->type == PAIR_PAREN) {
		// We have `[foo](bar)` or `![foo](bar)`

		temp_link = Content::explicitLink(source, content->getExtensions(), bracket, next);

		if (temp_link) {
			// Don't output brackets
			bracket->child->type = TEXT_EMPTY;
			bracket->child->mate->type = TEXT_EMPTY;

			// Skip over parentheses
			*skip_token = temp_short;
			return temp_link;
		}
	}

	if (next && next->type == PAIR_BRACKET) {
		// Is this a reference link? `[foo][bar]` or `![foo][bar]`
		temp_char = text_inside_pair(source, next);

		if (temp_char.empty()) {
			// Empty label, use first bracket (e.g. implicit link `[foo][]`)
			temp_char = text_inside_pair(source, bracket);
		}
	} else {
		// This may be a simplified implicit link, e.g. `[foo]`

		// But not if it's nested brackets, since it would not
		// end up being a valid reference
		token * walker = bracket->child;

		while (walker) {
			switch (walker->type) {
				case PAIR_BRACKET:
				case PAIR_BRACKET_CITATION:
				case PAIR_BRACKET_FOOTNOTE:
				case PAIR_BRACKET_GLOSSARY:
				case PAIR_BRACKET_VARIABLE:
				case PAIR_BRACKET_ABBREVIATION:
					return nullptr;
			}

			walker = walker->next;
		}

		temp_char = text_inside_pair(source, bracket);
		// Don't skip tokens
		temp_short = 0;
	}

	temp_link = content->getLink(temp_char);
	if (temp_link) {
		// Don't output brackets
		if (bracket->child) {
			bracket->child->type = TEXT_EMPTY;

			if (bracket->child->mate) {
				bracket->child->mate->type = TEXT_EMPTY;
			}
		}

		// Skip over second bracket if present
		*skip_token = temp_short;
		return temp_link;
	}

	return nullptr;
}

Content::Footnote *Processor::parseAbbrBracket(token * t) {
	// Get text inside bracket
	StringView text;

	if (t->child) {
		text = text_inside_pair(source, t);
		++ text;
	} else {
		text = StringView(&source[t->start], t->len);
	}

	auto abbr_id = content->getAbbreviation(text);
	if (!abbr_id) {
		// No match, this is an inline glossary -- create a new glossary entry
		if (t->child) {
			t->child->type = TEXT_EMPTY;
			t->child->mate->type = TEXT_EMPTY;
		}

		// Create glossary
		token * label = t->child;

		while (label && label->type != PAIR_PAREN) {
			label = label->next;
		}

		if (label) {
			Content::Footnote * temp = new Content::Footnote(source, label, label->next, false, Content::Footnote::Note); // force to Note

			// Adjust the properties
			temp->label_text = temp->clean_text;
			if (temp->content && temp->content.getToken()->child) {
				temp->clean_text = clean_string_from_range(source, temp->content.getToken()->child->start,
						t->start + t->len - t->child->mate->len - temp->content.getToken()->child->start, false);
			}

			// Store as used
			used_abbreviations.push_back(temp);
			temp->count = used_abbreviations.size();
			temp->reference = false;
			return temp;
		}
		return nullptr;
	} else {
		if (abbr_id->count == maxOf<size_t>()) {
			used_abbreviations.push_back(abbr_id);
			abbr_id->count = used_abbreviations.size();
		}
		// Glossary in stack
		return abbr_id;
	}
}

Content::Footnote *Processor::parseCitationBracket(token * t) {
	auto text = text_inside_pair(source, t);
	auto citation_id = content->getCitation(text);

	if (!citation_id) {
		// No match, this is an inline citation -- create a new one
		t->child->type = TEXT_EMPTY;
		t->child->mate->type = TEXT_EMPTY;

		// Create citation
		Content::Footnote * temp = new Content::Footnote(source, t, t->child, true, Content::Footnote::Citation);
		used_citations.push_back(temp);
		temp->count = used_citations.size();
		temp->reference = false;
		return temp;
	} else {
		if (citation_id->count == maxOf<size_t>()) {
			used_citations.push_back(citation_id);
			citation_id->count = used_citations.size();
		}
		return citation_id;
	}
}

Content::Footnote *Processor::parseFootnoteBracket(token * t) {
	// Get text inside bracket
	auto text = text_inside_pair(source, t);
	auto footnote_id = content->getFootnote(text);
	if (!footnote_id) {
		// No match, this is an inline footnote -- create a new one
		t->child->type = TEXT_EMPTY;
		t->child->mate->type = TEXT_EMPTY;

		// Create footnote
		Content::Footnote * temp = new Content::Footnote(source, NULL, t->child, true, Content::Footnote::Note);
		used_footnotes.push_back(temp);
		temp->count = used_footnotes.size();
		temp->reference = false;
		return temp;
	} else {
		if (footnote_id->count == maxOf<size_t>()) {
			used_footnotes.push_back(footnote_id);
			footnote_id->count = used_footnotes.size();
		}
		return footnote_id;
	}
}

Content::Footnote *Processor::parseGlossaryBracket(token * t) {
	// Get text inside bracket
	StringView text;

	if (t->child) {
		text = text_inside_pair(source, t);
		++ text;
	} else {
		text = StringView(&source[t->start], t->len);
	}

	auto glossary_id = content->getGlossary(text);

	if (!glossary_id) {
		// No match, this is an inline glossary -- create a new glossary entry
		if (t->child) {
			t->child->type = TEXT_EMPTY;
			t->child->mate->type = TEXT_EMPTY;
		}

		// Create glossary
		token * label = t->child;

		while (label && label->type != PAIR_PAREN) {
			label = label->next;
		}

		if (label) {
			Content::Footnote * temp = new Content::Footnote(source, label, label->next, false, Content::Footnote::Note); // forced as Note
			used_glossaries.push_back(temp);
			temp->count = used_glossaries.size();
			temp->reference = false;
			return temp;
		} else {
			return nullptr;
		}
	} else {
		if (glossary_id->count == maxOf<size_t>()) {
			used_glossaries.push_back(glossary_id);
			glossary_id->count = used_glossaries.size();
		}
		return glossary_id;
	}
}

StringView Processor::printToken(const StringView & source, token * t) {
	return StringView(&(source[t->start]), t->len);
}

/// Grab the first "word" after the end of the fence marker:
/// ````perl
/// or
/// ```` perl
StringView Processor::getFenceLanguageSpecifier(const StringView & source, token * fence) {
	size_t start = fence->start + fence->len;
	size_t len = 0;

	while (chars::isWhitespace(source[start])) {
		start++;
	}

	while (!chars::isWhitespaceOrLineEnding(source[start + len])) {
		len++;
	}

	if (len) {
		return StringView(&source[start], len);
	}

	return StringView();
}

bool Processor::rawFilterTextMatches(const StringView & pattern, const char *filter) {
	if (pattern.empty()) {
		return false;
	}

	if (pattern == "*") {
		return true;
	} else if (pattern == "{=*}") {
		return true;
	} else {
		StringView reader(pattern);
		reader.skipUntilString(StringView(filter), true);
		if (reader.is(filter)) {
			return true;
		}
	}

	return false;
}

/// Determine whether raw filter matches specified format
bool Processor::rawFilterMatches(token * t, const StringView & source, const char *filter) {
	bool result = false;
	if (t->type != PAIR_RAW_FILTER) {
		return result;
	}

	return rawFilterTextMatches(StringView(&source[t->child->start + 2], t->child->mate->start - t->child->start - 2), filter);
}

uint8_t Processor::rawLevelForHeader(token * header) {
	switch (header->type) {
		case BLOCK_H1:
		case BLOCK_SETEXT_1:
			return 1;

		case BLOCK_H2:
		case BLOCK_SETEXT_2:
			return 2;

		case BLOCK_H3:
			return 3;

		case BLOCK_H4:
			return 4;

		case BLOCK_H5:
			return 5;

		case BLOCK_H6:
			return 6;
	}

	return 0;
}

Content::String Processor::labelFromHeader(const StringView & source, token * t) {
	token * temp_token = manual_label_from_header(t, source.data());

	if (temp_token) {
		StringView r(&source[temp_token->start], temp_token->len);
		String ret; ret.reserve(source.size());
		while (!r.empty()) {
			auto tmp = r.readChars<
					StringView::CharGroup<CharGroupId::Alphanumeric>,
					StringView::Chars<'.', '_', ':', '-', ','>,
					stappler::chars::UniChar>();

			ret.append(tmp.data(), tmp.size());
			r.skipUntil<
					StringView::CharGroup<CharGroupId::Alphanumeric>,
					StringView::Chars<'.', '_', ':', '-', ','>,
					stappler::chars::UniChar>();
		}

		string::tolower(ret);
		return ret;
	} else {
		return label_from_token(source, t);
	}
}

bool Processor::isImageFigure(token *t) {
	auto temp_token = t->next;

	if (temp_token &&
	        ((temp_token->type == PAIR_BRACKET) ||
	         (temp_token->type == PAIR_PAREN))) {
		temp_token = temp_token->next;
	}

	if (temp_token && temp_token->type == TEXT_NL) {
		temp_token = temp_token->next;
	}

	if (temp_token && temp_token->type == TEXT_LINEBREAK) {
		temp_token = temp_token->next;
	}

	if (t->prev || temp_token) {
		return false;
	} else {
		return true;
	}
}

StringView Processor::localize(const StringView &str) {
	if (str == "return") {
		switch (quotes_lang) {
		case QuotesLanguage::Russian: return "Вернуться"; break;
		default: break;
		}
	} else if (str == "see citation") {
		switch (quotes_lang) {
		case QuotesLanguage::Russian: return "см. источник"; break;
		default: break;
		}
	} else if (str == "see footnote") {
		switch (quotes_lang) {
		case QuotesLanguage::Russian: return "см. сноску"; break;
		default: break;
		}
	} else if (str == "see glossary") {
		switch (quotes_lang) {
		case QuotesLanguage::Russian: return "см. глоссарий"; break;
		default: break;
		}
	} else if (str == "Footnotes") {
		switch (quotes_lang) {
		case QuotesLanguage::Russian: return "Сноски"; break;
		default: break;
		}
	} else if (str == "Glossary") {
		switch (quotes_lang) {
		case QuotesLanguage::Russian: return "Глоссарий"; break;
		default: break;
		}
	} else if (str == "Citations") {
		switch (quotes_lang) {
		case QuotesLanguage::Russian: return "Список источников"; break;
		default: break;
		}
	}
	return str;
}

NS_MMD_END
