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
#include "SPDataJsonBuffer.h"

#include "SPString.h"

NS_SP_EXT_BEGIN(data)

template <char ...Args>
using Chars = CharReaderBase::Chars<Args...>;

using StringSepChars = chars::Chars<char, '\\', '"'>;
using NextTokenChars = chars::Compose<char,
		chars::Chars<char, '"', 't', 'f', 'n', '+', '-', '[', '{', ']', '}', ':', ','>,
		chars::Range<char, '0', '9'>
>;

using ArraySepChars = chars::Chars<char, ' ', '\n', '\r', '\t', ','>;
using DictSepChars = chars::Chars<char, '"', '}'>;
using DictKeyChars = chars::Chars<char, ':', ' ', '\n', '\r', '\t'>;
using NumberChars = chars::Compose<char,
		chars::Chars<char, '-', '+', '.', 'E', 'e'>,
		chars::Range<char, '0', '9'>
>;
using PlainChars = chars::CharGroup<char, chars::CharGroupId::Alphanumeric>;

inline void JsonBuffer::reset() {
	buf.clear();
}

inline data::Value &JsonBuffer::emplaceArray() {
	back->arrayVal->emplace_back(Value::Type::EMPTY);
	return back->arrayVal->back();
}

inline data::Value &JsonBuffer::emplaceDict() {
	return back->dictVal->emplace(std::move(key), Value::Type::EMPTY).first->second;
}

inline void JsonBuffer::writeString(data::Value &val, const Reader &r) {
	val._type = Value::Type::CHARSTRING;
	val.strVal = new String(r.data(), r.size());
}

inline void JsonBuffer::writeNumber(data::Value &val, double d) {
	if (floor(d) == d) {
		val._type = Value::Type::INTEGER;
		val.intVal = (int64_t)round(d);
	} else {
		val._type = Value::Type::DOUBLE;
		val.doubleVal = d;
	}
}

inline void JsonBuffer::writePlain(data::Value &val, const Reader &r) {
	if (r == "nan") {
		val._type = Value::Type::DOUBLE;
		val.doubleVal = nan();
	} else if (r == "inf") {
		val._type = Value::Type::DOUBLE;
		val.doubleVal = NumericLimits<double>::infinity();
	} else if (r == "null") {
		val._type = Value::Type::EMPTY;
	} else if (r == "true") {
		val._type = Value::Type::BOOLEAN;
		val.boolVal = true;
	} else if (r == "false") {
		val._type = Value::Type::BOOLEAN;
		val.boolVal = false;
	}
}

inline void JsonBuffer::writeArray(data::Value &val) {
	val._type = Value::Type::ARRAY;
	val.arrayVal = new Array();
	val.arrayVal->reserve(8);
	back = &val;
	stack.push_back(&val);
	state = State::ArrayItem;
}

inline void JsonBuffer::writeDict(data::Value &val) {
	val._type = Value::Type::DICTIONARY;
	val.dictVal = new Dictionary();
	back = &val;
	stack.push_back(&val);
	state = State::DictKey;
}

void JsonBuffer::flushString(const Reader &r) {
	switch (state) {
	case State::None:
		writeString(root, r);
		state = State::End;
		break;
	case State::ArrayItem:
		writeString(emplaceArray(), r);
		state = State::ArrayNext;
		break;
	case State::DictKey:
		key.assign(r.data(), r.size());
		state = State::DictKeyValueSep;
		break;
	case State::DictValue:
		writeString(emplaceDict(), r);
		state = State::DictNext;
		break;
	default:
		break;
	}

	reset();
}

void JsonBuffer::flushNumber(const Reader &r) {
	char *end = nullptr;
	double d = strtod(r.data(), &end);

	switch (state) {
	case State::None:
		writeNumber(root, d);
		state = State::End;
		break;
	case State::ArrayItem:
		writeNumber(emplaceArray(), d);
		state = State::ArrayNext;
		break;
	case State::DictKey:
		key.assign(r.data(), r.size());
		state = State::DictKeyValueSep;
		break;
	case State::DictValue:
		writeNumber(emplaceDict(), d);
		state = State::DictNext;
		break;
	default:
		break;
	}

	reset();
}

