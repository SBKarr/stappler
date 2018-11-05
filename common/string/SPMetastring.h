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

#ifndef COMMON_STRING_SPMETASTRING_H_
#define COMMON_STRING_SPMETASTRING_H_

#include "SPStringView.h"

NS_SP_EXT_BEGIN(metastring)

template <char... Chars>
struct metastring {
	template <typename Interface = memory::DefaultInterface>
	static constexpr auto string() -> typename Interface::StringType { return { Chars ... }; }
	static constexpr std::array<char, sizeof... (Chars)> std_string() { return { Chars ... }; }
	static constexpr std::array<char, sizeof... (Chars)> memory_string() { return { Chars ... }; }
	static constexpr std::array<char, sizeof... (Chars)> array() { return {{ Chars ... }}; }

	template <typename Interface = memory::DefaultInterface>
	auto to_string() -> typename Interface::StringType const { return { Chars ... }; }
	std::string to_std_string() const { return {Chars ...}; }
	memory::string to_memory_string() const { return {Chars ...}; }
	constexpr std::array<char, sizeof... (Chars)> to_array() const { return {{Chars...}}; }

	operator memory::string() const { return {Chars ...}; }
	operator std::string() const { return {Chars ...}; }

	constexpr size_t size() const { return sizeof ... (Chars); }
};

template <char... Lhs, char ... Rhs>
constexpr inline auto merge(const metastring<Lhs ...> &, const metastring<Rhs ...> &) {
	return metastring<Lhs ..., Rhs ...>();
}

template <typename ... Args, char... Lhs, char ... Rhs>
constexpr inline auto merge(const metastring<Lhs ...> &lhs, const metastring<Rhs ...> &rhs, Args && ... args) {
	return merge(lhs, merge(rhs, args ...));
}

template <char... Lhs>
constexpr inline auto merge(const metastring<Lhs ...> &lhs) {
	return lhs;
}

constexpr inline auto merge() {
	return metastring<>();
}

constexpr int num_digits (size_t x) { return x < 10 ? 1 : 1 + num_digits (x / 10); }

template<int size, size_t x, char... args>
struct numeric_builder {
	using type = typename numeric_builder<size - 1, x / 10, '0' + (x % 10), args...>::type;
};

template<size_t x, char... args>
struct numeric_builder<2, x, args...> {
	using type = metastring<'0' + x / 10, '0' + (x % 10), args...>;
};

template<size_t x, char... args>
struct numeric_builder<1, x, args...> {
	using type = metastring<'0' + x, args...>;
};

template<size_t x>
using numeric = typename numeric_builder<num_digits (x), x>::type;

NS_SP_EXT_END(metastring)


NS_SP_BEGIN

/*template <char ... Chars>
inline std::basic_ostream<char> &
operator << (std::basic_ostream<char> & os, const metastring::metastring<Chars ...> &str) {
	auto arr = str.to_array();
	return os.write(arr.data(), arr.size());
}*/

template <typename CharType, CharType ... Chars> auto operator "" _meta() {
	return metastring::metastring<Chars ...>();
}

NS_SP_END


NS_SP_EXT_BEGIN(metastring)

template <char ... Chars>
inline std::basic_ostream<char> &
operator << (std::basic_ostream<char> & os, const metastring<Chars ...> &str) {
	auto arr = str.to_array();
	return os.write(arr.data(), arr.size());
}

NS_SP_EXT_END(metastring)

#endif /* COMMON_STRING_SPMETASTRING_H_ */
