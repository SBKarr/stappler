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

#include "SPDefine.h"
#include "SPDynamicTriangleArray.h"

NS_SP_BEGIN

void DynamicTriangleArray::clear() {
	_vec.clear();
	_quadsDirty = true;
}
void DynamicTriangleArray::resize(size_t size) {
	_vec.resize(size);
	_quadsDirty = true;
}
void DynamicTriangleArray::setCapacity(size_t size) {
	_vec.reserve(size);
	_quadsDirty = true;
}
void DynamicTriangleArray::shrinkToFit() {
	_vec.shrink_to_fit();
	_quadsDirty = true;
}

void DynamicTriangleArray::insert(const V3F_C4B_T2F_Triangle &quad) {
	_vec.push_back(quad);
	_quadsDirty = true;
}

size_t DynamicTriangleArray::emplace() {
	_vec.emplace_back();
	_quadsDirty = true;
	return _vec.size() - 1;
}

void DynamicTriangleArray::setTexturePoints(size_t index, const Vec2 &a, const Vec2 &b, const Vec2 &c, float texWidth, float texHeight) {
	_vec[index].a.texCoords.u = a.x / texWidth;
	_vec[index].a.texCoords.v = a.y / texHeight;
	_vec[index].b.texCoords.u = b.x / texWidth;
	_vec[index].b.texCoords.v = b.y / texHeight;
	_vec[index].c.texCoords.u = c.x / texWidth;
	_vec[index].c.texCoords.v = c.y / texHeight;

	_quadsDirty = true;
}

void DynamicTriangleArray::setGeometry(size_t index, const Vec2 &a, const Vec2 &b, const Vec2 &c, float positionZ) {
	_vec[index].a.vertices = Vec3( RENDER_IN_SUBPIXEL(a.x), RENDER_IN_SUBPIXEL(a.y), positionZ);
	_vec[index].b.vertices = Vec3( RENDER_IN_SUBPIXEL(b.x), RENDER_IN_SUBPIXEL(b.y), positionZ);
	_vec[index].c.vertices = Vec3( RENDER_IN_SUBPIXEL(c.x), RENDER_IN_SUBPIXEL(c.y), positionZ);

	_transform.transformPoint(&_vec[index].a.vertices);
	_transform.transformPoint(&_vec[index].b.vertices);
	_transform.transformPoint(&_vec[index].c.vertices);

	_quadsDirty = true;
}

void DynamicTriangleArray::setColor(size_t index, const Color4B &color) {
	_vec[index].a.colors = color;
	_vec[index].b.colors = color;
	_vec[index].c.colors = color;

	_quadsDirty = true;
}

void DynamicTriangleArray::setColor(size_t index, Color4B colors[3]) {
	_vec[index].a.colors = colors[0];
	_vec[index].b.colors = colors[1];
	_vec[index].c.colors = colors[2];

	_quadsDirty = true;
}

void DynamicTriangleArray::setColor3B(size_t index, const Color3B &color) {
	auto &q = _vec[index];
	q.a.colors.r = color.r; q.a.colors.g = color.g; q.a.colors.b = color.b;
	q.b.colors.r = color.r; q.b.colors.g = color.g; q.b.colors.b = color.b;
	q.c.colors.r = color.r; q.c.colors.g = color.g; q.c.colors.b = color.b;

	_quadsDirty = true;
}

void DynamicTriangleArray::setOpacity(size_t index, const uint8_t &o) {
	auto &q = _vec[index];
	q.a.colors.a = o;
	q.b.colors.a = o;
	q.c.colors.a = o;

	_quadsDirty = true;
}

void DynamicTriangleArray::setTransform(const Mat4 &t) {
	_transform = t;
	_quadsDirty = true;
}

void DynamicTriangleArray::updateTransform(const Mat4 &t) {
	if (_transform.m[12] == t.m[12] && _transform.m[13] == t.m[13]
			&& _transform.m[0] == t.m[0] && _transform.m[1] == t.m[1]
			&& _transform.m[5] == t.m[5] && _transform.m[4] == t.m[4]) {
		_transform = t;
		return;
	}
	if (!_vec.empty() && !_transform.isIdentity()) {
		_transform.inverse();
		for (auto &it : _vec) {
			_transform.transformPoint(&it.a.vertices);
			_transform.transformPoint(&it.b.vertices);
			_transform.transformPoint(&it.c.vertices);
		}
	}
	_transform = t;
	_quadsDirty = true;

	if (!_vec.empty() && !_transform.isIdentity()) {
		for (auto &it : _vec) {
			_transform.transformPoint(&it.a.vertices);
			_transform.transformPoint(&it.b.vertices);
			_transform.transformPoint(&it.c.vertices);
		}
	}
}

NS_SP_END
