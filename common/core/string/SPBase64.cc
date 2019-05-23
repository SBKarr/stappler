// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPCommon.h"
#include "SPString.h"

NS_SP_EXT_BEGIN(base64)

using Reader = DataReader<ByteOrder::Network>;

// Mapping from 6 bit pattern to ASCII character.
static const char * base64EncodeLookup = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Definition for "masked-out" areas of the base64DecodeLookup mapping
#define xx 65

static unsigned char base64DecodeLookup[256] = {
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, 62, xx, 62, xx, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, xx, xx, xx, xx, xx, xx,
    xx,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, xx, xx, xx, xx, 63,
    xx, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
};

// Fundamental sizes of the binary and base64 encode/decode units in bytes
constexpr int BinaryUnit = 3;
constexpr int Base64Unit = 4;

size_t encodeSize(size_t l) { return ((l / BinaryUnit) + ((l % BinaryUnit) ? 1 : 0)) * Base64Unit; }
size_t decodeSize(size_t l) { return ((l+Base64Unit-1) / Base64Unit) * BinaryUnit; }

template <typename Callback>
static void make_encode(const CoderSource &data, const Callback &cb) {
	Reader inputBuffer(data.data(), data.size());
	auto length = inputBuffer.size();

	size_t i = 0;
	for (; i + BinaryUnit - 1 < length; i += BinaryUnit) {
		// Inner loop: turn 48 bytes into 64 base64 characters
		cb(base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2]);
		cb(base64EncodeLookup[((inputBuffer[i] & 0x03) << 4) | ((inputBuffer[i + 1] & 0xF0) >> 4)]);
		cb(base64EncodeLookup[((inputBuffer[i + 1] & 0x0F) << 2) | ((inputBuffer[i + 2] & 0xC0) >> 6)]);
		cb(base64EncodeLookup[inputBuffer[i + 2] & 0x3F]);
	}

	if (i + 1 < length) {
		// Handle the single '=' case
		cb(base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2]);
		cb(base64EncodeLookup[((inputBuffer[i] & 0x03) << 4) | ((inputBuffer[i + 1] & 0xF0) >> 4)]);
		cb(base64EncodeLookup[(inputBuffer[i + 1] & 0x0F) << 2]);
		cb('=');
	} else if (i < length) {
		// Handle the double '=' case
		cb(base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2]);
		cb(base64EncodeLookup[(inputBuffer[i] & 0x03) << 4]);
		cb('=');
		cb('=');
	}
}

template <typename Callback>
static void make_decode(const CoderSource &data, const Callback &cb) {
	StringView inputBuffer((char *)data.data(), data.size());
	auto length = inputBuffer.size();

	size_t i = 0;
	while (i < length) {
		// Accumulate 4 valid characters (ignore everything else)
		unsigned char accumulated[Base64Unit];
		size_t accumulateIndex = 0;
		while (i < length) {
			unsigned char decode = base64DecodeLookup[(unsigned char)inputBuffer[i++]];
			if (decode != xx) {
				accumulated[accumulateIndex] = decode;
				accumulateIndex++;
				if (accumulateIndex == Base64Unit) {
					break;
				}
			}
		}

		if(accumulateIndex >= 2)
			cb((accumulated[0] << 2) | (accumulated[1] >> 4));
		if(accumulateIndex >= 3)
			cb((accumulated[1] << 4) | (accumulated[2] >> 2));
		if(accumulateIndex >= 4)
			cb((accumulated[2] << 6) | accumulated[3]);
	}
}

#undef xx

typename memory::PoolInterface::StringType __encode_pool(const CoderSource &source) {
	typename memory::PoolInterface::StringType output;
	output.reserve(encodeSize(source.size()));
	make_encode(source, [&] (const char &c) {
		output.push_back(c);
	});
	return output;
}
typename memory::StandartInterface::StringType __encode_std(const CoderSource &source) {
	typename memory::StandartInterface::StringType output;
	output.reserve(encodeSize(source.size()));
	make_encode(source, [&] (const char &c) {
		output.push_back(c);
	});
	return output;
}
void encode(std::basic_ostream<char> &stream, const CoderSource &source) {
	make_encode(source, [&] (const char &c) {
		stream << c;
	});
}

