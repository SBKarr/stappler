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

NS_LAYOUT_BEGIN

inline const Mat4 Mat4::operator+(const Mat4& mat) const {
	Mat4 result(*this);
	result.add(mat);
	return result;
}

inline Mat4& Mat4::operator+=(const Mat4& mat) {
	add(mat);
	return *this;
}

inline const Mat4 Mat4::operator-(const Mat4& mat) const {
	Mat4 result(*this);
	result.subtract(mat);
	return result;
}

inline Mat4& Mat4::operator-=(const Mat4& mat) {
	subtract(mat);
	return *this;
}

inline const Mat4 Mat4::operator-() const {
	Mat4 mat(*this);
	mat.negate();
	return mat;
}

inline const Mat4 Mat4::operator*(const Mat4& mat) const {
	Mat4 result(*this);
	result.multiply(mat);
	return result;
}

inline Mat4& Mat4::operator*=(const Mat4& mat) {
	multiply(mat);
	return *this;
}

inline Vec3& operator*=(Vec3& v, const Mat4& m) {
	m.transformVector(&v);
	return v;
}

inline const Vec3 operator*(const Mat4& m, const Vec3& v) {
	Vec3 x;
	m.transformVector(v, &x);
	return x;
}

inline Vec4& operator*=(Vec4& v, const Mat4& m) {
	m.transformVector(&v);
	return v;
}

inline const Vec4 operator*(const Mat4& m, const Vec4& v) {
	Vec4 x;
	m.transformVector(v, &x);
	return x;
}


inline bool Mat4::operator==(const Mat4 &t) const {
	return memcmp(this, &t, sizeof(Mat4)) == 0;

}
inline bool Mat4::operator!=(const Mat4 &t) const {
	return memcmp(this, &t, sizeof(Mat4)) != 0;
}

inline std::basic_ostream<char> &
operator << (std::basic_ostream<char> & os, const Mat4 & m) {
	os << "{\n\t( " << m.m[0] << ", " << m.m[4] << ", " << m.m[8] << ", " << m.m[12] << ")\n"
		  << "\t( " << m.m[1] << ", " << m.m[5] << ", " << m.m[9] << ", " << m.m[13] << ")\n"
		  << "\t( " << m.m[2] << ", " << m.m[6] << ", " << m.m[10] << ", " << m.m[14] << ")\n"
		  << "\t( " << m.m[3] << ", " << m.m[7] << ", " << m.m[11] << ", " << m.m[15] << ")\n"
		<< "}";
	return os;
}

NS_LAYOUT_END
