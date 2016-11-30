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

#ifndef COMMON_STREAM_SPDATADECOMPRESSBUFFER_H_
#define COMMON_STREAM_SPDATADECOMPRESSBUFFER_H_

#include "SPDataReader.h"
#include "SPData.h"
#include "SPBuffer.h"

NS_SP_EXT_BEGIN(data)

class DecompressBuffer : public AllocBase {
public:
	DecompressBuffer() { }

	size_t read(const uint8_t * s, size_t count) { return count; }

	data::Value & data() { return const_cast<data::Value &>(data::Value::Null); }
	const data::Value & data() const { return data::Value::Null; }

protected:
    Buffer buf;
};

NS_SP_EXT_END(data)

#endif /* COMMON_STREAM_SPDATADECOMPRESSBUFFER_H_ */
