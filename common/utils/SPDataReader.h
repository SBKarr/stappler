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

#ifndef COMMON_UTILS_SPDATAREADER_H_
#define COMMON_UTILS_SPDATAREADER_H_

#include "SPCharReader.h"
#include "SPByteOrder.h"
#include "SPHalfFloat.h"

NS_SP_BEGIN

template <ByteOrder::Endian Endianess = ByteOrder::Endian::Network>
class DataReader : public BytesReader<uint8_t> {
public:
	template <class T>
	using Converter = ConverterTraits<Endianess, T>;

	using Self = DataReader<Endianess>;

	using PoolBytes = typename memory::PoolInterface::BytesType;
	using StdBytes = typename memory::StandartInterface::BytesType;

	DataReader();
	DataReader(const uint8_t *p, size_t l);
	DataReader(const PoolBytes &vec);
	DataReader(const StdBytes &vec);

	Self & operator =(const PoolBytes &b);
	Self & operator =(const StdBytes &b);
	Self & operator =(const Self &b);

	Self & set(const PoolBytes &vec);
	Self & set(const StdBytes &vec);
	Self & set(const Self &vec);
	Self & set(const uint8_t *p, size_t l);

	Self & operator ++ ();
	Self & operator ++ (int);
	Self & operator += (size_t l);

	bool operator == (const Self &other) const;
	bool operator != (const Self &other) const;

private:
	template <typename T>
	auto convert(const uint8_t *data) -> T;

public:
	uint64_t readUnsigned64();
	uint32_t readUnsigned32();
	uint32_t readUnsigned24();
	uint16_t readUnsigned16();
	uint8_t readUnsigned();

	double readFloat64();
	float readFloat32();
	float readFloat16();


	CharReaderBase readString(); // read null-terminated string
	CharReaderBase readString(size_t s); // read fixed-size string
};

using DataReaderNetwork = DataReader<ByteOrder::Endian::Network>;
using DataReaderHost = DataReader<ByteOrder::Endian::Host>;

template <ByteOrder::Endian Endianess>
DataReader<Endianess>::DataReader() { }

template <ByteOrder::Endian Endianess>
DataReader<Endianess>::DataReader(const uint8_t *p, size_t l)
: BytesReader(p, l) { }

template <ByteOrder::Endian Endianess>
DataReader<Endianess>::DataReader(const PoolBytes &vec)
: BytesReader(vec.data(), vec.size()) { }

template <ByteOrder::Endian Endianess>
DataReader<Endianess>::DataReader(const StdBytes &vec)
: BytesReader(vec.data(), vec.size()) { }


template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::operator =(const PoolBytes &b) -> Self & {
	return set(b);
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::operator =(const StdBytes &b) -> Self & {
	return set(b);
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::operator =(const Self &b) -> Self & {
	return set(b);
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::set(const PoolBytes &vec) -> Self & {
	ptr = vec.data();
	len = vec.size();
	return *this;
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::set(const StdBytes &vec) -> Self & {
	ptr = vec.data();
	len = vec.size();
	return *this;
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::set(const Self &vec) -> Self & {
	ptr = vec.data();
	len = vec.size();
	return *this;
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::set(const uint8_t *p, size_t l) -> Self & {
	ptr = p;
	len = l;
	return *this;
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::operator ++ () -> Self & {
	if (len > 0) {
		ptr ++; len --;
	}
	return *this;
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::operator ++ (int) -> Self & {
	if (len > 0) {
		ptr ++; len --;
	}
	return *this;
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::operator += (size_t l) -> Self & {
	if (len > 0) {
		offset(l);
	}
	return *this;
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::operator == (const Self &other) const -> bool {
	return ptr == other.ptr && len == other.len;
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::operator != (const Self &other) const -> bool {
	return !(*this == other);
}

template <ByteOrder::Endian Endianess>
template <typename T>
auto DataReader<Endianess>::convert(const uint8_t *data) -> T {
	T res;
	memcpy(&res, data, sizeof(T));
	return Converter<T>::Swap(res);
};

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::readUnsigned64() -> uint64_t {
	uint64_t ret = 0;
	if (len >= 8) {
		ret = convert<uint64_t>(ptr);
		ptr += 8; len -= 8;
	}
	return ret;
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::readUnsigned32() -> uint32_t {
	uint32_t ret = 0;
	if (len >= 4) {
		ret = convert<uint32_t>(ptr);
		ptr += 4; len -= 4;
	}
	return ret;
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::readUnsigned24() -> uint32_t {
	uint32_t ret = 0;
	if (len >= 3) {
		ret = (*ptr << 16) + (*(ptr + 1) << 8) + *(ptr + 2);
		ptr += 3; len -= 3;
	}
	return ret;
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::readUnsigned16() -> uint16_t {
	uint16_t ret = 0;
	if (len >= 2) {
		ret = convert<uint16_t>(ptr);
		ptr += 2; len -= 2;
	}
	return ret;
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::readUnsigned() -> uint8_t {
	uint8_t ret = 0;
	if (len > 0) { ret = *ptr; ++ ptr; -- len; }
	return ret;
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::readFloat64() -> double{
	double ret = 0;
	if (len >= 8) {
		ret = convert<double>(ptr);
		ptr += 8; len -= 8;
	}
	return ret;
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::readFloat32() -> float {
	float ret = 0; if (len >= 4) {
		ret = convert<float>(ptr);
		ptr += 4; len -= 4;
	}
	return ret;
}

template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::readFloat16() -> float {
	return halffloat::decode(readUnsigned16());
}

// read null-terminated string
template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::readString() -> CharReaderBase {
	size_t offset = 0; while (len - offset && ptr[offset]) { offset ++; }
	CharReaderBase ret((const char *)ptr, offset);
	ptr += offset; len -= offset;
	if (len && *ptr == 0) { ++ ptr; -- len; }
	return ret;
}

// read fixed-size string
template <ByteOrder::Endian Endianess>
auto DataReader<Endianess>::readString(size_t s) -> CharReaderBase {
	if (len < s) {
		s = len;
	}
	size_t offset = 0; while (len - offset && ptr[offset]) { offset ++; }
	CharReaderBase ret((const char *)ptr, s);
	ptr += s; len -= s;
	return ret;
}

NS_SP_END

#endif /* COMMON_UTILS_SPDATAREADER_H_ */
