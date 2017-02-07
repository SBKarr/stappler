// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPLayout.h"
#include "SLParser.h"
#include "SLReader.h"
#include "SPString.h"

//#define SP_RTREADER_LOG(...) log::format("RTReader", __VA_ARGS__)
#define SP_RTREADER_LOG(...)

NS_LAYOUT_BEGIN

namespace parser {
	void checkCssComments(StringReader &s);

	template <char16_t F>
	String readCssSelector(StringReader &s, char finisher);

	template <char16_t F>
	String readCssIdentifier(StringReader &s, char finisher);

	template <char16_t F>
	String readCssMediaQuery(const String &, StringReader &s, char finisher);

	template <char16_t F>
	String readCssValue(StringReader &s, char finisher);

	template <char16_t F>
	void readHtmlStyleValue(StringReader &, const Function<void(const String &, const String &)> &);

	template <char16_t F>
	void readCssStyleValue(StringReader &, char finisher, const Function<void(const String &, const String &)> &);
}

namespace parser {
	void readQuotedString(StringReader &s, String &str, char quoted) {
		while (!s.empty() && !s.is(quoted)) {
			if (quoted == '"') {
				str += s.readUntil<Chars<u'\\'>, DoubleQuote>().str();
			} else {
				str += s.readUntil<Chars<u'\\'>, SingleQuote>().str();
			}
			if (s.is('\\')) {
				s ++;
				str += s.letter();
				s ++;
			}
		}

		if (s.is(quoted)) {
			s ++;
		}
	}

	String readHtmlTagName(StringReader &is) {
		auto s = is;
		s.skipUntil<HtmlIdentifier, Chars<'>'>>();
		String name(s.readChars<HtmlIdentifier, StringReader::Chars<'?'>>().str());
		string::tolower(name);
		if (name.size() > 1 && name.back() == '/') {
			name.pop_back();
			is += (is.size() - s.size() - 1);
		} else {
			s.skipUntil<HtmlIdentifier, Chars<'>'>>();
			is = s;
		}
		return name;
	}

	String readHtmlTagParamName(StringReader &s) {
		s.skipUntil<HtmlIdentifier>();
		String name(s.readChars<HtmlIdentifier>().str());
		string::tolower(name);
		return name;
	}

	String readHtmlTagParamValue(StringReader &s) {
		if (!s.is('=')) {
			s.skipUntil<HtmlIdentifier>();
			return "";
		}

		s ++;
		char quoted = 0;
		if (s.is('"') || s.is('\'')) {
			quoted = s[0];
			s ++;
		}

		String name;
		if (quoted) {
			readQuotedString(s, name, quoted);
		} else {
			name = s.readChars<HtmlIdentifier>().str();
		}

		s.skipUntil<HtmlIdentifier, Chars<u'>'>>();
		return name;
	}

	void checkCssComments(StringReader &s) {
		s.skipChars<Group<CharGroupId::WhiteSpace>>();
		while (s == "/*") {
			s.skipUntilString("*/", false);
			s.skipChars<Group<CharGroupId::WhiteSpace>>();
		}
	}

	template <char16_t F>
	String readCssSelector(StringReader &s, char finisher) {
		checkCssComments(s);
		s.skipUntil<CssSelector, Chars<F>>();
		if (s.is(finisher)) {
			return "";
		}

		String name = s.readChars<CssSelector>().str();
		string::trim(name);
		string::tolower(name);
		checkCssComments(s);
		return name;
	}

	template <char16_t F>
	String readCssIdentifier(StringReader &s, char finisher) {
		checkCssComments(s);
		s.skipUntil<CssIdentifier, Chars<'}', F>>();
		if (s.is(finisher) || s.is('}')) {
			return "";
		}

		String name = s.readChars<CssIdentifier>().str();
		string::trim(name);
		string::tolower(name);
		checkCssComments(s);
		return name;
	}

