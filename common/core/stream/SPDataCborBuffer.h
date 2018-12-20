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

#ifndef COMMON_STREAM_SPDATACBORBUFFER_H_
#define COMMON_STREAM_SPDATACBORBUFFER_H_

#include "SPDataReader.h"
#include "SPData.h"
#include "SPTransferBuffer.h"
#include "SPBuffer.h"

NS_SP_EXT_BEGIN(data)

template <typename Interface>
class CborBuffer : public AllocBase {
public:
	using Reader = DataReader<ByteOrder::Network>;

	using InterfaceType = Interface;
	using BufferType = BufferTemplate<Interface>;
	using ValueType = ValueTemplate<Interface>;
	using StringType = typename InterfaceType::StringType;
	using BytesType = typename InterfaceType::BytesType;
	using ArrayType = typename ValueType::ArrayType;
	using DictionaryType = typename ValueType::DictionaryType;

	CborBuffer(size_t block = Buffer::defsize) : buf(block) { }

	CborBuffer(CborBuffer &&) = delete;
	CborBuffer & operator = (CborBuffer &&) = delete;

	CborBuffer(const CborBuffer &) = delete;
	CborBuffer & operator = (const CborBuffer &) = delete;

	ValueType & data() {
		if (!buf.empty()) {
			read((const uint8_t *)"\xFF", 1);
		}
		return root;
	}

	size_t read(const uint8_t * s, size_t count);
	void clear();

protected:
	enum class State {
		Begin,
		Array,
		DictKey,
		DictValue,
		End,
	};

	enum class Literal {
		None,
		Head,
		Chars,
		CharSequence,
		Bytes,
		ByteSequence,
		Float,
		Unsigned,
		Negative,
		Tag,
		Array,
		Dict,
		CharSize,
		ByteSize,
		ArraySize,
		DictSize,
		EndSequence,
	};

	enum class Sequence {
		None,
		Head,
		Size,
		Value,
	};

	ValueType &emplaceArray() {
		auto &back = stack.back();
		back.first->arrayVal->emplace_back(ValueType::Type::EMPTY);
		return back.first->arrayVal->back();
	}

	ValueType &emplaceDict() {
		auto &back = stack.back();
		return back.first->dictVal->emplace(std::move(key), ValueType::Type::EMPTY).first->second;
	}

	void emplaceInt(ValueType &val, int64_t value) {
		val._type = ValueType::Type::INTEGER;
		val.intVal = value;
	}
	void emplaceBool(ValueType &val, bool value) {
		val._type = ValueType::Type::BOOLEAN;
		val.boolVal = value;
	}
	void emplaceDouble(ValueType &val, double value) {
		val._type = ValueType::Type::DOUBLE;
		val.doubleVal = value;
	}
	void emplaceCharString(ValueType &val, Reader &r, size_t size) {
		val._type = ValueType::Type::CHARSTRING;
		val.strVal = new StringType((const char *)r.data(), size);
		r += size;
	}
	void emplaceByteString(ValueType &val, Reader &r, size_t size) {
		val._type = ValueType::Type::BYTESTRING;
		auto bytes = new BytesType(); bytes->resize(size);
		memcpy(bytes->data(), r.data(), size);
		val.bytesVal = bytes;
		r += size;
	}

	void writeArray(ValueType &val, size_t s) {
		val._type = ValueType::Type::ARRAY;
		val.arrayVal = new ArrayType();
		val.arrayVal->reserve(s);
		stack.push_back(pair(&val, s));
		state = State::Array;
	}

	void writeDict(ValueType &val, size_t s) {
		val._type = ValueType::Type::DICTIONARY;
		val.dictVal = new DictionaryType();
		cbor::__DecoderMapReserve<Interface>::reserve(*val.dictVal, s);
		stack.push_back(pair(&val, s));
		state = State::DictKey;
	}
	void pop() {
		stack.pop_back();
		if (stack.empty()) {
			state = State::End;
		} else {
			auto &back = stack.back().first;
			state = (back->isArray())?State::Array:State::DictKey;
		}
	}

	size_t getLiteralSize(uint8_t f);
	Literal getLiteral(uint8_t t, uint8_t f);

	uint64_t readInt(Reader &r, size_t s) {
		switch (s) {
		case 1: return r.readUnsigned(); break;
		case 2: return r.readUnsigned16(); break;
		case 4: return r.readUnsigned32(); break;
		case 8: return r.readUnsigned64(); break;
		default: break;
		}
		return 0;
	}
	double readFloat(Reader &r, size_t s){
		switch (s) {
		case 2: return r.readFloat16(); break;
		case 4: return r.readFloat32(); break;
		case 8: return r.readFloat64(); break;
		default: break;
		}
		return 0.0;
	}

