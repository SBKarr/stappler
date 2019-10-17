#ifndef __CC_MATH_H__
#define __CC_MATH_H__

#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Mat4.h"
#include "Quaternion.h"

NS_CC_BEGIN


constexpr float MATH_DEG_TO_RAD(float x) { return x * 0.0174532925f; }
constexpr float MATH_RAD_TO_DEG(float x) { return x * 57.29577951f; }

constexpr float MATH_FLOAT_SMALL = 1.0e-37f;
constexpr float MATH_TOLERANCE = 2e-37f;
constexpr float MATH_PIOVER2 = 1.57079632679489661923f;
constexpr float MATH_EPSILON = 0.000001f;

NS_CC_END

#endif
