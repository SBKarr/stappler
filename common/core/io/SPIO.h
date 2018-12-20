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

#ifndef COMMON_IO_SPIO_H_
#define COMMON_IO_SPIO_H_

#include "SPCommon.h"
#include "SPIOCommon.h"
#include "SPIOConsumer.h"
#include "SPIOProducer.h"
#include "SPIOBuffer.h"
#include "SPBuffer.h"

NS_SP_EXT_BEGIN(io)

size_t read(const Producer &from, const Function<void(const Buffer &)> &);
size_t read(const Producer &from, const Buffer &, const Function<void(const Buffer &)> &);
size_t read(const Producer &from, const Consumer &to);
size_t read(const Producer &from, const Consumer &to, const Function<void(const Buffer &)> &);
size_t read(const Producer &from, const Consumer &to, const Buffer &);
size_t read(const Producer &from, const Consumer &to, const Buffer &, const Function<void(const Buffer &)> &);

template <typename T>
inline size_t tread(const Producer &from, const T &f) {
	StackBuffer<(size_t)1_KiB> buf;
	size_t ret = 0;
	size_t cap = buf.capacity();
	size_t c = cap;
	while (cap == c) {
		c = from.read(buf, cap);
		if (c > 0) {
			ret += c;
			f(buf);
		}
	}
	return ret;
}

template <typename T>
inline size_t tread(const Producer &from, const Buffer &buf, const T &f) {
	size_t ret = 0;
	size_t cap = buf.capacity();
	size_t c = cap;
	while (cap == c) {
		c = from.read(buf, cap);
		if (c > 0) {
			ret += c;
			f(buf);
		}
	}
	return ret;
}

template <typename T>
inline size_t tread(const Producer &from, const Consumer &to, const T &f) {
	StackBuffer<(size_t)1_KiB> buf;
	size_t ret = 0;
	size_t cap = buf.capacity();
	size_t c = cap;
	while (cap == c) {
		c = from.read(buf, cap);
		if (c > 0) {
			ret += c;
			f(buf);
			to.write(buf);
		}
	}
	return ret;
}

template <typename T>
inline size_t tread(const Producer &from, const Consumer &to, const Buffer & buf, const T &f) {
	size_t ret = 0;
	size_t cap = buf.capacity();
	size_t c = cap;
	while (cap == c) {
		c = from.read(buf, cap);
		if (c > 0) {
			ret += c;
			f(buf);
			to.write(buf);
		}
	}
	return ret;
}

NS_SP_EXT_END(io)

#endif /* COMMON_IO_SPIO_H_ */
