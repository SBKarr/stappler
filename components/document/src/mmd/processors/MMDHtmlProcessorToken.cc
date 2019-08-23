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
#include "MMDHtmlProcessor.h"
#include "MMDEngine.h"
#include "MMDContent.h"
#include "MMDChars.h"
#include "MMDCore.h"
#include "SPString.h"

NS_MMD_BEGIN

using Traits = string::ToStringTraits<memory::PoolInterface>;

static inline StringView printToken(const StringView & source, token * t) {
	return StringView(&(source[t->start]), t->len);
}

void HtmlProcessor::pad(std::ostream &out, uint16_t num) {
	if (!spExt) {
		while (num > padded) {
			out << '\n';
			padded++;
		}
	}
}

void HtmlProcessor::exportToken(std::ostream &out, token * t) {
	if (t == NULL) {
		return;
	}

	switch (t->type) {
		case AMPERSAND:
		case AMPERSAND_LONG:
			out << "&amp;";
			break;

		case ANGLE_LEFT:
			out << "&lt;";
			break;

		case ANGLE_RIGHT:
			out << "&gt;";
			break;

		case APOSTROPHE:
			if (!content->getExtensions().hasFlag(Extensions::Smart)) {
				out << printToken(source, t);
			} else {
				printLocalizedChar(out, APOSTROPHE);
			}

			break;

		case BACKTICK:
			exportBacktick(out, t);
			break;

		case BLOCK_BLOCKQUOTE:
			exportBlockquote(out, t);
			break;

		case BLOCK_DEFINITION:
			exportDefinition(out, t);
			break;

		case BLOCK_DEFLIST:
			exportDefList(out, t);
			break;

		case BLOCK_CODE_FENCED:
			exportFencedCodeBlock(out, t);
			break;

		case BLOCK_CODE_INDENTED:
			exportIndentedCodeBlock(out, t);
			break;

		case BLOCK_EMPTY:
			break;

		case BLOCK_H1:
		case BLOCK_H2:
		case BLOCK_H3:
		case BLOCK_H4:
		case BLOCK_H5:
		case BLOCK_H6:
			exportHeader(out, t);
			break;

		case BLOCK_HR:
			exportHr(out);
			break;

		case BLOCK_HTML:
			exportHtml(out, t);
			break;

		case BLOCK_LIST_BULLETED_LOOSE:
		case BLOCK_LIST_BULLETED:
			exportListBulleted(out, t);
			break;

		case BLOCK_LIST_ENUMERATED_LOOSE:
		case BLOCK_LIST_ENUMERATED:
			exportListEnumerated(out, t);
			break;

		case BLOCK_LIST_ITEM:
			exportListItem(out, t, false);
			break;

		case BLOCK_LIST_ITEM_TIGHT:
			exportListItem(out, t, true);
			break;

		case BLOCK_META:
			break;

		case BLOCK_PARA:
		case BLOCK_DEF_CITATION:
		case BLOCK_DEF_FOOTNOTE:
		case BLOCK_DEF_GLOSSARY:
			exportDefinitionBlock(out, t);
			break;

		case BLOCK_SETEXT_1:
			exportHeaderText(out, t, 1);
			break;

		case BLOCK_SETEXT_2:
			exportHeaderText(out, t, 2);
			break;

		case BLOCK_TABLE:
			exportTable(out, t);
			break;

		case BLOCK_TABLE_HEADER:
			exportTableHeader(out, t);
			break;

		case BLOCK_TABLE_SECTION:
			exportTableSection(out, t);
			break;

		case BLOCK_TERM:
			exportDefTerm(out, t);
			break;

		case BLOCK_TOC:
			exportToc(out, t);
			break;

		case BRACE_DOUBLE_LEFT:
			out << "{{";
			break;

		case BRACE_DOUBLE_RIGHT:
			out << "}}";
			break;

		case BRACKET_LEFT:
			out << "[";
			break;

		case BRACKET_ABBREVIATION_LEFT:
			out << "[>";
			break;

		case BRACKET_CITATION_LEFT:
			out << "[#";
			break;

		case BRACKET_FOOTNOTE_LEFT:
			out << "[^";
			break;

		case BRACKET_GLOSSARY_LEFT:
			out << "[?";
			break;

		case BRACKET_IMAGE_LEFT:
			out << "![";
			break;

		case BRACKET_VARIABLE_LEFT:
			out << "[\%";
			break;

		case BRACKET_RIGHT:
			out << "]";
			break;

		case COLON:
			out << ":";
			break;

		case CRITIC_ADD_OPEN:
			out << "{++";
			break;

		case CRITIC_ADD_CLOSE:
			out << "++}";
			break;

		case CRITIC_COM_OPEN:
			out << "{&gt;&gt;";
			break;

		case CRITIC_COM_CLOSE:
			out << "&lt;&lt;}";
			break;

		case CRITIC_DEL_OPEN:
			out << "{--";
			break;

		case CRITIC_DEL_CLOSE:
			out << "--}";
			break;

		case CRITIC_HI_OPEN:
			out << "{==";
			break;

		case CRITIC_HI_CLOSE:
			out << "==}";
			break;

		case CRITIC_SUB_OPEN:
			out << "{~~";
			break;

		case CRITIC_SUB_DIV:
			out << "~&gt;";
			break;

		case CRITIC_SUB_CLOSE:
			out << "~~}";
			break;

		case DASH_M:
			if (!content->getExtensions().hasFlag(Extensions::Smart)) {
				out << printToken(source, t);
			} else {
				printLocalizedChar(out, DASH_M);
			}
			break;

		case DASH_N:
			if (!content->getExtensions().hasFlag(Extensions::Smart)) {
				out << printToken(source, t);
			} else {
				printLocalizedChar(out, DASH_N);
			}
			break;

		case DOC_START_TOKEN:
			exportTokenTree(out, t->child);
			break;

		case ELLIPSIS:
			if (!content->getExtensions().hasFlag(Extensions::Smart)) {
				out << printToken(source, t);
			} else {
				printLocalizedChar(out, ELLIPSIS);
			}
			break;

		case EMPH_START:
			pushNode(nullptr, "em");
			break;

		case EMPH_STOP:
			popNode();
			break;

		case EQUAL:
			out << "=";
			break;

		case ESCAPED_CHARACTER:
			if (!content->getExtensions().hasFlag(Extensions::Compatibility) && (source[t->start + 1] == ' ')) {
				out << "&nbsp;";
			} else {
				printHtml(out, StringView(&source[t->start + 1], 1));
			}

			break;

		case HASH1:
		case HASH2:
		case HASH3:
		case HASH4:
		case HASH5:
		case HASH6:
			out << printToken(source, t);
			break;

		case HTML_ENTITY:
			out << printToken(source, t);
			break;

		case HTML_COMMENT_START:
			if (!content->getExtensions().hasFlag(Extensions::Smart)) {
				out << "&lt;!--";
			} else {
				out << "&lt;!";
				printLocalizedChar(out, DASH_N);
			}

			break;

		case HTML_COMMENT_STOP:
			if (!content->getExtensions().hasFlag(Extensions::Smart)) {
				out << "--&gt;";
			} else {
				printLocalizedChar(out, DASH_N);
				out << "&gt;";
			}
			break;

		case INDENT_SPACE:
			out << ' ';
			break;

		case INDENT_TAB:
			out << '\t';
			break;

		case LINE_LIST_BULLETED:
		case LINE_LIST_ENUMERATED:
			exportTokenTree(out, t->child);
			break;

		case LINE_SETEXT_2:
		case MARKER_BLOCKQUOTE:
		case MARKER_H1:
		case MARKER_H2:
		case MARKER_H3:
		case MARKER_H4:
		case MARKER_H5:
		case MARKER_H6:
			break;

		case MARKER_LIST_BULLET:
		case MARKER_LIST_ENUMERATOR:
			break;

		case MATH_BRACKET_OPEN:
			if (t->mate) {
				pushNode(nullptr, "span", { pair("class", "math") });
				out << "\\[";
			} else {
				out << "\\[";
			}

			break;

		case MATH_BRACKET_CLOSE:
			if (t->mate) {
				out << "\\]";
				popNode();
			} else {
				out << "\\]";
			}

			break;

		case MATH_DOLLAR_SINGLE:
			if (t->mate) {
				if (t->start < t->mate->start) {
					pushNode(nullptr, "span", { pair("class", "math") });
					out << "\\(";
				} else {
					out << "\\)";
					popNode();
				}
			} else {
				out << "$";
			}

			break;

		case MATH_DOLLAR_DOUBLE:
			if (t->mate) {
				if (t->start < t->mate->start) {
					pushNode(nullptr, "span", { pair("class", "math") });
					out << "\\[";
				} else {
					out << "\\]";
					popNode();
				}
			} else {
				out << "$$";
			}

			break;

		case MATH_PAREN_OPEN:
			if (t->mate) {
				pushNode(nullptr, "span", { pair("class", "math") });
				out << "\\(";
			} else {
				out << "\\(";
			}

			break;

		case MATH_PAREN_CLOSE:
			if (t->mate) {
				out << "\\)";
				popNode();
			} else {
				out << "\\)";
			}

			break;

		case NON_INDENT_SPACE:
			out << ' ';
			break;

		case PAIR_BACKTICK:
			exportPairBacktick(out, t);
			break;

		case PAIR_ANGLE:
			exportPairAngle(out, t);
			break;

		case PAIR_BRACE:
		case PAIR_BRACES:
		case PAIR_RAW_FILTER:
			exportTokenTree(out, t->child);
			break;

		case PAIR_BRACKET:
			if (content->getExtensions().hasFlag(Extensions::Notes) &&
			        (t->next && t->next->type == PAIR_BRACKET_CITATION)) {
				exportPairBracketCitation(out, t);
			} else {
				exportPairBracketImage(out, t);
			}
			break;

		case PAIR_BRACKET_IMAGE:
			exportPairBracketImage(out, t);
			break;

		case PAIR_BRACKET_ABBREVIATION:
			exportPairBracketAbbreviation(out, t);
			break;

		case PAIR_BRACKET_CITATION:
			exportPairBracketCitation(out, t);
			break;

		case PAIR_BRACKET_FOOTNOTE:
			exportPairBracketFootnote(out, t);
			break;

		case PAIR_BRACKET_GLOSSARY:
			exportPairBracketGlossary(out, t);
			break;

		case PAIR_BRACKET_VARIABLE:
			exportPairBracketVariable(out, t);
			break;

		case PAIR_CRITIC_ADD:
			exportCriticAdd(out, t);
			break;

		case PAIR_CRITIC_DEL:
			exportCriticDel(out, t);
			break;

		case PAIR_CRITIC_COM:
			exportCriticCom(out, t);
			break;

		case PAIR_CRITIC_HI:
			exportCriticHi(out, t);
			break;

		case CRITIC_SUB_DIV_A:
			out << "~";
			break;

		case CRITIC_SUB_DIV_B:
			out << "&gt;";
			break;

		case PAIR_CRITIC_SUB_DEL:
			exportCriticPairSubDel(out, t);
			break;

		case PAIR_CRITIC_SUB_ADD:
			exportCriticPairSubAdd(out, t);
			break;

		case PAIR_HTML_COMMENT:
			out << printToken(source, t);
			break;

		case PAIR_MATH:
			exportMath(out, t);
			break;

		case PAIR_EMPH:
		case PAIR_PAREN:
		case PAIR_QUOTE_DOUBLE:
		case PAIR_QUOTE_SINGLE:
		case PAIR_STAR:
		case PAIR_STRONG:
		case PAIR_SUBSCRIPT:
		case PAIR_SUPERSCRIPT:
		case PAIR_UL:
			exportTokenTree(out, t->child);
			break;

		case PAREN_LEFT:
			out << '(';
			break;

		case PAREN_RIGHT:
			out << ')';
			break;

		case PIPE:
			out << printToken(source, t);
			break;

		case PLUS:
			out << printToken(source, t);
			break;

		case QUOTE_SINGLE:
			if ((t->mate == NULL) || !content->getExtensions().hasFlag(Extensions::Smart)) {
				out << "'";
			} else {
				(t->start < t->mate->start) ? ( printLocalizedChar(out, QUOTE_LEFT_SINGLE) ) : ( printLocalizedChar(out, QUOTE_RIGHT_SINGLE) );
			}

			break;

		case QUOTE_DOUBLE:
			if ((t->mate == NULL) || !content->getExtensions().hasFlag(Extensions::Smart)) {
				out << "&quot;";
			} else {
				(t->start < t->mate->start) ? ( printLocalizedChar(out, QUOTE_LEFT_DOUBLE) ) : ( printLocalizedChar(out, QUOTE_RIGHT_DOUBLE) );
			}

			break;

		case QUOTE_RIGHT_ALT:
			if ((t->mate == NULL) || !content->getExtensions().hasFlag(Extensions::Smart)) {
				out << "''";
			} else {
				printLocalizedChar(out, QUOTE_RIGHT_DOUBLE);
			}

			break;

		case SLASH:
		case STAR:
			out << printToken(source, t);
			break;

		case STRONG_START:
			pushNode(nullptr, "strong");
			break;

		case STRONG_STOP:
			popNode();
			break;

		case SUBSCRIPT:
			exportSubscript(out, t);
			break;

		case SUPERSCRIPT:
			exportSuperscript(out, t);
			break;

		case TABLE_CELL:
			exportTableCell(out, t);
			break;

		case TABLE_DIVIDER:
			break;

		case TABLE_ROW:
			exportTableRow(out, t);
			break;

		case TEXT_LINEBREAK:
			exportLineBreak(out, t);
			break;

		case CODE_FENCE:
		case TEXT_EMPTY:
		case MANUAL_LABEL:
			break;

		case TEXT_NL:
			if (t->next) {
				out << '\n';
			}

			break;

		case RAW_FILTER_LEFT:
		case TEXT_BACKSLASH:
		case TEXT_BRACE_LEFT:
		case TEXT_BRACE_RIGHT:
		case TEXT_HASH:
		case TEXT_NUMBER_POSS_LIST:
		case TEXT_PERCENT:
		case TEXT_PERIOD:
		case TEXT_PLAIN:
		case TOC:
			out << printToken(source, t);
			break;

		case UL:
			out << printToken(source, t);
			break;

		default:
			std::cerr << "Unknown token type: " << t->type << " (" << t->start << ":" << t->len << ")\n";
			Token(t).describe(std::cerr, source);
			break;
	}
}