	void flush(ValueType &, Reader &r, size_t s);
	void flushKey(Reader &r, size_t s);
	void flush(Reader &r, size_t s);
	void flushValueRoot(uint8_t t, uint8_t f);
	void flushValueArray(uint8_t t, uint8_t f);
	void flushValueKey(uint8_t t, uint8_t f);
	void flushValueDict(uint8_t t, uint8_t f);

	void flushSequenceChars(ValueType &, Reader &, size_t);
	void flushSequenceBytes(ValueType &, Reader &, size_t);
	void flushSequence(Reader &, size_t);

	bool parse(Reader &r, size_t s);

	Literal getLiteral(char);

	bool readSequenceHead(Reader &r);
	bool readSequenceSize(Reader &, bool tryWhole);
	bool readSequenceValue(Reader &, bool tryWhole);

	bool readLiteral(Reader &, bool tryWhole);
	bool readSequence(Reader &, bool tryWhole);
	bool readControl(Reader &);

	bool beginLiteral(Reader &, Literal l);

protected:
    BufferType buf;
    ValueType root;

    typename InterfaceType::template ArrayType<Pair<ValueType *, size_t>> stack; // value, value size
	size_t remains = 3; // head size

	State state = State::Begin;
	Literal literal = Literal::Head;
	Sequence sequence = Sequence::None;
	StringType key;
};


template <typename Interface>
void CborBuffer<Interface>::flush(ValueType &val, Reader &r, size_t s) {
	switch (literal) {
	case Literal::Chars: emplaceCharString(val, r, s); break;
	case Literal::Bytes: emplaceByteString(val, r, s); break;
	case Literal::Unsigned: emplaceInt(val, readInt(r, s)); break;
	case Literal::Negative: emplaceInt(val, -1 - readInt(r, s)); break;
	case Literal::Float: emplaceDouble(val, readFloat(r, s)); break;
	default:
		break;
	}
}

template <typename Interface>
void CborBuffer<Interface>::flushKey(Reader &r, size_t s) {
	switch (literal) {
	case Literal::Chars: key.assign((const char *)r.data(), s); r += s; break;
	case Literal::Bytes: key = base64::encode<Interface>(CoderSource(r.data(), s)); r += s; break;
	case Literal::Unsigned: key = string::ToStringTraits<Interface>::toString(readInt(r,s)); break;
	case Literal::Negative: key = string::ToStringTraits<Interface>::toString((int64_t)-1 - readInt(r, s)); break;
	case Literal::Float: key = string::ToStringTraits<Interface>::toString(readFloat(r, s)); break;
	default: break;
	}
}

template <typename Interface>
void CborBuffer<Interface>::flush(Reader &r, size_t s) {
	switch (state) {
	case State::Begin: flush(root, r, s); break;
	case State::Array: flush(emplaceArray(), r, s); break;
	case State::DictKey: flushKey(r, s); state = State::DictValue; break;
	case State::DictValue: flush(emplaceDict(), r, s); state = State::DictKey; break;
	default: break;
	}

	literal = Literal::None;
}

template <typename Interface>
void CborBuffer<Interface>::flushValueRoot(uint8_t t, uint8_t f) {
	switch (cbor::MajorType(t)) {
	case cbor::MajorType::Unsigned: emplaceInt(root, f); break;
	case cbor::MajorType::Negative: emplaceInt(root, -1 - f); break;
	case cbor::MajorType::Simple:
		switch (cbor::SimpleValue(f)) {
		case cbor::SimpleValue::False: emplaceBool(root, false); break;
		case cbor::SimpleValue::True: emplaceBool(root, true); break;
		case cbor::SimpleValue::Null:
		case cbor::SimpleValue::Undefined:
			break;
		default:
			emplaceInt(root, f);
			break;
		}
		break;
	default: break;
	}
}

template <typename Interface>
void CborBuffer<Interface>::flushValueArray(uint8_t t, uint8_t f) {
	switch (cbor::MajorType(t)) {
	case cbor::MajorType::Unsigned: emplaceInt(emplaceArray(), f); break;
	case cbor::MajorType::Negative: emplaceInt(emplaceArray(), -1 - f); break;
	case cbor::MajorType::Simple:
		switch (cbor::SimpleValue(f)) {
		case cbor::SimpleValue::False: emplaceBool(emplaceArray(), false); break;
		case cbor::SimpleValue::True: emplaceBool(emplaceArray(), true); break;
		case cbor::SimpleValue::Null:
		case cbor::SimpleValue::Undefined:
			emplaceArray();
			break;
		default:
			emplaceInt(emplaceArray(), f);
			break;
		}
		break;
	default: break;
	}
}

