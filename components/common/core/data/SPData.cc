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
#include "SPData.h"

#include "SPStringView.h"
#include "SPFilesystem.h"
#include "SPString.h"
#include "SPDataStream.h"

#define LZ4_HC_STATIC_LINKING_ONLY 1
#include "lz4hc.h"

NS_SP_EXT_BEGIN(data)

EncodeFormat EncodeFormat::CborCompressed(EncodeFormat::Cbor, EncodeFormat::LZ4HCCompression);
EncodeFormat EncodeFormat::JsonCompressed(EncodeFormat::Json, EncodeFormat::LZ4HCCompression);

Value parseCommandLineOptions(int argc, const char * argv[],
		const Function<int (Value &ret, char c, const char *str)> &switchCallback,
		const Function<int (Value &ret, const StringView &str, int argc, const char * argv[])> &stringCallback) {
	if (argc == 0) {
		return Value();
	}

	Value ret;
	auto &args = ret.setValue(Value(Value::Type::ARRAY), "args");

	int i = argc;
	while (i > 0) {
		const char *value = argv[argc - i];
		char quoted = 0;
		if (value[0] == '\'' || value[0] == '"') {
			quoted = value[0];
			value ++;
		}
		if (value[0] == '-') {
			if (value[1] == '-') {
				if (stringCallback) {
					i -= (stringCallback(ret, &value[2], i - 1, &argv[argc - i + 1]) - 1);
				} else {
					i -= 1;
				}
			} else {
				if (switchCallback) {
					const char *str = &value[1];
					while (str[0] != 0) {
						str += switchCallback(ret, str[0], &str[1]);
					}
				}
			}
		} else {
			if (quoted > 0) {
				size_t len = strlen(value);
				if (len > 0 && value[len - 1] == quoted) {
					-- len;
				}
				args.addString(String(value, len));
			} else {
				if (i == argc) {
					args.addString(filesystem_native::nativeToPosix(value));
				} else {
					args.addString(value);
				}
			}
		}
		i --;
	}

	return ret;
}

Value parseCommandLineOptions(int argc, const char16_t * wargv[],
		const Function<int (Value &ret, char c, const char *str)> &switchCallback,
		const Function<int (Value &ret, const StringView &str, int argc, const char * argv[])> &stringCallback) {
	Vector<String> vec; vec.reserve(argc);
	Vector<const char *> argv; argv.reserve(argc);
	for (int i = 0; i < argc; ++ i) {
		vec.push_back(string::toUtf8(wargv[i]));
		argv.push_back(vec.back().c_str());
	}

	return parseCommandLineOptions(argc, argv.data(), switchCallback, stringCallback);
}

Transform::Transform() noexcept { }

Transform::Transform(const Transform &t) noexcept : order(t.order), map(t.map), subtransforms(t.subtransforms) { }
Transform::Transform(Transform &&t) noexcept : order(std::move(t.order)), map(std::move(t.map)), subtransforms(std::move(t.subtransforms)) { }

Transform & Transform::operator=(const Transform &t) noexcept {
	map = t.map;
	subtransforms = t.subtransforms;
	return *this;
}
Transform & Transform::operator=(Transform &&t) noexcept {
	map = std::move(t.map);
	subtransforms = std::move(t.subtransforms);
	return *this;
}

data::Value &Transform::transform(data::Value &value) const {
	Vector<data::Value *> stack;
	makeTransform(value, stack);
	return value;
}

Transform Transform::reverse() const {
	if (empty()) {
		return Transform();
	}
	Transform ret;
	Vector<Pair<const String *, Transform *>> stack;
	makeReverse(ret, stack);
	return ret;
}

data::Value Transform::data() const {
	data::Value ret;
	for (auto &ord : order) {
		if (ord.second == Mode::Map) {
			auto m = map.find(ord.first);
			if (m != map.end()) {
				auto &arr = ret.setValue(Value(Value::Type::ARRAY), m->first);
				for (auto &obj : m->second) {
					arr.addString(obj);
				}
			}
		} else {
			auto s = subtransforms.find(ord.first);
			if (s != subtransforms.end()) {
				ret.setValue(s->second.data(), s->first);
			}
		}
	}

	return ret;
}

bool Transform::empty() const {
	return map.empty() && subtransforms.empty();
}

String Transform::transformKey(const String &key) const {
	auto it = map.find(key);
	if (it != map.end() && it->second.size() == 1) {
		return it->second.front();
	}
	return String();
}

