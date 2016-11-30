// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPHtmlParser.h"

NS_SP_EXT_BEGIN(html)

using HtmlIdentifier = chars::Compose<char16_t,
		chars::Range<char16_t, u'0', u'9'>,
		chars::Range<char16_t, u'A', u'Z'>,
		chars::Range<char16_t, u'a', u'z'>,
		chars::Chars<char16_t, u'_', u'-', u'!', u'/', u':'>
>;

Tag::StringReader Tag::readName(StringReader &is) {
	StringReader s = is;
	s.skipUntil<HtmlIdentifier, Chars<'>', '?'>>();
	StringReader name(s.readChars<HtmlIdentifier, Chars<'?'>>());
	string::tolower_buf((char *)name.data(), name.size());
	if (name.size() > 1 && name.back() == '/') {
		name.set(name.data(), name.size() - 1);
		is += (is.size() - s.size() - 1);
	} else {
		s.skipUntil<HtmlIdentifier, Chars<'>'>>();
		is = s;
	}
	return name;
}

Tag::StringReader Tag::readAttrName(StringReader &s) {
	s.skipUntil<HtmlIdentifier>();
	StringReader name(s.readChars<HtmlIdentifier>());
	string::tolower_buf((char *)name.data(), name.size());
	return name;
}

Tag::StringReader Tag::readAttrValue(StringReader &s) {
	if (!s.is('=')) {
		s.skipUntil<HtmlIdentifier>();
		return StringReader();
	}

	s ++;
	char quoted = 0;
	if (s.is('"') || s.is('\'')) {
		quoted = s[0];
		s ++;
		StringReader tmp = s;
		while (!s.empty() && !s.is(quoted)) {
			if (quoted == '"') {
				s.skipUntil<Chars<u'\\', u'"'>>();
			} else {
				s.skipUntil<Chars<u'\\', u'\''>>();
			}
			if (s.is('\\')) {
				s += 2;
			}
		}

		StringReader ret(tmp.data(), tmp.size() - s.size());
		if (s.is(quoted)) {
			s ++;
		}
		s.skipUntil<HtmlIdentifier, Chars<'>'>>();
		return ret;
	}

	return s.readChars<HtmlIdentifier>();
}

NS_SP_EXT_END(html)
