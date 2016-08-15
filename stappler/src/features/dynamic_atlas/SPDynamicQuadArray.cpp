/*
 * SPDynamicQuadsArray.cpp
 *
 *  Created on: 01 апр. 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPDynamicQuadArray.h"
#include "SPImage.h"

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
    float x1 = pos.x;
    float y1 = pos.y;

    float x2 = x1 + size.width;
    float y2 = y1 + size.height;
    float x = _transform.m[12];
    float y = _transform.m[13];

    float cr = _transform.m[0];
    float sr = _transform.m[1];
    float cr2 = _transform.m[5];
    float sr2 = -_transform.m[4];
    float ax = x1 * cr - y1 * sr2 + x;
    float ay = x1 * sr + y1 * cr2 + y;

    float bx = x2 * cr - y1 * sr2 + x;
    float by = x2 * sr + y1 * cr2 + y;

    float cx = x2 * cr - y2 * sr2 + x;
    float cy = x2 * sr + y2 * cr2 + y;

    float dx = x1 * cr - y2 * sr2 + x;
    float dy = x1 * sr + y2 * cr2 + y;

    _quads[index].bl.vertices = cocos2d::Vec3( RENDER_IN_SUBPIXEL(ax), RENDER_IN_SUBPIXEL(ay), positionZ );
    _quads[index].br.vertices = cocos2d::Vec3( RENDER_IN_SUBPIXEL(bx), RENDER_IN_SUBPIXEL(by), positionZ );
    _quads[index].tl.vertices = cocos2d::Vec3( RENDER_IN_SUBPIXEL(dx), RENDER_IN_SUBPIXEL(dy), positionZ );
    _quads[index].tr.vertices = cocos2d::Vec3( RENDER_IN_SUBPIXEL(cx), RENDER_IN_SUBPIXEL(cy), positionZ );

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

    float x1 = pos.x;
    float y1 = pos.y;

    float x2 = x1 + rect.size.width;
    float y2 = y1 + rect.size.height;
    float x = _transform.m[12];
    float y = _transform.m[13];

    float cr = _transform.m[0];
    float sr = _transform.m[1];
    float cr2 = _transform.m[5];
    float sr2 = -_transform.m[4];
    float ax = x1 * cr - y1 * sr2 + x;
    float ay = x1 * sr + y1 * cr2 + y;

    float bx = x2 * cr - y1 * sr2 + x;
    float by = x2 * sr + y1 * cr2 + y;

    float cx = x2 * cr - y2 * sr2 + x;
    float cy = x2 * sr + y2 * cr2 + y;

    float dx = x1 * cr - y2 * sr2 + x;
    float dy = x1 * sr + y2 * cr2 + y;

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
	if (!_quads.empty()) {
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

	if (!_quads.empty()) {
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

bool DynamicQuadArray::drawChar(const Font::CharSpec &c, uint16_t xOffset, uint16_t yOffset,
		bool flip, float texWidth, float texHeight, bool invert) {
	_quads.emplace_back();
	auto id = _quads.size() - 1;

	uint16_t width = c.uCharPtr->width();
	uint16_t height = c.uCharPtr->height();

	setTextureRect(id, c.uCharPtr->rect, texWidth, texHeight, false, flip);

	int32_t posX = xOffset + c.posX + c.uCharPtr->xOffset;
	int32_t posY = 0;
	if (invert) {
		posY = yOffset - ((flip)?(-c.posY):c.posY) - (c.uCharPtr->yOffset + height) - c.uCharPtr->font->getDescender();
	} else {
		posY = yOffset + ((flip)?(-c.posY):c.posY) + c.uCharPtr->yOffset + c.uCharPtr->font->getDescender();
	}

	setGeometry(id, cocos2d::Vec2(posX, posY), cocos2d::Size(width, height), 0.0f);
	setColor(id, c.color);

	if (c.underline > 0) {
		posX = xOffset + c.posX;
		if (invert) {
			posY = yOffset - ((flip)?(-c.posY):c.posY) - c.uCharPtr->font->getUnderlinePosition() - c.uCharPtr->font->getUnderlineThickness() * c.underline;
		} else {
			posY = yOffset + ((flip)?(-c.posY):c.posY) + c.uCharPtr->font->getUnderlinePosition();
		}
		drawRect(posX, posY,
				c.uCharPtr->xAdvance, c.uCharPtr->font->getUnderlineThickness() * c.underline, c.color, texWidth, texHeight);
	}
	_quadsDirty = true;
	return true;
}

size_t DynamicQuadArray::getQuadsCount(const std::vector<Font::CharSpec> &chars) {
	size_t quadsSize = 0;
	for (auto &it : chars) {
		if (it.drawable() && (it.uCharPtr->rect.size.width == 0.0f || it.uCharPtr->rect.size.height == 0.0f)) {
			quadsSize ++;
			if (it.underline > 0) {
				quadsSize ++;
			}
		}
	}
	return quadsSize;
}
bool DynamicQuadArray::drawCharsInvert(const std::vector<Font::CharSpec> &vec, uint16_t yOffset) {
	return drawChars(vec, 0, yOffset, false, true);
}

bool DynamicQuadArray::drawChars(const std::vector<Font::CharSpec> &vec, uint16_t xOffset, uint16_t yOffset, bool flip, bool invert) {
	resize(getQuadsCount(vec));
	bool ret = true;
	if (!vec.empty()) {
		auto image = vec.front().uCharPtr->getFont()->getImage();
		float texWidth = image->getWidth();
		float texHeight = image->getHeight();

		for (auto &it : vec) {
			if (it.drawable() && it.uCharPtr->width() > 0 && it.uCharPtr->height() > 0) {
				if (!drawChar(it, xOffset, yOffset, flip, texWidth, texHeight, invert)) {
					ret = false;
					break;
				}
			}
		}
	}

	return ret;
}

NS_SP_END
