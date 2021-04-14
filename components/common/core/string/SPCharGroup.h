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

#ifndef COMMON_STRING_SPCHARMATCHING_H_
#define COMMON_STRING_SPCHARMATCHING_H_

#include "SPCore.h"

#define SPCHARMATCHING_LOG(...)

NS_SP_BEGIN

enum class CharGroupId : uint32_t {
	// displayable groups
	None = 0,
	PunctuationBasic = 1 << 1,
	Numbers = 1 << 2,
	Latin = 1 << 3,
	Cyrillic = 1 << 4,
	Currency = 1 << 5,
	GreekBasic = 1 << 6,
	Math = 1 << 7,
	Arrows = 1 << 8,
	Fractionals = 1 << 9,
	LatinSuppl1 = 1 << 10,
	PunctuationAdvanced = 1 << 11,
	GreekAdvanced = 1 << 12,

	// non-displayable groups
	WhiteSpace = 1 << 13,
	Controls = 1 << 14,
	NonPrintable = 1 << 15,

	LatinLowercase = 1 << 16,
	LatinUppercase = 1 << 17,

	Alphanumeric = 1 << 18,
	Hexadecimial = 1 << 19,
	Base64 = 1 << 20,

	BreakableWhiteSpace = 1 << 21,
	OpticalAlignmentSpecial = 1 << 22,
	OpticalAlignmentBullet = 1 << 23,

	TextPunctuation = 1 << 24,
};

SP_DEFINE_ENUM_AS_MASK(CharGroupId)

bool inCharGroup(CharGroupId mask, char16_t);
bool inCharGroupMask(CharGroupId mask, char16_t);
// WideString getCharGroup(CharGroupId mask);

NS_SP_END


NS_SP_EXT_BEGIN(chars)

template <typename CharType>
bool isupper(CharType);

template <typename CharType>
bool islower(CharType);

template <typename CharType>
bool isdigit(CharType);

template <typename CharType>
bool isxdigit(CharType);

template <typename CharType>
bool isspace(CharType);

/* Inlined templates for char-matching
 *
 * Chars < valiable-length-char-list > - matched every character in list
 * Range < first-char, last-char > - matched all characters in specific range
 * CharGroup < GroupId > - matched specific named char group
 *
 * Compose < Chars|Range|CharGroup variable length list >
 *
 */

using GroupId = CharGroupId;

struct UniChar {
	static inline bool match(char c) { return ((*(const uint8_t *)&c) & 128) != 0; }
};

template <typename CharType, CharType ... Args>
struct Chars {
	static SPINLINE bool match(CharType c);

	template <typename Func>
	static SPINLINE void foreach(const Func &);
};

template <typename CharType, CharType First, CharType Last>
struct Range {
	static inline bool match(CharType c) SPINLINE;

	template <typename Func>
	static inline void foreach(const Func &) SPINLINE;
};

template <typename CharType, typename ...Args>
struct Compose {
	static inline bool match(CharType c) SPINLINE;

	template <typename Func>
	static inline void foreach(const Func &) SPINLINE;
};


template <typename CharType, GroupId Group>
struct CharGroup;

template <>
struct CharGroup<char, GroupId::PunctuationBasic> : Compose<char,
		Range<char, '\x21', '\x2F'>, Range<char, '\x3A', '\x40'>, Range<char, '\x5B', '\x7F'>
> {
	static bool match(char c);
};

template <>
struct CharGroup<char, GroupId::Numbers> : Compose<char, Range<char, '0', '9'> > {
	static bool match(char c);
};

template <>
struct CharGroup<char, GroupId::Latin> : Compose<char, Range<char, 'A', 'Z'>, Range<char, 'a', 'z'> > {
	static bool match(char c);
};

template <>
struct CharGroup<char, GroupId::WhiteSpace> : Compose<char, Range<char, '\x09', '\x0D'>, Chars<char, '\x20'> > {
	static bool match(char c);
};

template <>
struct CharGroup<char, GroupId::Controls> : Compose<char, Range<char, '\x01', '\x20'> > { };

