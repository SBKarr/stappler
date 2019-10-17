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

#ifndef COMMON_DATA_SPDATAENCODESERENITY_H_
#define COMMON_DATA_SPDATAENCODESERENITY_H_

#include "SPDataValue.h"

NS_SP_EXT_BEGIN(data)

namespace serenity {

bool shouldEncodePercent(char c);

template <typename StringType>
inline void encodeString(OutputStream &stream, const StringType &str) {
	for (auto &i : str) {
		if (shouldEncodePercent(i)) {
			stream << '%' << base16::charToHex(i);
		} else {
			stream << i;
		}
	}
}

template <typename Interface>
struct RawEncoder : public Interface::AllocBaseType {
	using InterfaceType = Interface;
	using ValueType = ValueTemplate<Interface>;

	enum Type {
		Dict,
		Array,
		Plain,
	};

	inline RawEncoder(OutputStream *stream) : stream(stream) { }

	inline void write(nullptr_t) { (*stream) << "null"; }
	inline void write(bool value) { (*stream) << ((value)?"true":"false"); }
	inline void write(int64_t value) { (*stream) << value; }
	inline void write(double value) { (*stream) << value; }

	inline void write(const typename ValueType::StringType &str) {
		encodeString(*stream, str);
	}

	inline void write(const typename ValueType::BytesType &data) {
		(*stream) << '~';
		encodeString(*stream, data);
	}

	inline void onBeginArray(const typename ValueType::ArrayType &arr) {
		if (type == Type::Dict) {
			type = Type::Plain;
		} else {
			type = Type::Array;
			(*stream) << "~(";
		}
		preventKey = false;
	}

	inline void onEndArray(const typename ValueType::ArrayType &arr) {
		if (type != Type::Plain) {
			(*stream) << ')';
			preventKey = true;
		} else {
			preventKey = false;
		}
	}

	inline void onBeginDict(const typename ValueType::DictionaryType &dict) {
		(*stream) << '(';
		type = Type::Dict;
		preventKey = false;
	}
	inline void onEndDict(const typename ValueType::DictionaryType &dict) {
		(*stream) << ')';
		preventKey = true;
	}
	inline void onKey(const typename ValueType::StringType &str) { write(str); }
	inline void onNextValue() {
		if (!preventKey) {
			(*stream) << ((type == Type::Dict)?';':',');
		} else {
			preventKey = false;
		}
	}

	inline void onArrayValue(const ValueType &val) {
		auto tmpType = type;
		val.encode(*this);
		type = tmpType;
	}

	inline void onKeyValuePair(const typename ValueType::StringType &key, const ValueType &val) {
		auto tmpType = type;
		onKey(key);
		if (!val.isBool() || !val.asBool()) {
			if (!val.isDictionary()) {
				(*stream) << ':';
			}
			if (val.isArray() && val.size() < 2) {
				type = Type::Plain; // prevent plain array
			}
			val.encode(*this);
		} else {
			(*stream) << ":true";
		}
		type = tmpType;
	}

	bool preventKey = false;
	OutputStream *stream;
	Type type = Type::Dict;
};

template <typename Interface>
struct PrettyEncoder : public Interface::AllocBaseType {
	using InterfaceType = Interface;
	using ValueType = ValueTemplate<Interface>;

	enum Type {
		Dict,
		Array,
		Plain,
	};

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
		(*stream) << '~';
		encodeString(*stream, data);
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
		if (type == Type::Dict) {
			type = Type::Plain;
		} else {
			type = Type::Array;
			(*stream) << "~(";
		}

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
		if (type != Type::Plain) {
			(*stream) << ')';
		}
		popComplex = true;
	}

	void onBeginDict(const typename ValueType::DictionaryType &dict) {
		(*stream) << '(';
		type = Type::Dict;
		++ depth;
	}

	void onEndDict(const typename ValueType::DictionaryType &dict) {
		-- depth;
		(*stream) << '\n';
		for (size_t i = 0; i < depth; i++) {
			(*stream) << '\t';
		}
		(*stream) << ')';
		popComplex = true;
	}

	void onKey(const typename ValueType::StringType &str) {
		(*stream) << '\n';
		for (size_t i = 0; i < depth; i++) {
			(*stream) << '\t';
		}
		write(str);
		offsetted = true;
	}

	void onNextValue() {
		(*stream) << ((type == Type::Dict)?';':',');
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

	inline void onArrayValue(const ValueType &val) {
		auto tmpType = type;
		val.encode(*this);
		type = tmpType;
	}

	inline void onKeyValuePair(const typename ValueType::StringType &key, const ValueType &val) {
		auto tmpType = type;
		onKey(key);
		if (!val.isBool() || !val.asBool()) {
			(*stream) << ' ';
			if (!val.isDictionary()) {
				(*stream) << ": ";
			}
			if (val.isArray() && val.size() < 2) {
				type = Type::Plain; // prevent plain array
			}
			val.encode(*this);
		} else {
			(*stream) << ": true";
		}
		type = tmpType;
	}

	size_t depth = 0;
	bool popComplex = false;
	bool offsetted = false;
	OutputStream *stream;
	typename Interface::template ArrayType<bool> bstack;
	Type type = Type::Dict;
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
	OutputFileStream stream(path.data());
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

#endif /* COMMON_DATA_SPDATAENCODESERENITY_H_ */
