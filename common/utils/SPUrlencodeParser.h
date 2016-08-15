/*
 * SPUrlencodeParser.h
 *
 *  Created on: 9 июня 2016 г.
 *      Author: sbkarr
 */

#ifndef COMMON_UTILS_SPURLENCODEPARSER_H_
#define COMMON_UTILS_SPURLENCODEPARSER_H_

#include "SPData.h"
#include "SPCharReader.h"
#include "SPBuffer.h"

NS_SP_BEGIN

class UrlencodeParser : public AllocBase {
public:
	using Reader = CharReaderBase;

	enum class Literal {
		None,
		String,
		Percent,
		Open,
		Close,
		Delim,
	};

	enum class VarState {
		Key,
		SubKey,
		SubKeyEnd,
		Value,
		End,
	};

	UrlencodeParser(data::Value &target, size_t length = maxOf<size_t>(), size_t maxVarSize = maxOf<size_t>());

	size_t read(const uint8_t * s, size_t count);

	data::Value *flushString(CharReaderBase &r, data::Value *, VarState state);

	data::Value & data() { return *target; }
	const data::Value & data() const { return *target; }

protected:
	void bufferize(Reader &r);
	void bufferize(char c);
	void flush(Reader &r);

	data::Value *target;
	size_t length = maxOf<size_t>();
	size_t maxVarSize = maxOf<size_t>();

	bool skip = false;
	VarState state = VarState::Key;
	Literal literal = Literal::None;

    Buffer buf;
    data::Value *current = nullptr;
};

NS_SP_END

#endif /* COMMON_UTILS_SPURLENCODEPARSER_H_ */