void Transform::performTransform(data::Value &value, Vector<data::Value *> &stack, const Pair<String, Vector<String>> &it) const {
	if (value.hasValue(it.first)) {
		if (it.second.empty()) {
			value.erase(it.first);
		} else if (it.second.size() == 1) {
			if (it.second.front().empty()) {
				value.erase(it.first);
			} else if (it.first != it.second.front()) {
				value.setValue(std::move(value.getValue(it.first)), it.second.front());
				value.erase(it.first);
			}
		} else {
			if (it.first != it.second.front()) {
				data::Value *target = &value;
				size_t size = it.second.size();
				size_t stackLevel = 0;
				size_t i = 0;
				while (i < size && it.second.at(i).empty()) {
					++ stackLevel;
					++ i;
				}
				if (stackLevel <= stack.size() && i < size) {
					if (stackLevel > 0) {
						target = stack.at(stack.size() - stackLevel);
					}

					auto &key = it.second.at(i);
					for (; i < size - 1; i++) {
						auto &val = target->getValue(key);
						if (val.isDictionary()) {
							target = &val;
						} else if (val.isNull()) {
							target = &target->setValue(Value(Value::Type::DICTIONARY), key);
						} else {
							break;
						}
					}

					if (i == size - 1) {
						target->setValue(std::move(value.getValue(it.first)), it.second.back());
						value.erase(it.first);
					}
				}
			}
		}
	}
}

void Transform::performTransform(data::Value &value, Vector<data::Value *> &stack, const Pair<String, Transform> &it) const {
	stack.push_back(&value);
	auto & val = value.getValue(it.first);
	if (val.isDictionary()) {
		it.second.makeTransform(val, stack);
	}
	stack.pop_back();
}
void Transform::makeTransform(data::Value &value, Vector<data::Value *> &stack) const {
	for (auto &ord : order) {
		if (ord.second == Mode::Map) {
			auto m = map.find(ord.first);
			if (m != map.end()) {
				performTransform(value, stack, *m);
			}
		} else {
			auto s = subtransforms.find(ord.first);
			if (s != subtransforms.end()) {
				performTransform(value, stack, *s);
			}
		}
	}
}

void Transform::performReverse(Transform &ret, Vector<Pair<const String *, Transform *>> &stack,
		const Pair<String, Vector<String>> &it) const {
	if (it.second.size() == 1) {
		ret.map.emplace(it.second.front(), Vector<String>{it.first});
		ret.order.emplace_back(pair(it.second.front(), Mode::Map));
	} else if (it.second.size() > 1) {
		size_t size = it.second.size();
		size_t i = 0;
		size_t stackLevel = 0;
		Transform *target = &ret;
		Vector<String> vec; vec.reserve(size);
		while (i < size && it.second.at(i).empty()) {
			++ stackLevel;
			++ i;
			if (stackLevel <= stack.size()) {
				vec.emplace_back(*stack.at(stack.size() - stackLevel).first);
			}
		}
		if (stackLevel <= stack.size() && i < size) {
			if (stackLevel > 0) {
				target = stack.at(stack.size() - stackLevel).second;
			}
			for (; i < size - 1; ++ i) {
				auto &key = it.second.at(i);
				auto it = target->subtransforms.find(key);
				if (it != target->subtransforms.end()) {
					target = &it->second;
				} else {
					auto em_it = target->subtransforms.emplace(key, Transform()).first;
					target->order.emplace_back(pair(key, Mode::Sub));
					target = &em_it->second;
				}
				vec.push_back(String());
			}

			if (i == size - 1) {
				std::reverse(vec.begin(), vec.end());
				vec.push_back(it.first);
				target->map.emplace(it.second.back(), std::move(vec));
				target->order.emplace_back(pair(it.second.back(), Mode::Map));
			}
		}
	}
}
void Transform::performReverse(Transform &ret, Vector<Pair<const String *, Transform *>> &stack,
		const Pair<String, Transform> &it) const {
	stack.push_back(pair(&it.first, &ret));
	bool emplaced = false;
	auto t = ret.subtransforms.find(it.first);
	if (t == ret.subtransforms.end()) {
		t = ret.subtransforms.emplace(it.first, Transform()).first;
		emplaced = true;
	}

	it.second.makeReverse(t->second, stack);
	if (t->second.map.empty() && t->second.subtransforms.empty()) {
		ret.subtransforms.erase(t);
	} else if (emplaced) {
		ret.order.emplace_back(pair(t->first, Mode::Sub));
	}

	stack.pop_back();
}

