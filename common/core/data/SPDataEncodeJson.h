/**
Copyright (c) 2017-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_DATA_SPDATAENCODEJSON_H_
#define COMMON_DATA_SPDATAENCODEJSON_H_

#include "SPDataValue.h"

NS_SP_EXT_BEGIN(data)

namespace json {

template <typename StringType>
inline void encodeString(OutputStream &stream, const StringType &str) {
	stream << '"';
	for (auto &i : str) {
		switch (i) {
		case '\n' : stream << "\\n"; break;
		case '\t' : stream << "\\t"; break;
		case '\f' : stream << "\\f"; break;
		case '\b' : stream << "\\b"; break;
		case '\\' : stream << "\\\\"; break;
		case '\"' : stream << "\\\""; break;
		case ' ' : stream << " "; break;
		default:
			if (i >= 0 && i <= 0x20) {
				stream << "\\u" << std::setfill('0') << std::setw(4)
					<< std::hex << (int32_t)i << std::dec << std::setw(1) << std::setfill(' ');
			} else {
				stream << i;
			}
			break;
		}
	}
	stream << '"';
}

template <typename Interface>
struct RawEncoder : public Interface::AllocBaseType {
	using InterfaceType = Interface;
	using ValueType = ValueTemplate<Interface>;

	inline RawEncoder(OutputStream *stream) : stream(stream) { }

	inline void write(nullptr_t) { (*stream) << "null"; }
	inline void write(bool value) { (*stream) << ((value)?"true":"false"); }
	inline void write(int64_t value) { (*stream) << value; }
	inline void write(double value) { (*stream) << value; }

	inline void write(const typename ValueType::StringType &str) {
		encodeString(*stream, str);
	}

	inline void write(const typename ValueType::BytesType &data) { (*stream) << '"' << "BASE64:" << base64::encode(data) << '"'; }
	inline void onBeginArray(const typename ValueType::ArrayType &arr) { (*stream) << '['; }
	inline void onEndArray(const typename ValueType::ArrayType &arr) { (*stream) << ']'; }
	inline void onBeginDict(const typename ValueType::DictionaryType &dict) { (*stream) << '{'; }
	inline void onEndDict(const typename ValueType::DictionaryType &dict) { (*stream) << '}'; }
	inline void onKey(const typename ValueType::StringType &str) { write(str); (*stream) << ':'; }
	inline void onNextValue() { (*stream) << ','; }

	OutputStream *stream;
};

template <typename Interface>
struct PrettyEncoder : public Interface::AllocBaseType {
	using InterfaceType = Interface;
	using ValueType = ValueTemplate<Interface>;

	PrettyEncoder(OutputStream *stream) : stream(stream) { }

	void write(nullptr_t) { (*stream) << "null"; offsetted = false; }
	void write(bool value) { (*stream) << ((value)?"true":"false"); offsetted = false; }
	void write(int64_t value) { (*stream) << value; offsetted = false; }
	void write(double value) { (*stream) << value; offsetted = false; }

	void write(const typename ValueType::StringType &str) {
		encodeString(*stream, str);
		offsetted = false;
	}

	void write(const typename ValueType::BytesType &data) {
		(*stream) << '"'  << "BASE64:" << base64::encode(data) << '"';
		offsetted = false;
	}

	bool isObjectArray(const typename ValueType::ArrayType &arr) {
		for (auto &it : arr) {
			if (!it.isDictionary()) {
				return false;
			}
		}
		return true;
	}

	void onBeginArray(const typename ValueType::ArrayType &arr) {
		(*stream) << '[';
		if (!isObjectArray(arr)) {
			++ depth;
			bstack.push_back(false);
			offsetted = false;
		} else {
			bstack.push_back(true);
		}
	}

	void onEndArray(const typename ValueType::ArrayType &arr) {
		if (!bstack.empty()) {
			if (!bstack.back()) {
				-- depth;
				(*stream) << '\n';
				for (size_t i = 0; i < depth; i++) {
					(*stream) << '\t';
				}
			}
			bstack.pop_back();
		} else {
			-- depth;
			(*stream) << '\n';
			for (size_t i = 0; i < depth; i++) {
				(*stream) << '\t';
			}
		}
		(*stream) << ']';
		popComplex = true;
	}

	void onBeginDict(const typename ValueType::DictionaryType &dict) {
		(*stream) << '{';
		++ depth;
	}

	void onEndDict(const typename ValueType::DictionaryType &dict) {
		-- depth;
		(*stream) << '\n';
		for (size_t i = 0; i < depth; i++) {
			(*stream) << '\t';
		}
		(*stream) << '}';
		popComplex = true;
	}

	void onKey(const typename ValueType::StringType &str) {
		(*stream) << '\n';
		for (size_t i = 0; i < depth; i++) {
			(*stream) << '\t';
		}
		write(str);
		offsetted = true;
		(*stream) << ':' << ' ';
	}

	void onNextValue() {
		(*stream) << ',';
	}

	void onValue(const ValueType &val) {
		if (depth > 0) {
			if (popComplex && (val.isArray() || val.isDictionary())) {
				(*stream) << ' ';
			} else {
				if (!offsetted) {
					(*stream) << '\n';
					for (size_t i = 0; i < depth; i++) {
						(*stream) << '\t';
					}
					offsetted = true;
				}
			}
			popComplex = false;
		}
	}

	size_t depth = 0;
	bool popComplex = false;
	bool offsetted = false;
	OutputStream *stream;
	typename Interface::template ArrayType<bool> bstack;
};

template <typename Interface>
inline void write(std::ostream &stream, const ValueTemplate<Interface> &val, bool pretty) {
	if (pretty) {
		PrettyEncoder<Interface> encoder(&stream);
		val.encode(encoder);
	} else {
		RawEncoder<Interface> encoder(&stream);
		val.encode(encoder);
	}
}

template <typename Interface>
inline auto write(const ValueTemplate<Interface> &val, bool pretty = false) -> typename Interface::StringType {
	typename Interface::StringStreamType stream;
	write<Interface>(stream, val, pretty);
	return stream.str();
}

template <typename Interface>
bool save(const ValueTemplate<Interface> &val, const String &path, bool pretty) {
	OutputFileStream stream(path);
	if (stream.is_open()) {
		write(stream, val, pretty);
		stream.flush();
		stream.close();
		return true;
	}
	return false;
}

template <typename Interface>
auto toString(const ValueTemplate<Interface> &data, bool pretty) -> typename Interface::StringType {
	return write(data, pretty);
}

}

NS_SP_EXT_END(data)

#endif /* COMMON_DATA_SPDATAENCODEJSON_H_ */
