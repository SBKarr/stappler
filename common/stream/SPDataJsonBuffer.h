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

#ifndef COMMON_UTILS_SPDATAJSONSTREAM_H_
#define COMMON_UTILS_SPDATAJSONSTREAM_H_

#include "SPData.h"
#include "SPTransferBuffer.h"
#include "SPBuffer.h"

NS_SP_EXT_BEGIN(data)

class JsonBuffer : public AllocBase {
public:
	using Reader = CharReaderBase;

	JsonBuffer(size_t block = Buffer::defsize) : buf(block) { }

	JsonBuffer(JsonBuffer &&);
	JsonBuffer & operator = (JsonBuffer &&);

	JsonBuffer(const JsonBuffer &) = delete;
	JsonBuffer & operator = (const JsonBuffer &) = delete;

	data::Value & data() { return root; }
	const data::Value & data() const { return root; }

	size_t read(const uint8_t * s, size_t count);
	void clear();

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

	void reset();
	data::Value &emplaceArray();
	data::Value &emplaceDict();

	void writeString(data::Value &val, const Reader &r);
	void writeNumber(data::Value &val, double d);
	void writePlain(data::Value &val, const Reader &r);
	void writeArray(data::Value &val);
	void writeDict(data::Value &val);

	void flushString(const Reader &);
	void flushNumber(const Reader &);
	void flushPlain(const Reader &);

	bool parseString(Reader &r, bool tryWhole);
	bool parseNumber(Reader &r, bool tryWhole);
	bool parsePlain(Reader &r, bool tryWhole);

	void pushArray(bool r);
	void pushDict(bool r);

	Literal getLiteral(char);
	bool readLiteral(Reader &, bool tryWhole);
	bool beginLiteral(Reader &, Literal l);

	void pop();

protected:
    Buffer buf;
    data::Value root;

	data::Value *back = nullptr;
	Vector<Value *> stack;

	State state = State::None;
	Literal literal = Literal::None;
	String key;
};

NS_SP_EXT_END(data)

#endif /* COMMON_UTILS_SPDATAJSONSTREAM_H_ */