template <typename Interface>
void CborBuffer<Interface>::flushValueKey(uint8_t t, uint8_t f) {
	switch (cbor::MajorType(t)) {
	case cbor::MajorType::Unsigned: key = string::ToStringTraits<Interface>::toString(f); state = State::DictValue; break;
	case cbor::MajorType::Negative: key = string::ToStringTraits<Interface>::toString(-1 - f); state = State::DictValue; break;
	case cbor::MajorType::Simple:
		switch (cbor::SimpleValue(f)) {
		case cbor::SimpleValue::False: key = "false"; state = State::DictValue; break;
		case cbor::SimpleValue::True: key = "true"; state = State::DictValue; break;
		case cbor::SimpleValue::Null: key = "(null)"; state = State::DictValue; break;
		default: key = "(undefined)"; state = State::DictValue; break;
		}
		break;
	default: break;
	}
}

template <typename Interface>
void CborBuffer<Interface>::flushValueDict(uint8_t t, uint8_t f) {
	switch (cbor::MajorType(t)) {
	case cbor::MajorType::Unsigned: emplaceInt(emplaceDict(), f); state = State::DictKey; break;
	case cbor::MajorType::Negative: emplaceInt(emplaceDict(), -1 - f); state = State::DictKey; break;
	case cbor::MajorType::Simple:
		switch (cbor::SimpleValue(f)) {
		case cbor::SimpleValue::False: emplaceBool(emplaceDict(), false); state = State::DictKey; break;
		case cbor::SimpleValue::True: emplaceBool(emplaceDict(), true); state = State::DictKey; break;
		case cbor::SimpleValue::Null:
		case cbor::SimpleValue::Undefined:
			emplaceDict();
			state = State::DictKey;
			break;
		default:
			emplaceInt(emplaceDict(), f);
			state = State::DictKey;
			break;
		}
		break;
	default: break;
	}
}

template <typename Interface>
void CborBuffer<Interface>::flushSequenceChars(ValueType &val, Reader &r, size_t s) {
	if (val._type != ValueType::Type::CHARSTRING) {
		val._type = ValueType::Type::CHARSTRING;
		val.strVal = new StringType();
	}
	val.strVal->append((const char *)r.data(), s);
}

template <typename Interface>
void CborBuffer<Interface>::flushSequenceBytes(ValueType &val, Reader &r, size_t s) {
	if (val._type != ValueType::Type::BYTESTRING) {
		val._type = ValueType::Type::BYTESTRING;
		val.bytesVal = new BytesType();
	}
	auto &bytes = *val.bytesVal;
	auto ns = bytes.size();
	bytes.resize(ns + s);
	memcpy(bytes.data() + ns, r.data(), s);
}

template <typename Interface>
void CborBuffer<Interface>::flushSequence(Reader &r, size_t s) {
	switch (state) {
	case State::Array:
	case State::DictValue:
	case State::Begin: {
		switch (literal) {
		case Literal::CharSequence:
			flushSequenceChars(stack.empty()?root:*stack.back().first, r, s);
			break;
		case Literal::ByteSequence:
			flushSequenceBytes(stack.empty()?root:*stack.back().first, r, s);
			break;
		default:
			break;
		}
		r += remains;
	}
		break;
	case State::DictKey:
		switch (literal) {
		case Literal::CharSequence:
			key.append((const char *)r.data(), s);
			break;
		case Literal::ByteSequence:
			key.append(base64::encode<Interface>(CoderSource(r.data(), s)));
			break;
		default:
			break;
		}
		break;
	default:
		state = State::End;
		return;
	}
}

template <typename Interface>
bool CborBuffer<Interface>::parse(Reader &r, size_t s) {
	switch (literal) {
	case Literal::Head:
		if (r[0] == 0xd9 && r[1] == 0xd9 && r[2] == 0xf7) {
			literal = Literal::None;
			state = State::Begin;
		} else {
			state = State::End;
		}
		break;
	case Literal::ByteSize:
		remains = size_t(readInt(r, s));
		literal = Literal::Bytes;
		break;
	case Literal::CharSize:
		remains = size_t(readInt(r, s));
		literal = Literal::Chars;
		break;
	case Literal::ArraySize:
		stack.back().second = size_t(readInt(r, s));
		literal = Literal::None;
		state = State::Array;
		break;
	case Literal::DictSize:
		stack.back().second = size_t(readInt(r, s));
		literal = Literal::None;
		state = State::DictKey;
		break;
	default:
		flush(r, s);
		break;
	}

	buf.clear();
	return true;
}

