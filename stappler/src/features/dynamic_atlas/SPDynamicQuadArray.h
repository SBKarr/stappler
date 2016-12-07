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

#ifndef LIBS_STAPPLER_FEATURES_DYNAMIC_ATLAS_SPDYNAMICQUADARRAY_H_
#define LIBS_STAPPLER_FEATURES_DYNAMIC_ATLAS_SPDYNAMICQUADARRAY_H_

#include "SPFont.h"

NS_SP_BEGIN

class DynamicQuadArray : public Ref {
public:
	using iterator = Vector<cocos2d::V3F_C4B_T2F_Quad>::iterator;
	using const_iterator = Vector<cocos2d::V3F_C4B_T2F_Quad>::const_iterator;

public:
	void clear();
	void resize(size_t);
	void setCapacity(size_t);
	void shrinkToFit();

	void insert(const cocos2d::V3F_C4B_T2F_Quad &);
	size_t emplace();

	void setTextureRect(size_t index, const Rect &, float texWidth, float texHeight, bool flippedX, bool flippedY, bool rotated = false);
	void setTexturePoints(size_t index, const Vec2 &bl, const Vec2 &tl, const Vec2 &tr, const Vec2 &br, float texWidth, float texHeight);

	void setGeometry(size_t index, const Vec2 &pos, const Size &size, float positionZ);
	void setColor(size_t index, const Color4B &color);
	void setColor(size_t index, Color4B colors[4]);

	void setColor3B(size_t index, const Color3B &color);
	void setOpacity(size_t index, const uint8_t &o);

	void setNormalizedGeometry(size_t index, const Vec2 &pos, float positionZ, const Rect &, float texWidth, float texHeight, bool flippedX, bool flippedY, bool rotated = false);

	inline bool empty() const { return _quads.empty(); }

	inline size_t size() const { return _quads.size(); }
	inline size_t capacity() const { return _quads.capacity(); }

	inline const Vector<cocos2d::V3F_C4B_T2F_Quad> &getQuads() const { return _quads; }
	inline const cocos2d::V3F_C4B_T2F_Quad *getData() const { return _quads.data(); }

	inline bool isCapacityDirty() const { return _savedCapacity != _quads.capacity(); }
	inline bool isSizeDirty() const { return _savedSize != _quads.size(); }
	inline bool isQuadsDirty() const { return _quadsDirty; }

	inline void setDirty() { _quadsDirty = true; }

	inline void dropDirty() {
		_savedCapacity = _quads.capacity();
		_savedSize = _quads.size();
		_quadsDirty = false;
	}

	void setTransform(const Mat4 &); // just set transform matrix
	void updateTransform(const Mat4 &); // set transform matrix and update verticles

	inline iterator begin() { return _quads.begin(); };
	inline iterator end() { return _quads.end(); };

	inline const_iterator begin() const { return _quads.begin(); };
	inline const_iterator end() const { return _quads.end(); };

	bool drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const Color4B &color, float texWidth, float texHeight);

	bool drawChar(const font::Metrics &m, const font::CharLayout &l, const font::CharTexture &t, int16_t charX, uint16_t charY,
			const Color4B &color, uint8_t underline, float texWidth, float texHeight);

protected:
	Vector<cocos2d::V3F_C4B_T2F_Quad> _quads;

	size_t _savedSize = 0;
	size_t _savedCapacity = 0;
	bool _quadsDirty = true;
	Mat4 _transform = Mat4::IDENTITY;
};

NS_SP_END

#endif /* LIBS_STAPPLER_FEATURES_DYNAMIC_ATLAS_SPDYNAMICQUADARRAY_H_ */
