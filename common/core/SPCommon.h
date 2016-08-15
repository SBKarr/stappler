/*
 * SPDefault.h
 *
 *  Created on: 19 марта 2015 г.
 *      Author: sbkarr
 */

#ifndef COMMON_CORE_SPCOMMON_H_
#define COMMON_CORE_SPCOMMON_H_

#include "SPCore.h"

/*
 *   Toolkit-depended definition
 *
 *   Defines some universal STL-like types, that can be used in all
 *   toolkit variants
 *
 *   - String
 *   - Bytes
 *   - Vector
 *   - Map
 *   - Set
 *   - Function
 *   - StringStream
 *   - OutputStream
 *   - InputFileStream
 *   - OutputFileStream
 */

#ifdef SPDEFAULT
NS_SP_EXT_BEGIN(toolkit)

template <typename T>
struct BytesContainer;

struct AllocBase { };

struct TypeTraits {
	using string_type = std::string;
	using ucs2_string_type = std::u16string;
	using bytes_type = std::vector<uint8_t>;

	using string_stream = std::stringstream;
	using output_stream = std::ostream;

	using input_file_stream = std::ifstream;
	using output_file_stream = std::ofstream;

	using allocator_base = AllocBase;

	template <typename T, typename Compare = std::less<T>>
	using set_type = std::set<T, Compare>;

	template <typename T>
	using vector_type = std::vector<T>;

	template <typename K, typename V, typename Compare = std::less<K>>
	using map_type = std::map<K, V, Compare>;

	template <class T>
	using function_type = std::function<T>;

	template <class T>
	using bytes_container = BytesContainer<T>;
};

NS_SP_EXT_END(toolkit)
#else

#include "SPAprAllocStack.h"
#include "SPAprFunction.h"
#include "SPAprString.h"
#include "SPAprVector.h"
#include "SPAprStringStream.h"
#include "SPAprFileStream.h"
#include "SPAprSet.h"
#include "SPAprMap.h"

NS_SP_EXT_BEGIN(toolkit)

template <typename T>
struct BytesContainer;

struct TypeTraits {
	using string_type = apr::basic_string<char>;
	using ucs2_string_type = apr::basic_string<char16_t>;
	using bytes_type = apr::vector<uint8_t>;

	using string_stream = apr::basic_ostringstream<char>;
	using output_stream = std::ostream;

	using input_file_stream = apr::ifstream;
	using output_file_stream = apr::ofstream;

	using allocator_base = apr::AllocPool;

	template <typename T, typename Compare = std::less<>>
	using set_type = apr::set<T, Compare>;

	template <typename T>
	using vector_type = apr::vector<T>;

	template <typename K, typename V, typename Compare = std::less<>>
	using map_type = apr::map<K, V, Compare>;

	template <class T>
	using function_type = apr::function<T>;

	template <class T>
	using bytes_container = BytesContainer<T>;
};

NS_SP_EXT_END(toolkit)
#endif


NS_SP_EXT_BEGIN(toolkit)

template <>
struct BytesContainer<char> {
	using type = TypeTraits::string_type;
};

template <>
struct BytesContainer<uint8_t> {
	using type = TypeTraits::bytes_type;
};

template <>
struct BytesContainer<char16_t> {
	using type = TypeTraits::ucs2_string_type;
};

NS_SP_EXT_END(toolkit)

NS_SP_BEGIN

using String = toolkit::TypeTraits::string_type;
using WideString = toolkit::TypeTraits::ucs2_string_type;
using Bytes = toolkit::TypeTraits::bytes_type;

using StringStream = toolkit::TypeTraits::string_stream;
using OutputStream = toolkit::TypeTraits::output_stream;
using InputFileStream = toolkit::TypeTraits::input_file_stream;
using OutputFileStream = toolkit::TypeTraits::output_file_stream;

template <typename T>
using Vector = toolkit::TypeTraits::vector_type<T>;

template <typename K, typename V, typename Compare = std::less<void>>
using Map = toolkit::TypeTraits::map_type<K, V, Compare>;

template <class T>
using Function = toolkit::TypeTraits::function_type<T>;

template <typename T, typename Compare = std::less<void>>
using Set = toolkit::TypeTraits::set_type<T, Compare>;

using AllocBase = toolkit::TypeTraits::allocator_base;

NS_SP_END

#endif /* COMMON_CORE_SPCOMMON_H_ */
