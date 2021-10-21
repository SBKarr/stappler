/**
Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_COMMON_CORE_STRING_SPHASH_H_
#define COMPONENTS_COMMON_CORE_STRING_SPHASH_H_

#include <stdint.h>

// Based on XXH (https://cyan4973.github.io/xxHash/#benchmarks)
// constexpr implementation from https://github.com/ekpyron/xxhashct

// Requires C++17

namespace stappler::hash {

#define SP_HASH_INLINE [[gnu::always_inline]]

class xxh32 {
public:
	static constexpr uint32_t hash (const char *input, uint32_t len, uint32_t seed) {
		return finalize((len >= 16
				? h16bytes(input, len, seed)
				: seed + PRIME5)
			+ len, (input) + (len & ~0xF), len & 0xF);
	}

private:
	static constexpr uint32_t PRIME1 = 0x9E3779B1U;
	static constexpr uint32_t PRIME2 = 0x85EBCA77U;
	static constexpr uint32_t PRIME3 = 0xC2B2AE3DU;
	static constexpr uint32_t PRIME4 = 0x27D4EB2FU;
	static constexpr uint32_t PRIME5 = 0x165667B1U;

	SP_HASH_INLINE static constexpr uint32_t rotl (uint32_t x, int r) {
		return ((x << r) | (x >> (32 - r)));
	}
	SP_HASH_INLINE static constexpr uint32_t round (uint32_t acc, const uint32_t input) {
		return rotl(acc + (input * PRIME2), 13) * PRIME1;
	}
	SP_HASH_INLINE static constexpr uint32_t avalanche_step (const uint32_t h, const int rshift, const uint32_t prime) {
		return (h ^ (h >> rshift)) * prime;
	}
	SP_HASH_INLINE static constexpr uint32_t avalanche (const uint32_t h) {
		return avalanche_step(avalanche_step(avalanche_step(h, 15, PRIME2), 13, PRIME3), 16, 1);
	}
	SP_HASH_INLINE static constexpr uint32_t endian32 (const char *v) {
		return uint32_t(static_cast<uint8_t>(v[0])) | (uint32_t(static_cast<uint8_t>(v[1])) << 8)
				| (uint32_t(static_cast<uint8_t>(v[2])) << 16) | (uint32_t(static_cast<uint8_t>(v[3])) << 24);
	}
	SP_HASH_INLINE static constexpr uint32_t fetch32 (const char *p, const uint32_t v) {
		return round(v, endian32(p));
	}
	SP_HASH_INLINE static constexpr uint32_t finalize (uint32_t h, const char *p, uint32_t len) {
        while (len >= 4) {
        	h = rotl(h + (endian32(p) * PRIME3), 17) * PRIME4;
            len -= 4;
            p += 4;
        }
        while (len > 0) {
        	h = rotl(h + (static_cast<uint8_t>(*p++) * PRIME5), 11) * PRIME1;
            --len;
        }
		return avalanche(h);
	}
	SP_HASH_INLINE static constexpr uint32_t h16bytes (const char *p, uint32_t len, uint32_t v1, uint32_t v2, uint32_t v3, uint32_t v4) {
        const char * const limit = p + len - 16;
        do {
            v1 = fetch32(p, v1); p += 4;
            v2 = fetch32(p, v2); p += 4;
            v3 = fetch32(p, v3); p += 4;
            v4 = fetch32(p, v4); p += 4;
        } while (p <= limit);
		return rotl(v1, 1) + rotl(v2, 7) + rotl(v3, 12) + rotl(v4, 18);
	}
	SP_HASH_INLINE static constexpr uint32_t h16bytes (const char *p, uint32_t len, const uint32_t seed) {
		return h16bytes(p, len, seed + PRIME1 + PRIME2, seed + PRIME2, seed, seed - PRIME1);
	}
};

class xxh64 {
public:
	static constexpr uint64_t hash (const char *p, uint64_t len, uint64_t seed) {
		return finalize ((len >= 32
				? h32bytes (p, len, seed)
				: seed + PRIME5)
			+ len, p + (len & ~0x1F), len & 0x1F);
	}

private:
	static constexpr uint64_t PRIME1 = 0x9E3779B185EBCA87ULL; /*!< 0b1001111000110111011110011011000110000101111010111100101010000111 */
	static constexpr uint64_t PRIME2 = 0xC2B2AE3D27D4EB4FULL; /*!< 0b1100001010110010101011100011110100100111110101001110101101001111 */
	static constexpr uint64_t PRIME3 = 0x165667B19E3779F9ULL; /*!< 0b0001011001010110011001111011000110011110001101110111100111111001 */
	static constexpr uint64_t PRIME4 = 0x85EBCA77C2B2AE63ULL; /*!< 0b1000010111101011110010100111011111000010101100101010111001100011 */
	static constexpr uint64_t PRIME5 = 0x27D4EB2F165667C5ULL; /*!< 0b0010011111010100111010110010111100010110010101100110011111000101 */

	SP_HASH_INLINE static constexpr uint64_t rotl (uint64_t x, int r) {
		return ((x << r) | (x >> (64 - r)));
	}
	SP_HASH_INLINE static constexpr uint64_t mix1 (const uint64_t h, const uint64_t prime, int rshift) {
		return (h ^ (h >> rshift)) * prime;
	}
	SP_HASH_INLINE static constexpr uint64_t mix2 (const uint64_t p, const uint64_t v = 0) {
		return rotl (v + p * PRIME2, 31) * PRIME1;
	}
	SP_HASH_INLINE static constexpr uint64_t mix3 (const uint64_t h, const uint64_t v) {
		return (h ^ mix2 (v)) * PRIME1 + PRIME4;
	}
	SP_HASH_INLINE static constexpr uint32_t endian32(const char *v) {
		return uint32_t(static_cast<uint8_t>(v[0])) | (uint32_t(static_cast<uint8_t>(v[1])) << 8)
				| (uint32_t(static_cast<uint8_t>(v[2])) << 16) | (uint32_t(static_cast<uint8_t>(v[3])) << 24);
	}
	SP_HASH_INLINE static constexpr uint64_t endian64 (const char *v) {
		return uint64_t(static_cast<uint8_t>(v[0])) | (uint64_t(static_cast<uint8_t>(v[1])) << 8)
				| (uint64_t(static_cast<uint8_t>(v[2])) << 16) | (uint64_t(static_cast<uint8_t>(v[3])) << 24)
				| (uint64_t(static_cast<uint8_t>(v[4])) << 32) | (uint64_t(static_cast<uint8_t>(v[5])) << 40)
				| (uint64_t(static_cast<uint8_t>(v[6])) << 48) | (uint64_t(static_cast<uint8_t>(v[7])) << 56);
	}
	SP_HASH_INLINE static constexpr uint64_t fetch64 (const char *p, const uint64_t v = 0) {
		return mix2 (endian64 (p), v);
	}
	SP_HASH_INLINE static constexpr uint64_t fetch32 (const char *p) {
		return uint64_t (endian32 (p)) * PRIME1;
	}
	SP_HASH_INLINE static constexpr uint64_t finalize (uint64_t h, const char * __restrict__ p, uint64_t len) {
		while (len >= 8) {
			h = rotl (h ^ fetch64 (p), 27) * PRIME1 + PRIME4;
			p += 8;
			len -= 8;
		}
		if (len >= 4) {
			h = rotl (h ^ fetch32 (p), 23) * PRIME2 + PRIME3;
			p += 4;
			len -= 4;
		}
		while (len > 0) {
			h = rotl (h ^ ((static_cast<uint8_t>(*p++)) * PRIME5), 11) * PRIME1;
			--len;
		}
		return (mix1 (mix1 (mix1 (h, PRIME2, 33), PRIME3, 29), 1, 32));
	}
	SP_HASH_INLINE static constexpr uint64_t h32bytes(const char *__restrict__ p, uint64_t len, uint64_t v1, uint64_t v2, uint64_t v3, uint64_t v4) {
		const char *const limit = p + len - 32;
		do {
			v1 = fetch64(p, v1); p += 8;
			v2 = fetch64(p, v2); p += 8;
			v3 = fetch64(p, v3); p += 8;
			v4 = fetch64(p, v4); p += 8;
		} while (p <= limit);
		return mix3(mix3(mix3(mix3(rotl(v1, 1) + rotl(v2, 7) + rotl(v3, 12) + rotl(v4, 18), v1), v2), v3), v4);
	}
	SP_HASH_INLINE static constexpr uint64_t h32bytes(const char *__restrict__ p, uint64_t len, const uint64_t seed) {
		return h32bytes(p, len, seed + PRIME1 + PRIME2, seed + PRIME2, seed, seed - PRIME1);
	}
};

inline constexpr uint32_t hash32(const char* str, uint32_t len, uint32_t seed = 0) {
    return xxh32::hash(str, len, seed);
}

inline constexpr uint64_t hash64(const char* str, size_t len, uint64_t seed = 0) {
    return xxh64::hash(str, len, seed);
}

}

#endif /* COMPONENTS_COMMON_CORE_STRING_SPHASH_H_ */
