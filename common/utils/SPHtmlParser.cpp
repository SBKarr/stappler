/*
 * SPHtmlParser.cpp
 *
 *  Created on: 22 сент. 2016 г.
 *      Author: sbkarr
 */

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
