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

#ifndef COMMON_UTILS_NETWORK_SPURLENCODEPARSER_H_
#define COMMON_UTILS_NETWORK_SPURLENCODEPARSER_H_

#include "SPData.h"
#include "SPStringView.h"
#include "SPBuffer.h"

NS_SP_BEGIN

class UrlencodeParser : public AllocBase {
public:
	using Reader = StringView;

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

	data::Value *flushString(StringView &r, data::Value *, VarState state);

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

#endif /* COMMON_UTILS_NETWORK_SPURLENCODEPARSER_H_ */