void HtmlProcessor::exportTokenTree(std::ostream &out, token *t) {
	// Prevent stack overflow with "dangerous" input causing extreme recursion
	if (recurse_depth == kMaxExportRecursiveDepth) {
		return;
	}

	recurse_depth++;
	while (t != NULL) {
		if (skip_token) {
			skip_token--;
		} else {
			exportToken(out, t);
		}
		t = t->next;
	}
	recurse_depth--;
}

void HtmlProcessor::exportTokenRaw(std::ostream &out, token *t) {
	if (t == nullptr) {
		return;
	}

	switch (t->type) {
		case BACKTICK:
			out << printToken(source, t);
			break;

		case AMPERSAND:
			out << "&amp;";
			break;

		case AMPERSAND_LONG:
			out << "&amp;amp;";
			break;

		case ANGLE_RIGHT:
			out << "&gt;";
			break;

		case ANGLE_LEFT:
			out << "&lt;";
			break;

		case ESCAPED_CHARACTER:
			out << "\\";
			printHtml(out, StringView(&source[t->start + 1], 1));
			break;

		case HTML_ENTITY:
			out << "&amp;";
			out << StringView(&(source[t->start + 1]), t->len - 1);
			break;

		case MATH_BRACKET_OPEN:
			out << "\\\\[";
			break;

		case MATH_BRACKET_CLOSE:
			out << "\\\\]";
			break;

		case MATH_DOLLAR_SINGLE:
			if (t->mate) {
				out << ((t->start < t->mate->start) ? ( ("\\(") ) : ( ("\\)") ));
			} else {
				out << "$";
			}

			break;

		case MATH_DOLLAR_DOUBLE:
			if (t->mate) {
				out << (((t->start < t->mate->start) ? ( ("\\[") ) : ( ("\\]") )));
			} else {
				out << "$$";
			}

			break;

		case MATH_PAREN_OPEN:
			out << "\\\\(";
			break;

		case MATH_PAREN_CLOSE:
			out << "\\\\)";
			break;

		case QUOTE_DOUBLE:
			out << "&quot;";
			break;

		case SUBSCRIPT:
			if (t->child) {
				out << "~";
				exportTokenTreeRaw(out, t->child);
			} else {
				out << printToken(source, t);
			}

			break;

		case SUPERSCRIPT:
			if (t->child) {
				out << "^";
				exportTokenTreeRaw(out, t->child);
			} else {
				out << printToken(source, t);
			}

			break;

		case CODE_FENCE:
			if (t->next) {
				t->next->type = TEXT_EMPTY;
			}

		case TEXT_EMPTY:
			break;

		default:
			if (t->child) {
				exportTokenTreeRaw(out, t->child);
			} else {
				out << printToken(source, t);
			}

			break;
	}
}