	template <char16_t F>
	String readCssValue(StringReader &s, char finisher) {
		if (!s.is(':')) {
			s.skipUntil<CssIdentifier, Chars<F, ';', '}'>>();
			if (s.is(';') || s.is('}')) {
				s ++;
			}
			return "";
		}

		s ++;
		checkCssComments(s);
		String name;
		while (!s.empty() && !s.is(';') && !s.is('}') && !s.is(finisher)) {
			name += s.readUntil<Chars<'\'', '"', '}', ';', F>>().str();
			if (s.is(finisher)) {
				break;
			}
			if (s.is('\'') || s.is('"')) {
				auto quoted = s[0];
				s++;
				name.push_back(quoted);
				readQuotedString(s, name, quoted);
				name.push_back(quoted);
			}
		}

		checkCssComments(s);
		string::trim(name);
		if (name.size() > "!important"_len && name.compare(name.size() - "!important"_len, String::npos, "!important") == 0) {
			name = name.substr(0, name.size() - "!important"_len);
			string::trim(name);
		}
		return name;
	}

	template <char16_t F>
	String readCssMediaQuery(const String &selector, StringReader &s, char finisher) {
		String ret;
		if (selector.size() > "@media "_len) {
			ret += selector.substr("@media "_len);
		}
		checkCssComments(s);
		ret += s.readUntil<Chars<'{', F>>().str();
		if (s.is('{')) {
			string::trim(ret);
			string::tolower(ret);
			return ret;
			s ++;
		}

		return "";
	}

	void readHtmlStyleValue(StringReader &s, const Function<void(const String &, const String &)> &fn) {
		if (!s.is('=')) {
			s.skipUntil<HtmlIdentifier>();
			return;
		}

		s ++;
		char quoted = 0;
		if (s.is('"') || s.is('\'')) {
			quoted = s[0];
			s ++;
		}

		if (!quoted) {
			s.skipUntil<Group<CharGroupId::WhiteSpace>>();
			s.skipUntil<HtmlIdentifier>();
			return;
		}

		String name, value;
		while (!s.is(quoted)) {
			name.clear();
			value.clear();

			if (quoted == '\'') {
				name = readCssIdentifier<'\''>(s, quoted);
				value = readCssValue<'\''>(s, quoted);
			} else {
				name = readCssIdentifier<'"'>(s, quoted);
				value = readCssValue<'"'>(s, quoted);
			}

			if (!name.empty() && !value.empty()) {
				if (name != "font-family" && name != "background-image" && name != "color" && name != "background-color" && name != "src") {
					string::tolower(value);
				}
				fn(name, value);
			}
		}

		if (s.is(quoted)) {
			s ++;
		}
	}

	void readBracedString(StringReader &r, String &target) {
		r.skipUntil<StringReader::Chars<'('>>();
		if (r.is('(')) {
			++ r;
			r.skipChars<Group<CharGroupId::WhiteSpace>>();
			if (r.is('\'') || r.is('"')) {
				auto q = r[0];
				++ r;
				readQuotedString(r, target, q);
				r.skipUntil<StringReader::Chars<')'>>();
			} else {
				target = r.readUntil<StringReader::Chars<')'>>().str();
			}
			if (r.is(')')) {
				++ r;
			}
		}
	}

	void readFontFaceSrc(const String &src, Vector<String> &vec) {
		StringReader r(src);
		String url, format;
		while (!r.empty()) {
			url.clear();
			format.clear();
			checkCssComments(r);
			if (r == "url" || r == "local") {
				bool local = (r == "local");
				readBracedString(r, url);
				string::trim(url);
				if (local) {
					url = "local://" + url;
				}

			}
			checkCssComments(r);
			if (r == "format") {
				readBracedString(r, format);
				checkCssComments(r);
			}

			if (!url.empty()) {
				SP_RTREADER_LOG("font-face src: '%s' '%s'", url.c_str(), format.c_str());
				vec.emplace_back(url);
			}

			if (r.is(',')) {
				++ r;
			}
		}
	}

