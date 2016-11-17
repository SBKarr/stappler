/*
 * SPCharMatching.h
 *
 *  Created on: 10 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef COMMON_STRING_SPCHARMATCHING_H_
#define COMMON_STRING_SPCHARMATCHING_H_

#include "SPCommon.h"

#define SPCHARMATCHING_LOG(...)

NS_SP_EXT_BEGIN(chars)

/* Inlined templates for char-matching
 *
 * Chars < valiable-length-char-list > - matched every character in list
 * Range < first-char, last-char > - matched all characters in specific range
 * CharGroup < CharGroupId > - matched specific named char group
 *
 * Compose < Chars|Range|CharGroup variable length list >
 *
 */

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
};

SP_DEFINE_ENUM_AS_MASK(CharGroupId)

struct UniChar {
	static inline bool match(char c) { return (c & 128) != 0; }
	static inline void get(Set<char> &) { }
};

template <typename CharType, CharType ... Args>
struct Chars {
	static inline bool match(CharType c) SPINLINE;
	static inline void get(Set<CharType> &) SPINLINE;

	template <typename Func>
	static inline void foreach(const Func &) SPINLINE;

	static inline Set<CharType> toSet() {
		Set<CharType> ret;
		foreach([&] (char16_t c) {
			ret.insert(c);
		});
		return ret;
	}

	static inline Vector<CharType> toVector() {
		Vector<CharType> ret;
		foreach([&] (char16_t c) {
			ret.push_back(c);
		});
		return ret;
	}
};

template <typename CharType, CharType First, CharType Last>
struct Range {
	static inline bool match(CharType c) SPINLINE;
	static inline void get(Set<CharType> &) SPINLINE;

	template <typename Func>
	static inline void foreach(const Func &) SPINLINE;

	static inline Set<CharType> toSet() {
		Set<CharType> ret;
		foreach([&] (char16_t c) {
			ret.insert(c);
		});
		return ret;
	}

	static inline Vector<CharType> toVector() {
		Vector<CharType> ret;
		foreach([&] (char16_t c) {
			ret.push_back(c);
		});
		return ret;
	}
};

template <typename CharType, typename ...Args>
struct Compose {
	static inline bool match(CharType c) SPINLINE;
	static inline void get(Set<CharType> &) SPINLINE;

	template <typename Func>
	static inline void foreach(const Func &) SPINLINE;

	static inline Set<CharType> toSet() {
		Set<CharType> ret;
		foreach([&] (char16_t c) {
			ret.insert(c);
		});
		return ret;
	}

	static inline Vector<CharType> toVector() {
		Vector<CharType> ret;
		foreach([&] (char16_t c) {
			ret.push_back(c);
		});
		return ret;
	}
};


template <typename CharType, CharGroupId Group>
struct CharGroup;

template <>
struct CharGroup<char, CharGroupId::PunctuationBasic> : Compose<char,
		Range<char, '\x21', '\x2F'>, Range<char, '\x3A', '\x40'>, Range<char, '\x5B', '\x7F'>, Range<char, '\xA1', '\xBF'>
> { };

template <>
struct CharGroup<char, CharGroupId::Numbers> : Compose<char, Range<char, '0', '9'> > { };

template <>
struct CharGroup<char, CharGroupId::Latin> : Compose<char, Range<char, 'A', 'Z'>, Range<char, 'a', 'z'> > { };

template <>
struct CharGroup<char, CharGroupId::WhiteSpace> : Compose<char,
		Range<char, '\x09', '\x0D'>, Chars<char, '\x20', '\x85', '\xA0'>
> { };

template <>
struct CharGroup<char, CharGroupId::Controls> : Compose<char, Range<char, '\x01', '\x20'> > { };

template <>
struct CharGroup<char, CharGroupId::NonPrintable> : Compose<char,
		Range<char, '\x01', '\x20'>, Chars<char, '\x20', '\x85', '\xA0'>
> { };

template <>
struct CharGroup<char, CharGroupId::LatinLowercase> : Compose<char, Range<char, 'a', 'z'> > { };

template <>
struct CharGroup<char, CharGroupId::LatinUppercase> : Compose<char, Range<char, 'A', 'Z'> > { };

template <>
struct CharGroup<char, CharGroupId::Alphanumeric> : Compose<char,
		Range<char, '0', '9'>, Range<char, 'A', 'Z'>, Range<char, 'a', 'z'>
> { };

template <>
struct CharGroup<char, CharGroupId::Hexadecimial> : Compose<char,
		Range<char, '0', '9'>, Range<char, 'A', 'F'>, Range<char, 'a', 'f'>
> { };

template <>
struct CharGroup<char, CharGroupId::Base64> : Compose<char,
		Range<char, '0', '9'>, Range<char, 'A', 'Z'>, Range<char, 'a', 'z'>, Chars<char, '=', '/'>
> { };


template <>
struct CharGroup<char16_t, CharGroupId::PunctuationBasic> : Compose<char16_t,
		Range<char16_t, u'\u0021', u'\u002F'>,
		Range<char16_t, u'\u003A', u'\u0040'>,
		Range<char16_t, u'\u005B', u'\u0060'>,
		Range<char16_t, u'\u007B', u'\u007E'>,
		Range<char16_t, u'\u00A1', u'\u00BF'>,
		Chars<char16_t, u'\u00AD', u'\u2013', u'\u2014', u'\u2019', u'\u201c', u'\u201d', u'\u2116'>
> { };

template <>
struct CharGroup<char16_t, CharGroupId::Numbers> : Compose<char16_t, Range<char16_t, u'0', u'9'> > { };

template <>
struct CharGroup<char16_t, CharGroupId::Latin> : Compose<char16_t,
		Range<char16_t, u'A', u'Z'>,
		Range<char16_t, u'a', u'z'>
