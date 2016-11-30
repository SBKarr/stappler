// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPCommon.h"
#include "SPString.h"

NS_SP_BEGIN

namespace {

using Stream = std::basic_ostream<char>;

static void EncodeTarget_String_emplace(void *ptr, const char & c) { ((String *)ptr)->push_back(c); }
static void EncodeTarget_String_reserve(void *ptr, size_t s) { ((String *)ptr)->reserve(s); }

static void EncodeTarget_Stream_emplace(void *ptr, const char & c) { (*((Stream *)ptr)) << (c); }
static void EncodeTarget_Stream_reserve(void *ptr, size_t s) { }

struct EncodeTarget {
	using emplace_fn = void (*) (void *, const char &);
	using reserve_fn = void (*) (void *, size_t);

	EncodeTarget(String &s) : _ptr(&s)
	, _emplace(&EncodeTarget_String_emplace)
	, _reserve(&EncodeTarget_String_reserve) { }
	EncodeTarget(Stream &s) : _ptr(&s)
	, _emplace(&EncodeTarget_Stream_emplace)
	, _reserve(&EncodeTarget_Stream_reserve) { }

	void emplace(const char &c) { _emplace(_ptr, c); }
	void reserve(size_t s) { _reserve(_ptr, s); }

	void *_ptr;
	emplace_fn _emplace;
	reserve_fn _reserve;
};

static void DecodeTarget_Bytes_emplace(void *ptr, const uint8_t & c) { ((Bytes *)ptr)->emplace_back(c); }
static void DecodeTarget_Bytes_reserve(void *ptr, size_t s) { ((Bytes *)ptr)->reserve(s); }

struct DecodeTarget {
	using emplace_fn = void (*) (void *, const uint8_t &);
	using reserve_fn = void (*) (void *, size_t);

	DecodeTarget(Bytes &s) : _ptr(&s)
	, _emplace(&DecodeTarget_Bytes_emplace)
	, _reserve(&DecodeTarget_Bytes_reserve) { }

	void emplace(const uint8_t &c) { _emplace(_ptr, c); }
	void reserve(size_t s) { _reserve(_ptr, s); }

	void *_ptr;
	emplace_fn _emplace;
	reserve_fn _reserve;
};

}

NS_SP_END


NS_SP_EXT_BEGIN(base64)

using Reader = DataReader<ByteOrder::Network>;

// Mapping from 6 bit pattern to ASCII character.
static unsigned char base64EncodeLookup[65] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Definition for "masked-out" areas of the base64DecodeLookup mapping
#define xx 65

// Mapping from ASCII character to 6 bit pattern.
static unsigned char base64DecodeLookup[256] =
{
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, 62, xx, xx, xx, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, xx, xx, xx, xx, xx, xx,
    xx,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, xx, xx, xx, xx, xx,
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

static void make_encode(EncodeTarget & output, const Reader &data) {
	Reader inputBuffer(data);
	auto length = inputBuffer.size();

	// Byte accurate calculation of final buffer size
	size_t outputBufferSize = ((length / BinaryUnit) + ((length % BinaryUnit) ? 1 : 0)) * Base64Unit;

	// Include space for a terminating zero
	output.reserve(outputBufferSize + 1);

	size_t i = 0;

	for (; i + BinaryUnit - 1 < length; i += BinaryUnit) {
		// Inner loop: turn 48 bytes into 64 base64 characters
		output.emplace(base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2]);
		output.emplace(base64EncodeLookup[((inputBuffer[i] & 0x03) << 4) | ((inputBuffer[i + 1] & 0xF0) >> 4)]);
		output.emplace(base64EncodeLookup[((inputBuffer[i + 1] & 0x0F) << 2) | ((inputBuffer[i + 2] & 0xC0) >> 6)]);
		output.emplace(base64EncodeLookup[inputBuffer[i + 2] & 0x3F]);
	}

	if (i + 1 < length) {
		// Handle the single '=' case
		output.emplace(base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2]);
		output.emplace(base64EncodeLookup[((inputBuffer[i] & 0x03) << 4) | ((inputBuffer[i + 1] & 0xF0) >> 4)]);
		output.emplace(base64EncodeLookup[(inputBuffer[i + 1] & 0x0F) << 2]);
		output.emplace('=');
	} else if (i < length) {
		// Handle the double '=' case
		output.emplace(base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2]);
		output.emplace(base64EncodeLookup[(inputBuffer[i] & 0x03) << 4]);
		output.emplace('=');
		output.emplace('=');
	}
}

