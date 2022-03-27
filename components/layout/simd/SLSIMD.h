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


#ifndef COMPONENTS_LAYOUT_SIMD_SLSIMD_H_
#define COMPONENTS_LAYOUT_SIMD_SLSIMD_H_

#include "SPCommon.h"

#define SL_DEFAULT_SIMD_SSE 1
#define SL_DEFAULT_SIMD_NEON 2

#if __SSE__
#define SL_DEFAULT_SIMD SL_DEFAULT_SIMD_SSE
#elif __arm__
#define SL_DEFAULT_SIMD SL_DEFAULT_SIMD_NEON
#elif __aarch64__
#define SL_DEFAULT_SIMD SL_DEFAULT_SIMD_NEON
#else
#define SL_DEFAULT_SIMD SL_DEFAULT_SIMD_SSE
#endif


#if __SSE__
#define SL_SSE_STORE_VEC4(vec, value)	vec.v = (value)
#define SL_SSE_LOAD_VEC4(vec)			(vec.v)
#else
#define SL_SSE_STORE_VEC4(vec, value)	simde_mm_store_ps(&vec.x, value)
#define SL_SSE_LOAD_VEC4(vec)			simde_mm_load_ps(&vec.x)
#endif


#if SL_DEFAULT_SIMD == SL_DEFAULT_SIMD_NEON
#include "simde/arm/neon.h"
#include "simde/x86/sse.h"
#else
#include "simde/x86/sse.h"
#endif


namespace stappler::layout {

class Vec4;

struct FunctionTable {
	void (*multiplyVec4) (const Vec4 &, const Vec4 &, Vec4 &);
	void (*divideVec4) (const Vec4 &, const Vec4 &, Vec4 &);
};

extern FunctionTable *LayoutFunctionTable;

// in future, this will load more optimal LayoutFunctionTable based on cpuid
// instead of default compile-time selection (simde-based)
// noop for now
void initialize_simd();

}

#endif /* COMPONENTS_LAYOUT_SIMD_SLSIMD_H_ */