void JsonBuffer::flushPlain(const Reader &r) {
	switch (state) {
	case State::None:
		writePlain(root, r);
		state = State::End;
		break;
	case State::ArrayItem:
		writePlain(emplaceArray(), r);
		state = State::ArrayNext;
		break;
	case State::DictKey:
		key.assign(r.data(), r.size());
		state = State::DictKeyValueSep;
		break;
	case State::DictValue:
		writePlain(emplaceDict(), r);
		state = State::DictNext;
		break;
	default:
		break;
	}
}

bool JsonBuffer::parseString(Reader &r, bool tryWhole) {
#define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	static const char escape[256] = {
		Z16, Z16, 0, 0,'\"', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,'/',
		Z16, Z16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,'\\', 0, 0, 0,
		0, 0,'\b', 0, 0, 0,'\f', 0, 0, 0, 0, 0, 0, 0,'\n', 0,
		0, 0,'\r', 0,'\t', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16
	};
#undef Z16
	while (!r.empty() && (!r.is('"') || literal == Literal::StringBackslash)) {
		switch (literal) {
		case Literal::String: {
			Reader s = r.readUntil<StringSepChars>();
			if (s > 0) {
				if (tryWhole && r.is('"')) {
					flushString(s);
					r ++;
					literal = Literal::None;
					return true;
				} else {
					buf.put(s.data(), s.size());
				}
			}
			if (r.is('\\')) {
				++ r;
				literal = Literal::StringBackslash;
			}
		}
			break;
		case Literal::StringBackslash:
			if (r.is('u')) {
				++ r;
				literal = Literal::StringCode;
			} else {
				buf.putc(escape[(uint8_t)r[0]]);
				++ r;
				literal = Literal::String;
			}
			break;
		case Literal::StringCode:
			buf.putc(r[0]); ++ r; literal = Literal::StringCode2; // first unicode char
			break;
		case Literal::StringCode2:
			buf.putc(r[0]); ++ r; literal = Literal::StringCode3; // second unicode char
			break;
		case Literal::StringCode3:
			buf.putc(r[0]); ++ r; literal = Literal::StringCode4;// third unicode char
			break;
		case Literal::StringCode4: // all chars written
			buf.putc(r[0]);
			++ r;
			{
				uint32_t c = 0;
				auto e = buf.pop(4);
				for (int i = 0; i < 4; i++) {
					c = c << 4 | string::hexToChar( e[0] );
					++ e;
				}

				buf.putc((char16_t)c);
			}
			literal = Literal::String;
			break;
		default:
			break;
		}
		tryWhole = false;
	}
	if (r.is('"')) {
		auto bufSize = buf.size();
		if (tryWhole || bufSize == 0) {
			flushString(Reader());
		}
		++ r;
		literal = Literal::None;
		return true;
	}
	return false;
}

bool JsonBuffer::parseNumber(Reader &r, bool tryWhole) {
	Reader e = r.readChars<NumberChars>();
	if (e > 0) {
		if (tryWhole && !r.empty()) {
			flushNumber(e);
			literal = Literal::None;
			return true;
		} else {
			buf.put(e.data(), e.size());
		}
	}
	if (r.empty()) {
		return false;
	} else {
		literal = Literal::None;
		return true;
	}
}

bool JsonBuffer::parsePlain(Reader &r, bool tryWhole) {
	Reader e = r.readChars<PlainChars>();
	if (e > 0) {
		if (tryWhole && !r.empty()) {
			flushPlain(e);
			literal = Literal::None;
			return true;
		} else {
			buf.put(e.data(), e.size());
		}
	}
	if (r.empty()) {
		return false;
	} else {
		literal = Literal::None;
		return true;
	}
}

bool JsonBuffer::readLiteral(Reader &r, bool tryWhole) {
	switch (literal) {
	case Literal::String:
	case Literal::StringBackslash:
	case Literal::StringCode:
	case Literal::StringCode2:
	case Literal::StringCode3:
	case Literal::StringCode4:
		if (parseString(r, tryWhole)) {
			auto b = buf.get();
			if (b > 0) {
				flushString(b);
			}
			return true;
		} else {
			return false;
		}
		break;
	case Literal::Number:
		if (parseNumber(r, tryWhole)) {
			auto b = buf.get();
			if (b > 0) {
				buf.data()[b.size()] = 0;
				flushNumber(b);
			}
			return true;
		} else {
			return false;
		}
		break;
	case Literal::Plain:
		if (parsePlain(r, tryWhole)) {
			auto b = buf.get();
			if (b > 0) {
				flushPlain(b);
			}
			return true;
		} else {
			return false;
		}
		break;
	default:
		break;
	}
	state = State::End;
	return true;
}


