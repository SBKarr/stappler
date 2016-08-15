/*
 * SPDataReader.h
 *
 *  Created on: 8 нояб. 2015 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_CORE_STRUCT_SPDATAREADER_H_
#define STAPPLER_SRC_CORE_STRUCT_SPDATAREADER_H_

#include "SPCharReader.h"
#include "SPByteOrder.h"
#include "SPHalfFloat.h"

NS_SP_BEGIN

template <ByteOrder::Endian Endianess = ByteOrder::Endian::Network>
class DataReader : public BytesReader<uint8_t, Bytes> {
public:
	template <class T>
	using Converter = ConverterTraits<Endianess, T>;

	using Self = DataReader<Endianess>;

	inline DataReader() { }
	inline DataReader(const uint8_t *p, size_t l) : BytesReader(p, l) { }
	inline DataReader(const Bytes &vec) : BytesReader(vec.data(), vec.size()) { }

	inline Self & operator =(const Bytes &vec) { return set(vec); }
	inline Self & set(const Bytes &vec) { ptr = vec.data(); len = vec.size(); return *this; }
	inline Self & set(const uint8_t *p, size_t l) { ptr = p; len = l; return *this; }

	inline Self & operator ++ () { if (len > 0) { ptr ++; len --; } return *this; }
	inline Self & operator ++ (int) { if (len > 0) { ptr ++; len --; } return *this; }
	inline Self & operator += (size_t l) { if (len > 0) { offset(l); } return *this; }

	inline bool operator == (const Self &other) const { return ptr == other.ptr && len == other.len; }
	inline bool operator != (const Self &other) const { return !(*this == other); }

private:
	template <typename T>
	auto convert(const uint8_t *data) -> T {
		T res;
		memcpy(&res, data, sizeof(T));
		return Converter<T>::Swap(res);
	};

public:
	inline uint64_t readUnsigned64() {
		uint64_t ret = 0;
		if (len >= 8) {
			ret = convert<uint64_t>(ptr);
			ptr += 8; len -= 8;
		}
		return ret;
	}
	inline uint32_t readUnsigned32() {
		uint32_t ret = 0;
		if (len >= 4) {
			ret = convert<uint32_t>(ptr);
			ptr += 4; len -= 4;
		}
		return ret;
	}
	inline uint32_t readUnsigned24() {
		uint32_t ret = 0;
		if (len >= 3) {
			ret = (*ptr << 16) + (*(ptr + 1) << 8) + *(ptr + 2);
			ptr += 3; len -= 3;
		}
		return ret;
	}
	inline uint16_t readUnsigned16() {
		uint16_t ret = 0;
		if (len >= 2) {
			ret = convert<uint16_t>(ptr);
			ptr += 2; len -= 2;
		}
		return ret;
	}
	inline uint8_t readUnsigned() {
		uint8_t ret = 0;
		if (len > 0) { ret = *ptr; ++ ptr; -- len; }
		return ret;
	}

	inline double readFloat64() {
		double ret = 0;
		if (len >= 8) {
			ret = convert<double>(ptr);
			ptr += 8; len -= 8;
		}
		return ret;
	}
	inline float readFloat32() {
		float ret = 0; if (len >= 4) {
			ret = convert<float>(ptr);
			ptr += 4; len -= 4;
		}
		return ret;
	}
	inline float readFloat16() {
		return halffloat::decode(readUnsigned16());
	}

	// read null-terminated string
	inline CharReaderBase readString() {
		size_t offset = 0; while (len - offset && ptr[offset]) { offset ++; }
		CharReaderBase ret((const char *)ptr, offset);
		ptr += offset; len -= offset;
		if (len && *ptr == 0) { ++ ptr; -- len; }
		return ret;
	}

	// read fixed-size string
	inline CharReaderBase readString(size_t s) {
		if (len < s) {
			s = len;
		}
		size_t offset = 0; while (len - offset && ptr[offset]) { offset ++; }
		CharReaderBase ret((const char *)ptr, s);
		ptr += s; len -= s;
		return ret;
	}
};

using DataReaderNetwork = DataReader<ByteOrder::Endian::Network>;
using DataReaderHost = DataReader<ByteOrder::Endian::Host>;

NS_SP_END

#endif /* STAPPLER_SRC_CORE_STRUCT_SPDATAREADER_H_ */
