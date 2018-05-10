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

#ifndef COMMON_IO_SPIOPRODUCER_H_
#define COMMON_IO_SPIOPRODUCER_H_

#include "SPIOCommon.h"

NS_SP_EXT_BEGIN(io)

struct ProducerTraitsStream {
	using stream_type = std::basic_istream<char>;

	static size_t ReadFn(void *ptr, uint8_t *buf, size_t nbytes) {
		((stream_type *)ptr)->get((stream_type::char_type *)buf, nbytes);
		return ((stream_type *)ptr)->gcount();
	}

	static size_t SeekFn(void *ptr, int64_t offset, Seek s) {
		std::ios_base::seekdir d;
		switch(s) {
		case Seek::Set: d = std::ios_base::beg; break;
		case Seek::Current: d = std::ios_base::cur; break;
		case Seek::End: d = std::ios_base::end; break;
		}
		auto & stream = *((stream_type *)ptr);
		stream.seekg(offset, d);
		if (stream.fail()) {
			return maxOf<size_t>();
		} else {
			return (size_t)stream.tellg();
		}
	}

	static size_t TellFn(void *ptr) {
		auto & stream = *((stream_type *)ptr);
		return (size_t)stream.tellg();
	}
};

template <class T> size_t ReadFunction(T &, uint8_t *buf, size_t nbytes);
template <class T> size_t SeekFunction(T &, int64_t offset, Seek s);
template <class T> size_t TellFunction(T &);

template <class T>
struct ProducerTraitsOverload {
	static size_t ReadFn(void *ptr, uint8_t *buf, size_t nbytes) {
		return ReadFunction(*((T *)ptr), buf, nbytes);
	}

	static size_t SeekFn(void *ptr, int64_t offset, Seek s) {
		return SeekFunction(*((T *)ptr), offset, s);
	}

	static size_t TellFn(void *ptr) {
		return TellFunction(*((T *)ptr));
	}
};

template <typename T>
struct ProducerTraitsResolution {
	using type = typename std::conditional<
		std::is_base_of<std::basic_istream<char>, T>::value,
		ProducerTraitsStream,
		ProducerTraitsOverload<T>>::type;
};

template <typename T>
struct ProducerTraits {
	using traits_type = typename ProducerTraitsResolution<T>::type;
	static size_t ReadFn(void *ptr, uint8_t *buf, size_t nbytes) {
		return traits_type::ReadFn(ptr, buf, nbytes);
	}

	static size_t SeekFn(void *ptr, int64_t offset, Seek s) { return traits_type::SeekFn(ptr, offset, s); }
	static size_t TellFn(void *ptr) { return traits_type::TellFn(ptr); }
};

struct Producer {
	template <typename T, typename Traits = ProducerTraits<typename std::decay<T>::type>> Producer(T &t);

	size_t read(uint8_t *buf, size_t nbytes) const;
	size_t read(const Buffer &, size_t nbytes) const;
	size_t seek(int64_t offset, Seek s) const;

	template <typename T>
	size_t seekAndRead(size_t offset, T &&, size_t nbytes) const;

	size_t tell() const;

	void *ptr = nullptr;
	read_fn read_ptr = nullptr;
	seek_fn seek_ptr = nullptr;
	size_fn tell_ptr = nullptr;
};

template <typename T, typename Traits>
inline Producer::Producer(T &t)
: ptr((void *)(&t))
, read_ptr(&Traits::ReadFn)
, seek_ptr(&Traits::SeekFn)
, tell_ptr(&Traits::TellFn) { }

inline size_t Producer::read(uint8_t *buf, size_t nbytes) const {
	return read_ptr(ptr, buf, nbytes);
}

inline size_t Producer::seek(int64_t offset, Seek s) const {
	return seek_ptr(ptr, offset, s);
}

template <typename T>
inline size_t Producer::seekAndRead(size_t offset, T && t, size_t nbytes) const {
	seek(offset, Seek::Set);
	return read(std::forward<T>(t), nbytes);
}

inline size_t Producer::tell() const { return tell_ptr(ptr); }

NS_SP_EXT_END(io)

#endif /* COMMON_IO_SPIOPRODUCER_H_ */
