/*
 * SPDataBuffer.cpp
 *
 *  Created on: 9 дек. 2015 г.
 *      Author: sbkarr
 */

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
