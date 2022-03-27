/**
 Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SLSIMD.h"
#include "simde/x86/sse.h"

#if __SSE__
#define SL_SSE_STORE_VEC4(vec, value)	vec.v = (value)
#define SL_SSE_LOAD_VEC4(vec)			(vec.v)
#else
#define SL_SSE_STORE_VEC4(vec, value)	simde_mm_store_ps(&vec.x, value)
#define SL_SSE_LOAD_VEC4(vec)			simde_mm_load_ps(&vec.x)
#endif

namespace stappler::layout {

static void multiplyVec4_SSE (const Vec4 &a, const Vec4 &b, Vec4 &dst) {
	SL_SSE_STORE_VEC4(dst,
		simde_mm_mul_ps(
			SL_SSE_LOAD_VEC4(a),
			SL_SSE_LOAD_VEC4(b)));
}

static void divideVec4_SSE (const Vec4 &a, const Vec4 &b, Vec4 &dst) {
	SL_SSE_STORE_VEC4(dst,
		simde_mm_div_ps(
				SL_SSE_LOAD_VEC4(a),
				SL_SSE_LOAD_VEC4(b)));
}

[[maybe_unused]] static FunctionTable s_SseFunctionTable = {
	multiplyVec4_SSE,
	divideVec4_SSE
};

}
