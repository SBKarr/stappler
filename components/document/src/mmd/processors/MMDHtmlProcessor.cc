/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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
**/

#include "SPCommon.h"
#include "MMDHtmlProcessor.h"
#include "MMDEngine.h"
#include "MMDContent.h"
#include "MMDCore.h"
#include "SPString.h"
#include "SPHtmlParser.h"

NS_MMD_BEGIN

HtmlProcessor::HtmlProcessor() { }

bool HtmlProcessor::init(std::ostream *stream) {
	output = stream;
	return true;
}

void HtmlProcessor::process(const Content &c, const StringView &str, const Token &t) {
	Processor::process(c, str, t);

	if (html_header_level != maxOf<uint8_t>()) {
		base_header_level = html_header_level;
	}

	spExt = c.getExtensions().hasFlag(Extensions::StapplerLayout);

	processHtml(c, str, t);
}

void HtmlProcessor::processMeta(const StringView &key, const StringView &value) {
	if (key == "htmlheaderlevel") {
		html_header_level = StringToNumber<int>(value.data());
	} else if (key == "xhtmlheaderlevel") {
		html_header_level = StringToNumber<int>(value.data());
	} else {
		Processor::processMeta(key, value);
	}
}

void HtmlProcessor::processHtml(const Content &c, const StringView &str, const Token &t) {
	source = str;
	if (content->getExtensions().hasFlag(Extensions::Complete)) {
		startCompleteHtml(c);
	}

	exportTokenTree(buffer, t);
	exportFootnoteList(buffer);
	exportGlossaryList(buffer);
	exportCitationList(buffer);

	if (content->getExtensions().hasFlag(Extensions::Complete)) {
		endCompleteHtml();
	}
	flushBuffer();
}

void HtmlProcessor::printHtml(std::ostream &out, const StringView &str) {
	StringView r(str);
	while (!r.empty()) {
		switch(r[0]) {
		case '"': out << "&quot;"; break;
		case '&': out << "&amp;"; break;
		case '<': out << "&lt;"; break;
		case '>': out << "&gt;"; break;
		default: out << r[0]; break;
		}
		++ r;
	}
}

void HtmlProcessor::printLocalizedChar(std::ostream &out, uint16_t type) {
	switch (type) {
		case DASH_N: out << "&#8211;"; break;
		case DASH_M: out << "&#8212;"; break;
		case ELLIPSIS: out << "&#8230;"; break;
		case APOSTROPHE: out << "&#8217;"; break;
		case QUOTE_LEFT_SINGLE:
			switch (quotes_lang) {
				case QuotesLanguage::Swedish: out << "&#8217;"; break;
				case QuotesLanguage::French: out << "&#39;"; break;
				case QuotesLanguage::German: out << "&#8218;"; break;
				case QuotesLanguage::GermanGuill: out << "&#8250;"; break;
				default: out << "&#8216;";
			}
			break;

		case QUOTE_RIGHT_SINGLE:
			switch (quotes_lang) {
				case QuotesLanguage::German: out << "&#8216;"; break;
				case QuotesLanguage::GermanGuill: out << "&#8249;"; break;
				default: out << "&#8217;";
			}
			break;

		case QUOTE_LEFT_DOUBLE:
			switch (quotes_lang) {
				case QuotesLanguage::Dutch:
				case QuotesLanguage::German: out << "&#8222;"; break;
				case QuotesLanguage::GermanGuill: out << "&#187;"; break;
				case QuotesLanguage::French:
				case QuotesLanguage::Russian:
					out << "&#171;";
					break;
				case QuotesLanguage::Swedish: out << "&#8221;"; break;
				default: out << "&#8220;";
			}

			break;

		case QUOTE_RIGHT_DOUBLE:
			switch (quotes_lang) {
				case QuotesLanguage::German: out << "&#8220;"; break;
				case QuotesLanguage::GermanGuill: out << "&#171;"; break;
				case QuotesLanguage::French:
				case QuotesLanguage::Russian:
					out << "&#187;";
					break;
				case QuotesLanguage::Swedish:
				case QuotesLanguage::Dutch:
				default:
					out << "&#8221;";
			}

			break;
	}
}