void HtmlProcessor::exportTokenTreeRaw(std::ostream &out, token *t) {
	while (t != NULL) {
		if (skip_token) {
			skip_token--;
		} else {
			exportTokenRaw(out, t);
		}

		t = t->next;
	}
}

void HtmlProcessor::makeTokenHash(const Function<void(const StringView &)> &out, token *t) {
	if (t == nullptr) {
		return;
	}

	switch (t->type) {
		case TEXT_PLAIN:
		case TEXT_NUMBER_POSS_LIST:
			out(printToken(source, t));
			break;
		case PAIR_PAREN:
			if (!t->prev || t->prev->type != PAIR_BRACKET) {
				out(printToken(source, t));
			}
			break;
		default:
			if (t->child) {
				makeTokenTreeHash(out, t->child);
			}
			break;
	}
}

void HtmlProcessor::makeTokenTreeHash(const Function<void(const StringView &)> &out, token *t) {
	while (t != NULL) {
		if (skip_token) {
			skip_token--;
		} else {
			makeTokenHash(out, t);
		}

		t = t->next;
	}
}

void HtmlProcessor::exportTokenMath(std::ostream &out, token *t) {
	if (t == NULL) {
		return;
	}

	if (spExt) {
		switch (t->type) {
			case MATH_BRACKET_OPEN:
				out << "\\[";
				break;

			case MATH_BRACKET_CLOSE:
				out << "\\]";
				break;

			case MATH_PAREN_OPEN:
				out << "\\(";
				break;

			case MATH_PAREN_CLOSE:
				out << "\\)";
				break;

			case AMPERSAND:
				out << "&";
				break;

			case AMPERSAND_LONG:
				out << "&&";
				break;

			case ANGLE_RIGHT:
				out << ">";
				break;

			case ANGLE_LEFT:
				out << "<";
				break;

			case QUOTE_DOUBLE:
				out << "\"";
				break;

			default:
				exportTokenRaw(out, t);
				break;
		}
	} else {
		switch (t->type) {
			case MATH_BRACKET_OPEN:
				out << "\\[";
				break;

			case MATH_BRACKET_CLOSE:
				out << "\\]";
				break;

			case MATH_PAREN_OPEN:
				out << "\\(";
				break;

			case MATH_PAREN_CLOSE:
				out << "\\)";
				break;

			default:
				exportTokenRaw(out, t);
				break;
		}
	}
}

