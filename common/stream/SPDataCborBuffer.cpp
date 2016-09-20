/*
 * SPCborBuffer.cpp
 *
 *  Created on: 3 янв. 2016 г.
 *      Author: sbkarr
 */

#include "SPCommon.h"
#include "SPDataCborBuffer.h"
#include "SPDataCbor.h"
#include "SPString.h"

NS_SP_EXT_BEGIN(data)

inline data::Value &CborBuffer::emplaceArray() {
	auto &back = stack.back();
	back.first->arrayVal->emplace_back(Value::Type::EMPTY);
	return back.first->arrayVal->back();
}

inline data::Value &CborBuffer::emplaceDict() {
	auto &back = stack.back();
	return back.first->dictVal->emplace(std::move(key), Value::Type::EMPTY).first->second;
}

inline void CborBuffer::emplaceInt(data::Value &val, int64_t value) {
	val._type = Value::Type::INTEGER;
	val.intVal = value;
}
inline void CborBuffer::emplaceBool(data::Value &val, bool value) {
	val._type = Value::Type::BOOLEAN;
	val.boolVal = value;
}
inline void CborBuffer::emplaceDouble(data::Value &val, double value) {
	val._type = Value::Type::DOUBLE;
	val.doubleVal = value;
}
inline void CborBuffer::emplaceCharString(data::Value &val, Reader &r, size_t size) {
	val._type = Value::Type::CHARSTRING;
	val.strVal = new String((const char *)r.data(), size);
	r += size;
}
inline void CborBuffer::emplaceByteString(data::Value &val, Reader &r, size_t size) {
	val._type = Value::Type::BYTESTRING;
	auto bytes = new Bytes(); bytes->resize(size);
	memcpy(bytes->data(), r.data(), size);
	val.bytesVal = bytes;
	r += size;
}

void CborBuffer::writeArray(data::Value &val, size_t s) {
	val._type = Value::Type::ARRAY;
	val.arrayVal = new Array();
	val.arrayVal->reserve(s);
	stack.push_back(pair(&val, s));
	state = State::Array;
}

void CborBuffer::writeDict(data::Value &val, size_t s) {
	val._type = Value::Type::DICTIONARY;
	val.dictVal = new Dictionary();
	stack.push_back(pair(&val, s));
	state = State::DictKey;
}

inline uint64_t CborBuffer::readInt(Reader &r, size_t s) {
	switch (s) {
	case 1: return r.readUnsigned(); break;
	case 2: return r.readUnsigned16(); break;
	case 4: return r.readUnsigned32(); break;
	case 8: return r.readUnsigned64(); break;
	default: break;
	}
	return 0;
}

double CborBuffer::readFloat(Reader &r, size_t s) {
	switch (s) {
	case 2: return r.readFloat16(); break;
	case 4: return r.readFloat32(); break;
	case 8: return r.readFloat64(); break;
	default: break;
	}
	return 0.0;
}

void CborBuffer::flush(data::Value &val, Reader &r, size_t s) {
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

void CborBuffer::flushKey(Reader &r, size_t s) {
	switch (literal) {
	case Literal::Chars: key.assign((const char *)r.data(), s); r += s; break;
	case Literal::Bytes: key = base64::encode(r.data(), s); r += s; break;
	case Literal::Unsigned: key = stappler::toString(readInt(r,s)); break;
	case Literal::Negative: key = stappler::toString((int64_t)-1 - readInt(r, s)); break;
	case Literal::Float: key = stappler::toString(readFloat(r, s)); break;
	default: break;
	}
}

void CborBuffer::flush(Reader &r, size_t s) {
	switch (state) {
	case State::Begin: flush(root, r, s); break;
	case State::Array: flush(emplaceArray(), r, s); break;
	case State::DictKey: flushKey(r, s); state = State::DictValue; break;
	case State::DictValue: flush(emplaceDict(), r, s); state = State::DictKey; break;
	default: break;
	}

	literal = Literal::None;
}

void CborBuffer::flushValueRoot(uint8_t t, uint8_t f) {
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
		}
		break;
	default: break;
	}
}

void CborBuffer::flushValueArray(uint8_t t, uint8_t f) {
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
		}
		break;
	default: break;
	}
}