void Transform::makeReverse(Transform &ret, Vector<Pair<const String *, Transform *>> &stack) const {
	for (auto it = order.rbegin(); it != order.rend(); ++it) {
		if (it->second == Mode::Map) {
			auto m = map.find(it->first);
			if (m != map.end()) {
				performReverse(ret, stack, *m);
			}
		} else {
			auto s = subtransforms.find(it->first);
			if (s != subtransforms.end()) {
				performReverse(ret, stack, *s);
			}
		}
	}
}

int EncodeFormat::EncodeStreamIndex = std::ios_base::xalloc();

namespace serenity {

bool shouldEncodePercent(char c) {
#define V16 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	static uint8_t s_decTable[256] = {
		V16, V16, // 0-1, 0-F
		1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, // 2, 0-F
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, // 3, 0-F
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 4, 0-F
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, // 5, 0-F
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 6, 0-F
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, // 7, 0-F
		V16, V16, V16, V16, V16, V16, V16, V16,
	};

	return bool(s_decTable[*((uint8_t *)(&c))]);
}

}

template <>
template <>
auto ValueTemplate<memory::PoolInterface>::convert<memory::PoolInterface>() const -> ValueTemplate<memory::PoolInterface> {
	return ValueTemplate<memory::PoolInterface>(*this);
}

template <>
template <>
auto ValueTemplate<memory::StandartInterface>::convert<memory::StandartInterface>() const -> ValueTemplate<memory::StandartInterface> {
	return ValueTemplate<memory::StandartInterface>(*this);
}

template <>
template <>
auto ValueTemplate<memory::PoolInterface>::convert<memory::StandartInterface>() const -> ValueTemplate<memory::StandartInterface> {
	switch (_type) {
	case Type::INTEGER: return ValueTemplate<memory::StandartInterface>(intVal); break;
	case Type::DOUBLE: return ValueTemplate<memory::StandartInterface>(doubleVal); break;
	case Type::BOOLEAN: return ValueTemplate<memory::StandartInterface>(boolVal); break;
	case Type::CHARSTRING:
		return ValueTemplate<memory::StandartInterface>(memory::StandartInterface::StringType(strVal->data(), strVal->size()));
		break;
	case Type::BYTESTRING:
		return ValueTemplate<memory::StandartInterface>(memory::StandartInterface::BytesType(bytesVal->data(), bytesVal->data() + bytesVal->size()));
		break;
	case Type::ARRAY: {
		ValueTemplate<memory::StandartInterface> ret(ValueTemplate<memory::StandartInterface>::Type::ARRAY);
		auto &arr = ret.asArray();
		arr.reserve(arrayVal->size());
		for (auto &it : *arrayVal) {
			arr.emplace_back(it.convert<memory::StandartInterface>());
		}
		return ret;
		break;
	}
	case Type::DICTIONARY: {
		ValueTemplate<memory::StandartInterface> ret(ValueTemplate<memory::StandartInterface>::Type::DICTIONARY);
		auto &dict = ret.asDict();
		for (auto &it : *dictVal) {
			dict.emplace(StringView(it.first).str<memory::StandartInterface>(), it.second.convert<memory::StandartInterface>());
		}
		return ret;
		break;
	}
	default:
		break;
	}
	return ValueTemplate<memory::StandartInterface>();
}

template <>
template <>
auto ValueTemplate<memory::StandartInterface>::convert<memory::PoolInterface>() const -> ValueTemplate<memory::PoolInterface> {
	switch (_type) {
	case Type::INTEGER: return ValueTemplate<memory::PoolInterface>(intVal); break;
	case Type::DOUBLE: return ValueTemplate<memory::PoolInterface>(doubleVal); break;
	case Type::BOOLEAN: return ValueTemplate<memory::PoolInterface>(boolVal); break;
	case Type::CHARSTRING:
		return ValueTemplate<memory::PoolInterface>(memory::PoolInterface::StringType(strVal->data(), strVal->size()));
		break;
	case Type::BYTESTRING:
		return ValueTemplate<memory::PoolInterface>(memory::PoolInterface::BytesType(bytesVal->data(), bytesVal->data() + bytesVal->size()));
		break;
	case Type::ARRAY: {
		ValueTemplate<memory::PoolInterface> ret(ValueTemplate<memory::PoolInterface>::Type::ARRAY);
		auto &arr = ret.asArray();
		arr.reserve(arrayVal->size());
		for (auto &it : *arrayVal) {
			arr.emplace_back(it.convert<memory::PoolInterface>());
		}
		return ret;
		break;
	}
	case Type::DICTIONARY: {
		ValueTemplate<memory::PoolInterface> ret(ValueTemplate<memory::PoolInterface>::Type::DICTIONARY);
		auto &dict = ret.asDict();
		dict.reserve(dictVal->size());
		for (auto &it : *dictVal) {
			dict.emplace(StringView(it.first).str<memory::PoolInterface>(), it.second.convert<memory::PoolInterface>());
		}
		return ret;
		break;
	}
	default:
		break;
	}
	return ValueTemplate<memory::PoolInterface>();
}

