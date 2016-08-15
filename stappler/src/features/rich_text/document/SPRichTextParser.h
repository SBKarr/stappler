/*
 * SPRichTextParser.h
 *
 *  Created on: 28 июля 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTPARSER_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTPARSER_H_

#include "SPRichTextNode.h"
#include "SPCharReader.h"

NS_SP_EXT_BEGIN(rich_text)

template <char16_t First, char16_t Second>
using Range = chars::Range<char16_t, First, Second>;

template <char16_t ...Args>
using Chars = chars::Chars<char16_t, Args...>;

template <CharGroupId G>
using Group = chars::CharGroup<char16_t, G>;

namespace parser {
	String readHtmlTagName(StringReader &);
	String readHtmlTagParamName(StringReader &);
	String readHtmlTagParamValue(StringReader &);

	String resolveCssString(const String &);

	style::MediaQuery::Query readMediaQuery(StringReader &s, const style::CssStringFunction &cb);

	void readHtmlStyleValue(StringReader &s, const Function<void(const String &, const String &)> &fn);

	void readStyleTag(Reader &reader, StringReader &, HtmlPage::FontMap &);

	struct RefParser {
		using Callback = Function<void(const String &, const String &)>;

		RefParser(const String &content, const Callback &cb);
		void readRef(const char *ref, size_t offset);

		size_t idx = 0;
		const char *ptr = nullptr;
		Callback func;
	};
}

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTPARSER_H_ */
