/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_STREAM_SPDATAJSONSTREAM_H_
#define COMMON_STREAM_SPDATAJSONSTREAM_H_

#include "SPData.h"
#include "SPTransferBuffer.h"
#include "SPBuffer.h"

NS_SP_EXT_BEGIN(data)

template <typename Interface>
class JsonBuffer : public Interface::AllocBaseType {
public:
	using Reader = StringView;

	using InterfaceType = Interface;
	using BufferType = BufferTemplate<Interface>;
	using ValueType = ValueTemplate<Interface>;
	using StringType = typename InterfaceType::StringType;
	using BytesType = typename InterfaceType::BytesType;
	using ArrayType = typename ValueType::ArrayType;
	using DictionaryType = typename ValueType::DictionaryType;

	JsonBuffer(size_t block = BufferType::defsize) : buf(block) { }

	JsonBuffer(JsonBuffer &&) = delete;
	JsonBuffer & operator = (JsonBuffer &&) = delete;

	JsonBuffer(const JsonBuffer &) = delete;
	JsonBuffer & operator = (const JsonBuffer &) = delete;

	ValueType & data() {
		if (!buf.empty()) {
			read((const uint8_t *)"\0", 1);
		}
		return root;
	}

	size_t read(const uint8_t * s, size_t count);
	void clear() {
		root.setNull();
		buf.clear();
	}

protected:
	enum class State {
		None,
		ArrayItem,
		ArrayNext,
		DictKey,
		DictKeyValueSep,
		DictValue,
		DictNext,
		End,
	};

	enum class Literal {
		None,
		String,
		StringBackslash,
		StringCode,
		StringCode2,
		StringCode3,
		StringCode4,
		Number,
		Plain,

		ArrayOpen,
		ArrayClose,
		DictOpen,
		DictClose,
		DictSep,
		Next,
	};

	void reset() {
		buf.clear();
	}

	ValueType &emplaceArray() {
		back->arrayVal->emplace_back(ValueType::Type::EMPTY);
		return back->arrayVal->back();
	}
	ValueType &emplaceDict() {
		return back->dictVal->emplace(std::move(key), ValueType::Type::EMPTY).first->second;
	}

	void writeString(ValueType &val, const Reader &r) {
		val._type = ValueType::Type::CHARSTRING;
		val.strVal = new StringType(r.data(), r.size());
	}
	void writeNumber(ValueType &val, double d) {
		val._type = ValueType::Type::DOUBLE;
		val.doubleVal = d;
	}
	void writeInteger(ValueType &val, int64_t d) {
		val._type = ValueType::Type::INTEGER;
		val.intVal = (int64_t)round(d);
	}
	void writePlain(ValueType &val, const Reader &r) {
		if (r == "nan") {
			val._type = ValueType::Type::DOUBLE;
			val.doubleVal = nan();
		} else if (r == "inf") {
			val._type = ValueType::Type::DOUBLE;
			val.doubleVal = NumericLimits<double>::infinity();
		} else if (r == "null") {
			val._type = ValueType::Type::EMPTY;
		} else if (r == "true") {
			val._type = ValueType::Type::BOOLEAN;
			val.boolVal = true;
		} else if (r == "false") {
			val._type = ValueType::Type::BOOLEAN;
			val.boolVal = false;
		}
	}
	void writeArray(ValueType &val) {
		val._type = ValueType::Type::ARRAY;
		val.arrayVal = new ArrayType();
		val.arrayVal->reserve(8);
		back = &val;
		stack.push_back(&val);
		state = State::ArrayItem;
	}
	void writeDict(ValueType &val) {
		val._type = ValueType::Type::DICTIONARY;
		val.dictVal = new DictionaryType();
		back = &val;
		stack.push_back(&val);
		state = State::DictKey;
	}