template <typename Interface>
bool CborBuffer<Interface>::readLiteral(Reader &r, bool tryWhole) {
	if (tryWhole && r >= remains) {
		parse(r, remains);
		return true;
	}

	auto len = min(remains, r.size());
	buf.put((const char *)r.data(), len);
	r += len; remains -= len;

	if (remains > 0) {
		return false;
	}

	auto tmp = buf.template get<Reader>();
	parse(tmp, tmp.size());
	return true;
}

template <typename Interface>
bool CborBuffer<Interface>::readSequenceHead(Reader &r) {
	auto v = r.readUnsigned();
	auto f = cbor::flags(v);

	switch (f) {
	case cbor::Flags::AdditionalNumber8Bit: sequence = Sequence::Size; remains = 1; break;
	case cbor::Flags::AdditionalNumber16Bit: sequence = Sequence::Size; remains = 2; break;
	case cbor::Flags::AdditionalNumber32Bit: sequence = Sequence::Size; remains = 4; break;
	case cbor::Flags::AdditionalNumber64Bit: sequence = Sequence::Size; remains = 8; break;
	case cbor::Flags::UndefinedLength: sequence = Sequence::None; break;
	case cbor::Flags::Unassigned1:
	case cbor::Flags::Unassigned2:
	case cbor::Flags::Unassigned3:
		sequence = Sequence::None;
		state = State::End;
		break;
	default:
		remains = toInt(f);
		sequence = Sequence::Value;
		break;
	}

	return true;
}

template <typename Interface>
bool CborBuffer<Interface>::readSequenceSize(Reader &r, bool tryWhole) {
	if (tryWhole && r >= remains) {
		remains = size_t(readInt(r, remains));
		sequence = Sequence::Value;
		return true;
	}

	auto len = min(remains, r.size());
	buf.put((const char *)r.data(), len);
	r += len; remains -= len;

	if (remains > 0) {
		return false;
	}

	auto tmp = buf.template get<Reader>();
	remains = size_t(readInt(tmp, tmp.size()));
	sequence = Sequence::Value;
	buf.clear();
	return true;
}

template <typename Interface>
bool CborBuffer<Interface>::readSequenceValue(Reader &r, bool tryWhole) {
	if (tryWhole && r >= remains) {
		flushSequence(r, remains);
		sequence = Sequence::Head;
		return true;
	}

	auto len = min(remains, r.size());
	buf.put((const char *)r.data(), len);
	r += len; remains -= len;

	if (remains > 0) {
		return false;
	}

	auto tmp = buf.template get<Reader>();
	flushSequence(tmp, tmp.size());
	sequence = Sequence::Head;
	buf.clear();
	return true;
}

template <typename Interface>
bool CborBuffer<Interface>::readSequence(Reader &r, bool tryWhole) {
	while (sequence != Sequence::None && !r.empty()) {
		switch (sequence) {
		case Sequence::Head:
			readSequenceHead(r);
			break;
		case Sequence::Size:
			if (!readSequenceSize(r, tryWhole)) {
				return false;
			}
			break;
		case Sequence::Value:
			if (!readSequenceValue(r, tryWhole)) {
				return false;
			}
			break;
		default:
			break;
		}
		tryWhole = true;
	}
	return true;
}

template <typename Interface>
size_t CborBuffer<Interface>::getLiteralSize(uint8_t f) {
	switch (cbor::Flags(f)) {
	case cbor::Flags::AdditionalNumber8Bit: return 1; break;
	case cbor::Flags::AdditionalNumber16Bit: return 2; break;
	case cbor::Flags::AdditionalNumber32Bit: return 4; break;
	case cbor::Flags::AdditionalNumber64Bit: return 8; break;
	case cbor::Flags::UndefinedLength: return maxOf<size_t>(); break;
	case cbor::Flags::Unassigned1:
	case cbor::Flags::Unassigned2:
	case cbor::Flags::Unassigned3:
		return 0;
		break;
	default:
		return f;
		break;
	}
}