void HtmlProcessor::exportTokenTreeMath(std::ostream &out, token *t) {
	while (t != NULL) {
		if (skip_token) {
			skip_token--;
		} else {
			exportTokenMath(out, t);
		}

		t = t->next;
	}
}

void HtmlProcessor::exportFootnoteList(std::ostream &out) {
	if (!used_footnotes.empty()) {
		token * content;

		pad(out, 2);
		pushNode(nullptr, "div", { pair("class", "footnotes") });
		if (!spExt) { out << "\n"; }
		pushInlineNode(nullptr, "hr");
		if (!spExt) { out << "\n"; }

		pushNode(nullptr, "h6", { pair("class", "footnotes_header") });
		out << localize("Footnotes");
		popNode();

		if (!spExt) { out << "\n"; }
		pushNode(nullptr, "ol");


		padded = 0;

		auto i = 0;
		for (auto &it : used_footnotes) {
			auto & note = it;
			pad(out, 2);

			String id = Traits::toString("fn_", (i + 1));
			pushNode(nullptr, "li", { pair("id", id) });
			if (!spExt) { out << "\n"; }
			padded = 6;

			content = note->content;
			footnote_para_counter = 0;

			// We need to know which block is the last one in the footnote
			while (content) {
				if (content->type == BLOCK_PARA) {
					footnote_para_counter++;
				}

				content = content->next;
			}

			content = note->content;
			footnote_being_printed = i + 1;

			exportTokenTree(out, content);

			pad(out, 1);
			popNode();
			padded = 0;
			++ i;
		}

		pad(out, 2);
		popNode();
		if (!spExt) { out << "\n"; }
		popNode();
		padded = 0;
		footnote_being_printed = 0;
	}
}