void CborBuffer::flushValueKey(uint8_t t, uint8_t f) {
	switch (cbor::MajorType(t)) {
	case cbor::MajorType::Unsigned: key = stappler::toString(f); state = State::DictValue; break;
	case cbor::MajorType::Negative: key = stappler::toString(-1 - f); state = State::DictValue; break;
	case cbor::MajorType::Simple:
		switch (cbor::SimpleValue(f)) {
		case cbor::SimpleValue::False: key = "false"; state = State::DictValue; break;
		case cbor::SimpleValue::True: key = "true"; state = State::DictValue; break;
		case cbor::SimpleValue::Null: key = "(null)"; state = State::DictValue; break;
		case cbor::SimpleValue::Undefined: key = "(undefined)"; state = State::DictValue; break;
		}
		break;
	default: break;
	}
}

void CborBuffer::flushValueDict(uint8_t t, uint8_t f) {
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
		}
		break;
	default: break;
	}
}

void CborBuffer::flushSequenceChars(data::Value &val, Reader &r, size_t s) {
	if (val._type != Value::Type::CHARSTRING) {
		val._type = Value::Type::CHARSTRING;
		val.strVal = new String();
	}
	val.strVal->append((const char *)r.data(), s);
}

void CborBuffer::flushSequenceBytes(data::Value &val, Reader &r, size_t s) {
	if (val._type != Value::Type::BYTESTRING) {
		val._type = Value::Type::BYTESTRING;
		val.bytesVal = new Bytes();
	}
	auto &bytes = *val.bytesVal;
	auto ns = bytes.size();
	bytes.resize(ns + s);
	memcpy(bytes.data() + ns, r.data(), s);
}

void CborBuffer::flushSequence(Reader &r, size_t s) {
	switch (state) {
	case State::Array:
	case State::DictValue:
	case State::Begin: {
		switch (literal) {
		case Literal::CharSequence:
			flushSequenceChars(*stack.back().first, r, s);
			break;
		case Literal::ByteSequence:
			flushSequenceBytes(*stack.back().first, r, s);
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
			key.append(base64::encode(r.data(), s));
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

bool CborBuffer::parse(Reader &r, size_t s) {
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
		remains = readInt(r, s);
		literal = Literal::Bytes;
		break;
	case Literal::CharSize:
		remains = readInt(r, s);
		literal = Literal::Chars;
		break;
	case Literal::ArraySize:
		stack.back().second = readInt(r, s);
		literal = Literal::None;
		state = State::Array;
		break;
	case Literal::DictSize:
		stack.back().second = readInt(r, s);
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

bool CborBuffer::readLiteral(Reader &r, bool tryWhole) {
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

	auto tmp = buf.get<Reader>();
	parse(tmp, tmp.size());
	return true;
}

bool CborBuffer::readSequenceHead(Reader &r) {
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

bool CborBuffer::readSequenceSize(Reader &r, bool tryWhole) {
	if (tryWhole && r >= remains) {
		remains = readInt(r, remains);
		sequence = Sequence::Value;
		return true;
	}

	auto len = min(remains, r.size());
	buf.put((const char *)r.data(), len);
	r += len; remains -= len;

	if (remains > 0) {
		return false;
	}

	auto tmp = buf.get<Reader>();
	remains = readInt(tmp, tmp.size());
	sequence = Sequence::Value;
	buf.clear();
	return true;
}

bool CborBuffer::readSequenceValue(Reader &r, bool tryWhole) {
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

	auto tmp = buf.get<Reader>();
	flushSequence(tmp, tmp.size());
	sequence = Sequence::Head;
	buf.clear();
	return true;
}

bool CborBuffer::readSequence(Reader &r, bool tryWhole) {
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

inline size_t CborBuffer::getLiteralSize(uint8_t f) {
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

CborBuffer::Literal CborBuffer::getLiteral(uint8_t t, uint8_t f) {
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
		return fullLiteral ? Literal::None : Literal::Float;
		break;
	case cbor::MajorType::Tag:
		return fullLiteral ? Literal::None : Literal::Tag;
		break;
	}
	return Literal::None;
}

void CborBuffer::pop() {
	stack.pop_back();
	if (stack.empty()) {
		state = State::End;
	} else {
		auto &back = stack.back().first;
		state = (back->isArray())?State::Array:State::DictKey;
	}
}

bool CborBuffer::readControl(Reader &r) {
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

	return true;
}

size_t CborBuffer::read(const uint8_t * s, size_t count) {
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
