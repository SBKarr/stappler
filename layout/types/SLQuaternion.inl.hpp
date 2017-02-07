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

#include "SLQuaternion.h"

NS_LAYOUT_BEGIN

inline const Quaternion Quaternion::operator*(const Quaternion& q) const {
	Quaternion result(*this);
	result.multiply(q);
	return result;
}

inline Quaternion& Quaternion::operator*=(const Quaternion& q) {
	multiply(q);
	return *this;
}

inline Vec3 Quaternion::operator*(const Vec3& v) const {
	Vec3 uv, uuv;
	Vec3 qvec(x, y, z);
	Vec3::cross(qvec, v, &uv);
	Vec3::cross(qvec, uv, &uuv);

	uv *= (2.0f * w);
	uuv *= 2.0f;

	return v + uv + uuv;
}

NS_LAYOUT_END