void HtmlProcessor::exportGlossaryList(std::ostream &out) {
	if (!used_glossaries.empty()) {
		token * content;

		pad(out, 2);
		pushNode(nullptr, "div", { pair("class", "glossary") });
		if (!spExt) { out << "\n"; }
		pushInlineNode(nullptr, "hr");
		if (!spExt) { out << "\n"; }

		pushNode(nullptr, "h6", { pair("class", "glossary_header") });
		out << localize("Glossary");
		popNode();

		if (!spExt) { out << "\n"; }
		pushNode(nullptr, "ol");
		padded = 0;

		auto i = 0;
		for (auto &it : used_glossaries) {
			auto & note = it;
			// Export glossary
			pad(out, 2);

			String id = Traits::toString("gn_", (i + 1));
			pushNode(nullptr, "li", { pair("id", id) });
			if (!spExt) { out << "\n"; }
			padded = 6;

			content = note->content;

			// Print term
			printHtml(out, note->clean_text);
			out << ": ";

			// Print contents
			footnote_para_counter = 0;

			// We need to know which block is the last one in the footnote
			while (content) {
				if (content->type == BLOCK_PARA) {
					footnote_para_counter++;
				}

				content = content->next;
			}

			content = note->content;
			glossary_being_printed = i + 1;
			exportTokenTree(out, content);
			pad(out, 1);
			popNode();
			padded = 0;
			++ i;
		}

		pad(out, 2);
		popNode();
		if (!spExt) { out << "\n"; }
		popNode();
		padded = 0;
		glossary_being_printed = 0;
	}
}

