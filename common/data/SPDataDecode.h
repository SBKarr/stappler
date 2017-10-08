/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_DATA_SPDATADECODE_H_
#define COMMON_DATA_SPDATADECODE_H_

#include "SPDataDecodeCbor.h"
#include "SPDataDecodeJson.h"
#include "SPDataDecodeSerenity.h"
#include "SPDataStream.h"
#include "SPFilesystem.h"

NS_SP_EXT_BEGIN(data)

enum class DataFormat {
	Json,
	Cbor,
	Serenity

	// for future implementations
	// LZ4,
	// LZ4HC,
	// Encrypt,
};

inline DataFormat detectDataFormat(const uint8_t *ptr, size_t size) {
	if (size > 3 && ptr[0] == 0xd9 && ptr[1] == 0xd9 && ptr[2] == 0xf7) {
		return DataFormat::Cbor;
	} else if (ptr[0] == '(') {
		return DataFormat::Serenity;
	} else {
		return DataFormat::Json;
	}
}

template <typename StringType, typename Interface = DefaultInterface>
auto read(const StringType &data, const String &key = "") -> ValueTemplate<Interface> {
	if (data.size() == 0) {
		return ValueTemplate<Interface>();
	}
	auto ff = detectDataFormat((const uint8_t *)data.data(), data.size());
	switch (ff) {
	case DataFormat::Cbor:
		return cbor::read<Interface>(data);
		break;
	case DataFormat::Json:
		return json::read<Interface>(StringView((char *)data.data(), data.size()));
		break;
	case DataFormat::Serenity:
		return serenity::read<Interface>(StringView((char *)data.data(), data.size()));
		break;
	default:
		break;
	}
	return ValueTemplate<Interface>();
}

template <typename Interface = DefaultInterface>
auto readFile(const String &filename, const String &key = "") -> ValueTemplate<Interface> {
	Stream stream;
	filesystem::readFile(stream, filename);
	return stream.extract<Interface>();
}

NS_SP_EXT_END(data)

#endif /* COMMON_DATA_SPDATADECODE_H_ */
