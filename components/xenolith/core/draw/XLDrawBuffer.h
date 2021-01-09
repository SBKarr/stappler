/**
 Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_XENOLITH_CORE_DRAW_XLDRAWBUFFER_H_
#define COMPONENTS_XENOLITH_CORE_DRAW_XLDRAWBUFFER_H_

#include "XLDraw.h"

namespace stappler::xenolith::draw {

using BufferHandleGlAcquire = void (*) (void *, const Callback<void(BytesView)> &);

struct BufferHandleGl {
	void *glHandle;
	BufferHandleGlAcquire acquire;
};

struct BufferHandle : memory::AllocPool {
	static BufferHandle *create(memory::pool_t *, size_t reserve = 0); // with data allocated from pool

	BufferHandleType type = BufferHandleType::Data;
	void *handle = nullptr;

	bool append(BytesView);

	void acquireData(const Callback<void(BytesView)> &);
	void setGl(void *, BufferHandleGlAcquire);
};

}

#endif /* COMPONENTS_XENOLITH_CORE_DRAW_XLDRAWBUFFER_H_ */