template <typename Interface>
auto CborBuffer<Interface>::getLiteral(uint8_t t, uint8_t f) -> Literal {
	bool fullLiteral = (f < 24);
	bool undefined = f == cbor::Flags::UndefinedLength;
	switch (cbor::MajorType(t)) {
	case cbor::MajorType::Array:
		return (fullLiteral || undefined) ? Literal::Array : Literal::ArraySize;
		break;
	case cbor::MajorType::Map:
		return (fullLiteral || undefined) ? Literal::Dict : Literal::DictSize;
		break;
	case cbor::MajorType::Unsigned:
		return fullLiteral ? Literal::None : Literal::Unsigned;
		break;
	case cbor::MajorType::Negative:
		return fullLiteral ? Literal::None : Literal::Negative;
		break;
	case cbor::MajorType::CharString:
		return fullLiteral ? Literal::Chars : (undefined) ? Literal::CharSequence : Literal::CharSize;
		break;
	case cbor::MajorType::ByteString:
		return fullLiteral ? Literal::Bytes : (undefined) ? Literal::ByteSequence : Literal::ByteSize;
		break;
	case cbor::MajorType::Simple:
		return fullLiteral ? Literal::None : (f == cbor::Flags::Simple8Bit ? Literal::Unsigned : Literal::Float);
		break;
	case cbor::MajorType::Tag:
		return fullLiteral ? Literal::None : Literal::Tag;
		break;
	}
	return Literal::None;
}

template <typename Interface>
bool CborBuffer<Interface>::readControl(Reader &r) {
	auto v = r.readUnsigned();
	auto t = cbor::type(v);
	auto f = cbor::data(v);

	Literal l = getLiteral(toInt(t), f);
	size_t s = getLiteralSize(f);

	while (!stack.empty() && stack.back().second == 0) {
		pop();
	}

	switch (state) {
	case State::Begin:
		switch (l) {
		case Literal::None: flushValueRoot(toInt(t), f); break;
		case Literal::Array: writeArray(root, s); break;
		case Literal::ArraySize: remains = s; writeArray(root, 0); literal = l; break;
		case Literal::Dict: writeDict(root, s); break;
		case Literal::DictSize: remains = s; writeDict(root, 0); literal = l; break;
		case Literal::EndSequence: state = State::End; literal = Literal::None; break;
		default: literal = l; remains = s; break;
		}
		break;
	case State::Array:
		-- stack.back().second; // reduce expected array size
		switch (l) {
		case Literal::None: flushValueArray(toInt(t), f); break;
		case Literal::Array: writeArray(emplaceArray(), s); break;
		case Literal::ArraySize: remains = s; writeArray(emplaceArray(), 0); literal = l; break;
		case Literal::Dict: writeDict(emplaceArray(), s); break;
		case Literal::DictSize: remains = s; writeDict(emplaceArray(), 0); literal = l; break;
		case Literal::EndSequence: pop(); break;
		default: literal = l; remains = s; break;
		}
		break;
	case State::DictKey:
		switch (l) {
		case Literal::None: flushValueKey(toInt(t), f); break;
		case Literal::Array: state = State::End; literal = Literal::None; break;
		case Literal::ArraySize: state = State::End; literal = Literal::None; break;
		case Literal::Dict: state = State::End; literal = Literal::None; break;
		case Literal::DictSize: state = State::End; literal = Literal::None; break;
		case Literal::EndSequence: pop(); break;
		default: literal = l; remains = s; break;
		}
		break;
	case State::DictValue:
		-- stack.back().second; // reduce expected dict size
		switch (l) {
		case Literal::None: flushValueDict(toInt(t), f); break;
		case Literal::Array: writeArray(emplaceDict(), s); break;
		case Literal::ArraySize: remains = s; writeArray(emplaceDict(), 0); literal = l; break;
		case Literal::Dict: writeDict(emplaceDict(), s); break;
		case Literal::DictSize: remains = s; writeDict(emplaceDict(), 0); literal = l; break;
		case Literal::EndSequence: state = State::End; literal = Literal::None; break;
		default: literal = l; remains = s; break;
		}
		break;
	case State::End:
		return false;
		break;
	}

	if (literal == Literal::ByteSequence || literal == Literal::CharSequence) {
		sequence = Sequence::Head;
	}

	return true;
}

template <typename Interface>
size_t CborBuffer<Interface>::read(const uint8_t * s, size_t count) {
	bool tryWhole = false;
	Reader r((const uint8_t *)s, count);

	while (!r.empty() && state != State::End) {
		switch (literal) {
		case Literal::CharSequence:
		case Literal::ByteSequence:
			if (!readSequence(r, tryWhole)) {
				return count;
			}
			tryWhole = true;
			break;
		case Literal::None:
			readControl(r);
			if (remains == 0 && (literal == Literal::Chars || literal == Literal::Bytes)) {
				readLiteral(r, true);
			}
			break;
		default:
			if (!readLiteral(r, tryWhole)) {
				return count;
			}
		}

		tryWhole = true;
	}

	return count;
}

NS_SP_EXT_END(data)

#endif /* COMMON_STREAM_SPDATACBORBUFFER_H_ */