size_t getCompressBounds(size_t size, EncodeFormat::Compression c) {
	switch (c) {
	case EncodeFormat::LZ4Compression:
	case EncodeFormat::LZ4HCCompression: {
		if (size < LZ4_MAX_INPUT_SIZE) {
			return LZ4_compressBound(size) + ((size <= 0xFFFF) ? 2 : 4);
		}
		return 0;
		break;
	}
	case EncodeFormat::BrotliLowCompression:
		break;
	case EncodeFormat::BrotliHighCompression:
		break;
	case EncodeFormat::NoCompression:
		break;
	}
	return 0;
}

thread_local uint8_t tl_lz4HCEncodeState[std::max(sizeof(LZ4_streamHC_t), sizeof(LZ4_stream_t))];
thread_local uint8_t tl_compressBuffer[128_KiB];

uint8_t *getLZ4EncodeState() {
	return tl_lz4HCEncodeState;
}

size_t compressData(const uint8_t *src, size_t srcSize, uint8_t *dest, size_t destSize, EncodeFormat::Compression c) {
	switch (c) {
	case EncodeFormat::LZ4Compression: {
		const int offSize = ((srcSize <= 0xFFFF) ? 2 : 4);
		const int ret = LZ4_compress_fast_extState(tl_lz4HCEncodeState, (const char *)src, (char *)dest + offSize, srcSize, destSize - offSize, 1);
		if (ret > 0) {
			if (srcSize <= 0xFFFF) {
				uint16_t sz = srcSize;
				memcpy(dest, &sz, sizeof(sz));
			} else {
				uint32_t sz = srcSize;
				memcpy(dest, &sz, sizeof(sz));
			}
			return ret + offSize;
		}
		break;
	}
	case EncodeFormat::LZ4HCCompression: {
		const int offSize = ((srcSize <= 0xFFFF) ? 2 : 4);
		const int ret = LZ4_compress_HC_extStateHC(tl_lz4HCEncodeState, (const char *)src, (char *)dest + offSize, srcSize, destSize + offSize, LZ4HC_CLEVEL_MAX);
		if (ret > 0) {
			if (srcSize <= 0xFFFF) {
				uint16_t sz = srcSize;
				memcpy(dest, &sz, sizeof(sz));
			} else {
				uint32_t sz = srcSize;
				memcpy(dest, &sz, sizeof(sz));
			}
			return ret + offSize;
		}
		break;
	}
	case EncodeFormat::BrotliLowCompression:
		break;
	case EncodeFormat::BrotliHighCompression:
		break;
	case EncodeFormat::NoCompression:
		break;
	}
	return 0;
}

void writeCompressionMark(uint8_t *data, size_t sourceSize, EncodeFormat::Compression c) {
	switch (c) {
	case EncodeFormat::LZ4Compression:
	case EncodeFormat::LZ4HCCompression:
		if (sourceSize <= 0xFFFF) {
			memcpy(data, "LZ4S", 4);
		} else {
			memcpy(data, "LZ4W", 4);
		}
		break;
	case EncodeFormat::BrotliLowCompression:
	case EncodeFormat::BrotliHighCompression:
		memcpy(data, "SPBr", 4);
		break;
	case EncodeFormat::NoCompression:
		break;
	}
}

