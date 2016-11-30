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

#ifndef COMMON_IO_SPIOCONSUMER_H_
#define COMMON_IO_SPIOCONSUMER_H_

#include "SPIOCommon.h"

NS_SP_EXT_BEGIN(io)

struct ConsumerTraitsStream {
	using stream_type = std::basic_ostream<char>;
	static size_t WriteFn(void *ptr, const uint8_t *buf, size_t nbytes) {
		((stream_type *)ptr)->write((const char *)buf, nbytes);
		return nbytes;
	}

	static void FlushFn(void *ptr) { ((stream_type *)ptr)->flush(); }
};

template <typename T> size_t WriteFunction(T &, const uint8_t *buf, size_t nbytes);
template <typename T> void FlushFunction(T &);

template <class T>
struct ConsumerTraitsOverload {
	static size_t WriteFn(void *ptr, const uint8_t *buf, size_t nbytes) {
		return WriteFunction(*((T *)ptr), buf, nbytes);
	}

	static void FlushFn(void *ptr) { FlushFunction(*((T *)ptr)); }
};

template <typename T>
struct ConsumerTraitsReolution {
	using type = typename std::conditional<
			std::is_base_of<std::basic_ostream<char>, T>::value,
			ConsumerTraitsStream,
			ConsumerTraitsOverload<T>>::type;
};

template <typename T>
struct ConsumerTraits {
	using traits_type = typename ConsumerTraitsReolution<T>::type;

	static size_t WriteFn(void *ptr, const uint8_t *buf, size_t nbytes) {
		return traits_type::WriteFn(ptr, buf, nbytes);
	}

	static void FlushFn(void *ptr) { traits_type::FlushFn(ptr); }
};

struct Consumer {
	template <typename T, typename Traits = ConsumerTraits<typename std::decay<T>::type>> Consumer(T &t);

	size_t write(const uint8_t *buf, size_t nbytes) const;
	size_t write(const Buffer &) const;
	void flush() const;

	void *ptr = nullptr;
	write_fn write_ptr = nullptr;
	flush_fn flush_ptr = nullptr;
};

template <typename T, typename Traits>
inline Consumer::Consumer(T &t)
: ptr((void *)(&t))
, write_ptr(&Traits::WriteFn)
, flush_ptr(&Traits::FlushFn) { }

inline size_t Consumer::write(const uint8_t *buf, size_t nbytes) const { return write_ptr(ptr, buf, nbytes); }
inline void Consumer::flush() const { flush_ptr(ptr); }

NS_SP_EXT_END(io)

#endif /* COMMON_IO_SPIOCONSUMER_H_ */
