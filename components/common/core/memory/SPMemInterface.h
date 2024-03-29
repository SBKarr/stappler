/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_COMMON_CORE_MEMORY_SPMEMINTERFACE_H_
#define COMPONENTS_COMMON_CORE_MEMORY_SPMEMINTERFACE_H_

#include "SPCore.h"

#include "SPMemFunction.h"
#include "SPMemString.h"
#include "SPMemVector.h"
#include "SPMemStringStream.h"
#include "SPMemSet.h"
#include "SPMemMap.h"
#include "SPMemDict.h"

NS_SP_EXT_BEGIN(memory)

struct PoolInterface : public memory::AllocPool {
	using AllocBaseType = memory::AllocPool;
	using StringType = memory::string;
	using WideStringType = memory::u16string;
	using BytesType = memory::vector<uint8_t>;

	template <typename Value> using BasicStringType = memory::basic_string<Value>;
	template <typename Value> using ArrayType = memory::vector<Value>;
	template <typename Value> using DictionaryType = memory::map<StringType, Value, std::less<>>;
	template <typename Value> using VectorType = memory::vector<Value>;

	template <typename K, typename V, typename Compare = std::less<>>
	using MapType = memory::map<K, V, Compare>;

	template <typename T, typename Compare = std::less<>>
	using SetType = memory::set<T, Compare>;

	using StringStreamType = memory::ostringstream;

	static constexpr bool usesMemoryPool() { return true; }
};

struct StandartInterface : public memory::AllocBase {
	using AllocBaseType = memory::AllocBase;
	using StringType = std::string;
	using WideStringType = std::u16string;
	using BytesType = std::vector<uint8_t>;

	template <typename Value> using BasicStringType = std::basic_string<Value>;
	template <typename Value> using ArrayType = std::vector<Value>;
	template <typename Value> using DictionaryType = std::map<StringType, Value, std::less<>>;
	template <typename Value> using VectorType = std::vector<Value>;

	template <typename K, typename V, typename Compare = std::less<>>
	using MapType = std::map<K, V, Compare>;

	template <typename T, typename Compare = std::less<>>
	using SetType = std::set<T, Compare>;

	using StringStreamType = std::ostringstream;

	static constexpr bool usesMemoryPool() { return false; }
};

NS_SP_EXT_END(memory)

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

#if STELLATOR
NS_SP_EXT_BEGIN(toolkit)
struct TypeTraits {
	using string_type = memory::basic_string<char>;
	using ucs2_string_type = memory::basic_string<char16_t>;
	using bytes_type = memory::vector<uint8_t>;

	template <typename T>
	using basic_string_stream = memory::basic_ostringstream<T>;

	using string_stream = memory::basic_ostringstream<char>;
	using output_stream = std::ostream;

	using input_file_stream = std::ifstream;
	using output_file_stream = std::ofstream;

	using allocator_base = memory::AllocPool;
	using mutex_type = std::mutex;

	template <typename T, typename Compare = std::less<>>
	using set_type = memory::set<T, Compare>;

	template <typename T>
	using vector_type = memory::vector<T>;

	template <typename K, typename V, typename Compare = std::less<>>
	using map_type = memory::map<K, V, Compare>;

	template <typename K, typename V>
	using unordered_map_type = std::unordered_map<K, V>;

	template <class T>
	using function_type = memory::function<T>;

	using primary_interface = memory::PoolInterface;
	using secondary_interface = memory::StandartInterface;
};
NS_SP_EXT_END(toolkit)
#elif SPDEFAULT
NS_SP_EXT_BEGIN(toolkit)
struct TypeTraits {
	using string_type = std::string;
	using ucs2_string_type = std::u16string;
	using bytes_type = std::vector<uint8_t>;

	template <typename T>
	using basic_string_stream = std::basic_stringstream<T>;

	using string_stream = std::stringstream;
	using output_stream = std::ostream;

	using input_file_stream = std::ifstream;
	using output_file_stream = std::ofstream;

	using allocator_base = memory::AllocBase;
	using mutex_type = std::mutex;

	template <typename T, typename Compare = std::less<>>
	using set_type = std::set<T, Compare>;

	template <typename T>
	using vector_type = std::vector<T>;

	template <typename K, typename V, typename Compare = std::less<>>
	using map_type = std::map<K, V, Compare>;

	template <typename K, typename V>
	using unordered_map_type = std::unordered_map<K, V>;

	template <class T>
	using function_type = std::function<T>;