inline JsonBuffer::Literal JsonBuffer::getLiteral(char c) {
	switch(c) {
	case '"':
		return Literal::String;
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
		return Literal::Number;
		break;

	case '[': return Literal::ArrayOpen; break;
	case ']': return Literal::ArrayClose; break;
	case '{': return Literal::DictOpen; break;
	case '}': return Literal::DictClose; break;
	case ':': return Literal::DictSep; break;
	case ',': return Literal::Next; break;

	default:
		return Literal::Plain;
		break;
	}
}

bool JsonBuffer::beginLiteral(Reader &r, Literal l) {
	if (l == Literal::String) {
		++ r;
	}

	literal = l;
	return readLiteral(r, true);
}

inline void JsonBuffer::pop() {
	stack.pop_back();
	if (stack.empty()) {
		back = nullptr;
		state = State::End;
	} else {
		back = stack.back();
		state = (back->isArray())?State::ArrayNext:State::DictNext;
	}
}

size_t JsonBuffer::read(const uint8_t* s, size_t count) {
	Reader r((const char *)s, count);
	if (literal != Literal::None) {
		if (!readLiteral(r, false)) {
			return count;
		}
	}

	r.skipUntil<NextTokenChars>();
	while (r > 0) {
		auto l = getLiteral(r[0]);
		switch (state) {
		case State::None:
			switch (l) {
			case Literal::ArrayOpen: ++ r; writeArray(root); break;
			case Literal::DictOpen: ++ r;  writeDict(root); break;
			case Literal::String:
			case Literal::Number:
			case Literal::Plain:
				if (!beginLiteral(r, l)) { return count; } break;
			default: state = State::End; break;
			}

			break;
		case State::ArrayItem:
			switch (l) {
			case Literal::ArrayOpen: ++ r; writeArray(emplaceArray()); break;
			case Literal::DictOpen: ++ r; writeDict(emplaceArray()); break;
			case Literal::String:
			case Literal::Number:
			case Literal::Plain:
				if (!beginLiteral(r, l)) { return count; } break;
			case Literal::Next: ++r; break;
			case Literal::ArrayClose: r ++; back->arrayVal->shrink_to_fit(); pop(); break;
			default: state = State::End; break;
			}
			break;
		case State::ArrayNext:
			switch (l) {
			case Literal::Next: ++r; state = State::ArrayItem; break;
			case Literal::ArrayClose: r ++; back->arrayVal->shrink_to_fit(); pop(); break;
			default: state = State::End; break;
			}
			break;
		case State::DictKey:
			switch (l) {
			case Literal::String:
			case Literal::Number:
			case Literal::Plain:
				if (!beginLiteral(r, l)) { return count; } break;
			case Literal::Next: ++r; break;
			case Literal::DictClose: r ++; pop(); break;
			default: state = State::End; break;
			}
			break;
		case State::DictKeyValueSep:
			switch (l) {
			case Literal::DictSep: ++r; state = State::DictValue; break;
			case Literal::DictClose: key.clear(); r ++; pop(); break;
			case Literal::Next: ++r; break;
			default: state = State::End; break;
			}
			break;
		case State::DictValue:
			switch (l) {
			case Literal::ArrayOpen: ++ r; writeArray(emplaceDict()); break;
			case Literal::DictOpen: ++ r; writeDict(emplaceDict()); break;
			case Literal::String:
			case Literal::Number:
			case Literal::Plain:
				if (!beginLiteral(r, l)) { return count; } break;
			case Literal::Next: ++r; break;
			case Literal::DictClose: key.clear(); r ++; pop(); break;
			default: state = State::End; break;
			}
			break;
		case State::DictNext:
			switch (l) {
			case Literal::Next: ++r; state = State::DictKey; break;
			case Literal::DictClose: r ++; pop(); break;
			default: state = State::End; break;
			}
			break;
		case State::End:
			return 0; // invalid state
			break;
		}

		r.skipUntil<NextTokenChars>();
	}

	return count;
}

void JsonBuffer::clear() {
	root.setNull();
	buf.clear();
}

NS_SP_EXT_END(data)