	void flushString(const Reader &r) {
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
	void flushNumber(const Reader &r) {
		bool isFloat = false;
		auto tmp = r;
		auto value = json::decodeNumber(tmp, isFloat);

		switch (state) {
		case State::None:
			if (isFloat) {
				value.readDouble().unwrap([&] (double value) { writeNumber(root, value); });
			} else {
				value.readInteger().unwrap([&] (int64_t value) { writeInteger(root, value); });
			}
			state = State::End;
			break;
		case State::ArrayItem:
			if (isFloat) {
				value.readDouble().unwrap([&] (double value) { writeNumber(emplaceArray(), value); });
			} else {
				value.readInteger().unwrap([&] (int64_t value) { writeInteger(emplaceArray(), value); });
			}
			state = State::ArrayNext;
			break;
		case State::DictKey:
			key.assign(value.data(), value.size());
			state = State::DictKeyValueSep;
			break;
		case State::DictValue:
			if (isFloat) {
				value.readDouble().unwrap([&] (double value) { writeNumber(emplaceDict(), value); });
			} else {
				value.readInteger().unwrap([&] (int64_t value) { writeInteger(emplaceDict(), value); });
			}
			state = State::DictNext;
			break;
		default:
			break;
		}

		reset();
	}
	void flushPlain(const Reader &r) {
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

	bool parseString(Reader &r, bool tryWhole);
	bool parseNumber(Reader &r, bool tryWhole);
	bool parsePlain(Reader &r, bool tryWhole);

	void pushArray(bool r);
	void pushDict(bool r);

	Literal getLiteral(char);
	bool readLiteral(Reader &, bool tryWhole);
	bool beginLiteral(Reader &, Literal l);

	void pop() {
		stack.pop_back();
		if (stack.empty()) {
			back = nullptr;
			state = State::End;
		} else {
			back = stack.back();
			state = (back->isArray())?State::ArrayNext:State::DictNext;
		}
	}

protected:
	BufferType buf;
    ValueType root;

    ValueType *back = nullptr;
    typename InterfaceType::template ArrayType<ValueType *> stack;

	State state = State::None;
	Literal literal = Literal::None;
	StringType key;
};


template <typename Interface>
bool JsonBuffer<Interface>::parseString(Reader &r, bool tryWhole) {
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
			Reader s = r.readUntil<Reader::Chars<'\\', '"'>>();
			if (!s.empty()) {
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
				auto e = buf.pop(4);
				buf.putc(char16_t(base16::hexToChar(e[0], e[1]) << 8 | base16::hexToChar(e[2], e[3]) ));
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

template <typename Interface>
bool JsonBuffer<Interface>::parseNumber(Reader &r, bool tryWhole) {
	Reader e = r.readChars<Reader::Chars<'-', '+', '.', 'E', 'e'>, Reader::Range<'0', '9'>>();
	if (!e.empty()) {
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

template <typename Interface>
bool JsonBuffer<Interface>::parsePlain(Reader &r, bool tryWhole) {
	Reader e = r.readChars<Reader::CharGroup<CharGroupId::Alphanumeric>>();
	if (!e.empty()) {
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

template <typename Interface>
bool JsonBuffer<Interface>::readLiteral(Reader &r, bool tryWhole) {
	switch (literal) {
	case Literal::String:
	case Literal::StringBackslash:
	case Literal::StringCode:
	case Literal::StringCode2:
	case Literal::StringCode3:
	case Literal::StringCode4:
		if (parseString(r, tryWhole)) {
			auto b = buf.get();
			if (!b.empty()) {
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
			if (!b.empty()) {
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
			if (!b.empty()) {
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

template <typename Interface>
auto JsonBuffer<Interface>::getLiteral(char c) -> Literal {
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

template <typename Interface>
bool JsonBuffer<Interface>::beginLiteral(Reader &r, Literal l) {
	if (l == Literal::String) {
		++ r;
	}

	literal = l;
	return readLiteral(r, true);
}

template <typename Interface>
size_t JsonBuffer<Interface>::read(const uint8_t* s, size_t count) {
	Reader r((const char *)s, count);
	if (literal != Literal::None) {
		if (!readLiteral(r, false)) {
			return count;
		}
	}

	r.skipUntil<Reader::Chars<'"', 't', 'f', 'n', '+', '-', '[', '{', ']', '}', ':', ','>, Reader::Range<'0', '9'>>();
	while (!r.empty()) {
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

		r.skipUntil<Reader::Chars<'"', 't', 'f', 'n', '+', '-', '[', '{', ']', '}', ':', ','>, Reader::Range<'0', '9'>>();
	}

	return count;
}

NS_SP_EXT_END(data)

#endif /* COMMON_STREAM_SPDATAJSONSTREAM_H_ */
