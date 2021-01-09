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

#ifndef COMPONENTS_XENOLITH_CORE_DIRECTOR_XLDRAWSCHEME_H_
#define COMPONENTS_XENOLITH_CORE_DIRECTOR_XLDRAWSCHEME_H_

#include "XLDraw.h"

namespace stappler::xenolith::draw {

struct DrawScheme : memory::AllocPool {
	static DrawScheme *create();
	static void destroy(DrawScheme *);

	memory::pool_t *pool = nullptr;
	draw::CommandGroup *group = nullptr;

	draw::BufferHandle *draw = nullptr; // not binded
	draw::BufferHandle *drawCount = nullptr; // not binded

	draw::BufferHandle *index = nullptr; // index binding

	draw::BufferHandle *data = nullptr; // uniforms[0] : draw::DrawData
	draw::BufferHandle *transforms = nullptr; // uniforms[1] : layout::Mat4

	memory::map<draw::VertexBufferFormat, draw::BufferHandle *> vertex;
};

}

#endif /* COMPONENTS_XENOLITH_CORE_DIRECTOR_XLDRAWSCHEME_H_ */