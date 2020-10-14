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
#include "SPString.h"
#include "SPUnicode.h"

NS_SP_EXT_BEGIN(string)

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

uint8_t footprint_4(char16_t val) {
	switch (val) {
	/* 0  */ case u'0': case u'1': case u'2': case u'3': case u'4': case u'5': case u'6': case u'7': case u'8': case u'9':
	 	 	 case u'Ъ': case u'ъ': case u'X': case u'x': case u'Ь': case u'ь':
	 	 	 case u'Ж': case u'ж': case u'J': case u'j': case u'Ц': case u'ц': case u'Ч': case u'ч': return 0; break;

	/* 1  */ case u'А': case u'а': case u'A': case u'a': case u'О': case u'о': case u'O': case u'o': return 1; break;

	/* 2  */ case u'Б': case u'б': case u'B': case u'b': case u'П': case u'п': case u'P': case u'p': return 2; break;

	/* 3  */ case u'В': case u'в': case u'V': case u'v': case u'W': case u'w': case u'Ф': case u'ф':
			 case u'F': case u'f': return 3; break;

	/* 4  */ case u'Г': case u'г': case u'G': case u'g': case u'К': case u'к': case u'K': case u'k': return 4; break;
	/* 5  */ case u'Д': case u'д': case u'D': case u'd': return 5; break;
	/* 6  */ case u'Е': case u'е': case u'E': case u'e': case u'Ё': case u'ё': case u'Э': case u'э': return 6; break;

	/* 7  */ case u'И': case u'и': case u'I': case u'i': case u'Ы': case u'ы': return 7; break;
	/* 8  */ case u'Й': case u'й': case u'Q': case u'q': case u'Х': case u'х': case u'H': case u'h': return 8; break;

	/* 9  */ case u'Л': case u'л': case u'L': case u'l': return 9; break;
	/* 10 */ case u'М': case u'м': case u'M': case u'm': return 10; break;
	/* 11 */ case u'Н': case u'н': case u'N': case u'n': return 11; break;
	/* 12 */ case u'Р': case u'р': case u'R': case u'r': return 12; break;
	/* 13 */ case u'Т': case u'т': case u'T': case u't': return 13; break;

	/* 14 */ case u'Ш': case u'ш': case u'S': case u's': case u'Щ': case u'щ': case u'С': case u'с': case u'C': case u'c':
			 case u'З': case u'з': case u'Z': case u'z': return 14; break;

	/* 15 */ case u'Я': case u'я': case u'Y': case u'y': case u'У': case u'у': case u'U': case u'u': case u'Ю': case u'ю': return 15; break;
	}
	return 0;
}

uint8_t footprint_3(char16_t val) {
	switch (val) {
	/* 0  */ case u'0': case u'1': case u'2': case u'3': case u'4': case u'5': case u'6': case u'7': case u'8': case u'9':
	 	 	 case u'Ъ': case u'ъ': case u'X': case u'x': case u'Ь': case u'ь':
	 	 	 case u'Ж': case u'ж': case u'J': case u'j': case u'Ц': case u'ц': case u'Ч': case u'ч':
	 	 	 case u'Л': case u'л': case u'L': case u'l': return 0; break;

	/* 1  */ case u'А': case u'а': case u'A': case u'a': case u'О': case u'о': case u'O': case u'o': return 1; break;
	/* 2  */ case u'Е': case u'е': case u'E': case u'e': case u'Ё': case u'ё': case u'Э': case u'э': return 2; break;
	/* 3  */ case u'И': case u'и': case u'I': case u'i': case u'Ы': case u'ы': return 3; break;

	/* 4 */ case u'Г': case u'г': case u'G': case u'g': case u'К': case u'к': case u'K': case u'k':
			case u'Б': case u'б': case u'B': case u'b': case u'П': case u'п': case u'P': case u'p':
			case u'Р': case u'р': case u'R': case u'r': return 4; break;

	/* 5 */ case u'Д': case u'д': case u'D': case u'd': case u'Т': case u'т': case u'T': case u't':
			case u'В': case u'в': case u'V': case u'v': case u'W': case u'w': case u'Ф': case u'ф':
			case u'F': case u'f': return 5; break;

	/* 6 */ case u'Ш': case u'ш': case u'S': case u's': case u'Щ': case u'щ': case u'С': case u'с': case u'C': case u'c':
			case u'З': case u'з': case u'Z': case u'z': case u'М': case u'м': case u'M': case u'm': return 6; break;

	/* 7 */ case u'Я': case u'я': case u'Y': case u'y': case u'У': case u'у': case u'U': case u'u': case u'Ю': case u'ю':
			case u'Й': case u'й': case u'Q': case u'q': case u'Х': case u'х': case u'H': case u'h':
			case u'Н': case u'н': case u'N': case u'n': return 7; break;
	}
	return 0;
}