> { };

template <>
struct CharGroup<char16_t, CharGroupId::Cyrillic> : Compose<char16_t,
		Range<char16_t, u'А', u'Я'>,
		Range<char16_t, u'а', u'я'>,
		Chars<char16_t, u'Ё', u'ё'>
> { };

template <>
struct CharGroup<char16_t, CharGroupId::Currency> : Compose<char16_t, Range<char16_t, u'\u20A0', u'\u20BE'> > { };

template <>
struct CharGroup<char16_t, CharGroupId::GreekBasic> : Compose<char16_t,
	Range<char16_t, u'\u0391', u'\u03AB'>,
	Range<char16_t, u'\u03B1', u'\u03CB'>
> { };

template <>
struct CharGroup<char16_t, CharGroupId::Math> : Compose<char16_t, Range<char16_t, u'\u2200', u'\u22FF'> > { };

template <>
struct CharGroup<char16_t, CharGroupId::Arrows> : Compose<char16_t, Range<char16_t, u'\u2190', u'\u21FF'> > { };

template <>
struct CharGroup<char16_t, CharGroupId::Fractionals> : Compose<char16_t, Range<char16_t, u'\u2150', u'\u215F'> > { };

template <>
struct CharGroup<char16_t, CharGroupId::LatinSuppl1> : Compose<char16_t, Range<char16_t, u'\u00C0', u'\u00FF'> > { };

template <>
struct CharGroup<char16_t, CharGroupId::PunctuationAdvanced> : Compose<char16_t,
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
struct CharGroup<char16_t, CharGroupId::GreekAdvanced> : Compose<char16_t,
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
struct CharGroup<char16_t, CharGroupId::WhiteSpace> : Compose<char16_t,
		Range<char16_t, u'\u0009', u'\u000D'>,
		Range<char16_t, u'\u2000', u'\u200D'>,
		Chars<char16_t, u'\u0020', u'\u0085', u'\u00A0', u'\u1680', u'\u2028', u'\u2029',
				 u'\u202F', u'\u205F', u'\u2060', u'\u3000', u'\uFEFF'>
> { };

template <>
struct CharGroup<char16_t, CharGroupId::Controls> : Compose<char16_t, Range<char16_t, u'\u0001', u'\u0020'> > { };

template <>
struct CharGroup<char16_t, CharGroupId::NonPrintable> : Compose<char16_t,
		Range<char16_t, u'\u0001', u'\u0020'>,
		Range<char16_t, u'\u2000', u'\u200D'>,
		Chars<char16_t, u'\u0020', u'\u0085', u'\u00A0', u'\u1680', u'\u2028', u'\u2029',
				 u'\u202F', u'\u205F', u'\u2060', u'\u3000', u'\uFEFF'>
> { };

template <>
struct CharGroup<char16_t, CharGroupId::LatinLowercase> : Compose<char16_t, Range<char16_t, u'a', u'z'> > { };

template <>
struct CharGroup<char16_t, CharGroupId::LatinUppercase> : Compose<char16_t, Range<char16_t, u'A', u'Z'> > { };

template <>
struct CharGroup<char16_t, CharGroupId::Alphanumeric> : Compose<char16_t,
		Range<char16_t, u'0', u'9'>,
		Range<char16_t, u'A', u'Z'>,
		Range<char16_t, u'a', u'z'>
> { };

template <>
struct CharGroup<char16_t, CharGroupId::Hexadecimial> : Compose<char16_t,
		Range<char16_t, u'0', u'9'>,
		Range<char16_t, u'A', u'F'>,
		Range<char16_t, u'a', u'f'>
> { };

template <>
struct CharGroup<char16_t, CharGroupId::Base64> : Compose<char16_t,
		Range<char16_t, u'0', u'9'>,
		Range<char16_t, u'A', u'Z'>,
		Range<char16_t, u'a', u'z'>,
		Chars<char16_t, u'=', u'/'>
> { };

template <>
struct CharGroup<char16_t, CharGroupId::BreakableWhiteSpace> : Compose<char16_t,
		Range<char16_t, u'\u0009', u'\u000D'>,
		Range<char16_t, u'\u2000', u'\u200D'>,
		Chars<char16_t, u'\u0020', u'\u0085', u'\u1680', u'\u2028', u'\u2029',
				 u'\u205F', u'\u2060', u'\u3000', u'\uFEFF'>
> { };

template <>
struct CharGroup<char16_t, CharGroupId::OpticalAlignmentSpecial> : Compose<char16_t,
		Chars<char16_t, u'(', u'[', u'{', u'"', u'\'', u'\\', u'<', u'«', u'„', u'.', u',', u'\u00AD', u'-'>
> { };

template <>
struct CharGroup<char16_t, CharGroupId::OpticalAlignmentBullet> : Compose<char16_t,
		Range<char16_t, u'0', u'9'>,
		Chars<char16_t, u'—', u'–', u'―', u'•', u'‣', u'⁃', u'-', u'*', u'◦', u'■', u'.', u',', u')'>
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
inline void Chars<CharType, Args...>::get(Set<CharType> &s) {
	foreach([&] (char16_t c) {
		s.insert(c);
	});
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
inline void Range<CharType, First, Last>::get(Set<CharType> &s) {
	foreach([&] (char16_t c) {
		s.insert(c);
	});
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
inline void Compose<CharType, Args...>::get(Set<CharType> &s) {
	foreach([&] (char16_t c) {
		s.insert(c);
	});
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
	for (CharType c = First; c <= Last; c++) {
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

NS_SP_EXT_END(chars)

#endif /* COMMON_STRING_SPCHARMATCHING_H_ */
