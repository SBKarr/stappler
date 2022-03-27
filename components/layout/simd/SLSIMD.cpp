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

#include "SPLayout.h"

#include "SLSIMD.h"

#include "SLSIMD_SSE.cc"
#include "SLSIMD_NEON.cc"

namespace stappler::layout {

#if SL_DEFAULT_SIMD == SL_DEFAULT_SIMD_NEON
FunctionTable *LayoutFunctionTable = &s_NeonFunctionTable;
#else
FunctionTable *LayoutFunctionTable = &s_SseFunctionTable;
#endif

void initialize_simd() {
#if SL_DEFAULT_SIMD == SL_DEFAULT_SIMD_NEON
	LayoutFunctionTable = &s_NeonFunctionTable;
#else
	LayoutFunctionTable = &s_SseFunctionTable;
#endif
}

}
