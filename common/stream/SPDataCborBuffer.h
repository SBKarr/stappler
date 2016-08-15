/*
 * SPCborBuffer.h
 *
 *  Created on: 3 янв. 2016 г.
 *      Author: sbkarr
 */

#ifndef COMMON_STREAM_SPDATACBORBUFFER_H_
#define COMMON_STREAM_SPDATACBORBUFFER_H_

#include "SPData.h"
#include "SPTransferBuffer.h"
#include "SPBuffer.h"
#include "SPDataReader.h"

NS_SP_EXT_BEGIN(data)

class CborBuffer : public AllocBase {
public:
	using Reader = DataReader<ByteOrder::Network>;

	CborBuffer(size_t block = Buffer::defsize) : buf(block) { }

	CborBuffer(CborBuffer &&);
	CborBuffer & operator = (CborBuffer &&);

	CborBuffer(const CborBuffer &) = delete;
	CborBuffer & operator = (const CborBuffer &) = delete;

	data::Value & data() { return root; }
	const data::Value & data() const { return root; }

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

	data::Value &emplaceArray();
	data::Value &emplaceDict();

	void emplaceInt(data::Value &d, int64_t value);
	void emplaceBool(data::Value &d, bool value);
	void emplaceDouble(data::Value &d, double value);
	void emplaceCharString(data::Value &d, Reader &r, size_t size);
	void emplaceByteString(data::Value &d, Reader &r, size_t size);

	void writeArray(data::Value &val, size_t s);
	void writeDict(data::Value &val, size_t s);
	void pop();

	size_t getLiteralSize(uint8_t f);
	Literal getLiteral(uint8_t t, uint8_t f);

	uint64_t readInt(Reader &r, size_t s);
	double readFloat(Reader &r, size_t s);

	void flush(data::Value &, Reader &r, size_t s);
	void flushKey(Reader &r, size_t s);
	void flush(Reader &r, size_t s);
	void flushValueRoot(uint8_t t, uint8_t f);
	void flushValueArray(uint8_t t, uint8_t f);
	void flushValueKey(uint8_t t, uint8_t f);
	void flushValueDict(uint8_t t, uint8_t f);

	void flushSequenceChars(data::Value &, Reader &, size_t);
	void flushSequenceBytes(data::Value &, Reader &, size_t);
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
    Buffer buf;
    data::Value root;

	Vector<Pair<Value *, size_t>> stack; // value, value size
	size_t remains = 3; // head size

	State state = State::Begin;
	Literal literal = Literal::Head;
	Sequence sequence = Sequence::None;
	String key;
};

NS_SP_EXT_END(data)

#endif /* COMMON_STREAM_SPDATACBORBUFFER_H_ */
