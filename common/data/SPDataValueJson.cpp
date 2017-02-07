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
#include "SPData.h"
#include "SPFilesystem.h"
#include "SPString.h"
#include "SPCharReader.h"
#include "SPUnicode.h"

NS_SP_EXT_BEGIN(data)

struct JsonDecoder {
	enum BackType {
		BackIsArray,
		BackIsDict,
		BackIsEmpty
	};

	JsonDecoder(CharReaderBase &r) : backType(BackIsEmpty), r(r), back(nullptr) {
		stack.reserve(10);
	}

	inline void parseBufferString(String &ref);
	inline void parseJsonNumber(Value &ref) SPINLINE;

	inline void parseValue(Value &current);
	void parseJson(Value &val);

	inline void push(BackType, Value *) SPINLINE;
	inline void pop() SPINLINE;

	BackType backType;
	CharReaderBase r;
	String buf;
	data::Value *back;
	Vector<Value *> stack;
};

template <char ...Args>
using Chars = CharReaderBase::Chars<Args...>;

using StringSepChars = chars::Chars<char, '\\', '"'>;
using NextTokenChars = chars::Compose<char,
		chars::Chars<char, '"', 't', 'f', 'n', '+', '-', '[', '{', ']', '}'>,
		chars::Range<char, '0', '9'>
>;

using ArraySepChars = chars::Chars<char, ' ', '\n', '\r', '\t', ','>;
using DictSepChars = chars::Chars<char, '"', '}'>;
using DictKeyChars = chars::Chars<char, ':', ' ', '\n', '\r', '\t'>;

inline void JsonDecoder::parseBufferString(String &ref) {
#define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
		static const char escape[256] = {
			Z16, Z16, 0, 0,'\"', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,'/',
			Z16, Z16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,'\\', 0, 0, 0,
			0, 0,'\b', 0, 0, 0,'\f', 0, 0, 0, 0, 0, 0, 0,'\n', 0,
			0, 0,'\r', 0,'\t', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16
		};
#undef Z16
	if (r.is('"')) { r ++; }
	auto s = r.readUntil<StringSepChars>();
	ref.assign(s.data(), s.size());
	while (!r.empty() && !r.is('"')) {
		if (r.is('\\')) {
			++ r;
			if (r.is('u')) {
				++ r;
				unicode::utf8Encode(ref, char16_t(base16::hexToChar(r[0], r[1]) << 8 | base16::hexToChar(r[2], r[3]) ));
				r += 4;
			} else {
				ref.push_back(escape[(uint8_t)r[0]]);
				++ r;
			}
		}
		auto s = r.readUntil<StringSepChars>();
		ref.append(s.data(), s.size());
	}
	if (r.is('"')) { ++ r; }
}

inline void JsonDecoder::parseJsonNumber(Value &result) {
	char *end = nullptr;
	double d = strtod(r.data(), &end);
	size_t size = end - r.data();

	r += size;

	if (size == 0) {
		return; // invalid number litheral
	}

	if (floor(d) == d) {
		result._type = Value::Type::INTEGER;
		result.intVal = (int64_t)round(d);
	} else {
		result._type = Value::Type::DOUBLE;
		result.doubleVal = d;
	}
}

inline void JsonDecoder::parseValue(Value &current) {
	switch(r[0]) {
	case '"':
		current._type = Value::Type::CHARSTRING;
		parseBufferString(buf);
		current.strVal = new String(std::move(buf));
		break;
	case 't':
		current._type = Value::Type::BOOLEAN;
		current.boolVal = true;
		r += 4;
		break;
	case 'f':
		current._type = Value::Type::BOOLEAN;
		current.boolVal = false;
		r += 5;
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '+':
	case '-':
		parseJsonNumber(current);
		break;
	case '[':
		current._type = Value::Type::ARRAY;
		current.arrayVal = new Array();
		current.arrayVal->reserve(10);
		push(BackIsArray, &current);
		break;
	case '{':
		current._type = Value::Type::DICTIONARY;
		current.dictVal = new Dictionary();
		push(BackIsDict, &current);
		break;
	case 'n':
        if (r == "nan") {
            current._type = Value::Type::DOUBLE;
            current.doubleVal = nan();
            r += 3;
        } else {
            r += 4;
        }
		break;
	default:
		r.skipUntil<NextTokenChars>();
		break;
	}
}

