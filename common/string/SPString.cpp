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
#include "SPString.h"
#include "SPUnicode.h"

NS_SP_EXT_BEGIN(string)

static inline WideString & ltrim(WideString &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<char16_t, bool>(stappler::string::isspace))));
	return s;
}

static inline WideString & rtrim(WideString &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<char16_t, bool>(stappler::string::isspace))).base(), s.end());
	return s;
}

String &trim(String &s) {
	if (s.empty()) {
		return s;
	}
	auto ptr = s.c_str();
	size_t len = s.length();

	size_t loffset = 0;
	size_t roffset = 0;

	char16_t c; uint8_t off;
	std::tie(c, off) = string::read(ptr);
	while (c && isspace(c)) {
		ptr += off;
		len -= off;
		loffset += off;
		std::tie(c, off) = string::read(ptr);
	}

	if (len == 0) {
		s = "";
		return s;
	} else {
		off = 0;
		do {
			roffset += off;
			off = 1;
			while (unicode::isUtf8Surrogate(ptr[len - roffset - off])) {
				off ++;
			}
		} while (isspace(&ptr[len - roffset - off]));
	}

	if (loffset > 0 || roffset > 0) {
		s.assign(s.data() + loffset, len - roffset);
	}
	return s;
}

WideString &trim(WideString &s) {
	if (s.empty()) {
		return s;
	}
	return ltrim(rtrim(s));
}

static inline bool isUrlencodeChar(char c) {
    if (('a' <= c && c <= 'z') ||
        ('A' <= c && c <= 'Z') ||
        ('0' <= c && c <= '9') ||
        c == '-' ||
        c == '_' ||
        c == '~' ||
        c == '.') {
    	return false;
    } else {
    	return true;
    }
}

String urlencode(const String &data) {
	String ret; ret.reserve(data.length() * 2);
	for (auto &c : data) {
        if (isUrlencodeChar(c)) {
        	ret.push_back('%');
        	ret.append(base16::charToHex(c), 2);
        } else {
        	ret.push_back(c);
        }
	}
	return ret;
}

String urlencode(const Bytes &data) {
	String ret; ret.reserve(data.size() * 2);
	for (auto &c : data) {
        if (isUrlencodeChar(c)) {
        	ret.push_back('%');
        	ret.append(base16::charToHex(c), 2);
        } else {
        	ret.push_back(c);
        }
	}
	return ret;
}

String urldecode(const String &str) {
	String ret; ret.reserve(str.size());

	CharReaderBase r(str);
	while (!r.empty()) {
		CharReaderBase tmp = r.readUntil<CharReaderBase::Chars<'%'>>();
		ret.append(tmp.data(), tmp.size());

		if (r.is('%') && r > 2) {
			CharReaderBase hex(r.data() + 1, 2);
			hex.skipChars<CharReaderBase::CharGroup<CharGroupId::Hexadecimial>>();
			if (hex.empty()) {
				ret.push_back(base16::hexToChar(r[1], r[2]));
			} else {
				ret.append(r.data(), 3);
			}
			r += 3;
		} else if (!r.empty()) {
			ret.append(r.data(), r.size());
		}
	}

	return ret;
}
String format(const char *fmt, ...) {
	String ret;
    va_list args;
    va_start(args, fmt);

	char stackBuf[1_KiB];
	int size = vsnprintf(stackBuf, size_t(1_KiB - 1), fmt, args);
	if (size > int(1_KiB - 1)) {
		char *buf = new char[size + 1];
		size = vsnprintf(buf, size_t(size), fmt, args);
		ret.assign(buf, size);
		delete [] buf;
	} else if (size >= 0) {
		ret.assign(stackBuf, size);
	}

    va_end(args);
	return ret;
}

static String transliterateChar(char16_t c) {
	switch (c) {
	case u'А': return "A"; break;
	case u'а': return "a"; break;
	case u'Б': return "B"; break;
	case u'б': return "b"; break;
	case u'В': return "V"; break;
	case u'в': return "v"; break;
	case u'Г': return "G"; break;
	case u'г': return "g"; break;
	case u'Д': return "D"; break;
	case u'д': return "d"; break;
	case u'Е': return "E"; break;
	case u'е': return "e"; break;
	case u'Ё': return "Yo"; break;
	case u'ё': return "yo"; break;
	case u'Ж': return "Zh"; break;
	case u'ж': return "zh"; break;
	case u'З': return "Z"; break;
	case u'з': return "z"; break;
	case u'И': return "I"; break;
	case u'и': return "i"; break;
	case u'Й': return "Y"; break;
	case u'й': return "y"; break;
	case u'К': return "K"; break;
	case u'к': return "k"; break;
	case u'Л': return "L"; break;
	case u'л': return "l"; break;
	case u'М': return "M"; break;
	case u'м': return "m"; break;
	case u'Н': return "N"; break;
	case u'н': return "n"; break;
	case u'О': return "O"; break;
	case u'о': return "o"; break;
	case u'П': return "P"; break;
	case u'п': return "p"; break;
	case u'Р': return "R"; break;
	case u'р': return "r"; break;
	case u'С': return "S"; break;
	case u'с': return "s"; break;
	case u'Т': return "T"; break;
	case u'т': return "t"; break;
	case u'У': return "U"; break;
	case u'у': return "u"; break;
	case u'Ф': return "F"; break;
	case u'ф': return "f"; break;
	case u'Х': return "Kh"; break;
	case u'х': return "kh"; break;
	case u'Ц': return "Ts"; break;
	case u'ц': return "ts"; break;
	case u'Ч': return "Ch"; break;
	case u'ч': return "ch"; break;
	case u'Ш': return "Sh"; break;
	case u'ш': return "sh"; break;
	case u'Щ': return "Sch"; break;
	case u'щ': return "sch"; break;
	case u'Ъ': return ""; break;
	case u'ъ': return "’"; break;
	case u'Ы': return "Y"; break;
	case u'ы': return "y"; break;
	case u'Ь': return ""; break;
	case u'ь': return "’"; break;
	case u'Э': return "E"; break;
	case u'э': return "e"; break;
	case u'Ю': return "Yu"; break;
	case u'ю': return "yu"; break;
	case u'Я': return "Ya"; break;
	case u'я': return "ya"; break;
	default: break;
	}
	char cc = (char)c;
	return String(&cc);
}

String transliterate(const String &s) {
	String ret;
	WideString str = toUtf16(s);
	ret.reserve(str.length());
	for (auto &c : str) {
		ret.append(transliterateChar(c));
	}
	return ret;
}

NS_SP_EXT_END(string)
