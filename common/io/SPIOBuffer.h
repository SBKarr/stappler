/*
 * SPIOBuffer.h
 *
 *  Created on: 3 апр. 2016 г.
 *      Author: sbkarr
 */

#ifndef COMMON_IO_SPIOBUFFER_H_
#define COMMON_IO_SPIOBUFFER_H_

#include "SPIOCommon.h"

NS_SP_EXT_BEGIN(io)

struct Buffer {
	template <typename T, typename Traits = BufferTraits<T>> Buffer(T &t);

	uint8_t * prepare(size_t & size) const;
	void save(uint8_t *, size_t source, size_t nbytes) const;

	size_t capacity() const;
	size_t size() const;
	uint8_t *data() const;
	void clear() const;

	void *ptr = nullptr;
	prepare_fn prepare_ptr = nullptr;
	save_fn save_ptr = nullptr;
	size_fn size_ptr = nullptr;
	size_fn capacity_ptr = nullptr;
	data_fn data_ptr = nullptr;
	clear_fn clear_ptr = nullptr;
};

template <typename T, typename Traits>
inline Buffer::Buffer(T &t)
: ptr((void *)&t)
, prepare_ptr(&Traits::PrepareFn)
, save_ptr(&Traits::SaveFn)
, size_ptr(&Traits::SizeFn)
, capacity_ptr(&Traits::CapacityFn)
, data_ptr(&Traits::DataFn)
, clear_ptr(&Traits::ClearFn) { }

// reserve memory block in buffer
inline uint8_t * Buffer::prepare(size_t & size) const { return prepare_ptr(ptr, size); }
inline void Buffer::save(uint8_t *buf, size_t source, size_t nbytes) const { save_ptr(ptr, buf, source, nbytes); }
inline size_t Buffer::capacity() const { return capacity_ptr(ptr); }
inline size_t Buffer::size() const { return size_ptr(ptr); }
inline uint8_t *Buffer::data() const { return data_ptr(ptr); }
inline void Buffer::clear() const { clear_ptr(ptr); }

NS_SP_EXT_END(io)

#endif /* COMMON_IO_SPIOBUFFER_H_ */
