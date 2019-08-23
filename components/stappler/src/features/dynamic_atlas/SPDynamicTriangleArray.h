/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef STAPPLER_SRC_FEATURES_DYNAMIC_ATLAS_SPDYNAMICTRIANGLEARRAY_H_
#define STAPPLER_SRC_FEATURES_DYNAMIC_ATLAS_SPDYNAMICTRIANGLEARRAY_H_

#include "SPFont.h"

NS_SP_BEGIN

struct V3F_C4B_T2F_Triangle {
	cocos2d::V3F_C4B_T2F a;
	cocos2d::V3F_C4B_T2F b;
	cocos2d::V3F_C4B_T2F c;
};

class DynamicTriangleArray : public Ref {
public:
	using iterator = Vector<V3F_C4B_T2F_Triangle>::iterator;
	using const_iterator = Vector<V3F_C4B_T2F_Triangle>::const_iterator;

public:
	void clear();
	void resize(size_t);
	void setCapacity(size_t);
	void shrinkToFit();

	void insert(const V3F_C4B_T2F_Triangle &);
	size_t emplace();

	void setTexturePoints(size_t index, const Vec2 &a, const Vec2 &b, const Vec2 &c, float texWidth, float texHeight);
	void setGeometry(size_t index, const Vec2 &a, const Vec2 &b, const Vec2 &c, float positionZ = 0.0f);
	void setColor(size_t index, const Color4B &color);
	void setColor(size_t index, Color4B colors[3]);

	void setColor3B(size_t index, const Color3B &color);
	void setOpacity(size_t index, const uint8_t &o);

	inline bool empty() const { return _vec.empty(); }

	inline size_t size() const { return _vec.size(); }
	inline size_t capacity() const { return _vec.capacity(); }

	inline const Vector<V3F_C4B_T2F_Triangle> &getVec() const { return _vec; }
	inline const V3F_C4B_T2F_Triangle *getData() const { return _vec.data(); }

	inline bool isCapacityDirty() const { return _savedCapacity != _vec.capacity(); }
	inline bool isSizeDirty() const { return _savedSize != _vec.size(); }
	inline bool isQuadsDirty() const { return _quadsDirty; }

	inline void setDirty() { _quadsDirty = true; }

	inline void dropDirty() {
		_savedCapacity = _vec.capacity();
		_savedSize = _vec.size();
		_quadsDirty = false;
	}

	void setTransform(const Mat4 &); // just set transform matrix
	void updateTransform(const Mat4 &); // set transform matrix and update verticles

	inline iterator begin() { return _vec.begin(); };
	inline iterator end() { return _vec.end(); };

	inline const_iterator begin() const { return _vec.begin(); };
	inline const_iterator end() const { return _vec.end(); };

protected:
	Vector<V3F_C4B_T2F_Triangle> _vec;

	size_t _savedSize = 0;
	size_t _savedCapacity = 0;
	bool _quadsDirty = true;
	Mat4 _transform = Mat4::IDENTITY;
};

NS_SP_END

#endif /* STAPPLER_SRC_FEATURES_DYNAMIC_ATLAS_SPDYNAMICTRIANGLEARRAY_H_ */
