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
#include "simde/arm/neon.h"

namespace stappler::layout {

static void multiplyVec4_NEON (const Vec4 &a, const Vec4 &b, Vec4 &dst) {
	simde_vst1q_f32(&dst.x,
		simde_vmulq_f32(
			simde_vld1q_f32(&a.x),
			simde_vld1q_f32(&b.x)));
}

static void divideVec4_NEON (const Vec4 &a, const Vec4 &b, Vec4 &dst) {
#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	simde_vst1q_f32(&dst.x,
		vdivq_f32(
			simde_vld1q_f32(&a.x),
			simde_vld1q_f32(&b.x)));
#else
	// vdivq_f32 is not defied in simde, use SSE-based replacement
	divideVec4_SSE(a, b, dst);
#endif
}

[[maybe_unused]] static FunctionTable s_NeonFunctionTable = {
	multiplyVec4_NEON,
	divideVec4_NEON
};

}
