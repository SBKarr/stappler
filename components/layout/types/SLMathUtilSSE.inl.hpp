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

NS_LAYOUT_BEGIN

#ifdef __SSE__

void MathUtil::addMatrix(const __m128 m[4], float scalar, __m128 dst[4]) {
	__m128 s = _mm_set1_ps(scalar);
	dst[0] = _mm_add_ps(m[0], s);
	dst[1] = _mm_add_ps(m[1], s);
	dst[2] = _mm_add_ps(m[2], s);
	dst[3] = _mm_add_ps(m[3], s);
}

void MathUtil::addMatrix(const __m128 m1[4], const __m128 m2[4], __m128 dst[4]) {
	dst[0] = _mm_add_ps(m1[0], m2[0]);
	dst[1] = _mm_add_ps(m1[1], m2[1]);
	dst[2] = _mm_add_ps(m1[2], m2[2]);
	dst[3] = _mm_add_ps(m1[3], m2[3]);
}

void MathUtil::subtractMatrix(const __m128 m1[4], const __m128 m2[4], __m128 dst[4]) {
	dst[0] = _mm_sub_ps(m1[0], m2[0]);
	dst[1] = _mm_sub_ps(m1[1], m2[1]);
	dst[2] = _mm_sub_ps(m1[2], m2[2]);
	dst[3] = _mm_sub_ps(m1[3], m2[3]);
}

void MathUtil::multiplyMatrix(const __m128 m[4], float scalar, __m128 dst[4]) {
	__m128 s = _mm_set1_ps(scalar);
	dst[0] = _mm_mul_ps(m[0], s);
	dst[1] = _mm_mul_ps(m[1], s);
	dst[2] = _mm_mul_ps(m[2], s);
	dst[3] = _mm_mul_ps(m[3], s);
}

void MathUtil::multiplyMatrix(const __m128 m1[4], const __m128 m2[4], __m128 dst[4]) {
	__m128 dst0, dst1, dst2, dst3;
	{
		__m128 e0 = _mm_shuffle_ps(m2[0], m2[0], _MM_SHUFFLE(0, 0, 0, 0));
		__m128 e1 = _mm_shuffle_ps(m2[0], m2[0], _MM_SHUFFLE(1, 1, 1, 1));
		__m128 e2 = _mm_shuffle_ps(m2[0], m2[0], _MM_SHUFFLE(2, 2, 2, 2));
		__m128 e3 = _mm_shuffle_ps(m2[0], m2[0], _MM_SHUFFLE(3, 3, 3, 3));

		__m128 v0 = _mm_mul_ps(m1[0], e0);
		__m128 v1 = _mm_mul_ps(m1[1], e1);
		__m128 v2 = _mm_mul_ps(m1[2], e2);
		__m128 v3 = _mm_mul_ps(m1[3], e3);

		__m128 a0 = _mm_add_ps(v0, v1);
		__m128 a1 = _mm_add_ps(v2, v3);
		__m128 a2 = _mm_add_ps(a0, a1);

		dst0 = a2;
	}

	{
		__m128 e0 = _mm_shuffle_ps(m2[1], m2[1], _MM_SHUFFLE(0, 0, 0, 0));
		__m128 e1 = _mm_shuffle_ps(m2[1], m2[1], _MM_SHUFFLE(1, 1, 1, 1));
		__m128 e2 = _mm_shuffle_ps(m2[1], m2[1], _MM_SHUFFLE(2, 2, 2, 2));
		__m128 e3 = _mm_shuffle_ps(m2[1], m2[1], _MM_SHUFFLE(3, 3, 3, 3));

		__m128 v0 = _mm_mul_ps(m1[0], e0);
		__m128 v1 = _mm_mul_ps(m1[1], e1);
		__m128 v2 = _mm_mul_ps(m1[2], e2);
		__m128 v3 = _mm_mul_ps(m1[3], e3);

		__m128 a0 = _mm_add_ps(v0, v1);
		__m128 a1 = _mm_add_ps(v2, v3);
		__m128 a2 = _mm_add_ps(a0, a1);

		dst1 = a2;
	}

	{
		__m128 e0 = _mm_shuffle_ps(m2[2], m2[2], _MM_SHUFFLE(0, 0, 0, 0));
		__m128 e1 = _mm_shuffle_ps(m2[2], m2[2], _MM_SHUFFLE(1, 1, 1, 1));
		__m128 e2 = _mm_shuffle_ps(m2[2], m2[2], _MM_SHUFFLE(2, 2, 2, 2));
		__m128 e3 = _mm_shuffle_ps(m2[2], m2[2], _MM_SHUFFLE(3, 3, 3, 3));

		__m128 v0 = _mm_mul_ps(m1[0], e0);
		__m128 v1 = _mm_mul_ps(m1[1], e1);
		__m128 v2 = _mm_mul_ps(m1[2], e2);
		__m128 v3 = _mm_mul_ps(m1[3], e3);

		__m128 a0 = _mm_add_ps(v0, v1);
		__m128 a1 = _mm_add_ps(v2, v3);
		__m128 a2 = _mm_add_ps(a0, a1);

		dst2 = a2;
	}

	{
		__m128 e0 = _mm_shuffle_ps(m2[3], m2[3], _MM_SHUFFLE(0, 0, 0, 0));
		__m128 e1 = _mm_shuffle_ps(m2[3], m2[3], _MM_SHUFFLE(1, 1, 1, 1));
		__m128 e2 = _mm_shuffle_ps(m2[3], m2[3], _MM_SHUFFLE(2, 2, 2, 2));
		__m128 e3 = _mm_shuffle_ps(m2[3], m2[3], _MM_SHUFFLE(3, 3, 3, 3));

		__m128 v0 = _mm_mul_ps(m1[0], e0);
		__m128 v1 = _mm_mul_ps(m1[1], e1);
		__m128 v2 = _mm_mul_ps(m1[2], e2);
		__m128 v3 = _mm_mul_ps(m1[3], e3);

		__m128 a0 = _mm_add_ps(v0, v1);
		__m128 a1 = _mm_add_ps(v2, v3);
		__m128 a2 = _mm_add_ps(a0, a1);

		dst3 = a2;
	}
	dst[0] = dst0;
	dst[1] = dst1;
	dst[2] = dst2;
	dst[3] = dst3;
}

