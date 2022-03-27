/**
 Copyright 2013 BlackBerry Inc.
 Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 Original file from GamePlay3D: http://gameplay3d.org

 This file was modified to fit the cocos2d-x project
 */

#include "SLMat4.h"
#include "SLVec4.h"

#include "SLSIMD.h"

namespace stappler::layout::simd_inline {

#if SL_DEFAULT_SIMD == SL_DEFAULT_SIMD_NEON

static void multiplyVec4_Inline (const Vec4 &a, const Vec4 &b, Vec4 &dst) {
	simde_vst1q_f32(&dst.x,
		simde_vmulq_f32(
			simde_vld1q_f32(&a.x),
			simde_vld1q_f32(&b.x)));
}

static void multiplyVec4Scalar_Inline (const Vec4 &a, const float &b, Vec4 &dst) {
	simde_vst1q_f32(&dst.x,
		simde_vmulq_f32(
			simde_vld1q_f32(&a.x),
			simde_vld1q_dup_f32(&b)));
}

static void divideVec4_Inline (const Vec4 &a, const Vec4 &b, Vec4 &dst) {
#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	simde_vst1q_f32(&dst.x,
		vdivq_f32(
			simde_vld1q_f32(&a.x),
			simde_vld1q_f32(&b.x)));
#else
	// vdivq_f32 is not defied in simde, use SSE-based replacement
	SL_SSE_STORE_VEC4(dst,
		simde_mm_div_ps(
			SL_SSE_LOAD_VEC4(a),
			SL_SSE_LOAD_VEC4(b)));
#endif
}

#else

inline void multiplyVec4_Inline (const Vec4 &a, const Vec4 &b, Vec4 &dst) {
	SL_SSE_STORE_VEC4(dst,
		simde_mm_mul_ps(
			SL_SSE_LOAD_VEC4(a),
			SL_SSE_LOAD_VEC4(b)));
}

inline void multiplyVec4Scalar_Inline (const Vec4 &a, const float &b, Vec4 &dst) {
	SL_SSE_STORE_VEC4(dst,
		simde_mm_mul_ps(
			SL_SSE_LOAD_VEC4(a),
			simde_mm_load_ps1(&b)));
}

inline void divideVec4_Inline (const Vec4 &a, const Vec4 &b, Vec4 &dst) {
	SL_SSE_STORE_VEC4(dst,
		simde_mm_div_ps(
				SL_SSE_LOAD_VEC4(a),
				SL_SSE_LOAD_VEC4(b)));
}

#endif

}

NS_LAYOUT_BEGIN

inline const Vec4 Vec4::operator+(const Vec4& v) const {
	Vec4 result(*this);
	result.add(v);
	return result;
}

inline Vec4& Vec4::operator+=(const Vec4& v) {
	add(v);
	return *this;
}

inline const Vec4 Vec4::operator-(const Vec4& v) const {
	Vec4 result(*this);
	result.subtract(v);
	return result;
}

inline Vec4& Vec4::operator-=(const Vec4& v) {
	subtract(v);
	return *this;
}

inline const Vec4 Vec4::operator-() const {
	Vec4 result(*this);
	result.negate();
	return result;
}

inline const Vec4 Vec4::operator*(float s) const {
	Vec4 result(*this);
	result.scale(s);
	return result;
}

inline Vec4& Vec4::operator*=(float s) {
	scale(s);
	return *this;
}

inline const Vec4 Vec4::operator/(const float s) const {
	return Vec4(this->x / s, this->y / s, this->z / s, this->w / s);
}

inline bool Vec4::operator<(const Vec4& v) const {
	if (x == v.x) {
		if (y == v.y) {
			if (z < v.z) {
				if (w < v.w) {
					return w < v.w;
				}
			}
			return z < v.z;
		}
		return y < v.y;
	}
	return x < v.x;
}

inline bool Vec4::operator==(const Vec4& v) const {
	return x == v.x && y == v.y && z == v.z && w == v.w;
}

inline bool Vec4::operator!=(const Vec4& v) const {
	return x != v.x || y != v.y || z != v.z || w != v.w;
}

inline const Vec4 operator*(float x, const Vec4& v) {
	Vec4 result(v);
	result.scale(x);
	return result;
}

inline void Vec4::scale(float scalar) {
	simd_inline::multiplyVec4Scalar_Inline(*this, scalar, *this);
}

inline Vec4 operator*(const Vec4 &l, const Vec4 &r) {
	Vec4 dst;
	simd_inline::multiplyVec4_Inline(l, r, dst);
	return dst;
}

inline Vec4 operator/(const Vec4 &l, const Vec4 &r) {
	Vec4 dst;
	simd_inline::divideVec4_Inline(l, r, dst);
	return dst;
}

inline std::basic_ostream<char> &
operator << (std::basic_ostream<char> & os, const Vec4 & vec) {
	os << "(x: " << vec.x << "; y: " << vec.y << "; z: " << vec.z << "; w: " << vec.w << ")";
	return os;
}

NS_LAYOUT_END