size_t _footprint_size(const StringView &str) {
	using MatchChars = chars::Compose<char16_t,
		WideStringView::MatchCharGroup<CharGroupId::Alphanumeric>,
		WideStringView::MatchCharGroup<CharGroupId::Cyrillic>>;

	size_t ret = 0;
	auto p = str.data();
	const auto e = str.data() + str.size();
	while (p < e) {
		uint8_t clen = 0;
		const char16_t c = unicode::utf8Decode(p, clen);
		if (MatchChars::match(c)) {
			++ ret;
		}
		p += clen;
	}
	return (ret + 1) / 2;
}

size_t _footprint_size(const WideStringView &str) {
	size_t ret = 0;
	WideStringView r(str);
	while (!r.empty()) {
		ret += r.readChars<WideStringView::CharGroup<CharGroupId::Alphanumeric>,
				WideStringView::CharGroup<CharGroupId::Cyrillic>>().size();
	}
	return (ret + 1) / 2;
}

void _make_footprint(const StringView &str, uint8_t *buf) {
	using MatchChars = chars::Compose<char16_t,
		WideStringView::MatchCharGroup<CharGroupId::Alphanumeric>,
		WideStringView::MatchCharGroup<CharGroupId::Cyrillic>>;

	size_t ret = 0;
	auto p = str.data();
	const auto e = str.data() + str.size();
	while (p < e) {
		uint8_t clen = 0;
		const char16_t c = unicode::utf8Decode(p, clen);
		if (MatchChars::match(c)) {
			auto value = footprint_4(c);
			if (ret % 2 == 0) {
				*buf |= ((value << 4) & 0xF0);
			} else {
				*buf |= value;
				++ buf;
			}

			++ ret;
		}
		p += clen;
	}
}

void _make_footprint(const WideStringView &str, uint8_t *buf) {
	using MatchChars = chars::Compose<char16_t,
		WideStringView::MatchCharGroup<CharGroupId::Alphanumeric>,
		WideStringView::MatchCharGroup<CharGroupId::Cyrillic>>;

	size_t ret = 0;
	auto p = str.data();
	const auto e = str.data() + str.size();
	while (p < e) {
		if (MatchChars::match(*p)) {
			auto value = footprint_4(*p);
			if (ret % 2 == 0) {
				*buf |= ((value << 4) & 0xF0);
			} else {
				*buf |= value;
				++ buf;
			}

			++ ret;
		}
		++ p;
	}
}

template <typename IntType, typename Char>
inline size_t unsigned_to_decimal(IntType number, Char *buffer) {
	if (number != 0) {
		Char *p_first = buffer;
		while (number != 0) {
			*buffer++ = Char('0') + number % 10;
			number /= 10;
		}
		std::reverse( p_first, buffer );
		return buffer - p_first;
	} else {
		*buffer++ = Char('0');
		return 1;
	}
	return 0;
}

size_t _to_decimal(int64_t number, char* buffer) {
	if (number < 0) {
		buffer[0] = '-';
		return unsigned_to_decimal( uint64_t(-number), buffer + 1 ) + 1;
	} else {
		return unsigned_to_decimal( uint64_t(number), buffer );
	}
}

size_t _to_decimal(uint64_t number, char* buffer) {
	return unsigned_to_decimal( uint64_t(number), buffer );
}

size_t _to_decimal(int64_t number, char16_t* buffer) {
	if (number < 0) {
		buffer[0] = '-';
		return unsigned_to_decimal( uint64_t(-number), buffer + 1 ) + 1;
	} else {
		return unsigned_to_decimal( uint64_t(number), buffer );
	}
}

size_t _to_decimal(uint64_t number, char16_t* buffer) {
	return unsigned_to_decimal( uint64_t(number), buffer );
}

NS_SP_EXT_END(string)