void MathUtil::negateMatrix(const __m128 m[4], __m128 dst[4]) {
	__m128 z = _mm_setzero_ps();
	dst[0] = _mm_sub_ps(z, m[0]);
	dst[1] = _mm_sub_ps(z, m[1]);
	dst[2] = _mm_sub_ps(z, m[2]);
	dst[3] = _mm_sub_ps(z, m[3]);
}

void MathUtil::transposeMatrix(const __m128 m[4], __m128 dst[4]) {
	__m128 tmp0 = _mm_shuffle_ps(m[0], m[1], 0x44);
	__m128 tmp2 = _mm_shuffle_ps(m[0], m[1], 0xEE);
	__m128 tmp1 = _mm_shuffle_ps(m[2], m[3], 0x44);
	__m128 tmp3 = _mm_shuffle_ps(m[2], m[3], 0xEE);

	dst[0] = _mm_shuffle_ps(tmp0, tmp1, 0x88);
	dst[1] = _mm_shuffle_ps(tmp0, tmp1, 0xDD);
	dst[2] = _mm_shuffle_ps(tmp2, tmp3, 0x88);
	dst[3] = _mm_shuffle_ps(tmp2, tmp3, 0xDD);
}

void MathUtil::transformVec4(const __m128 m[4], const __m128& v, __m128& dst) {
	__m128 col1 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
	__m128 col2 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
	__m128 col3 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
	__m128 col4 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3));

	dst = _mm_add_ps(
			_mm_add_ps(_mm_mul_ps(m[0], col1), _mm_mul_ps(m[1], col2)),
			_mm_add_ps(_mm_mul_ps(m[2], col3), _mm_mul_ps(m[3], col4))
	);
}

void MathUtil::multiplyVec4(const __m128 &a, const __m128 &b, __m128& dst) {
	dst = _mm_mul_ps(a, b);
}

void MathUtil::divideVec4(const __m128 &a, const __m128 &b, __m128& dst) {
	dst = _mm_div_ps(a, b);
}


#endif

NS_LAYOUT_END
