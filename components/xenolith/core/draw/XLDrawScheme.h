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

using Buffer = memory::impl::mem_large<uint8_t, 0>;

struct DrawScheme : memory::AllocPool {
	static DrawScheme *create(memory::pool_t *);
	static void destroy(DrawScheme *);

	void pushDrawIndexed(CommandGroup *g, vk::Pipeline *, SpanView<Vertex_V4F_C4F_T2F> vertexes, SpanView<uint16_t> indexes);

	memory::pool_t *pool = nullptr;
	CommandGroup *group = nullptr;

	Buffer draw; // not binded
	Buffer index; // index binding

	Buffer data; // uniforms[0] : draw::DrawData
	Buffer transforms; // uniforms[1] : layout::Mat4

	memory::map<VertexFormat, Buffer> vertex; // buffer[N] : VertexFormat
};

}

#endif /* COMPONENTS_XENOLITH_CORE_DIRECTOR_XLDRAWSCHEME_H_ */
