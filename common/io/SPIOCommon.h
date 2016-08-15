/*
 * SPIOCommon.h
 *
 *  Created on: 3 апр. 2016 г.
 *      Author: sbkarr
 */

#ifndef COMMON_IO_SPIOCOMMON_H_
#define COMMON_IO_SPIOCOMMON_H_

#include "SPCore.h"

NS_SP_EXT_BEGIN(io)

enum class Seek {
	Set,
	Current,
	End,
};

using read_fn = size_t (*) (void *, uint8_t *buf, size_t nbytes);
using seek_fn = size_t (*) (void *, int64_t offset, Seek);
using size_fn = size_t (*) (void *);

using write_fn = size_t (*) (void *, const uint8_t *buf, size_t nbytes);
using flush_fn = void (*) (void *);
using clear_fn = void (*) (void *);
using data_fn = uint8_t * (*) (void *);

using prepare_fn = uint8_t * (*) (void *, size_t &);
using save_fn = void (*) (void *, uint8_t *, size_t, size_t);

template <typename T> struct ProducerTraits;
template <typename T> struct ConsumerTraits;
template <typename T> struct BufferTraits;

struct Producer;
struct Consumer;
struct Buffer;

NS_SP_EXT_END(io)

#endif /* COMMON_IO_SPIOCOMMON_H_ */