	using primary_interface = memory::StandartInterface;
	using secondary_interface = memory::PoolInterface;
};
NS_SP_EXT_END(toolkit)
#else

#include "SPAprFileStream.h"
#include "SPAprMutex.h"

NS_SP_EXT_BEGIN(toolkit)

struct TypeTraits {
	using string_type = memory::basic_string<char>;
	using ucs2_string_type = memory::basic_string<char16_t>;
	using bytes_type = memory::vector<uint8_t>;

	template <typename T>
	using basic_string_stream = memory::basic_ostringstream<T>;

	using string_stream = memory::basic_ostringstream<char>;
	using output_stream = std::ostream;

	using input_file_stream = apr::ifstream;
	using output_file_stream = apr::ofstream;

	using allocator_base = memory::AllocPool;
	using mutex_type = apr::mutex;

	template <typename T, typename Compare = std::less<>>
	using set_type = memory::set<T, Compare>;

	template <typename T>
	using vector_type = memory::vector<T>;

	template <typename K, typename V, typename Compare = std::less<>>
	using map_type = memory::map<K, V, Compare>;

	template <typename K, typename V>
	using unordered_map_type = std::unordered_map<K, V>;

	template <class T>
	using function_type = memory::function<T>;

	using primary_interface = memory::PoolInterface;
	using secondary_interface = memory::StandartInterface;
};

NS_SP_EXT_END(toolkit)

#endif

NS_SP_EXT_BEGIN(memory)

using DefaultInterface = toolkit::TypeTraits::primary_interface;
using AlternativeInterface = toolkit::TypeTraits::secondary_interface;

NS_SP_EXT_END(memory)

NS_SP_BEGIN

using String = toolkit::TypeTraits::string_type;
using WideString = toolkit::TypeTraits::ucs2_string_type;
using Bytes = toolkit::TypeTraits::bytes_type;

using StringStream = toolkit::TypeTraits::string_stream;
using OutputStream = toolkit::TypeTraits::output_stream;
using InputFileStream = toolkit::TypeTraits::input_file_stream;
using OutputFileStream = toolkit::TypeTraits::output_file_stream;

// Fix for CDT parser on Windows (MSYS)
#if __CDT_PARSER__ && MSYS
template <typename T>
using Vector = memory::PoolInterface::VectorType<T>;

template <typename T>
using Function = memory::function<T>;

#else

template <typename T>
using Vector = toolkit::TypeTraits::vector_type<T>;

template <typename T>
using Function = toolkit::TypeTraits::function_type<T>;

#endif

template <typename K, typename V, typename Compare = std::less<void>>
using Map = toolkit::TypeTraits::map_type<K, V, Compare>;

template <typename K, typename V>
using HashMap = toolkit::TypeTraits::unordered_map_type<K, V>;

template <typename T>
using Callback = memory::callback<T>;

template <typename T, typename Compare = std::less<void>>
using Set = toolkit::TypeTraits::set_type<T, Compare>;

using AllocBase = toolkit::TypeTraits::allocator_base;
using Mutex = toolkit::TypeTraits::mutex_type;

template <typename T> auto StringToNumber(const memory::StandartInterface::StringType &str) -> T {
	return StringToNumber<T>(str.data(), nullptr, 0);
}

template <typename T> auto StringToNumber(const memory::PoolInterface::StringType &str) -> T {
	return StringToNumber<T>(str.data(), nullptr, 0);
}

template <typename T> auto StringToNumber(const char *str) -> T {
	return StringToNumber<T>(str, nullptr, 0);
}

NS_SP_END

NS_SP_BEGIN

#if __CDT_PARSER__ && MSYS
template <typename T>
using StdVector = memory::PoolInterface::VectorType<T>;

class StdThread {
public:
	StdThread() noexcept;
	StdThread( StdThread&& other ) noexcept;

	template< class Function, class... Args >
	explicit StdThread( Function&& f, Args&&... args );

	StdThread( const StdThread& ) = delete;

	~StdThread();

	StdThread& operator=( StdThread&& other ) noexcept;
	void swap( std::thread& other ) noexcept;

	bool joinable() const noexcept;
	std::thread::id get_id() const noexcept;
	void * native_handle();
	void join();
	void detach();
};

#else
template <typename T>
using StdVector = std::vector<T>;

using StdThread = std::thread;
#endif

NS_SP_END

#endif /* COMPONENTS_COMMON_CORE_MEMORY_SPMEMINTERFACE_H_ */
