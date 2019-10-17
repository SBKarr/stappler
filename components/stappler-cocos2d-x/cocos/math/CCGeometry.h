/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2013-2014 Chukong Technologies

http://www.cocos2d-x.org

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
****************************************************************************/

#ifndef __MATH_CCGEOMETRY_H__
#define __MATH_CCGEOMETRY_H__

#include <math.h>

#include "platform/CCPlatformMacros.h"
#include "base/ccMacros.h"
#include "math/CCMath.h"

/**
 * @addtogroup base
 * @{
 */

NS_CC_BEGIN

using Size = stappler::layout::Size;
using Rect = stappler::layout::Rect;

inline Rect RectApplyTransform(const Rect& rect, const Mat4& transform) {
	return stappler::layout::TransformRect(rect, transform);
}

inline Vec2 PointApplyTransform(const Vec2& point, const Mat4& transform) {
	return stappler::layout::TransformPoint(point, transform);
}

NS_CC_END

// end of base group
/// @}

#endif // __MATH_CCGEOMETRY_H__