	void readFontFace(StringReader &s, HtmlPage::FontMap &fonts) {
		String fontFamily;
		String src;
		String selector;
		style::StyleVec style;
		while (!s.empty() && !s.is('}') && !s.is('<')) {
			parser::readCssStyleValue<'<'>(s, '<', [&] (const String &name, const String &value) {
				SP_RTREADER_LOG("font-face tag: '%s' : '%s'", name.c_str(), value.c_str());
				if (name == "font-family") {
					fontFamily = value;
				} else if (name == "src") {
					if (src.empty()) {
						src = value;
					} else {
						src.append(", ").append(value);
					}
				} else {
					style.push_back(pair(name, value));
				}
			});
		}

		if (fontFamily.size() >= 2) {
			if ((fontFamily.front() == '\'' && fontFamily.back() == '\'') || (fontFamily.front() == '"' && fontFamily.back() == '"')) {
				fontFamily = fontFamily.substr(1, fontFamily.size() - 2);
			}
		}

		if (!fontFamily.empty() && !src.empty()) {
			style::FontFace face;
			readFontFaceSrc(src, face.src);

			auto it = fonts.find(fontFamily);
			if (it == fonts.end()) {
				it = fonts.emplace(fontFamily, Vector<style::FontFace>{}).first;
			}

			style::ParameterList list;
			list.read(style);
			for (const style::Parameter & val : list.data) {
				switch (val.name) {
				case style::ParameterName::FontWeight: face.fontWeight = val.value.fontWeight; break;
				case style::ParameterName::FontStyle: face.fontStyle = val.value.fontStyle; break;
				case style::ParameterName::FontStretch: face.fontStretch = val.value.fontStretch; break;
				default: break;
				}
			}
			it->second.push_back(std::move(face));
		}
	}

	void readStyleTag(Reader &reader, StringReader &s, HtmlPage::FontMap &fonts) {
		String selector;
		style::StyleVec style;
		bool hasMediaQuery = false;
		style::MediaQuery mediaQuery;
		MediaQueryId mediaQueryId = MediaQueryNone();

		while (!s.empty() && !s.is('<')) {
			selector.clear();
			style.clear();

			s.skipUntil<CssIdentifier, Chars<'<', '}'>>();
			if (s.is('<')) {
				break;
			}

			if (s.is('}')) {
				if (hasMediaQuery) {
					hasMediaQuery = false;
					mediaQuery.clear();
					mediaQueryId = MediaQueryNone();
					s ++;
					SP_RTREADER_LOG("pop media");
					continue;
				}
			}

			selector = parser::readCssSelector<'<'>(s, '<');

			if (selector == "@font-face") {
				s.skipUntil<CssIdentifier, Chars<'<', '{'>>();
				if (s.is('{')) {
					readFontFace(s, fonts);
				}

				continue;
			}

			if (selector.compare(0, "@media"_len, "@media") == 0 && !hasMediaQuery) {
				hasMediaQuery = true;
				String q = parser::readCssMediaQuery<'<'>(selector, s, '<');
				mediaQuery.parse(q, [&] (CssStringId id, const CharReaderBase &str) {
					reader.addCssString(id, str.str());
				});
				mediaQueryId = reader.addMediaQuery(std::move(mediaQuery));
				SP_RTREADER_LOG("push media: %d %s", mediaQueryId, q.c_str());
				continue;
			}

			if (!selector.empty() && s.is('{')) {
				SP_RTREADER_LOG("selector: %s", selector.c_str());
				parser::readCssStyleValue<'<'>(s, '<', [&] (const String &name, const String &value) {
					SP_RTREADER_LOG("style tag: '%s' : '%s'", name.c_str(), value.c_str());
					if (name == "font-family" || name == "background-image") {
						reader.addCssString(value);
					}
					style.push_back(pair(name, value));
				});
			}

			if (!selector.empty() && !style.empty()) {
				string::split(selector, ",", [&] (const CharReaderBase &r) {
					String sel = r.str(); string::trim(sel);
					reader.onStyleObject(sel, style, mediaQueryId);
				});
			}

			parser::checkCssComments(s);
			if (s.is('}')) {
				s ++;
			}
			parser::checkCssComments(s);
			s.readUntil<CssIdentifier, Chars<'<', '}'>>();
		}
	}