bool HtmlProcessor::shouldWriteMeta(const StringView &view) {
	if (view == "baseheaderlevel" ||
			view == "bibliostyle" ||
			view == "bibtex" ||
			view == "htmlfooter" ||
			view == "htmlheader" ||
			view == "htmlheaderlevel" ||
			view == "language" ||
			view == "latexbegin" ||
			view == "latexconfig" ||
			view == "latexfooter" ||
			view == "latexheaderlevel" ||
			view == "latexinput" ||
			view == "latexleader" ||
			view == "latexmode" ||
			view == "mmdfooter" ||
			view == "mmdheader" ||
			view == "quoteslanguage" ||
			view == "transcludebase" ||
			view == "xhtmlheader" ||
			view == "xhtmlheaderlevel") {
		return false;
	}
	return true;
}

void HtmlProcessor::startCompleteHtml(const Content &c) {
	auto &m = c.getMetaDict();
	buffer << "<!DOCTYPE html>\n<html xmlns=\"http://www.w3.org/1999/xhtml\"";

	auto it = m.find("language");
	if (it != m.end()) {
		buffer << " lang=\"" << it->second << "\"";
	}
	buffer << ">\n<head>\n\t<meta charset=\"utf-8\"/>\n";

	for (auto &it : m) {
		if (it.first == "css") {
			buffer << "\t<link type=\"text/css\" rel=\"stylesheet\" href=\"" << it.second << "\"/>\n";
		} else if (it.first == "htmlheader") {
			buffer << it.second << '\n';
		} else if (it.first == "title") {
			buffer << "\t<title>";
			printHtml(buffer, it.second);
			buffer << "</title>\n";
		} else if (it.first == "xhtmlheader") {
			buffer << it.second << '\n';
		} else {
			if (shouldWriteMeta(it.first)) {
				buffer << "\t<meta name=\"";
				printHtml(buffer, it.first);
				buffer << "\" content=\"";
				printHtml(buffer, it.second);
				buffer << "\"/>\n";
			}
		}
	}

	buffer << "</head>\n";
	pushNode(nullptr, "body");
	buffer << "\n\n";
}

void HtmlProcessor::endCompleteHtml() {
	buffer << "\n\n";
	popNode();
	buffer << "\n</html>\n";
}

void HtmlProcessor::exportLink(std::ostream &out, token * text, Content::Link * link) {
	auto & a = link->attributes;
	VecList attr; attr.reserve(256 / sizeof(VecList::value_type));

	String url;
	if (text) {
		flushBuffer();
		printHtml(buffer, link->url);
		url = buffer.str();
		buffer.clear();
	}

	attr.emplace_back("href", url);

	String title;
	if (!link->title.empty()) {
		flushBuffer();
		printHtml(buffer, link->title);
		title = buffer.str();
		buffer.clear();
		attr.emplace_back("title", title);
	}

	for (auto &it : a) {
		attr.emplace_back(it.first, it.second);
	}

	if (spExt) {
		if (!url.empty() && url.front() == '#') {
			attr.emplace_back("target", "_self");
		}
	}

	pushNode(nullptr, "a", { }, move(attr));

	// If we're printing contents of bracket as text, then ensure we include it all
	if (text && text->child && text->child->len > 1) {
		text->child->next->start--;
		text->child->next->len++;
	}

	if (text && text->child) {
		exportTokenTree(out, text->child);
	}

	popNode();
}