void JsonDecoder::parseJson(Value &val) {
	do {
		switch (backType) {
		case BackIsArray:
			r.skipChars<ArraySepChars>();
			if (!r.is(']')) {
				back->arrayVal->emplace_back(Value::Type::EMPTY);
				parseValue(back->arrayVal->back());
			} else {
				back->arrayVal->shrink_to_fit();
				pop();
			}
			break;
		case BackIsDict:
			r.skipUntil<DictSepChars>();
			if (!r.is('}')) {
				parseBufferString(buf);
				r.skipChars<DictKeyChars>();
				parseValue(back->dictVal->emplace(std::move(buf), Value::Type::EMPTY).first->second);
			} else {
				pop();
			}
			break;
		case BackIsEmpty:
			parseValue(val);
			break;
		}
	} while (!r.empty() && !stack.empty());
}

inline void JsonDecoder::push(BackType t, Value *v) {
	++ r;
	back = v;
	stack.push_back(v);
	backType = t;
}

inline void JsonDecoder::pop() {
	r ++;
	stack.pop_back();
	if (stack.empty()) {
		back = nullptr;
		backType = BackIsEmpty;
	} else {
		back = stack.back();
		backType = (back->isArray())?BackIsArray:BackIsDict;
	}
}

struct RawJsonEncoder {
	inline RawJsonEncoder(OutputStream *stream) : stream(stream) { }

	inline void write(nullptr_t) { (*stream) << "null"; }
	inline void write(bool value) { (*stream) << ((value)?"true":"false"); }
	inline void write(int64_t value) { (*stream) << value; }
	inline void write(double value) { (*stream) << value; }

	inline void write(const String &str) {
		(*stream) << '"';
		for (auto i : str) {
			if (i == '\n') { (*stream) << "\\n"; }
			else if (i == '\r') { (*stream) << "\\r"; }
			else if (i == '\t') { (*stream) << "\\t"; }
			else if (i == '\f') { (*stream) << "\\f"; }
			else if (i == '\b') { (*stream) << "\\b"; }
			else if (i == '\\') { (*stream) << "\\\\"; }
			else if (i == '\"') { (*stream) << "\\\""; }
			else if (i == ' ') { (*stream) << ' '; }
			else if (i >= 0 && i <= 0x20) {
				(*stream) << "\\u" << std::setfill('0') << std::setw(4)
					<< std::hex << (int)i << std::dec << std::setw(1) << std::setfill(' ');
			} else {
				(*stream) << i;
			}
		}
		(*stream) << '"';
	}

	inline void write(const Bytes &data) { (*stream) << '"' << "BASE64:" << base64::encode(data) << '"'; }
	inline void onBeginArray(const data::Array &arr) { (*stream) << '['; }
	inline void onEndArray(const data::Array &arr) { (*stream) << ']'; }
	inline void onBeginDict(const data::Dictionary &dict) { (*stream) << '{'; }
	inline void onEndDict(const data::Dictionary &dict) { (*stream) << '}'; }
	inline void onKey(const String &str) { write(str); (*stream) << ':'; }
	inline void onNextValue() { (*stream) << ','; }

	OutputStream *stream;
};

struct PrettyJsonEncoder {
	PrettyJsonEncoder(OutputStream *stream) : stream(stream) { }

	void write(nullptr_t) {
		(*stream) << "null";
		offsetted = false;
	}

	void write(bool value) {
		(*stream) << ((value)?"true":"false");
		offsetted = false;
	}

