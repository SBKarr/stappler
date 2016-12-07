// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPDynamicQuadArray.h"

#if CC_SPRITEBATCHNODE_RENDER_SUBPIXEL
#define RENDER_IN_SUBPIXEL
#else
#define RENDER_IN_SUBPIXEL
//#define RENDER_IN_SUBPIXEL(__ARGS__) (ceil(__ARGS__))
#endif

NS_SP_BEGIN

void DynamicQuadArray::clear() {
	_quads.clear();
	_quadsDirty = true;
}
void DynamicQuadArray::resize(size_t size) {
	_quads.resize(size);
	_quadsDirty = true;
}
void DynamicQuadArray::setCapacity(size_t size) {
	_quads.reserve(size);
	_quadsDirty = true;
}
void DynamicQuadArray::shrinkToFit() {
	_quads.shrink_to_fit();
	_quadsDirty = true;
}

void DynamicQuadArray::insert(const cocos2d::V3F_C4B_T2F_Quad &quad) {
	_quads.push_back(quad);
	_quadsDirty = true;
}

size_t DynamicQuadArray::emplace() {
	_quads.emplace_back();
	_quadsDirty = true;
	return _quads.size() - 1;
}

void DynamicQuadArray::setTextureRect(size_t index, const cocos2d::Rect &texRect, float texWidth, float texHeight,
		bool flippedX, bool flippedY, bool rotated) {
	float texLeft = texRect.origin.x / texWidth;
	float texRight = (texRect.origin.x + texRect.size.width) / texWidth;
	float texTop = texRect.origin.y / texHeight;
	float texBottom = (texRect.origin.y + texRect.size.height) / texHeight;

	if (flippedX) {
		CC_SWAP(texLeft, texRight, float);
	}

	if (flippedY) {
		CC_SWAP(texTop, texBottom, float);
	}

	if (!rotated) {
		_quads[index].bl.texCoords.u = texLeft;
		_quads[index].bl.texCoords.v = texBottom;
		_quads[index].br.texCoords.u = texRight;
		_quads[index].br.texCoords.v = texBottom;
		_quads[index].tl.texCoords.u = texLeft;
		_quads[index].tl.texCoords.v = texTop;
		_quads[index].tr.texCoords.u = texRight;
		_quads[index].tr.texCoords.v = texTop;
	} else {
		_quads[index].bl.texCoords.u = texRight;
		_quads[index].bl.texCoords.v = texTop;
		_quads[index].br.texCoords.u = texRight;
		_quads[index].br.texCoords.v = texBottom;
		_quads[index].tl.texCoords.u = texLeft;
		_quads[index].tl.texCoords.v = texTop;
		_quads[index].tr.texCoords.u = texLeft;
		_quads[index].tr.texCoords.v = texBottom;
	}

	_quadsDirty = true;
}

void DynamicQuadArray::setTexturePoints(size_t index, const cocos2d::Vec2 &bl, const cocos2d::Vec2 &tl,
		const cocos2d::Vec2 &tr, const cocos2d::Vec2 &br, float texWidth, float texHeight) {

	_quads[index].bl.texCoords.u = bl.x / texWidth;
	_quads[index].bl.texCoords.v = bl.y / texHeight;
	_quads[index].br.texCoords.u = br.x / texWidth;
	_quads[index].br.texCoords.v = br.y / texHeight;
	_quads[index].tl.texCoords.u = tl.x / texWidth;
	_quads[index].tl.texCoords.v = tl.y / texHeight;
	_quads[index].tr.texCoords.u = tr.x / texWidth;
	_quads[index].tr.texCoords.v = tr.y / texHeight;

	_quadsDirty = true;
}

