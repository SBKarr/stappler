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

#ifndef STAPPLER_SRC_CORE_STRUCT_SPHALFFLOAT_H_
#define STAPPLER_SRC_CORE_STRUCT_SPHALFFLOAT_H_

#include "SPCommon.h"

NS_SP_EXT_BEGIN(halffloat)

// see https://en.wikipedia.org/wiki/Half_precision_floating-point_format

constexpr uint16_t nan() { return (uint16_t)0x7e00; }
constexpr uint16_t posinf() { return (uint16_t)(31 << 10); }
constexpr uint16_t neginf() { return (uint16_t)(63 << 10); }

float decode(uint16_t half);
uint16_t encode(float val);


NS_SP_EXT_END(halffloat)

#endif /* STAPPLER_SRC_CORE_STRUCT_SPHALFFLOAT_H_ */