typename memory::PoolInterface::BytesType __decode_pool(const CoderSource &source) {
	typename memory::PoolInterface::BytesType output;
	output.reserve(encodeSize(source.size()));
	make_decode(source, [&] (const uint8_t &c) {
		output.emplace_back(c);
	});
	return output;
}
typename memory::StandartInterface::BytesType __decode_std(const CoderSource &source) {
	typename memory::StandartInterface::BytesType output;
	output.reserve(encodeSize(source.size()));
	make_decode(source, [&] (const uint8_t &c) {
		output.emplace_back(c);
	});
	return output;
}
void decode(std::basic_ostream<char> &stream, const CoderSource &source) {
	make_decode(source, [&] (const uint8_t &c) {
		stream << char(c);
	});
}

NS_SP_EXT_END(base64)

NS_SP_EXT_BEGIN(base64url)

using Reader = DataReader<ByteOrder::Network>;

// Mapping from 6 bit pattern to ASCII character.
static const char * base64EncodeLookup = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

// Fundamental sizes of the binary and base64 encode/decode units in bytes
constexpr int BinaryUnit = 3;
constexpr int Base64Unit = 4;

template <typename Callback>
static void make_encode(const CoderSource &data, const Callback &cb) {
	Reader inputBuffer(data.data(), data.size());
	auto length = inputBuffer.size();

	size_t i = 0;
	for (; i + BinaryUnit - 1 < length; i += BinaryUnit) {
		// Inner loop: turn 48 bytes into 64 base64 characters
		cb(base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2]);
		cb(base64EncodeLookup[((inputBuffer[i] & 0x03) << 4) | ((inputBuffer[i + 1] & 0xF0) >> 4)]);
		cb(base64EncodeLookup[((inputBuffer[i + 1] & 0x0F) << 2) | ((inputBuffer[i + 2] & 0xC0) >> 6)]);
		cb(base64EncodeLookup[inputBuffer[i + 2] & 0x3F]);
	}

	if (i + 1 < length) {
		// Handle the single '=' case
		cb(base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2]);
		cb(base64EncodeLookup[((inputBuffer[i] & 0x03) << 4) | ((inputBuffer[i + 1] & 0xF0) >> 4)]);
		cb(base64EncodeLookup[(inputBuffer[i + 1] & 0x0F) << 2]);
	} else if (i < length) {
		// Handle the double '=' case
		cb(base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2]);
		cb(base64EncodeLookup[(inputBuffer[i] & 0x03) << 4]);
	}
}

typename memory::PoolInterface::StringType __encode_pool(const CoderSource &source) {
	typename memory::PoolInterface::StringType output;
	output.reserve(encodeSize(source.size()));
	make_encode(source, [&] (const char &c) {
		output.push_back(c);
	});
	return output;
}
typename memory::StandartInterface::StringType __encode_std(const CoderSource &source) {
	typename memory::StandartInterface::StringType output;
	output.reserve(encodeSize(source.size()));
	make_encode(source, [&] (const char &c) {
		output.push_back(c);
	});
	return output;
}
void encode(std::basic_ostream<char> &stream, const CoderSource &source) {
	make_encode(source, [&] (const char &c) {
		stream << c;
	});
}

NS_SP_EXT_END(base64url)

NS_SP_EXT_BEGIN(base16)

using Reader = DataReader<ByteOrder::Network>;

static const char* s_hexTable[256] = {
    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0a", "0b", "0c", "0d", "0e", "0f", "10", "11",
    "12", "13", "14", "15", "16", "17", "18", "19", "1a", "1b", "1c", "1d", "1e", "1f", "20", "21", "22", "23",
    "24", "25", "26", "27", "28", "29", "2a", "2b", "2c", "2d", "2e", "2f", "30", "31", "32", "33", "34", "35",
    "36", "37", "38", "39", "3a", "3b", "3c", "3d", "3e", "3f", "40", "41", "42", "43", "44", "45", "46", "47",
    "48", "49", "4a", "4b", "4c", "4d", "4e", "4f", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59",
    "5a", "5b", "5c", "5d", "5e", "5f", "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6a", "6b",
    "6c", "6d", "6e", "6f", "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7a", "7b", "7c", "7d",
    "7e", "7f", "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8a", "8b", "8c", "8d", "8e", "8f",
    "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9a", "9b", "9c", "9d", "9e", "9f", "a0", "a1",
    "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "aa", "ab", "ac", "ad", "ae", "af", "b0", "b1", "b2", "b3",
    "b4", "b5", "b6", "b7", "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf", "c0", "c1", "c2", "c3", "c4", "c5",
    "c6", "c7", "c8", "c9", "ca", "cb", "cc", "cd", "ce", "cf", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
    "d8", "d9", "da", "db", "dc", "dd", "de", "df", "e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7", "e8", "e9",
    "ea", "eb", "ec", "ed", "ee", "ef", "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "fa", "fb",
    "fc", "fd", "fe", "ff"
};