void HtmlProcessor::exportCitationList(std::ostream &out) {
	if (!used_citations.empty()) {
		token * content;

		pad(out, 2);
		pushNode(nullptr, "div", { pair("class", "citations") });
		if (!spExt) { out << "\n"; }
		pushInlineNode(nullptr, "hr");
		if (!spExt) { out << "\n"; }

		pushNode(nullptr, "h6", { pair("class", "citations_header") });
		out << localize("Citations");
		popNode();

		if (!spExt) { out << "\n"; }
		pushNode(nullptr, "ol");
		padded = 0;

		auto i = 0;
		for (auto &it : used_citations) {
			auto & note = it;
			// Export footnote
			pad(out, 2);

			String id = Traits::toString("cn_", (i + 1));
			pushNode(nullptr, "li", { pair("id", id) });
			if (!spExt) { out << "\n"; }
			padded = 6;

			content = note->content;
			footnote_para_counter = 0;

			// We need to know which block is the last one in the footnote
			while (content) {
				if (content->type == BLOCK_PARA) {
					footnote_para_counter++;
				}

				content = content->next;
			}

			content = note->content;
			citation_being_printed = i + 1;

			exportTokenTree(out, content);

			pad(out, 1);
			popNode();
			padded = 0;
			++ i;
		}

		pad(out, 2);
		popNode();
		if (!spExt) { out << "\n"; }
		popNode();
		padded = 0;
		citation_being_printed = 0;
	}
}

NS_MMD_END
