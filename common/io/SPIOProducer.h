/*
 * SPIOProduser.h
 *
 *  Created on: 3 апр. 2016 г.
 *      Author: sbkarr
 */

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
struct ProducerTraitsReolution {
	using type = typename std::conditional<
			std::is_base_of<std::basic_istream<char>, T>::value,
			ProducerTraitsStream,
			ProducerTraitsOverload<T>>::type;
};

template <typename T>
struct ProducerTraits {
	using traits_type = typename ProducerTraitsReolution<T>::type;
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