	void write(int64_t value) {
		(*stream) << value;
		offsetted = false;
	}

	void write(double value) {
		(*stream) << value;
		offsetted = false;
	}

	void write(const String &str) {
		(*stream) << '"';
		for (auto i : str) {
			if (i == '\n') {
				(*stream) << "\\n";
			} else if (i == '\r') {
				(*stream) << "\\r";
			} else if (i == '\t') {
				(*stream) << "\\t";
			} else if (i == '\f') {
				(*stream) << "\\f";
			} else if (i == '\b') {
				(*stream) << "\\b";
			} else if (i == '\\') {
				(*stream) << "\\\\";
			} else if (i == '\"') {
				(*stream) << "\\\"";
			} else if (i == ' ') {
				(*stream) << ' ';
			} else if (i >= 0 && i <= 0x20) {
				(*stream) << "\\u";
				(*stream).fill('0');
				(*stream).width(4);
				(*stream) << std::hex << (int)i << std::dec;
				(*stream).width(1);
				(*stream).fill(' ');
			} else {
				(*stream) << i;
			}
		}
		(*stream) << '"';
		offsetted = false;
	}

	void write(const Bytes &data) {
		(*stream) << '"';
		(*stream) << "BASE64:" << base64::encode(data);
		(*stream) << '"';
		offsetted = false;
	}

	bool isObjectArray(const data::Array &arr) {
		for (auto &it : arr) {
			if (!it.isDictionary()) {
				return false;
			}
		}
		return true;
	}

	void onBeginArray(const data::Array &arr) {
		(*stream) << '[';
		if (!isObjectArray(arr)) {
			++ depth;
			bstack.push_back(false);
			offsetted = false;
		} else {
			bstack.push_back(true);
		}
	}

	void onEndArray(const data::Array &arr) {
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

	void onBeginDict(const data::Dictionary &dict) {
		(*stream) << '{';
		++ depth;
	}

	void onEndDict(const data::Dictionary &dict) {
		-- depth;
		(*stream) << '\n';
		for (size_t i = 0; i < depth; i++) {
			(*stream) << '\t';
		}
		(*stream) << '}';
		popComplex = true;
	}

	void onKey(const String &str) {
		(*stream) << '\n';
		for (size_t i = 0; i < depth; i++) {
			(*stream) << '\t';
		}
		write(str);
		offsetted = true;
		(*stream) << ':';
		(*stream) << ' ';
	}

	void onNextValue() {
		(*stream) << ',';
	}

	void onValue(const data::Value &val) {
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
	Vector<bool> bstack;
};

Value readJson(const CharReaderBase &n) {
	auto r = n;
	if (r.empty() || r == "null") {
		return Value();
	}

	r.skipChars<CharReaderBase::Chars<' ', '\n', '\r', '\t'>>();
	JsonDecoder dec(r);
	Value ret;
	dec.parseJson(ret);
	return ret;
}

String writeJson(const data::Value &val, bool pretty) {
	StringStream stream;
	if (pretty) {
		PrettyJsonEncoder encoder(&stream);
		val.encode(encoder);
	} else {
		RawJsonEncoder encoder(&stream);
		val.encode(encoder);
	}
	return stream.str();
}

bool writeJson(std::ostream &stream, const data::Value &val, bool pretty) {
	if (pretty) {
		PrettyJsonEncoder encoder(&stream);
		val.encode(encoder);
	} else {
		RawJsonEncoder encoder(&stream);
		val.encode(encoder);
	}
	return true;
}

bool saveJson(const data::Value &val, const String &path, bool pretty) {
	OutputFileStream stream(path);
	if (stream.is_open()) {
		if (pretty) {
			PrettyJsonEncoder encoder(&stream);
			val.encode(encoder);
		} else {
			RawJsonEncoder encoder(&stream);
			val.encode(encoder);
		}
		stream.flush();
		stream.close();
		return true;
	}
	return false;
}

NS_SP_EXT_END(data)
