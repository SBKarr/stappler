// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

using HtmlIdentifier16 = chars::Compose<char16_t,
		chars::Range<char16_t, u'0', u'9'>,
		chars::Range<char16_t, u'A', u'Z'>,
		chars::Range<char16_t, u'a', u'z'>,
		chars::Chars<char16_t, u'_', u'-', u'!', u'/', u':'>
>;

using HtmlIdentifier8 = chars::Compose<char,
		chars::Range<char, u'0', u'9'>,
		chars::Range<char, u'A', u'Z'>,
		chars::Range<char, u'a', u'z'>,
		chars::Chars<char, u'_', u'-', u'!', u'/', u':'>
>;


template <> StringViewUtf8 Tag_readName<StringViewUtf8>(StringViewUtf8 &is, bool keepClean) {
	StringViewUtf8 s = is;
	s.skipUntil<HtmlIdentifier16, StringViewUtf8::MatchChars<'>', '?'>>();
	StringViewUtf8 name(s.readChars<HtmlIdentifier16, StringViewUtf8::MatchChars<'?'>>());
	if (!keepClean) {
		string::tolower_buf((char *)name.data(), name.size());
	}
	if (name.size() > 1 && name.back() == '/') {
		name.set(name.data(), name.size() - 1);
		is += (is.size() - s.size() - 1);
	} else {
		s.skipUntil<HtmlIdentifier16, StringViewUtf8::MatchChars<'>'>>();
		is = s;
	}
	return name;
}

template <> StringViewUtf8 Tag_readAttrName<StringViewUtf8>(StringViewUtf8 &s, bool keepClean) {
	s.skipUntil<HtmlIdentifier16>();
	StringViewUtf8 name(s.readChars<HtmlIdentifier16>());
	if (!keepClean) {
		string::tolower_buf((char *)name.data(), name.size());
	}
	return name;
}

template <> StringViewUtf8 Tag_readAttrValue<StringViewUtf8>(StringViewUtf8 &s, bool keepClean) {
	if (!s.is('=')) {
		s.skipUntil<HtmlIdentifier16>();
		return StringViewUtf8();
	}

	s ++;
	char quoted = 0;
	if (s.is('"') || s.is('\'')) {
		quoted = s[0];
		s ++;
		StringViewUtf8 tmp = s;
		while (!s.empty() && !s.is(quoted)) {
			if (quoted == '"') {
				s.skipUntil<StringViewUtf8::MatchChars<u'\\', u'"'>>();
			} else {
				s.skipUntil<StringViewUtf8::MatchChars<u'\\', u'\''>>();
			}
			if (s.is('\\')) {
				s += 2;
			}
		}

		StringViewUtf8 ret(tmp.data(), tmp.size() - s.size());
		if (s.is(quoted)) {
			s ++;
		}
		s.skipUntil<HtmlIdentifier16, StringViewUtf8::MatchChars<'>'>>();
		return ret;
	}

	return s.readChars<HtmlIdentifier16>();
}


template <> StringView Tag_readName<StringView>(StringView &is, bool keepClean) {
	StringView s = is;
	s.skipUntil<HtmlIdentifier8, StringView::MatchChars<'>', '?'>>();
	StringView name(s.readChars<HtmlIdentifier8, StringView::MatchChars<'?'>>());
	if (!keepClean) {
		string::tolower_buf((char *)name.data(), name.size());
	}
	if (name.size() > 1 && name.back() == '/') {
		name.set(name.data(), name.size() - 1);
		is += (is.size() - s.size() - 1);
	} else {
		s.skipUntil<HtmlIdentifier8, StringView::MatchChars<'>'>>();
		is = s;
	}
	return name;
}

template <> StringView Tag_readAttrName<StringView>(StringView &s, bool keepClean) {
	s.skipUntil<HtmlIdentifier8>();
	StringView name(s.readChars<HtmlIdentifier8>());
	if (!keepClean) {
		string::tolower_buf((char *)name.data(), name.size());
	}
	return name;
}

template <> StringView Tag_readAttrValue<StringView>(StringView &s, bool keepClean) {
	if (!s.is('=')) {
		s.skipUntil<HtmlIdentifier8>();
		return StringView();
	}

	s ++;
	char quoted = 0;
	if (s.is('"') || s.is('\'')) {
		quoted = s[0];
		s ++;
		StringView tmp = s;
		while (!s.empty() && !s.is(quoted)) {
			if (quoted == '"') {
				s.skipUntil<StringView::MatchChars<'\\', '"'>>();
			} else {
				s.skipUntil<StringView::MatchChars<'\\', '\''>>();
			}
			if (s.is('\\')) {
				s += 2;
			}
		}

		StringView ret(tmp.data(), tmp.size() - s.size());
		if (s.is(quoted)) {
			s ++;
		}
		s.skipUntil<HtmlIdentifier8, StringView::MatchChars<'>'>>();
		return ret;
	}

	return s.readChars<HtmlIdentifier8>();
}


template <> WideStringView Tag_readName<WideStringView>(WideStringView &is, bool keepClean) {
	WideStringView s = is;
	s.skipUntil<HtmlIdentifier16, WideStringView::MatchChars<u'>', u'?'>>();
	WideStringView name(s.readChars<HtmlIdentifier16, WideStringView::MatchChars<u'?'>>());
	if (!keepClean) {
		string::tolower_buf((char16_t *)name.data(), name.size());
	}
	if (name.size() > 1 && name.back() == '/') {
		name.set(name.data(), name.size() - 1);
		is += (is.size() - s.size() - 1);
	} else {
		s.skipUntil<HtmlIdentifier16, WideStringView::MatchChars<u'>'>>();
		is = s;
	}
	return name;
}

template <> WideStringView Tag_readAttrName<WideStringView>(WideStringView &s, bool keepClean) {
	s.skipUntil<HtmlIdentifier16>();
	WideStringView name(s.readChars<HtmlIdentifier16>());
	if (!keepClean) {
		string::tolower_buf((char16_t *)name.data(), name.size());
	}
	return name;
}

template <> WideStringView Tag_readAttrValue<WideStringView>(WideStringView &s, bool keepClean) {
	if (!s.is('=')) {
		s.skipUntil<HtmlIdentifier16>();
		return WideStringView();
	}

	s ++;
	char16_t quoted = 0;
	if (s.is(u'"') || s.is(u'\'')) {
		quoted = s[0];
		s ++;
		WideStringView tmp = s;
		while (!s.empty() && !s.is(quoted)) {
			if (quoted == '"') {
				s.skipUntil<WideStringView::MatchChars<u'\\', u'"'>>();
			} else {
				s.skipUntil<WideStringView::MatchChars<u'\\', u'\''>>();
			}
			if (s.is('\\')) {
				s += 2;
			}
		}

		WideStringView ret(tmp.data(), tmp.size() - s.size());
		if (s.is(quoted)) {
			s ++;
		}
		s.skipUntil<HtmlIdentifier16, WideStringView::MatchChars<u'>'>>();
		return ret;
	}

	return s.readChars<HtmlIdentifier16>();
}

NS_SP_EXT_END(html)