template <>
struct CharGroup<char, GroupId::NonPrintable> : Compose<char,
		Range<char, '\x01', '\x20'>, Chars<char, '\x20'>
> { };

template <>
struct CharGroup<char, GroupId::LatinLowercase> : Compose<char, Range<char, 'a', 'z'> > {
	static bool match(char c);
};

template <>
struct CharGroup<char, GroupId::LatinUppercase> : Compose<char, Range<char, 'A', 'Z'> > {
	static bool match(char c);
};

template <>
struct CharGroup<char, GroupId::Alphanumeric> : Compose<char,
		Range<char, '0', '9'>, Range<char, 'A', 'Z'>, Range<char, 'a', 'z'>
> {
	static bool match(char c);
};

template <>
struct CharGroup<char, GroupId::Hexadecimial> : Compose<char,
		Range<char, '0', '9'>, Range<char, 'A', 'F'>, Range<char, 'a', 'f'>
> {
	static bool match(char c);
};

template <>
struct CharGroup<char, GroupId::Base64> : Compose<char,
		Range<char, '0', '9'>, Range<char, 'A', 'Z'>, Range<char, 'a', 'z'>, Chars<char, '=', '/', '+', '_', '-'>
> {
	static bool match(char c);
};

template <>
struct CharGroup<char, GroupId::TextPunctuation> : Compose<char,
		Chars<char, '=', '/', '(', ')', '.', ',', '-', '\'', '"', ':', ';', '?', '!', '@', '#', '$', '%', '^', '*', '\\', '_', '+', '[', ']'>
> {
	static bool match(char c);
};


template <>
struct CharGroup<char16_t, GroupId::PunctuationBasic> : Compose<char16_t,
		Range<char16_t, u'\u0021', u'\u002F'>,
		Range<char16_t, u'\u003A', u'\u0040'>,
		Range<char16_t, u'\u005B', u'\u0060'>,
		Range<char16_t, u'\u007B', u'\u007E'>,
		Range<char16_t, u'\u00A1', u'\u00BF'>,
		Chars<char16_t, u'\u00AD', u'\u2013', u'\u2014', u'\u2019', u'\u201c', u'\u201d', u'\u2116'>
> { };

template <>
struct CharGroup<char16_t, GroupId::Numbers> : Compose<char16_t, Range<char16_t, u'0', u'9'> > { };

template <>
struct CharGroup<char16_t, GroupId::Latin> : Compose<char16_t,
		Range<char16_t, u'A', u'Z'>,
		Range<char16_t, u'a', u'z'>
> { };

template <>
struct CharGroup<char16_t, GroupId::Cyrillic> : Compose<char16_t,
		Range<char16_t, u'А', u'Я'>,
		Range<char16_t, u'а', u'я'>,
		Chars<char16_t, u'Ё', u'ё'>
> { };

template <>
struct CharGroup<char16_t, GroupId::Currency> : Compose<char16_t, Range<char16_t, u'\u20A0', u'\u20BE'> > { };

template <>
struct CharGroup<char16_t, GroupId::GreekBasic> : Compose<char16_t,
	Range<char16_t, u'\u0391', u'\u03AB'>,
	Range<char16_t, u'\u03B1', u'\u03CB'>
> { };

template <>
struct CharGroup<char16_t, GroupId::Math> : Compose<char16_t, Range<char16_t, u'\u2200', u'\u22FF'> > { };

template <>
struct CharGroup<char16_t, GroupId::Arrows> : Compose<char16_t, Range<char16_t, u'\u2190', u'\u21FF'> > { };

template <>
struct CharGroup<char16_t, GroupId::Fractionals> : Compose<char16_t, Range<char16_t, u'\u2150', u'\u215F'> > { };

template <>
struct CharGroup<char16_t, GroupId::LatinSuppl1> : Compose<char16_t, Range<char16_t, u'\u00C0', u'\u00FF'> > { };