	template <char16_t F>
	void readCssStyleValue(StringReader &s, char finisher, const Function<void(const String &, const String &)> &fn) {
		if (s.is('{')) {
			s ++;
		}

		checkCssComments(s);

		String name, value;
		while (!s.empty() && !s.is('}') && !s.is(finisher)) {
			name.clear();
			value.clear();

			name = readCssIdentifier<F>(s, finisher);
			value = readCssValue<F>(s, finisher);

			if (!name.empty() && !value.empty()) {
				if (name != "font-family" && name != "background-image" && name != "color" && name != "background-color" && name != "src") {
					string::tolower(value);
				}
				fn(name, value);
			}

			checkCssComments(s);
			s.skipChars<Group<CharGroupId::WhiteSpace>, Chars<';'>>();
			checkCssComments(s);
		}
	}

	CharReaderBase resolveCssString(const CharReaderBase &origStr) {
		CharReaderBase str(origStr);
		str.trimChars<CharReaderBase::CharGroup<chars::CharGroupId::WhiteSpace>>();

		CharReaderBase tmp(str);
		tmp.trimUntil<CharReaderBase::Chars<'(', '"', '\''>>();
		if (tmp.size() > 2) {
			if (tmp.is('(') && tmp.back() == ')') {
				tmp = CharReaderBase(tmp.data() + 1, tmp.size() - 2);
				tmp.trimChars<CharReaderBase::CharGroup<chars::CharGroupId::WhiteSpace>>();
				return tmp;
			} else if (tmp.is('"') && tmp.back() == '"') {
				return CharReaderBase(tmp.data() + 1, tmp.size() - 2);
			} else if (tmp.is('\'') && tmp.back() == '\'') {
				return CharReaderBase(tmp.data() + 1, tmp.size() - 2);
			}
		}

		return str;
	}

	style::MediaQuery::Query readMediaQuery(StringReader &s, const style::CssStringFunction &cb) {
		style::MediaQuery::Query q;

		s.skipChars<Group<CharGroupId::WhiteSpace>>();

		if (!s.empty() && !s.is('(')) {
			if (s == "not") {
				q.negative = true;
				s.skipString("not");
				s.skipChars<Group<CharGroupId::WhiteSpace>>();
			}

			auto id = parser::readCssIdentifier<'\x00'>(s, 0);
			if (!q.setMediaType(id)) {
				return q;
			}

			if (s != "and") {
				return q;
			} else {
				s.skipString("and");
				s.skipChars<Group<CharGroupId::WhiteSpace>>();
			}
		}

		while (s.is('(')) {
			s ++;

			String identifier = readCssIdentifier<')'>(s, ')');
			String value = readCssValue<')'>(s, ')');

			if (s.is(')')) {
				++ s;
			}

			s.skipChars<Group<CharGroupId::WhiteSpace>>();

			if (identifier.empty() && value.empty()) {
				return q;
			} else {
				style::readMediaParameter(q.params, identifier, CharReaderBase(value), cb);
			}

			if (s != "and") {
				return q; // invalid mediatype
			} else {
				s.skipString("and");
				s.skipChars<Group<CharGroupId::WhiteSpace>>();
			}
		}

		return q;
	}

	RefParser::RefParser(const String &content, const Callback &cb)
	: ptr(content.c_str()), func(cb) {
		bool isStart = true;
		while (ptr[idx] != 0) {
			if (isStart && strncasecmp(ptr + idx, "www.", 4) == 0) {
				readRef("www.", 4);
			} else if (isStart && strncasecmp(ptr + idx, "http://", 7) == 0 ) {
				readRef("http://", 7);
			} else if (isStart && strncasecmp(ptr + idx, "https://", 8) == 0 ) {
				readRef("https://", 8);
			} else if (isStart && strncasecmp(ptr + idx, "mailto:", 7) == 0 ) {
				readRef("mailto:", 7);
			} else {
				isStart = string::isspace(ptr + idx);
				idx += unicode::utf8DecodeLength(ptr[idx]);
			}
		}

		if (func) {
			if (idx != 0) {
				func(String(ptr, idx), "");
			} else {
				func("", "");
			}
		}
	}

	void RefParser::readRef(const char *ref, size_t offset) {
		while(ptr[idx + offset] && !string::isspace(&ptr[idx + offset])) {
			offset ++;
		}
		if (func) {
			func(String(ptr, idx), String(ptr + idx, offset));
		}

		ptr += (idx + offset);
		idx = 0;
	}
}

NS_LAYOUT_END