void HtmlProcessor::exportImage(std::ostream &out, token * text, Content::Link * link, bool is_figure) {
	auto & a = link->attributes;
	VecList attr; attr.reserve(256 / sizeof(VecList::value_type));

	StringView width, height, align, type;

	for (auto &it : a) {
		if (it.first == "width") {
			width = it.second;
		} else if (it.first == "height") {
			height = it.second;
		} else if (spExt && it.first == "align") {
			align = it.second;
		} else if (it.first == "type") {
			type = it.second;
			attr.emplace_back(it.first, it.second);
		} else {
			attr.emplace_back(it.first, it.second);
		}
	}

	// Compatibility mode doesn't allow figures
	if (content->getExtensions().hasFlag(Extensions::Compatibility)) {
		is_figure = false;
	}

	if (is_figure) {
		if (spExt) {
			auto idStr = toString("figure_", figureId);
			pushNode(nullptr, "figure", { pair("class", align), pair("id", idStr), pair("type", type) });
		} else {
			pushNode(nullptr, "figure");
		}
		if (!spExt) { out << "\n"; }
	}

	attr.emplace_back("src", link->url);

	String alt;
	if (text) {
		flushBuffer();
		exportTokenTree(buffer, text->child);
		alt = buffer.str();
		buffer.clear();
		attr.emplace_back("alt", alt);
	}

	String id;
	if (link->label && !content->getExtensions().hasFlag(Extensions::Compatibility)) {
		id = label_from_token(source, link->label);
		attr.emplace_back("id", id);
	}

	if (!link->title.empty()) {
		attr.emplace_back("title", link->title);
	}

	stappler::String idStr;
	if (spExt) {
		idStr = toString("#figure_", figureId);
		attr.emplace_back("href", idStr);
		attr.emplace_back("type", type);
	}

	String style;
	if (!height.empty() || !width.empty()) {
		flushBuffer();
		if (!height.empty()) {
			out << "height:" << height << ";";
		}
		if (!width.empty()) {
			out << "width:" << width << ";";
		}
		style = buffer.str();
		buffer.clear();

		attr.emplace_back("style", style);
	}

	if (spExt && !align.empty()) {
		if (!is_figure && !idStr.empty()) {
			auto idStr = toString("figure_", figureId);
			pushInlineNode(nullptr, "img", { pair("class", align), pair("id", idStr) }, move(attr));
		} else {
			pushInlineNode(nullptr, "img", { pair("class", align) }, move(attr));
		}
	} else {
		pushInlineNode(nullptr, "img", { }, move(attr));
	}

	if (is_figure) {
		if (text) {
			if (!spExt) { out << "\n"; }
			if (spExt && !align.empty()) {
				auto idStr = toString("figcaption_", figureId);
				pushNode(text, "figcaption", { pair("class", align), pair("id", idStr) });
			} else {
				pushNode(text, "figcaption");
			}
			exportTokenTree(out, text->child);
			popNode();
		}

		if (!spExt) { out << "\n"; }
		popNode();
	}

	++ figureId;
}

void HtmlProcessor::pushHtmlEntity(std::ostream &out, token *t) {
	StringView r(&source[t->start], t->len);

	if (r.is('<')) {
		pushHtmlEntityText(out, r, t);
	} else {
		printTokenRaw(out, t);
	}
}

void HtmlProcessor::pushHtmlEntityText(std::ostream &out, StringView r, token *t) {
	while (!r.empty()) {
		if (!r.is('<')) {
			break;
		}
		++ r;
		if (r.is('/')) {
			r.skipUntil<StringView::Chars<'>'>>();
			if (r.is('>')) {
				++ r;
				popNode();
			}
		} else {
			auto name = html::Tag_readName(r, true);
			if (name.empty()) { // found tag without readable name
				if (t) {
					printTokenRaw(out, t);
				}
				return;
			}

			Vector<Pair<StringView, StringView>> attrs;

			StringView attrName;
			StringView attrValue;
			while (!r.empty() && !r.is('>') && !r.is('/')) {
				attrName.clear();
				attrValue.clear();

				attrName = html::Tag_readAttrName(r, true);
				if (attrName.empty()) {
					continue;
				}

				attrValue = html::Tag_readAttrValue(r, true);
				attrs.emplace_back(attrName, attrValue);
			}

			bool inlineTag = false;
			if (r.is('/')) {
				inlineTag = true;
			}

			r.skipUntil<StringView::Chars<'>'>>();
			if (r.is('>')) {
				++ r;
			}

			flushBuffer();
			if (inlineTag) {
				pushInlineNode(nullptr, name, { }, move(attrs));
			} else {
				pushNode(nullptr, name, { }, move(attrs));
			}

			out << r.readUntil<StringView::Chars<'<'>>();
		}
	}

	if (!r.empty()) {
		out << r;
	}
}

NS_MMD_END
