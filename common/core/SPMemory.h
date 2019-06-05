/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_CORE_SPMEMORY_H_
#define COMMON_CORE_SPMEMORY_H_

#include "SPCommon.h"
#include "SPStringView.h"
#include "SPData.h"

NS_SP_BEGIN


namespace mem_pool {

namespace pool = memory::pool;

using pool_t = memory::pool_t;

using Time = stappler::Time;
using TimeInterval = stappler::TimeInterval;

using StringView = stappler::StringView;
using Interface = stappler::memory::PoolInterface;

using AllocBase = stappler::memory::AllocPool;

using String = stappler::memory::string;
using WideString = stappler::memory::u16string;
using Bytes = stappler::memory::vector<uint8_t>;

using StringStream = stappler::memory::ostringstream;
using OutputStream = std::ostream;

template <typename T>
using Vector = stappler::memory::vector<T>;

template <typename K, typename V, typename Compare = std::less<void>>
using Map = stappler::memory::map<K, V, Compare>;

template <typename T, typename Compare = std::less<void>>
using Set = stappler::memory::set<T, Compare>;

template <typename T>
using Function = stappler::memory::function<T>;

template <typename T>
using Callback = stappler::memory::callback<T>;

template <typename T, typename V>
using Pair = std::pair<T, V>;

using Value = stappler::data::ValueTemplate<stappler::memory::PoolInterface>;
using Array = Value::ArrayType;
using Dictionary = Value::DictionaryType;
using EncodeFormat = stappler::data::EncodeFormat;

inline auto writeData(const Value &data, EncodeFormat fmt = EncodeFormat()) -> Bytes {
	return stappler::data::EncodeTraits<stappler::memory::PoolInterface>::write(data, fmt);
}

inline bool writeData(std::ostream &stream, const Value &data, EncodeFormat fmt = EncodeFormat()) {
	return stappler::data::EncodeTraits<stappler::memory::PoolInterface>::write(stream, data, fmt);
}

template <typename T>
inline mem_pool::String toString(const T &t) {
	return stappler::string::ToStringTraits<Interface>::toString(t);
}

template <typename T>
inline void toStringStream(mem_pool::StringStream &stream, T value) {
	stream << value;
}

template <typename T, typename... Args>
inline void toStringStream(mem_pool::StringStream &stream, T value, Args && ... args) {
	stream << value;
	mem_pool::toStringStream(stream, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
inline mem_pool::String toString(T t, Args && ... args) {
	mem_pool::StringStream stream;
	mem_pool::toStringStream(stream, t);
	mem_pool::toStringStream(stream, std::forward<Args>(args)...);
    return stream.str();
}

}


namespace mem_std {

using pool_t = memory::pool_t;

using Time = stappler::Time;
using TimeInterval = stappler::TimeInterval;

using StringView = stappler::StringView;
using Interface = stappler::memory::StandartInterface;

using AllocBase = stappler::memory::AllocPool;

using String = std::string;
using WideString = std::u16string;
using Bytes = std::vector<uint8_t>;

using StringStream = std::stringstream;
using OutputStream = std::ostream;

template <typename T>
using Vector = std::vector<T>;

template <typename K, typename V, typename Compare = std::less<void>>
using Map = std::map<K, V, Compare>;

template <typename T, typename Compare = std::less<void>>
using Set = std::set<T, Compare>;

template <typename T>
using Function = std::function<T>;

template <typename T>
using Callback = memory::callback<T>;

template <typename T, typename V>
using Pair = std::pair<T, V>;

using Value = stappler::data::ValueTemplate<stappler::memory::StandartInterface>;
using Array = Value::ArrayType;
using Dictionary = Value::DictionaryType;
using EncodeFormat = stappler::data::EncodeFormat;

inline auto writeData(const Value &data, EncodeFormat fmt = EncodeFormat()) -> Bytes {
	return stappler::data::EncodeTraits<stappler::memory::StandartInterface>::write(data, fmt);
}

inline bool writeData(std::ostream &stream, const Value &data, EncodeFormat fmt = EncodeFormat()) {
	return stappler::data::EncodeTraits<stappler::memory::StandartInterface>::write(stream, data, fmt);
}

template <typename T>
inline mem_std::String toString(const T &t) {
	return stappler::string::ToStringTraits<Interface>::toString(t);
}

template <typename T>
inline void toStringStream(mem_std::StringStream &stream, T value) {
	stream << value;
}

template <typename T, typename... Args>
inline void toStringStream(mem_std::StringStream &stream, T value, Args && ... args) {
	stream << value;
	mem_std::toStringStream(stream, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
inline mem_std::String toString(T t, Args && ... args) {
	mem_std::StringStream stream;
	mem_std::toStringStream(stream, t);
	mem_std::toStringStream(stream, std::forward<Args>(args)...);
    return stream.str();
}

}


NS_SP_END

#endif /* COMMON_CORE_SPMEMORY_H_ */
