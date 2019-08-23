// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPCommon.h"
#include "SPIO.h"
#include "SPBuffer.h"

NS_SP_EXT_BEGIN(io)

size_t Producer::read(const Buffer &buf, size_t nbytes) const {
	auto pbuf = buf.prepare(nbytes);
	auto size = read_ptr(ptr, pbuf, nbytes);
	buf.save(pbuf, nbytes, size);
	return size;
}

size_t Consumer::write(const Buffer &buf) const {
	return write_ptr(ptr, buf.data(), buf.size());
}

size_t read(const Producer &from, const Function<void(const Buffer &)> &f) {
	StackBuffer<(size_t)1_KiB> buf;
	size_t ret = 0;
	size_t cap = buf.capacity();
	size_t c = cap;
	while (cap == c) {
		c = from.read(buf, cap);
		if (c > 0) {
			ret += c;
			if (f) {
				f(buf);
			}
		}
	}
	return ret;
}

size_t read(const Producer &from, const Buffer &buf, const Function<void(const Buffer &)> &f) {
	size_t ret = 0;
	size_t cap = buf.capacity();
	size_t c = cap;
	while (cap == c) {
		c = from.read(buf, cap);
		if (c > 0) {
			ret += c;
			if (f) {
				f(buf);
			}
		}
	}
	return ret;
}

size_t read(const Producer &from, const Consumer &to) {
	StackBuffer<(size_t)1_KiB> buf;
	size_t ret = 0;
	size_t cap = buf.capacity();
	size_t c = cap;
	while (cap == c) {
		c = from.read(buf, cap);
		if (c > 0) {
			ret += c;
			to.write(buf);
		}
	}
	return ret;
}
size_t read(const Producer &from, const Consumer &to, const Function<void(const Buffer &)> &f) {
	StackBuffer<(size_t)1_KiB> buf;
	return io::read(from, to, buf, f);
}
size_t read(const Producer &from, const Consumer &to, const Buffer & buf) {
	size_t ret = 0;
	size_t cap = buf.capacity();
	size_t c = cap;
	while (cap == c) {
		c = from.read(buf, cap);
		if (c > 0) {
			ret += c;
			to.write(buf);
		}
	}
	return ret;
}
size_t read(const Producer &from, const Consumer &to, const Buffer & buf, const Function<void(const Buffer &)> &f) {
	size_t ret = 0;
	size_t cap = buf.capacity();
	size_t c = cap;
	while (cap == c) {
		c = from.read(buf, cap);
		if (c > 0) {
			ret += c;
			if (f) {
				f(buf);
			}
			to.write(buf);
		}
	}
	return ret;
}

NS_SP_EXT_END(io)
