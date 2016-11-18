/*
 * SPIO.h
 *
 *  Created on: 31 марта 2016 г.
 *      Author: sbkarr
 */

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
