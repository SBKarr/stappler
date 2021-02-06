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

#ifndef COMPONENTS_XENOLITH_CORE_DRAW_XLDRAW_H_
#define COMPONENTS_XENOLITH_CORE_DRAW_XLDRAW_H_

#include "XLDefine.h"

namespace stappler::xenolith::draw {

// Designed to use with SSBO and std140
struct alignas(16) Vertex_V4F_C4F_T2F {
	alignas(16) Vec4 pos;
	alignas(16) Color4F color;
	alignas(16) Vec2 tex;
};

struct Triangle_V3F_C4F_T2F {
	Vertex_V4F_C4F_T2F a;
	Vertex_V4F_C4F_T2F b;
	Vertex_V4F_C4F_T2F c;
};

struct Quad_V3F_C4F_T2F {
	Vertex_V4F_C4F_T2F bl;
	Vertex_V4F_C4F_T2F br;
	Vertex_V4F_C4F_T2F tl;
	Vertex_V4F_C4F_T2F tr;
};

}

#endif /* COMPONENTS_XENOLITH_CORE_DRAW_XLDRAW_H_ */