template <>
struct CharGroup<char16_t, GroupId::PunctuationAdvanced> : Compose<char16_t,
		Range<char16_t, u'\u0021', u'\u002F'>,
		Range<char16_t, u'\u003A', u'\u0040'>,
		Range<char16_t, u'\u005B', u'\u0060'>,
		Range<char16_t, u'\u007B', u'\u007F'>,
		Range<char16_t, u'\u00A1', u'\u00BF'>,
		Range<char16_t, u'\u2010', u'\u201F'>,
		Range<char16_t, u'\u2024', u'\u2027'>,
		Range<char16_t, u'\u2030', u'\u203D'>,
		Chars<char16_t, u'\u2013', u'\u2014', u'\u2019', u'\u201c', u'\u201d', u'\u2116'>
> { };

template <>
struct CharGroup<char16_t, GroupId::GreekAdvanced> : Compose<char16_t,
		Range<char16_t, u'\u0391', u'\u03AB'>,
		Range<char16_t, u'\u03B1', u'\u03CB'>,
		Range<char16_t, u'\u0370', u'\u0377'>,
		Range<char16_t, u'\u037A', u'\u037F'>,
		Range<char16_t, u'\u0384', u'\u038A'>,
		Range<char16_t, u'\u038E', u'\u0390'>,
		Range<char16_t, u'\u03AC', u'\u03B0'>,
		Range<char16_t, u'\u03CC', u'\u03FF'>,
		Chars<char16_t, u'\u038C'>
> { };

template <>
struct CharGroup<char16_t, GroupId::WhiteSpace> : Compose<char16_t,
		Range<char16_t, u'\u0009', u'\u000D'>,
		Range<char16_t, u'\u2000', u'\u200D'>,
		Chars<char16_t, u'\u0020', u'\u0085', u'\u00A0', u'\u1680', u'\u2028', u'\u2029',
				 u'\u202F', u'\u205F', u'\u2060', u'\u3000', u'\uFEFF', u'\uFFFF'>
> { };

template <>
struct CharGroup<char16_t, GroupId::Controls> : Compose<char16_t, Range<char16_t, u'\u0001', u'\u0020'> > { };

template <>
struct CharGroup<char16_t, GroupId::NonPrintable> : Compose<char16_t,
		Range<char16_t, u'\u0001', u'\u0020'>,
		Range<char16_t, u'\u2000', u'\u200D'>,
		Chars<char16_t, u'\u0020', u'\u0085', u'\u00A0', u'\u1680', u'\u2028', u'\u2029',
				 u'\u202F', u'\u205F', u'\u2060', u'\u3000', u'\uFEFF', u'\uFFFF'>
> { };

template <>
struct CharGroup<char16_t, GroupId::LatinLowercase> : Compose<char16_t, Range<char16_t, u'a', u'z'> > { };

template <>
struct CharGroup<char16_t, GroupId::LatinUppercase> : Compose<char16_t, Range<char16_t, u'A', u'Z'> > { };

template <>
struct CharGroup<char16_t, GroupId::Alphanumeric> : Compose<char16_t,
		Range<char16_t, u'0', u'9'>,
		Range<char16_t, u'A', u'Z'>,
		Range<char16_t, u'a', u'z'>
> { };

template <>
struct CharGroup<char16_t, GroupId::Hexadecimial> : Compose<char16_t,
		Range<char16_t, u'0', u'9'>,
		Range<char16_t, u'A', u'F'>,
		Range<char16_t, u'a', u'f'>
> { };

template <>
struct CharGroup<char16_t, GroupId::Base64> : Compose<char16_t,
		Range<char16_t, u'0', u'9'>,
		Range<char16_t, u'A', u'Z'>,
		Range<char16_t, u'a', u'z'>,
		Chars<char16_t, u'=', u'/', u'+', u'-', u'_'>
> { };