void DynamicQuadArray::setGeometry(size_t index, const cocos2d::Vec2 &pos, const cocos2d::Size &size, float positionZ) {
	const float x1 = pos.x;
	const float y1 = pos.y;

	const float x2 = x1 + size.width;
	const float y2 = y1 + size.height;
	const float x = _transform.m[12];
	const float y = _transform.m[13];

	const float cr = _transform.m[0];
	const float sr = _transform.m[1];
	const float cr2 = _transform.m[5];
	const float sr2 = -_transform.m[4];

	const float ax = x1 * cr - y1 * sr2 + x;
	const float ay = x1 * sr + y1 * cr2 + y;

	const float bx = x2 * cr - y1 * sr2 + x;
	const float by = x2 * sr + y1 * cr2 + y;

	const float cx = x2 * cr - y2 * sr2 + x;
	const float cy = x2 * sr + y2 * cr2 + y;

	const float dx = x1 * cr - y2 * sr2 + x;
	const float dy = x1 * sr + y2 * cr2 + y;

	_quads[index].bl.vertices = cocos2d::Vec3( RENDER_IN_SUBPIXEL(ax), RENDER_IN_SUBPIXEL(ay), positionZ);
	_quads[index].br.vertices = cocos2d::Vec3( RENDER_IN_SUBPIXEL(bx), RENDER_IN_SUBPIXEL(by), positionZ);
	_quads[index].tl.vertices = cocos2d::Vec3( RENDER_IN_SUBPIXEL(dx), RENDER_IN_SUBPIXEL(dy), positionZ);
	_quads[index].tr.vertices = cocos2d::Vec3( RENDER_IN_SUBPIXEL(cx), RENDER_IN_SUBPIXEL(cy), positionZ);

	_quadsDirty = true;
}
void DynamicQuadArray::setColor(size_t index, const cocos2d::Color4B &color) {
	_quads[index].bl.colors = color;
	_quads[index].br.colors = color;
	_quads[index].tl.colors = color;
	_quads[index].tr.colors = color;

	_quadsDirty = true;
}
void DynamicQuadArray::setColor(size_t index, cocos2d::Color4B colors[4]) {
	_quads[index].bl.colors = colors[0];
	_quads[index].br.colors = colors[1];
	_quads[index].tl.colors = colors[2];
	_quads[index].tr.colors = colors[3];

	_quadsDirty = true;
}

void DynamicQuadArray::setColor3B(size_t index, const cocos2d::Color3B &color) {
	cocos2d::V3F_C4B_T2F_Quad &q = _quads[index];
	q.bl.colors.r = color.r; q.bl.colors.g = color.g; q.bl.colors.b = color.b;
	q.br.colors.r = color.r; q.br.colors.g = color.g; q.br.colors.b = color.b;
	q.tl.colors.r = color.r; q.tl.colors.g = color.g; q.tl.colors.b = color.b;
	q.tr.colors.r = color.r; q.tr.colors.g = color.g; q.tr.colors.b = color.b;

	_quadsDirty = true;
}
void DynamicQuadArray::setOpacity(size_t index, const uint8_t &o) {
	cocos2d::V3F_C4B_T2F_Quad &q = _quads[index];
	q.bl.colors.a = o;
	q.br.colors.a = o;
	q.tl.colors.a = o;
	q.tr.colors.a = o;

	_quadsDirty = true;
}

void DynamicQuadArray::setNormalizedGeometry(size_t index, const cocos2d::Vec2 &pos, float positionZ,
		const cocos2d::Rect &rect, float texWidth, float texHeight, bool flippedX, bool flippedY, bool rotated) {
	float texLeft = rect.origin.x / texWidth;
	float texRight = (rect.origin.x + rect.size.width) / texWidth;
	float texTop = rect.origin.y / texHeight;
	float texBottom = (rect.origin.y + rect.size.height) / texHeight;

	if (flippedX) {
		CC_SWAP(texLeft, texRight, float);
	}

	if (flippedY) {
		CC_SWAP(texTop, texBottom, float);
	}

	if (!rotated) {
		_quads[index].bl.texCoords.u = texLeft;
		_quads[index].bl.texCoords.v = texBottom;
		_quads[index].br.texCoords.u = texRight;
		_quads[index].br.texCoords.v = texBottom;
		_quads[index].tl.texCoords.u = texLeft;
		_quads[index].tl.texCoords.v = texTop;
		_quads[index].tr.texCoords.u = texRight;
		_quads[index].tr.texCoords.v = texTop;
	} else {
		_quads[index].bl.texCoords.u = texRight;
		_quads[index].bl.texCoords.v = texTop;
		_quads[index].br.texCoords.u = texRight;
		_quads[index].br.texCoords.v = texBottom;
		_quads[index].tl.texCoords.u = texLeft;
		_quads[index].tl.texCoords.v = texTop;
		_quads[index].tr.texCoords.u = texLeft;
		_quads[index].tr.texCoords.v = texBottom;
	}

	const float x1 = pos.x;
	const float y1 = pos.y;

	const float x2 = x1 + rect.size.width;
	const float y2 = y1 + rect.size.height;
	const float x = _transform.m[12];
	const float y = _transform.m[13];

	const float cr = _transform.m[0];
	const float sr = _transform.m[1];
	const float cr2 = _transform.m[5];
	const float sr2 = -_transform.m[4];

	const float ax = x1 * cr - y1 * sr2 + x;
	const float ay = x1 * sr + y1 * cr2 + y;

	const float bx = x2 * cr - y1 * sr2 + x;
	const float by = x2 * sr + y1 * cr2 + y;

	const float cx = x2 * cr - y2 * sr2 + x;
	const float cy = x2 * sr + y2 * cr2 + y;

	const float dx = x1 * cr - y2 * sr2 + x;
	const float dy = x1 * sr + y2 * cr2 + y;

	_quads[index].bl.vertices = cocos2d::Vec3( RENDER_IN_SUBPIXEL(ax), RENDER_IN_SUBPIXEL(ay), positionZ );
	_quads[index].br.vertices = cocos2d::Vec3( RENDER_IN_SUBPIXEL(bx), RENDER_IN_SUBPIXEL(by), positionZ );
	_quads[index].tl.vertices = cocos2d::Vec3( RENDER_IN_SUBPIXEL(dx), RENDER_IN_SUBPIXEL(dy), positionZ );
	_quads[index].tr.vertices = cocos2d::Vec3( RENDER_IN_SUBPIXEL(cx), RENDER_IN_SUBPIXEL(cy), positionZ );

	_quadsDirty = true;
}