static void make_decode(DecodeTarget & outputBuffer, const CharReaderBase &data) {
	CharReaderBase inputBuffer(data);
	auto length = inputBuffer.size();

	size_t outputBufferSize = ((length+Base64Unit-1) / Base64Unit) * BinaryUnit;
	outputBuffer.reserve(outputBufferSize);

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
			outputBuffer.emplace((accumulated[0] << 2) | (accumulated[1] >> 4));
		if(accumulateIndex >= 3)
			outputBuffer.emplace((accumulated[1] << 4) | (accumulated[2] >> 2));
		if(accumulateIndex >= 4)
			outputBuffer.emplace((accumulated[2] << 6) | accumulated[3]);
	}
}

String encode(const uint8_t *buffer, size_t length) { return base64::encode(Reader(buffer, length)); }
String encode(const Bytes &inputBuffer) { return base64::encode(Reader(inputBuffer.data(), inputBuffer.size())); }
String encode(const String &inputBuffer) { return base64::encode(Reader((const uint8_t *)inputBuffer.data(), inputBuffer.size())); }
String encode(const DataReader<ByteOrder::Network> &data) {
	String output;
	EncodeTarget target(output);
	make_encode(target, data);
	return output;
}

void encode(Stream &stream, const Bytes &data) { base64::encode(stream, Reader(data.data(), data.size())); }
void encode(Stream &stream, const String &data) { base64::encode(stream, Reader((const uint8_t *)data.data(), data.size())); }
void encode(Stream &stream, const uint8_t *data, size_t len) { base64::encode(stream, Reader(data, len)); }
void encode(Stream &stream, const Reader &data) {
	EncodeTarget target(stream);
	make_encode(target, data);
}

Bytes decode(const char *data, size_t len) { return base64::decode(CharReaderBase(data, len)); }
Bytes decode(const String &input) {	return base64::decode(CharReaderBase(input.data(), input.size())); }
Bytes decode(const CharReaderBase &data) {
	Bytes outputBuffer;
	DecodeTarget target(outputBuffer);
	make_decode(target, data);
	return outputBuffer;
}

NS_SP_EXT_END(base64)

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

static const char s_decTable[256] = {
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

String encode(const uint8_t *buffer, size_t length) { return base16::encode(Reader(buffer, length)); }
String encode(const Bytes &input) { return stappler::base16::encode(Reader(input.data(), input.size())); }
String encode(const String &input) { return stappler::base16::encode(Reader((const uint8_t *)input.data(), input.size())); }

String encode(const Reader &data) {
	Reader inputBuffer(data);
	auto length = inputBuffer.size();

#if SPAPR
	String output; output.resize(length * 2);
    for (size_t i = 0; i < length; ++i) {
    	memcpy(output.data() + i * 2, s_hexTable[inputBuffer[i]], 2);
    }
#else
	String output; output.reserve(length * 2);
    for (size_t i = 0; i < length; ++i) {
    	output.append(s_hexTable[inputBuffer[i]], 2);
    }
#endif

    return output;
}

void encode(Stream &stream, const Bytes &data) { return base16::encode(stream, Reader(data.data(), data.size())); }
void encode(Stream &stream, const String &data) { return base16::encode(stream, Reader((const uint8_t *)data.data(), data.size())); }
void encode(Stream &stream, const uint8_t *data, size_t len) { return base16::encode(stream, Reader(data, len)); }
void encode(Stream &stream, const Reader &data) {
	Reader inputBuffer(data);
	auto length = inputBuffer.size();
    for (size_t i = 0; i < length; ++i) {
    	stream << s_hexTable[inputBuffer[i]];
    }
}


Bytes decode(const char *data, size_t len) { return stappler::base16::decode(CharReaderBase(data, len)); }
Bytes decode(const String &input) { return stappler::base16::decode(CharReaderBase(input.data(), input.size())); }
Bytes decode(const CharReaderBase &data) {
	CharReaderBase inputBuffer(data);
	auto length = inputBuffer.size();

	Bytes outputBuffer; outputBuffer.reserve(length / 2);
	for (size_t i = 0; i < length; i += 2) {
		char a = s_decTable[(unsigned char)inputBuffer[i]];
		char b = s_decTable[(unsigned char)inputBuffer[i + 1]];

		outputBuffer.push_back((a << 4) | b);
	}
	return outputBuffer;
}

NS_SP_EXT_END(base16)
