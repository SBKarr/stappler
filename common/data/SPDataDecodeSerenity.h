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

#ifndef COMMON_DATA_SPDATADECODESERENITY_H_
#define COMMON_DATA_SPDATADECODESERENITY_H_

#include "SPDataValue.h"
#include "SPDataDecodeJson.h"

NS_SP_EXT_BEGIN(data)

namespace serenity {

template <typename Interface>
struct Decoder : public Interface::AllocBaseType {
	using InterfaceType = Interface;
	using ValueType = ValueTemplate<Interface>;
	using StringType = typename InterfaceType::StringType;
	using BytesType = typename InterfaceType::BytesType;

	enum BackType {
		BackIsPlain,
		BackIsArray,
		BackIsDict,
		BackIsEmpty
	};

	enum TokenType {
		Generic,
		Binary,
		Text,
	};

	Decoder(StringView &r) : backType(BackIsEmpty), r(r), back(nullptr) {
		stack.reserve(10);
	}

	inline void parseBufferString(StringType &ref);
	inline void parseNumber(StringView &token, ValueType &ref) SPINLINE;

	inline void parseToken(StringType &current);
	inline void parseToken(ValueType &current, TokenType type);
	inline void parseValue(ValueType &current);
	void parse(ValueType &val);

	inline void push(BackType t, ValueType *v) {
		if (t != BackIsPlain) {
			++ r;
		}
		back = v;
		stack.push_back(pair(t, v));
		backType = t;
	}

	inline void pop() {
		if (backType != BackIsPlain || r.is(';')) {
			r ++;
		}
		stack.pop_back();
		if (stack.empty()) {
			back = nullptr;
			backType = BackIsEmpty;
		} else {
			back = stack.back().second;
			backType = stack.back().first;
		}
	}