template <typename Interface>
static inline auto doCompress(const uint8_t *src, size_t size, EncodeFormat::Compression c, bool conditional) -> typename Interface::BytesType {
	auto bufferSize = getCompressBounds(size, c);
	if (bufferSize == 0) {
		return typename Interface::BytesType();
	} else if (bufferSize <= sizeof(tl_compressBuffer)) {
		auto encodeSize = compressData(src, size, tl_compressBuffer, sizeof(tl_compressBuffer), c);
		if (encodeSize == 0 || (conditional && encodeSize + 4 > size)) { return typename Interface::BytesType(); }
		typename Interface::BytesType ret; ret.resize(encodeSize + 4);
		writeCompressionMark(ret.data(), size, c);
		memcpy(ret.data() + 4, tl_compressBuffer, encodeSize);
		return ret;
	} else {
		typename Interface::BytesType ret; ret.resize(bufferSize + 4);
		auto encodeSize = compressData(src, size, ret.data() + 4, bufferSize, c);
		if (encodeSize == 0 || (conditional && encodeSize + 4 > size)) { return typename Interface::BytesType(); }
		writeCompressionMark(ret.data(), size, c);
		ret.resize(encodeSize + 4);
		ret.shrink_to_fit();
		return ret;
	}
	return typename Interface::BytesType();
}

template <>
auto compress<memory::PoolInterface>(const uint8_t *src, size_t size, EncodeFormat::Compression c, bool conditional) -> memory::PoolInterface::BytesType {
	return doCompress<memory::PoolInterface>(src, size, c, conditional);
}

template <>
auto compress<memory::StandartInterface>(const uint8_t *src, size_t size, EncodeFormat::Compression c, bool conditional) -> memory::StandartInterface::BytesType {
	return doCompress<memory::StandartInterface>(src, size, c, conditional);
}

using decompress_ptr = const uint8_t *;

static bool doDecompressLZ4Frame(const uint8_t *src, size_t srcSize, uint8_t *dest, size_t destSize) {
	return LZ4_decompress_safe((const char *)src, (char *)dest, srcSize, destSize) > 0;
}

template <typename Interface>
static inline auto doDecompressLZ4(BytesView data, bool sh) -> ValueTemplate<Interface> {
	size_t size = sh ? data.readUnsigned16() : data.readUnsigned32();

	ValueTemplate<Interface> ret;
	if (size <= sizeof(tl_compressBuffer)) {
		if (doDecompressLZ4Frame(data.data(), data.size(), tl_compressBuffer, size)) {
			ret = data::read<BytesView, Interface>(BytesView(tl_compressBuffer, size));
		}
	} else {
		typename Interface::BytesType res; res.resize(size);
		if (doDecompressLZ4Frame(data.data(), data.size(), res.data(), size)) {
			ret = data::read<typename Interface::BytesType, Interface>(res);
		}
	}
	return ret;
}

template <>
auto decompressLZ4(const uint8_t *srcPtr, size_t srcSize, bool sh) -> ValueTemplate<memory::PoolInterface> {
	return doDecompressLZ4<memory::PoolInterface>(BytesView(srcPtr, srcSize), sh);
}

template <>
auto decompressLZ4(const uint8_t *srcPtr, size_t srcSize, bool sh) -> ValueTemplate<memory::StandartInterface> {
	return doDecompressLZ4<memory::StandartInterface>(BytesView(srcPtr, srcSize), sh);
}

template <>
auto decompressBrotli(const uint8_t *data, size_t size) -> ValueTemplate<memory::PoolInterface> {
	return ValueTemplate<memory::PoolInterface>();
}

template <>
auto decompressBrotli(const uint8_t *data, size_t size) -> ValueTemplate<memory::StandartInterface> {
	return ValueTemplate<memory::StandartInterface>();
}

template <typename Interface>
static inline auto doDecompress(const uint8_t *d, size_t size) -> typename Interface::BytesType {
	BytesView data(d, size);
	auto ff = detectDataFormat(data.data(), data.size());
	switch (ff) {
	case DataFormat::LZ4_Short: {
		data += 4;
		size_t size = data.readUnsigned16();
		typename Interface::BytesType res; res.resize(size);
		if (doDecompressLZ4Frame(data.data(), data.size(), res.data(), size)) {
			return res;
		}
		break;
	}
	case DataFormat::LZ4_Word: {
		data += 4;
		size_t size = data.readUnsigned32();
		typename Interface::BytesType res; res.resize(size);
		if (doDecompressLZ4Frame(data.data(), data.size(), res.data(), size)) {
			return res;
		}
		break;
	}
	case DataFormat::Brotli: break;
	default: break;
	}
	return typename Interface::BytesType();
}

template <>
auto decompress<memory::PoolInterface>(const uint8_t *d, size_t size) -> typename memory::PoolInterface::BytesType {
	return doDecompress<memory::PoolInterface>(d, size);
}

template <>
auto decompress<memory::StandartInterface>(const uint8_t *d, size_t size) -> typename memory::StandartInterface::BytesType {
	return doDecompress<memory::StandartInterface>(d, size);
}

NS_SP_EXT_END(data)
