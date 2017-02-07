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


template <> CharReaderUtf8 Tag_readName<CharReaderUtf8>(CharReaderUtf8 &is) {
	CharReaderUtf8 s = is;
	s.skipUntil<HtmlIdentifier16, CharReaderUtf8::MatchChars<'>', '?'>>();
	CharReaderUtf8 name(s.readChars<HtmlIdentifier16, CharReaderUtf8::MatchChars<'?'>>());
	string::tolower_buf((char *)name.data(), name.size());
	if (name.size() > 1 && name.back() == '/') {
		name.set(name.data(), name.size() - 1);
		is += (is.size() - s.size() - 1);
	} else {
		s.skipUntil<HtmlIdentifier16, CharReaderUtf8::MatchChars<'>'>>();
		is = s;
	}
	return name;
}

template <> CharReaderUtf8 Tag_readAttrName<CharReaderUtf8>(CharReaderUtf8 &s) {
	s.skipUntil<HtmlIdentifier16>();
	CharReaderUtf8 name(s.readChars<HtmlIdentifier16>());
	string::tolower_buf((char *)name.data(), name.size());
	return name;
}

template <> CharReaderUtf8 Tag_readAttrValue<CharReaderUtf8>(CharReaderUtf8 &s) {
	if (!s.is('=')) {
		s.skipUntil<HtmlIdentifier16>();
		return CharReaderUtf8();
	}

	s ++;
	char quoted = 0;
	if (s.is('"') || s.is('\'')) {
		quoted = s[0];
		s ++;
		CharReaderUtf8 tmp = s;
		while (!s.empty() && !s.is(quoted)) {
			if (quoted == '"') {
				s.skipUntil<CharReaderUtf8::MatchChars<u'\\', u'"'>>();
			} else {
				s.skipUntil<CharReaderUtf8::MatchChars<u'\\', u'\''>>();
			}
			if (s.is('\\')) {
				s += 2;
			}
		}

		CharReaderUtf8 ret(tmp.data(), tmp.size() - s.size());
		if (s.is(quoted)) {
			s ++;
		}
		s.skipUntil<HtmlIdentifier16, CharReaderUtf8::MatchChars<'>'>>();
		return ret;
	}

	return s.readChars<HtmlIdentifier16>();
}


template <> CharReaderBase Tag_readName<CharReaderBase>(CharReaderBase &is) {
	CharReaderBase s = is;
	s.skipUntil<HtmlIdentifier8, CharReaderBase::MatchChars<'>', '?'>>();
	CharReaderBase name(s.readChars<HtmlIdentifier8, CharReaderBase::MatchChars<'?'>>());
	string::tolower_buf((char *)name.data(), name.size());
	if (name.size() > 1 && name.back() == '/') {
		name.set(name.data(), name.size() - 1);
		is += (is.size() - s.size() - 1);
	} else {
		s.skipUntil<HtmlIdentifier8, CharReaderBase::MatchChars<'>'>>();
		is = s;
	}
	return name;
}

template <> CharReaderBase Tag_readAttrName<CharReaderBase>(CharReaderBase &s) {
	s.skipUntil<HtmlIdentifier8>();
	CharReaderBase name(s.readChars<HtmlIdentifier8>());
	string::tolower_buf((char *)name.data(), name.size());
	return name;
}

template <> CharReaderBase Tag_readAttrValue<CharReaderBase>(CharReaderBase &s) {
	if (!s.is('=')) {
		s.skipUntil<HtmlIdentifier8>();
		return CharReaderBase();
	}

	s ++;
	char quoted = 0;
	if (s.is('"') || s.is('\'')) {
		quoted = s[0];
		s ++;
		CharReaderBase tmp = s;
		while (!s.empty() && !s.is(quoted)) {
			if (quoted == '"') {
				s.skipUntil<CharReaderBase::MatchChars<'\\', '"'>>();
			} else {
				s.skipUntil<CharReaderBase::MatchChars<'\\', '\''>>();
			}
			if (s.is('\\')) {
				s += 2;
			}
		}

		CharReaderBase ret(tmp.data(), tmp.size() - s.size());
		if (s.is(quoted)) {
			s ++;
		}
		s.skipUntil<HtmlIdentifier8, CharReaderBase::MatchChars<'>'>>();
		return ret;
	}

	return s.readChars<HtmlIdentifier8>();
}


template <> CharReaderUcs2 Tag_readName<CharReaderUcs2>(CharReaderUcs2 &is) {
	CharReaderUcs2 s = is;
	s.skipUntil<HtmlIdentifier16, CharReaderUcs2::MatchChars<u'>', u'?'>>();
	CharReaderUcs2 name(s.readChars<HtmlIdentifier16, CharReaderUcs2::MatchChars<u'?'>>());
	string::tolower_buf((char *)name.data(), name.size());
	if (name.size() > 1 && name.back() == '/') {
		name.set(name.data(), name.size() - 1);
		is += (is.size() - s.size() - 1);
	} else {
		s.skipUntil<HtmlIdentifier16, CharReaderUcs2::MatchChars<u'>'>>();
		is = s;
	}
	return name;
}

template <> CharReaderUcs2 Tag_readAttrName<CharReaderUcs2>(CharReaderUcs2 &s) {
	s.skipUntil<HtmlIdentifier16>();
	CharReaderUcs2 name(s.readChars<HtmlIdentifier16>());
	string::tolower_buf((char *)name.data(), name.size());
	return name;
}

template <> CharReaderUcs2 Tag_readAttrValue<CharReaderUcs2>(CharReaderUcs2 &s) {
	if (!s.is('=')) {
		s.skipUntil<HtmlIdentifier16>();
		return CharReaderUcs2();
	}

	s ++;
	char quoted = 0;
	if (s.is('"') || s.is('\'')) {
		quoted = s[0];
		s ++;
		CharReaderUcs2 tmp = s;
		while (!s.empty() && !s.is(quoted)) {
			if (quoted == '"') {
				s.skipUntil<CharReaderUcs2::MatchChars<u'\\', u'"'>>();
			} else {
				s.skipUntil<CharReaderUcs2::MatchChars<u'\\', u'\''>>();
			}
			if (s.is('\\')) {
				s += 2;
			}
		}

		CharReaderUcs2 ret(tmp.data(), tmp.size() - s.size());
		if (s.is(quoted)) {
			s ++;
		}
		s.skipUntil<HtmlIdentifier16, CharReaderUcs2::MatchChars<u'>'>>();
		return ret;
	}

	return s.readChars<HtmlIdentifier16>();
}

NS_SP_EXT_END(html)