	bool stop = false;
	BackType backType;
	StringView r;
	ValueType *back;
	typename InterfaceType::template ArrayType<Pair<BackType, ValueType *>> stack;
};

template <typename Interface>
inline void Decoder<Interface>::parseNumber(StringView &token, ValueType &result) {
	bool isFloat = false;
	if (token == "+inf") {
		result._type = ValueType::Type::DOUBLE;
		result.doubleVal = NumericLimits<double>::infinity();
	} else if (token == "-inf") {
		result._type = ValueType::Type::DOUBLE;
		result.doubleVal = - NumericLimits<double>::infinity();
	} else {
		auto data = token.data();
		auto size = token.size();
		auto value = json::decodeNumber(token, isFloat);
		if (value.empty()) {
			return;
		} else if (value.size() != size) {
			result._type = ValueType::Type::CHARSTRING;
			result.strVal = new StringType(data, size);
		} else {
			if (isFloat) {
				result._type = ValueType::Type::DOUBLE;
				result.doubleVal = value.readDouble();
			} else {
				result._type = ValueType::Type::INTEGER;
				result.intVal = value.readInteger();
			}
		}
	}
}

template <typename Interface>
inline void Decoder<Interface>::parseToken(StringType &current) {
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	StringView token = r.readUntil<StringView::Chars<':', ',', ';', ')', '('>, StringView::CharGroup<CharGroupId::WhiteSpace>>();
	string::urldecode(current, token);
}

template <typename Interface>
inline void Decoder<Interface>::parseToken(ValueType &current, TokenType type) {
	StringView token = r.readUntil<StringView::Chars<':', ',', ';', ')'>, StringView::CharGroup<CharGroupId::WhiteSpace>>();

	switch (type) {
	case Generic:
		switch (token[0]) {
		case '0': case '1': case '2': case '3': case '4': case '5': case '6':
		case '7': case '8': case '9': case '+': case '-':
			parseNumber(token, current);
			return;
			break;
		case 't':
			if (token == "true" || token == "t") {
				current._type = ValueType::Type::BOOLEAN;
				current.boolVal = true;
				return;
			}
			break;
		case 'f':
			if (token == "false" || token == "f") {
				current._type = ValueType::Type::BOOLEAN;
				current.boolVal = false;
				return;
			}
			break;
		case 'n':
			if (token == "nan") {
				current._type = ValueType::Type::DOUBLE;
				current.doubleVal = nan();
				return;
			} else if (token == "null" || token == "n") {
				current._type = ValueType::Type::EMPTY;
				return;
			}
			break;
		case 'i':
			if (token == "inf") {
				current._type = ValueType::Type::DOUBLE;
				current.doubleVal = NumericLimits<double>::infinity();
				return;
			}
			break;
		}
		current._type = ValueType::Type::CHARSTRING;
		current.strVal = new StringType();
		string::urldecode(*current.strVal, token);
		break;
	case Text:
		current._type = ValueType::Type::CHARSTRING;
		current.strVal = new StringType();
		string::urldecode(*current.strVal, token);
		break;
	case Binary:
		current._type = ValueType::Type::BYTESTRING;
		current.bytesVal = new BytesType();
		string::urldecode(*current.bytesVal, token);
		break;
	}

	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
}

template <typename Interface>
inline void Decoder<Interface>::parseValue(ValueType &current) {
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	switch (r[0]) {
	case '(':
		current._type = ValueType::Type::DICTIONARY;
		current.dictVal = new typename ValueType::DictionaryType();
		push(BackIsDict, &current);
		return;
		break;
	case '~':
		++ r;
		if (r.is('(')) {
			current._type = ValueType::Type::ARRAY;
			current.arrayVal = new typename ValueType::ArrayType();
			push(BackIsArray, &current);
			return;
		} else {
			parseToken(current, Binary);
			return;
		}
		break;
	default:
		parseToken(current, Generic);
		break;
	}
}

using TokenSpecials = StringView::Chars<'/', '?', '@', '-', '.', '_', '!', '$', '\'', '*', '+'>;

template <typename Interface>
void Decoder<Interface>::parse(ValueType &val) {
	StringType key;
	do {
		switch (backType) {
		case BackIsPlain:
			r.skipUntil<TokenSpecials, StringView::Chars<'(', '~', ';', ')', ','>, StringView::CharGroup<CharGroupId::Alphanumeric>>();
			if (r.is(';') || r.is(')')) {
				back->arrayVal->shrink_to_fit();
				pop();
			} else if (r.is(',') || r.is('~') || r.is('(')) {
				if (r.is(',')) {
					++ r;
				}
				back->arrayVal->emplace_back(ValueType::Type::EMPTY);
				parseValue(back->arrayVal->back());
			} else {
				back->arrayVal->shrink_to_fit();
				pop();
			}
			break;
		case BackIsArray:
			r.skipUntil<TokenSpecials, StringView::Chars<')', '(', '~'>, StringView::CharGroup<CharGroupId::Alphanumeric>>();
			if (!r.is(')')) {
				back->arrayVal->emplace_back(ValueType::Type::EMPTY);
				parseValue(back->arrayVal->back());
			} else {
				back->arrayVal->shrink_to_fit();
				pop();
			}
			break;
		case BackIsDict:
			r.skipUntil<TokenSpecials, StringView::Chars<')', '(', '~', ','>, StringView::CharGroup<CharGroupId::Alphanumeric>>();
			if (!r.is(')')) {
				if (r.is(',') || r.is('~') || r.is('(')) {
					typename ValueType::ArrayType arr;
					auto dictIt = back->dictVal->find(key);
					if (dictIt != back->dictVal->end()) {
						arr.emplace_back(move(dictIt->second));
						dictIt->second = ValueType(move(arr));
						push(BackIsPlain, &dictIt->second);
					} else {
						push(BackIsPlain, &back->dictVal->emplace(key, ValueType::Type::ARRAY).first->second);
					}
				} else {
					key.clear();
					parseToken(key);
					r.skipUntil<StringView::Chars<':', ';', '(', ')'>>();
					if (r.is(':')) {
						++ r;
						parseValue(back->dictVal->emplace(key, ValueType::Type::EMPTY).first->second);
					} else if (r.is('(')) {
						parseValue(back->dictVal->emplace(key, ValueType::Type::EMPTY).first->second);
					} else {
						back->dictVal->emplace(key, ValueType(true));
					}
				}
			} else {
				pop();
			}
			break;
		case BackIsEmpty:
			parseValue(val);
			break;
		}
	} while (!r.empty() && !stack.empty() && !stop);
}

template <typename Interface>
auto read(StringView &n) -> ValueTemplate<Interface> {
	auto r = n;
	if (r.empty() || r == "null") {
		return ValueTemplate<Interface>();
	}

	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	Decoder<Interface> dec(r);
	ValueTemplate<Interface> ret;
	dec.parse(ret);
	n = dec.r;
	return ret;
}

template <typename Interface>
auto read(const StringView &r) -> ValueTemplate<Interface> {
	StringView tmp(r);
	return read<Interface>(tmp);
}

}

NS_SP_EXT_END(data)

#endif /* COMMON_DATA_SPDATADECODESERENITY_H_ */
