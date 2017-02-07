/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef LAYOUT_DOCUMENT_SLPARSER_H_
#define LAYOUT_DOCUMENT_SLPARSER_H_

#include "SLNode.h"
#include "SLReader.h"
#include "SPCharReader.h"

NS_LAYOUT_BEGIN

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

	CharReaderBase resolveCssString(const CharReaderBase &);

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

NS_LAYOUT_END

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTPARSER_H_ */