template <>
struct CharGroup<char16_t, GroupId::BreakableWhiteSpace> : Compose<char16_t,
		Range<char16_t, u'\u0009', u'\u000D'>,
		Range<char16_t, u'\u2000', u'\u200D'>,
		Chars<char16_t, u'\u0020', u'\u0085', u'\u1680', u'\u2028', u'\u2029',
				 u'\u205F', u'\u2060', u'\u3000', u'\uFEFF'>
> { };

template <>
struct CharGroup<char16_t, GroupId::OpticalAlignmentSpecial> : Compose<char16_t,
		Chars<char16_t, u'(', u'[', u'{', u'"', u'\'', u'\\', u'<', u'«', u'„', u'.', u',', u'\u00AD', u'-'>
> { };

template <>
struct CharGroup<char16_t, GroupId::OpticalAlignmentBullet> : Compose<char16_t,
		Range<char16_t, u'0', u'9'>,
		Chars<char16_t, u'—', u'–', u'―', u'•', u'‣', u'⁃', u'-', u'*', u'◦', u'■', u'.', u',', u')'>
> { };

template <>
struct CharGroup<char16_t, GroupId::TextPunctuation> : Compose<char16_t,
		Chars<char16_t, u'=', u'/', u'(', u')', u'.', u',', u'-', u'\'', u'"'
		, u':', u';', u'?', u'!', u'@', u'#', u'$', u'%', u'^', u'*', u'\\'
		, u'_', u'+', u'[', u']', u'«', u'»'>
> { };

class MatchTraits {
public:
	template <typename CharType, CharType ... Args>
	static inline bool matchChar(CharType c) SPINLINE;

	template <typename CharType, CharType First, CharType Last>
	static inline bool matchPair(CharType c) SPINLINE;

	template <typename CharType, typename ...Args>
	static inline bool matchCompose(CharType c) SPINLINE;


	template <typename CharType, typename Func, CharType ... Args>
	static inline void foreachChar(const Func &) SPINLINE;

	template <typename CharType, typename Func, CharType First, CharType Last>
	static inline void foreachPair(const Func &) SPINLINE;

	template <typename CharType, typename Func, typename ...Args>
	static inline void foreachCompose(const Func &) SPINLINE;

private:
	template <typename CharType, CharType T>
	static inline bool _matchChar(CharType c) SPINLINE;

	template <typename CharType, CharType T, CharType T1, CharType ... Args>
	static inline bool _matchChar(CharType c) SPINLINE;

	template <typename CharType, typename T>
	static inline bool _matchCompose(CharType c) SPINLINE;

	template <typename CharType, typename T, typename T1, typename ... Args>
	static inline bool _matchCompose(CharType c) SPINLINE;


	template <typename CharType, typename Func, CharType T>
	static inline void _foreachChar(const Func &) SPINLINE;

	template <typename CharType, typename Func, CharType T, CharType T1, CharType ... Args>
	static inline void _foreachChar(const Func &) SPINLINE;

	template <typename CharType, typename Func, typename T>
	static inline void _foreachCompose(const Func &) SPINLINE;

	template <typename CharType, typename Func, typename T, typename T1, typename ... Args>
	static inline void _foreachCompose(const Func &) SPINLINE;
};

template <typename CharType, CharType ... Args>
inline bool Chars<CharType, Args...>::match(CharType c) {
	return MatchTraits::matchChar<CharType, Args...>(c);
}

template <typename CharType, CharType ... Args>
template <typename Func>
inline void Chars<CharType, Args...>::foreach(const Func &f) {
	MatchTraits::foreachChar<CharType, Func, Args...>(f);
}

template <typename CharType, CharType First, CharType Last>
inline bool Range<CharType, First, Last>::match(CharType c) {
	return MatchTraits::matchPair<CharType, First, Last>(c);
}

template <typename CharType, CharType First, CharType Last>
template <typename Func>
inline void Range<CharType, First, Last>::foreach(const Func &f) {
	MatchTraits::foreachPair<CharType, Func, First, Last>(f);
}

template <typename CharType, typename ...Args>
inline bool Compose<CharType, Args...>::match(CharType c) {
	return MatchTraits::matchCompose<CharType, Args...>(c);
}