void DynamicQuadArray::setTransform(const cocos2d::Mat4 &t) {
	_transform = t;
	_quadsDirty = true;
}

void DynamicQuadArray::updateTransform(const cocos2d::Mat4 &t) {
	if (_transform.m[12] == t.m[12] && _transform.m[13] == t.m[13]
			&& _transform.m[0] == t.m[0] && _transform.m[1] == t.m[1]
			&& _transform.m[5] == t.m[5] && _transform.m[4] == t.m[4]) {
		_transform = t;
		return;
	}
	if (!_quads.empty() && !_transform.isIdentity()) {
		_transform.inverse();
		for (auto &it : _quads) {
			_transform.transformPoint(&it.bl.vertices);
			_transform.transformPoint(&it.br.vertices);
			_transform.transformPoint(&it.tl.vertices);
			_transform.transformPoint(&it.tr.vertices);
		}
	}
	_transform = t;
	_quadsDirty = true;

	if (!_quads.empty() && !_transform.isIdentity()) {
		for (auto &it : _quads) {
			_transform.transformPoint(&it.bl.vertices);
			_transform.transformPoint(&it.br.vertices);
			_transform.transformPoint(&it.tl.vertices);
			_transform.transformPoint(&it.tr.vertices);
		}
	}
}

bool DynamicQuadArray::drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
		const cocos2d::Color4B &color, float texWidth, float texHeight) {
	_quads.emplace_back();
	auto id = _quads.size() - 1;
	setTextureRect(id, cocos2d::Rect(texWidth - 1.0f, texHeight - 1.0f, 1.0f, 1.0f),
			texWidth, texHeight, false, false);
	setGeometry(id, cocos2d::Vec2(x, y), cocos2d::Size(width, height), 0.0f);
	setColor(id, color);
	_quadsDirty = true;
	return true;
}

bool DynamicQuadArray::drawChar(const font::Metrics &m, const font::CharLayout &l, const font::CharTexture &t, int16_t charX, uint16_t charY,
		const Color4B &color, uint8_t underline, float texWidth, float texHeight) {
	_quads.emplace_back();
	const auto id = _quads.size() - 1;

	const uint16_t width = t.width;
	const uint16_t height = t.height;

	setTextureRect(id, Rect(t.x, t.y, t.width, t.height), texWidth, texHeight, false, false);

	setGeometry(id, Vec2(charX + l.xOffset, charY - (l.yOffset + height) - m.descender), Size(width, height), 0.0f);
	setColor(id, color);

	/*if (underline > 0) {
		drawRect(charX, charY, l.xAdvance, m.height * underline / 16.0f, color, texWidth, texHeight);
	}*/
	_quadsDirty = true;
	return true;
}

NS_SP_END
