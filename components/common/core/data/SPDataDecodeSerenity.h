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
		BackIsPlainList,
		BackIsPlainStop,
		BackIsArray,
		BackIsDict,
		BackIsGeneric,
	};

	enum TokenType {
		Generic,
	};

	Decoder(StringView &r) : backType(BackIsGeneric), r(r), back(nullptr) {
		stack.reserve(10);
	}

	inline void parseBufferString(StringType &ref);
	inline void parseNumber(StringView &token, ValueType &ref) SPINLINE;

	inline void parsePlainToken(ValueType &current, StringView v);

	inline void transformToDict(ValueType &current);

	void parse(ValueType &val);

	inline void push(BackType t, ValueType *v) {
		if (t != BackIsPlain && t != BackIsGeneric) {
			++ r;
		}
		back = v;
		stack.push_back(pair(t, v));
		backType = t;
	}

	inline void pop() {
		if (backType != BackIsPlain && backType != BackIsPlainList && backType != BackIsPlainStop) {
			r ++;
		}
		stack.pop_back();
		if (stack.empty()) {
			back = nullptr;
			backType = BackIsGeneric;
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
				value.readDouble().unwrap([&] (double v) {
					result._type = ValueType::Type::DOUBLE;
					result.doubleVal = v;
				});
			} else {
				value.readInteger().unwrap([&] (int64_t v) {
					result._type = ValueType::Type::INTEGER;
					result.intVal = v;
				});
			}
		}
	}
}

