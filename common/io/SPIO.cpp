/*
 * SPIO.cpp
 *
 *  Created on: 31 марта 2016 г.
 *      Author: sbkarr
 */

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

size_t read(const Producer &from, const Consumer &to) {
	StackBuffer<(size_t)1_KiB> buf;
	return io::read(from, to, buf, nullptr);
}
size_t read(const Producer &from, const Consumer &to, const Function<void(const Buffer &)> &f) {
	StackBuffer<(size_t)1_KiB> buf;
	return io::read(from, to, buf, f);
}
size_t read(const Producer &from, const Consumer &to, const Buffer & buf) {
	return io::read(from, to, buf, nullptr);
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