template <typename CharType, typename ... Args>
template <typename Func>
inline void Compose<CharType, Args...>::foreach(const Func &f) {
	MatchTraits::foreachCompose<CharType, Func, Args...>(f);
}

template <typename CharType, CharType ... Args>
inline bool MatchTraits::matchChar(CharType c) {
	return _matchChar<CharType, Args...>(c);
}

template <typename CharType, CharType First, CharType Last>
inline bool MatchTraits::matchPair(CharType c) {
	SPCHARMATCHING_LOG("Match range %d - %d :  %d %d", First, Last, c, First <= c && c <= Last);
	return First <= c && c <= Last;
}

template <typename CharType, typename ... Args>
inline bool MatchTraits::matchCompose(CharType c) {
	SPCHARMATCHING_LOG("begin compose %d", c);
	auto ret = _matchCompose<CharType, Args...>(c);
	SPCHARMATCHING_LOG("end compose %d %d", c, ret);
	return ret;
}

template <typename CharType, typename Func, CharType ... Args>
inline void MatchTraits::foreachChar(const Func &f) {
	return _foreachChar<CharType, Func, Args...>(f);
}

template <typename CharType, typename Func, CharType First, CharType Last>
inline void MatchTraits::foreachPair(const Func &f) {
	for (CharType c = First; c >= 0 && c <= Last; c++) {
		f(c);
	}
}

template <typename CharType, typename Func, typename ... Args>
inline void MatchTraits::foreachCompose(const Func &f) {
	_foreachCompose<CharType, Func, Args...>(f);
}

template <typename CharType, CharType C>
inline bool MatchTraits::_matchChar(CharType c) {
	SPCHARMATCHING_LOG("Match char %d %d %d", C, c, C == c);
	return C == c;
}

template <typename CharType, CharType T, CharType T2, CharType ... Args>
inline bool MatchTraits::_matchChar(CharType c) {
	return _matchChar<CharType, T>(c) || _matchChar<CharType, T2, Args...>(c);
}

template <typename CharType, typename C>
inline bool MatchTraits::_matchCompose(CharType c) {
	return C::match(c);
}

template <typename CharType, typename T, typename T1, typename ... Args>
inline bool MatchTraits::_matchCompose(CharType c) {
	return _matchCompose<CharType, T>(c) || _matchCompose<CharType, T1, Args...>(c);
}


template <typename CharType, typename Func, CharType T>
inline void MatchTraits::_foreachChar(const Func &f) {
	f(T);
}

template <typename CharType, typename Func, CharType T, CharType T1, CharType ... Args>
inline void MatchTraits::_foreachChar(const Func &f) {
	_foreachChar<CharType, Func, T>(f);
	_foreachChar<CharType, Func, T1, Args...>(f);
}

template <typename CharType, typename Func, typename T>
inline void MatchTraits::_foreachCompose(const Func &f) {
	T::foreach(f);
}

template <typename CharType, typename Func, typename T, typename T1, typename ... Args>
inline void MatchTraits::_foreachCompose(const Func &f) {
	_foreachCompose<CharType, Func, T>(f);
	_foreachCompose<CharType, Func, T1, Args...>(f);
}

template <typename CharType>
inline bool isupper(CharType c) {
	return CharGroup<CharType, GroupId::LatinUppercase>::match(c);
}

template <typename CharType>
inline bool islower(CharType c) {
	return CharGroup<CharType, GroupId::LatinLowercase>::match(c);
}

template <typename CharType>
inline bool isdigit(CharType c) {
	return CharGroup<CharType, GroupId::Numbers>::match(c);
}

template <typename CharType>
inline bool isxdigit(CharType c) {
	return CharGroup<CharType, GroupId::Hexadecimial>::match(c);
}

template <typename CharType>
bool isspace(CharType c) {
	return CharGroup<CharType, GroupId::WhiteSpace>::match(c);
}

NS_SP_EXT_END(chars)

#endif /* COMMON_STRING_SPCHARMATCHING_H_ */
