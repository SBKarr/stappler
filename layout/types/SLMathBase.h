/**
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2013-2014 Chukong Technologies
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef LAYOUT_TYPES_SLMATHBASE_H_
#define LAYOUT_TYPES_SLMATHBASE_H_

#include "SPLayout.h"

/**
 * @addtogroup base
 * @{
 */

NS_LAYOUT_BEGIN

constexpr float MATH_DEG_TO_RAD(float x) { return x * 0.0174532925f; }
constexpr float MATH_RAD_TO_DEG(float x) { return x * 57.29577951f; }

/**
@{ Util macro for const float such as epsilon, small float and float precision tolerance.
*/
constexpr float MATH_FLOAT_SMALL = 1.0e-37f;
constexpr float MATH_TOLERANCE = 2e-37f;
constexpr float MATH_PIOVER2 = 1.57079632679489661923f;
constexpr float MATH_EPSILON = 0.000001f;
/**@}*/

//#define MATH_PIOVER4                0.785398163397448309616f
//#define MATH_PIX2                   6.28318530717958647693f
//#define MATH_E                      2.71828182845904523536f
//#define MATH_LOG10E                 0.4342944819032518f
//#define MATH_LOG2E                  1.442695040888963387f
//#define MATH_PI                     3.14159265358979323846f
//#define MATH_RANDOM_MINUS1_1()      ((2.0f*((float)rand()/RAND_MAX))-1.0f)      // Returns a random float between -1 and 1.
//#define MATH_RANDOM_0_1()           ((float)rand()/RAND_MAX)                    // Returns a random float between 0 and 1.
//#define MATH_CLAMP(x, lo, hi)       ((x < lo) ? lo : ((x > hi) ? hi : x))
//#ifndef M_1_PI
//#define M_1_PI                      0.31830988618379067154

NS_LAYOUT_END

/**
 * end of base group
 * @}
 */

#endif // LAYOUT_TYPES_SLMATHBASE_H_
