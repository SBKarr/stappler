// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPCommon.h"

#ifndef SPAPR

NS_SP_EXT_BEGIN(data)

struct EncodingBuffer {
	static constexpr size_t InitialBufferSize() {
		return 1_KiB;
	}

	static void EncodingBufferDestructor(void *data) {
		if (data) {
			delete ((Bytes *)data);
		}
	}

	EncodingBuffer() {
	    pthread_key_create(&_key, &EncodingBufferDestructor);
	}

	Bytes *get() {
		Bytes *ret = (Bytes *)pthread_getspecific(_key);
		if (!ret) {
			ret = new Bytes();
			ret->reserve(InitialBufferSize());
			pthread_setspecific(_key, ret);
		}
		ret->clear();
		return ret;
	}

	pthread_key_t _key;
};

static EncodingBuffer s_buffer;

Bytes *acquireBuffer() {
	return s_buffer.get();
}

NS_SP_EXT_END(data)

#endif
