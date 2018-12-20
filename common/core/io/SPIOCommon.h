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
