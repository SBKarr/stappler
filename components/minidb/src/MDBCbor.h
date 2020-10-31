/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_MINIDB_SRC_MDBCBOR_H_
#define COMPONENTS_MINIDB_SRC_MDBCBOR_H_

#include "MDB.h"

NS_MDB_BEGIN

namespace cbor {

static constexpr size_t CBOR_STACK_DEFAULT_SIZE = 8; // Preallocated iterator stack

using MajorType = stappler::data::cbor::MajorType;
using SimpleValue = stappler::data::cbor::SimpleValue;
using Flags = stappler::data::cbor::Flags;
using MajorTypeEncoded = stappler::data::cbor::MajorTypeEncoded;
using Tag = stappler::data::cbor::Tag;

extern uint32_t CborHeaderSize;
extern const uint8_t CborHeaderData[];

bool data_is_cbor(const uint8_t *, uint32_t);

struct Data {
	uint32_t size;
	const uint8_t * ptr;

	bool empty() const { return size != 0; }

	bool offset(uint32_t s) {
		if (size > s) {
			ptr += s;
			size -= s;
			return true;
		}

		size = 0;
		return false;
	}

	uint8_t getUnsigned() const;
	uint16_t getUnsigned16() const;
	uint32_t getUnsigned32() const;
	uint64_t getUnsigned64() const;
	float getFloat16() const;
	float getFloat32() const;
	double getFloat64() const;
	uint64_t getUnsignedValue(uint8_t hint) const;
	uint64_t readUnsignedValue(uint8_t hint);
};

enum class IteratorToken {
	Done,
	Key,
	Value,
	BeginArray,
	EndArray,
	BeginObject,
	EndObject,
	BeginByteStrings,
	EndByteStrings,
	BeginCharStrings,
	EndCharStrings,
};

enum class StackType {
	None,
	Array,
	Object,
	CharString,
	ByteString,
};

enum class Type {
	Unsigned,
	Negative,
	ByteString,
	CharString,
	Array,
	Map,
	Tag,
	Simple,
	Float,
	True,
	False,
	Null,
	Undefined,
	Unknown,
} CborType;

struct IteratorStackValue {
	StackType type;
	uint32_t position;
	uint32_t count;
	const uint8_t *ptr;
};

struct IteratorContext {
	using CborIteratorPathCallback = const Data (*) (void *);

	/* Container being iterated */
	Data current;

	uint8_t type;
	uint8_t info;

	bool isStreaming;
	IteratorToken token;
	const uint8_t *value;

	uint32_t objectSize;

	uint32_t stackSize;
	uint32_t stackCapacity;
	IteratorStackValue *currentStack;
	IteratorStackValue *stackHead;

	IteratorStackValue *extendedStack; // palloc'ed stack
	IteratorStackValue defaultStack[CBOR_STACK_DEFAULT_SIZE]; // preallocated stack

	bool init(const uint8_t *, size_t);
	void finalize();
	void reset();

	IteratorToken next();

	Type getType() const;
	StackType getContainerType() const;

	uint32_t getContainerSize() const;
	uint32_t getContainerPosition() const;
	uint32_t getObjectSize() const;

	int64_t getInteger() const;
	uint64_t getUnsigned() const;
	double getFloat() const;
	const char *getCharPtr() const;
	const uint8_t *getBytePtr() const;

	/** returns pointer of current value beginning within data block
	 * useful for value extraction
	 */
	const uint8_t *getCurrentValuePtr() const;

	/* read until the end of current value, returns pointer to the next value
	 * useful for value extraction
	 *
	 * data between ( CborIteratorGetCurrentValuePtr(iter), CborIteratorReadCurrentValue(iter) )
	 * should be valid CBOR without header */
	const uint8_t *readCurrentValue();

	/** Stop at i-th value in array. Iterator should be stopped at CborIteratorTokenBeginArray */
	bool getIth(size_t lindex);

	/** Stop at value with specific object key. Iterator should be stopped at CborIteratorTokenBeginObject */
	bool getKey(const char *, uint32_t);

	/** Stop iterator at value, defined by path (e.g. { "objKey", "42", "valueKey" })
	 *  Generic callback variant: callback mast return CborData { 0, NULL } to stop */
	bool path(CborIteratorPathCallback, void *ptr);

	/** Stop iterator at value, defined by path (e.g. "objKey", "42", "valueKey")
	 *  null-terminated strings variant: it can be counted or null-terminated list
	 *  if list is null-terminated, npath should be INT_MAX */
	bool pathStrings(const char **, int npath);
};

}

NS_MDB_END

#endif /* COMPONENTS_MINIDB_SRC_MDBCBOR_H_ */
