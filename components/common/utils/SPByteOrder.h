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

#ifndef COMMON_UTILS_SPBYTEORDER_H_
#define COMMON_UTILS_SPBYTEORDER_H_

#include "SPCore.h"

#ifndef __has_builtin         // Optional of course
  #define __has_builtin(x) 0  // Compatibility with non-clang compilers
#endif

//  Adapted code from BOOST_ENDIAN_INTRINSICS
//  GCC and Clang recent versions provide intrinsic byte swaps via builtins
#if (defined(__clang__) && __has_builtin(__builtin_bswap32) && __has_builtin(__builtin_bswap64)) \
  || (defined(__GNUC__ ) && \
  (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)))

// prior to 4.8, gcc did not provide __builtin_bswap16 on some platforms so we emulate it
// see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=52624
// Clang has a similar problem, but their feature test macros make it easier to detect
NS_SP_EXT_BEGIN(intrinsics)
# if (defined(__clang__) && __has_builtin(__builtin_bswap16)) \
  || (defined(__GNUC__) &&(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)))
inline uint16_t bswap16(uint16_t x) { return __builtin_bswap16(x); }
# else
inline uint16_t bswap16(uint16_t x) { return __builtin_bswap32(x) << 16; }
# endif
inline uint32_t bswap32(uint32_t x) { return __builtin_bswap32(x); }
inline uint64_t bswap64(uint64_t x) { return __builtin_bswap64(x); }
NS_SP_EXT_END(intrinsics)

//  Linux systems provide the byteswap.h header, with
#elif defined(__linux__)
//  don't check for obsolete forms defined(linux) and defined(__linux) on the theory that
//  compilers that predefine only these are so old that byteswap.h probably isn't present.
# include <byteswap.h>
NS_SP_EXT_BEGIN(intrinsics)
inline uint16_t bswap16(uint16_t x) { return bswap_16(x); }
inline uint32_t bswap32(uint32_t x) { return bswap_32(x); }
inline uint64_t bswap64(uint64_t x) { return bswap_64(x); }
NS_SP_EXT_END(intrinsics)

#elif defined(_MSC_VER)
//  Microsoft documents these as being compatible since Windows 95 and specificly
//  lists runtime library support since Visual Studio 2003 (aka 7.1).
# include <cstdlib>
inline uint16_t bswap16(uint16_t x) { return _byteswap_ushort(x); }
inline uint32_t bswap32(uint32_t x) { return _byteswap_ulong(x); }
inline uint64_t bswap64(uint64_t x) { return _byteswap_uint64(x); }
#else
NS_SP_EXT_BEGIN(intrinsics)
inline uint16_t bswap16(uint16_t x) {
	return (x & 0xFF) << 8 | ((x >> 8) & 0xFF);
}

inline uint32_t bswap32(uint32_t x) {
	return x & 0xFF << 24
		| (x >> 8 & 0xFF) << 16
		| (x >> 16 & 0xFF) << 8
		| (x >> 24 & 0xFF);
}
inline uint64_t bswap64(uint64_t x) {
	return x & 0xFF << 56
		| (x >> 8 & 0xFF) << 48
		| (x >> 16 & 0xFF) << 40
		| (x >> 24 & 0xFF) << 32
		| (x >> 32 & 0xFF) << 24
		| (x >> 40 & 0xFF) << 16
		| (x >> 48 & 0xFF) << 8
		| (x >> 56 & 0xFF);
}
NS_SP_EXT_END(intrinsics)
#endif


NS_SP_BEGIN

static constexpr size_t Bit8Size = 1;
static constexpr size_t Bit16Size = 2;
static constexpr size_t Bit32Size = 4;
static constexpr size_t Bit64Size = 8;

template <class T, bool DoByteSwap, size_t Size>
struct Converter;

template <class T>
struct Converter<T, true, Bit8Size> {
	static inline T Swap(T value) {
		return value;
	}
};

template <class T>
struct Converter<T, true, Bit16Size> {
	static inline T Swap(T value) {
		return reinterpretValue<T>(intrinsics::bswap16(reinterpretValue<uint16_t>(value)));
	}
};

template <class T>
struct Converter<T, true, Bit32Size> {
	static inline T Swap(T value) {
		return reinterpretValue<T>(intrinsics::bswap32(reinterpretValue<uint32_t>(value)));
	}
};

template <class T>
struct Converter<T, true, Bit64Size> {
	static inline T Swap(T value) {
		return reinterpretValue<T>(intrinsics::bswap64(reinterpretValue<uint64_t>(value)));
	}
};

template <class T, size_t Size>
struct Converter<T, true, Size> {
	static inline T Swap(T value) {
		union {
			T val;
			uint8_t bytes[sizeof(T)];
		} source, dest;
		source.val = value;

		for (size_t k = 0; k < sizeof(T); k++) {
			dest.bytes[k] = source.bytes[sizeof(T) - k - 1];
		}

		return dest.val;
	}
};

template <class T, size_t Size>
struct Converter<T, false, Size> {
	static inline T Swap(T value) { return value; }
};

// While N3620 is not implemented, we use platform-specific macro to detect endianess
struct ByteOrder {
	enum Endian {
		Big,
		Little,
		Network = Big,
#if (__i386__) || (_M_IX86) || (__x86_64__) || (_M_X64) || (__arm__) || (_M_ARM) || (__arm64__) || (__arm64) || defined (__aarch64__)
		Host = Little,
#else
		Host = Big,
#endif

	};

	static constexpr bool isLittleEndian() {
	    return Endian::Host == Endian::Little;
	}

	template <class T>
	using NetworkConverter = Converter<T, Endian::Host != Endian::Network, sizeof(T)>;

	template <class T>
	using LittleConverter = Converter<T, Endian::Host != Endian::Little, sizeof(T)>;

	template <class T>
	using BigConverter = Converter<T, Endian::Host != Endian::Big, sizeof(T)>;

	template <class T>
	using HostConverter = Converter<T, Endian::Host != Endian::Host, sizeof(T)>;

	template <class T>
	static inline T HostToNetwork(T value) {
		return NetworkConverter<T>::Swap(value);
	}

	template <class T>
	static inline T NetworkToHost(T value) {
		return NetworkConverter<T>::Swap(value);
	}

	template <class T>
	static inline T HostToLittle(T value) {
		return LittleConverter<T>::Swap(value);
	}

	template <class T>
	static inline T LittleToHost(T value) {
		return LittleConverter<T>::Swap(value);
	}

	template <class T>
	static inline T HostToBig(T value) {
		return BigConverter<T>::Swap(value);
	}

	template <class T>
	static inline T BigToHost(T value) {
		return BigConverter<T>::Swap(value);
	}
};

template <ByteOrder::Endian Endianess, typename T>
struct ConverterTraits;

template <typename T>
struct ConverterTraits<ByteOrder::Endian::Big, T> : ByteOrder::BigConverter<T> { };

template <typename T>
struct ConverterTraits<ByteOrder::Endian::Little, T> : ByteOrder::LittleConverter<T> { };

NS_SP_END

#endif /* COMMON_UTILS_SPBYTEORDER_H_ */