static uint8_t s_decTable[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

size_t encodeSize(size_t length) { return length * 2; }
size_t decodeSize(size_t length) { return length / 2; }

const char *charToHex(const char &c) {
	return s_hexTable[reinterpret_cast<const uint8_t &>(c)];
}

uint8_t hexToChar(const char &c) {
	return s_decTable[reinterpret_cast<const uint8_t &>(c)];
}

uint8_t hexToChar(const char &c, const char &d) {
	return (hexToChar(c) << 4) | hexToChar(d);
}


template <>
auto encode<memory::PoolInterface>(const CoderSource &source) -> typename memory::PoolInterface::StringType {
	Reader inputBuffer(source.data(), source.size());
	const auto length = inputBuffer.size();

	memory::PoolInterface::StringType output; output.resize(length * 2);
	for (size_t i = 0; i < length; ++i) {
		memcpy(output.data() + i * 2, s_hexTable[inputBuffer[i]], 2);
	}
    return output;
}

template <>
auto encode<memory::StandartInterface>(const CoderSource &source) -> typename memory::StandartInterface::StringType {
	Reader inputBuffer(source.data(), source.size());
	const auto length = inputBuffer.size();

	memory::StandartInterface::StringType output; output.reserve(length * 2);
    for (size_t i = 0; i < length; ++i) {
    	output.append(s_hexTable[inputBuffer[i]], 2);
    }

    return output;
}

void encode(std::basic_ostream<char> &stream, const CoderSource &source) {
	Reader inputBuffer(source.data(), source.size());
	const auto length = inputBuffer.size();
    for (size_t i = 0; i < length; ++i) {
    	stream << s_hexTable[inputBuffer[i]];
    }
}
size_t encode(char *buf, size_t bsize, const CoderSource &source) {
	Reader inputBuffer(source.data(), source.size());
	const auto length = inputBuffer.size();

	size_t bytes = 0;
    for (size_t i = 0; i < length; ++i) {
    	if (bytes + 2 <= bsize) {
        	memcpy(buf + i * 2, s_hexTable[inputBuffer[i]], 2);
        	bytes += 2;
    	} else {
    		break;
    	}
    }
    return bytes;
}

template <>
auto decode<memory::PoolInterface>(const CoderSource &source) -> typename memory::PoolInterface::BytesType {
	const auto length = source.size();
	memory::PoolInterface::BytesType outputBuffer; outputBuffer.reserve(length / 2);
	for (size_t i = 0; i < length; i += 2) {
		outputBuffer.push_back(
				(s_decTable[source[i]] << 4)
					| s_decTable[source[i + 1]]);
	}
	return outputBuffer;
}

template <>
auto decode<memory::StandartInterface>(const CoderSource &source) -> typename memory::StandartInterface::BytesType {
	const auto length = source.size();
	memory::StandartInterface::BytesType outputBuffer; outputBuffer.reserve(length / 2);
	for (size_t i = 0; i < length; i += 2) {
		outputBuffer.push_back(
				(s_decTable[source[i]] << 4)
					| s_decTable[source[i + 1]]);
	}
	return outputBuffer;
}

void decode(std::basic_ostream<char> &stream, const CoderSource &source) {
	const auto length = source.size();

	for (size_t i = 0; i < length; i += 2) {
		stream << char(
				(s_decTable[source[i]] << 4)
					| s_decTable[source[i + 1]]);
	}
}
size_t decode(uint8_t *buf, size_t bsize, const CoderSource &source) {
	const auto length = source.size();

	size_t bytes = 0;
	for (size_t i = 0; i < length; i += 2) {
		if (bytes + 1 <= bsize) {
			buf[bytes] = uint8_t(
					(s_decTable[source[i]] << 4)
						| s_decTable[source[i + 1]]);
			++ bytes;
		} else {
			break;
		}
	}
	return bytes;
}

NS_SP_EXT_END(base16)