template <typename Interface>
inline void Decoder<Interface>::parsePlainToken(ValueType &current, StringView token) {
	switch (token[0]) {
	case '0': case '1': case '2': case '3': case '4': case '5': case '6':
	case '7': case '8': case '9': case '+': case '-':
		parseNumber(token, current);
		return;
		break;
	case 't':
		if (token == "true") {
			current._type = ValueType::Type::BOOLEAN;
			current.boolVal = true;
			return;
		}
		break;
	case 'f':
		if (token == "false") {
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
		} else if (token == "null") {
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
	case '~':
		current._type = ValueType::Type::BYTESTRING;
		current.bytesVal = new BytesType();
		string::urldecode(*current.bytesVal, token);
		return;
		break;
	}
	current._type = ValueType::Type::CHARSTRING;
	current.strVal = new StringType();
	string::urldecode(*current.strVal, token);
}

template <typename Interface>
inline void Decoder<Interface>::transformToDict(ValueType &current) {
	typename ValueType::DictionaryType dict;
	for (auto &it : *current.arrayVal) {
		auto str = it.asString();
		if (!str.empty()) {
			dict.emplace(move(str), ValueType(true));
		} else {
			log::text("DataSerenityDecoder", "Invalid token within SubArray");
		}
	}
	current = ValueType(move(dict));
}

using TokenSpecials = StringView::Chars<'/', '?', '@', '-', '.', '_', '!', '$', '\'', '*', '+', '%'>;

template <typename Interface>
void Decoder<Interface>::parse(ValueType &val) {
	backType = BackIsGeneric;
	back = &val;
	stack.push_back(pair(backType, back));

	StringType key;
	do {
		r.skipUntil<TokenSpecials, StringView::Chars<'(', '~', ';', ')', ','>, StringView::CharGroup<CharGroupId::Alphanumeric>>();
		switch (backType) {
		case BackIsPlain:
			if (r.is(')') || r.is(';')) {
				pop();
			} else if (r.is(',')) {
				typename ValueType::ArrayType arr;
				arr.emplace_back(move(back));
				*back = ValueType(move(arr));
				backType = stack.back().first = BackIsPlainList;
			} else if (r.is('(') || r.is("~(")) {
				backType = stack.back().first = BackIsPlainStop;
				push(BackIsGeneric, back);
			} else {
				r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
				StringView token = r.readUntil<StringView::Chars<'~', ':', ',', ';', '(', ')'>, StringView::CharGroup<CharGroupId::WhiteSpace>>();
				r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
				if (r.is(':')) {
					log::text("DataSerenityDecoder", "Colon sequence within plain list is invalid");
					stop = true;
					break;
				} else if (token.empty()) {
					// do nothing
				} else if (r.is('(') || r.is("~(")) {
					key.clear(); string::urldecode(key, token);
					back->_type = ValueType::Type::DICTIONARY;
					back->dictVal = new typename ValueType::DictionaryType();
					backType = stack.back().first = BackIsPlainList;
					push(BackIsGeneric, &back->dictVal->emplace(key, ValueType::Type::EMPTY).first->second);
				} else if (r.is(',')) {
					back->_type = ValueType::Type::ARRAY;
					back->arrayVal = new typename ValueType::ArrayType();
					back->arrayVal->emplace_back(ValueType::Type::EMPTY);
					backType = stack.back().first = BackIsPlainList;
					parsePlainToken(back->arrayVal->back(), token);
				} else if (r.empty() || r.is(')') || r.is(';')) {
					parsePlainToken(*back, token);
					pop();
				}
			}
			break;
		case BackIsPlainList:
			if (r.is(')') || r.is(';')) {
				pop();
				continue;
			} else if (r.is('(') || r.is("~(") || r.is(",~(") || r.is(",(")) {
				if (r.is(',')) {
					++ r;
				}
				if (back->_type == ValueType::Type::ARRAY) {
					back->arrayVal->emplace_back(ValueType::Type::EMPTY);
					push(BackIsGeneric, &back->arrayVal->back());
				} else {
					log::text("DataSerenityDecoder", "Generic value can not be used as key");
					stop = true;
					break;
				}
				break;
			} else if (r.is(',') || r.is<TokenSpecials>() || r.is<CharGroupId::Alphanumeric>()) {
				if (r.is(',')) {
					++ r;
					r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
				}

				r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
				StringView token = r.readUntil<StringView::Chars<'~', ':', ',', ';', '(', ')'>, StringView::CharGroup<CharGroupId::WhiteSpace>>();
				r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
				if (r.is(':')) {
					pop();
					if (backType == BackIsDict) {
						push(BackIsPlain, &back->dictVal->emplace(key, ValueType::Type::EMPTY).first->second);
					} else {
						log::text("DataSerenityDecoder", "Colon sequence within plain list is invalid");
						stop = true;
						break;
					}
				} else if (token.empty()) {
					// do nothing
				}
				if (back->_type == ValueType::Type::ARRAY) {
					if (r.is('(')) {
						key.clear(); string::urldecode(key, token);
						transformToDict(*back);
						push(BackIsGeneric, &back->dictVal->emplace(key, ValueType::Type::EMPTY).first->second);
					} else if (r.is("~(")) {
						back->arrayVal->emplace_back(ValueType::Type::EMPTY);
						push(BackIsGeneric, &back->arrayVal->back());
					} else {
						back->arrayVal->emplace_back(ValueType::Type::EMPTY);
						parsePlainToken(back->arrayVal->back(), token);
					}
				} else {
					key.clear(); string::urldecode(key, token);
					if (r.is('(') || r.is("~(")) {
						push(BackIsGeneric, &back->dictVal->emplace(key, ValueType::Type::EMPTY).first->second);
					} else {
						back->dictVal->emplace(key, ValueType(true));
					}
				}
			} else {
				log::text("DataSerenityDecoder", "Invalid token in plain list");
				stop = true;
				break;
			}
			break;
		case BackIsPlainStop:
			if (r.is(')') || r.is(';')) {
				pop();
				continue;
			} else if (r.is(',') || r.is("~(")) {
				typename ValueType::ArrayType arr;
				arr.emplace_back(move(*back));
				*back = ValueType(move(arr));
				backType = stack.back().first = BackIsPlainList;
			} else if (r.is<TokenSpecials>() || r.is<CharGroupId::Alphanumeric>()) {
				pop();
				continue;
			} else {
				log::text("DataSerenityDecoder", "Invalid token in plain stop");
				stop = true;
			}
			break;
		case BackIsArray:
			if (!r.is(')')) {
				if (r.is(';') || r.is(',')) {
					++ r;
					r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
				}
				if (r.is('(')) {
					back->arrayVal->emplace_back(ValueType::Type::EMPTY);
					backType = stack.back().first = BackIsArray;
					push(BackIsGeneric, &back->arrayVal->back());
				} else {
					r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
					StringView token = r.readUntil<StringView::Chars<'~', ':', ',', ';', '(', ')'>, StringView::CharGroup<CharGroupId::WhiteSpace>>();
					r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
					if (token.empty()) {
						// do nothing
					} else if (r.is(':')) {
						key.clear(); string::urldecode(key, token);
						transformToDict(*back);
						backType = stack.back().first = BackIsDict;
						push(BackIsPlain, &back->dictVal->emplace(key, ValueType::Type::EMPTY).first->second);
					} else if (r.is('(') || r.is("~(")) {
						key.clear();
						string::urldecode(key, token);
						transformToDict(*back);
						backType = stack.back().first = BackIsDict;
						push(BackIsGeneric, &back->dictVal->emplace(key, ValueType::Type::EMPTY).first->second);
					} else {
						back->arrayVal->emplace_back(ValueType::Type::EMPTY);
						parsePlainToken(back->arrayVal->back(), token);
						// continue
					}
				}
			} else {
				back->arrayVal->shrink_to_fit();
				pop();
			}
			break;
		case BackIsDict:
			if (!r.is(')')) {
				if (r.is(';') || r.is(',')) {
					++ r;
				}

				r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
				if (r.is(')')) {
					pop();
					continue;
				}

				if (!r.is<TokenSpecials>() && !r.is<StringView::CharGroup<CharGroupId::Alphanumeric>>()) {
					stop = true;
					log::text("DataSerenityDecoder", "Invalid key");
					break;
				}

				StringView token = r.readUntil<StringView::Chars<'~', ':', ',', ';', '(', ')'>, StringView::CharGroup<CharGroupId::WhiteSpace>>();
				r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
				key.clear(); string::urldecode(key, token);
				if (r.is(':')) {
					push(BackIsPlain, &back->dictVal->emplace(key, ValueType::Type::EMPTY).first->second);
				} else if (r.is('(') || r.is("~(")) {
					push(BackIsGeneric, &back->dictVal->emplace(key, ValueType::Type::EMPTY).first->second);
				} else if (r.is(';') || r.is(',')) {
					back->dictVal->emplace(key, ValueType(true));
				} else {
					stop = true;
					log::text("DataSerenityDecoder", "Invalid token in value");
					break;
				}
			} else {
				pop();
			}
			break;
		case BackIsGeneric:
			if (r.is('(') || r.is("~(")) {
				if (r.is('(')) {
					++ r;
				} else if (r.is("~(")) {
					r += 2;
				}
				if (r.is('(')) {
					back->_type = ValueType::Type::ARRAY;
					back->arrayVal = new typename ValueType::ArrayType();
					back->arrayVal->emplace_back(ValueType::Type::EMPTY);
					backType = stack.back().first = BackIsArray;
					push(BackIsGeneric, &back->arrayVal->back());
				} else {
					r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
					StringView token = r.readUntil<StringView::Chars<'~', ':', ',', ';', '(', ')'>, StringView::CharGroup<CharGroupId::WhiteSpace>>();
					r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
					if (token.empty()) {
						pop();
					} else if (r.is(')') || r.empty()) {
						parsePlainToken(*back, token);
						pop();
					} else if (r.is(':')) {
						key.clear(); string::urldecode(key, token);
						back->_type = ValueType::Type::DICTIONARY;
						back->dictVal = new typename ValueType::DictionaryType();
						backType = stack.back().first = BackIsDict;
						push(BackIsPlain, &back->dictVal->emplace(key, ValueType::Type::EMPTY).first->second);
					} else if (r.is('(') || r.is("~(")) {
						key.clear(); string::urldecode(key, token);
						back->_type = ValueType::Type::DICTIONARY;
						back->dictVal = new typename ValueType::DictionaryType();
						backType = stack.back().first = BackIsDict;
						push(BackIsGeneric, &back->dictVal->emplace(key, ValueType::Type::EMPTY).first->second);
					} else if (r.is(',') || r.is(';')) {
						back->_type = ValueType::Type::ARRAY;
						back->arrayVal = new typename ValueType::ArrayType();
						back->arrayVal->emplace_back(ValueType::Type::EMPTY);
						backType = stack.back().first = BackIsArray;
						parsePlainToken(back->arrayVal->back(), token);
					}
				}
			} else {
				log::vtext("DataSerenityDecoder", "Invalid token in plain stop: '", r.sub(16), "'");
			}

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
