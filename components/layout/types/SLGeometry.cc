// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**

Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2013-2014 Chukong Technologies
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPLayout.h"
#include "SLGeometry.h"

NS_LAYOUT_BEGIN

Size2& Size2::operator= (const Vec2& point) {
	setSize(point.x, point.y);
	return *this;
}

Size2 Size2::operator+(const Size2& right) const {
	return Size(this->width + right.width, this->height + right.height);
}

Size2 Size2::operator-(const Size2& right) const {
	return Size(this->width - right.width, this->height - right.height);
}

Size2 Size2::operator*(float a) const {
	return Size2(this->width * a, this->height * a);
}

Size2 Size2::operator/(float a) const {
	SPASSERT(a!=0, "CCSize division by 0.");
	return Size2(this->width / a, this->height / a);
}

void Size2::setSize(float w, float h) {
	this->width = w;
	this->height = h;
}

bool Size2::equals(const Size2& target) const {
	return
			(fabs(this->width - target.width) < NumericLimits<float>::epsilon()) &&
			(fabs(this->height - target.height) < NumericLimits<float>::epsilon());
}

Size3& Size3::operator=(const Vec3& point) {
	width = point.x;
	height = point.y;
	depth = point.z;
	return *this;
}

Size3 Size3::operator+(const Size3& right) const {
	Size3 ret(*this);
	ret.width += right.width;
	ret.height += right.height;
	ret.depth += right.depth;
	return ret;
}
Size3 Size3::operator-(const Size3& right) const {
	Size3 ret(*this);
	ret.width -= right.width;
	ret.height -= right.height;
	ret.depth -= right.depth;
	return ret;
}
Size3 Size3::operator*(float a) const {
	Size3 ret(*this);
	ret.width *= a;
	ret.height *= a;
	ret.depth *= a;
	return ret;
}
Size3 Size3::operator/(float a) const {
	Size3 ret(*this);
	ret.width /= a;
	ret.height /= a;
	ret.depth /= a;
	return ret;
}

bool Size3::equals(const Size3& target) const {
	return
			(fabs(this->width - target.width) < NumericLimits<float>::epsilon()) &&
			(fabs(this->height - target.height) < NumericLimits<float>::epsilon());
}


void Rect::setRect(float x, float y, float width, float height) {
	// CGRect can support width<0 or height<0
	// CCASSERT(width >= 0.0f && height >= 0.0f, "width and height of Rect must not less than 0.");

	origin.x = x;
	origin.y = y;

	size.width = width;
	size.height = height;
}

bool Rect::equals(const Rect& rect) const {
	return (origin.equals(rect.origin) &&
			size.equals(rect.size));
}

float Rect::getMaxX() const {
	return origin.x + size.width;
}

float Rect::getMidX() const {
	return origin.x + size.width / 2.0f;
}

float Rect::getMinX() const {
	return origin.x;
}

float Rect::getMaxY() const {
	return origin.y + size.height;
}

float Rect::getMidY() const {
	return origin.y + size.height / 2.0f;
}

float Rect::getMinY() const {
	return origin.y;
}

bool Rect::containsPoint(const Vec2& point) const {
	bool bRet = false;

	if (point.x >= getMinX() && point.x <= getMaxX() && point.y >= getMinY() && point.y <= getMaxY()) {
		bRet = true;
	}

	return bRet;
}

bool Rect::intersectsRect(const Rect& rect) const {
	return !( getMaxX() < rect.getMinX() ||
			rect.getMaxX() < getMinX() ||
			getMaxY() < rect.getMinY() ||
			rect.getMaxY() < getMinY());
}

bool Rect::intersectsCircle(const Vec2 &center, float radius) const {
	Vec2 rectangleCenter((origin.x + size.width / 2),
			(origin.y + size.height / 2));

	float w = size.width / 2;
	float h = size.height / 2;

	float dx = fabs(center.x - rectangleCenter.x);
	float dy = fabs(center.y - rectangleCenter.y);

	if (dx > (radius + w) || dy > (radius + h)) {
		return false;
	}

	Vec2 circleDistance(fabs(center.x - origin.x - w),
			fabs(center.y - origin.y - h));

	if (circleDistance.x <= (w)) {
		return true;
	}

	if (circleDistance.y <= (h)) {
		return true;
	}

	float cornerDistanceSq = powf(circleDistance.x - w, 2) + powf(circleDistance.y - h, 2);

	return (cornerDistanceSq <= (powf(radius, 2)));
}

void Rect::merge(const Rect& rect) {
	float top1 = getMaxY();
	float left1 = getMinX();
	float right1 = getMaxX();
	float bottom1 = getMinY();

	float top2 = rect.getMaxY();
	float left2 = rect.getMinX();
	float right2 = rect.getMaxX();
	float bottom2 = rect.getMinY();
	origin.x = std::min(left1, left2);
	origin.y = std::min(bottom1, bottom2);
	size.width = std::max(right1, right2) - origin.x;
	size.height = std::max(top1, top2) - origin.y;
}

Rect Rect::unionWithRect(const Rect & rect) const {
	float thisLeftX = origin.x;
	float thisRightX = origin.x + size.width;
	float thisTopY = origin.y + size.height;
	float thisBottomY = origin.y;

	if (thisRightX < thisLeftX) {
		std::swap(thisRightX, thisLeftX);   // This rect has negative width
	}

	if (thisTopY < thisBottomY) {
		std::swap(thisTopY, thisBottomY);   // This rect has negative height
	}

	float otherLeftX = rect.origin.x;
	float otherRightX = rect.origin.x + rect.size.width;
	float otherTopY = rect.origin.y + rect.size.height;
	float otherBottomY = rect.origin.y;

	if (otherRightX < otherLeftX) {
		std::swap(otherRightX, otherLeftX);   // Other rect has negative width
	}

	if (otherTopY < otherBottomY) {
		std::swap(otherTopY, otherBottomY);   // Other rect has negative height
	}

	float combinedLeftX = std::min(thisLeftX, otherLeftX);
	float combinedRightX = std::max(thisRightX, otherRightX);
	float combinedTopY = std::max(thisTopY, otherTopY);
	float combinedBottomY = std::min(thisBottomY, otherBottomY);

	return Rect(combinedLeftX, combinedBottomY, combinedRightX - combinedLeftX, combinedTopY - combinedBottomY);
}

Vec2 TransformPoint(const Vec2& point, const Mat4& transform) {
	Vec3 vec(point.x, point.y, 0);
	transform.transformPoint(&vec);
	return Vec2(vec.x, vec.y);
}

Rect TransformRect(const Rect& rect, const Mat4& transform) {
	const float top = rect.getMinY();
	const float left = rect.getMinX();
	const float right = rect.getMaxX();
	const float bottom = rect.getMaxY();

	Vec3 topLeft(left, top, 0);
	Vec3 topRight(right, top, 0);
	Vec3 bottomLeft(left, bottom, 0);
	Vec3 bottomRight(right, bottom, 0);
	transform.transformPoint(&topLeft);
	transform.transformPoint(&topRight);
	transform.transformPoint(&bottomLeft);
	transform.transformPoint(&bottomRight);

	const float minX = min(min(topLeft.x, topRight.x), min(bottomLeft.x, bottomRight.x));
	const float maxX = max(max(topLeft.x, topRight.x), max(bottomLeft.x, bottomRight.x));
	const float minY = min(min(topLeft.y, topRight.y), min(bottomLeft.y, bottomRight.y));
	const float maxY = max(max(topLeft.y, topRight.y), max(bottomLeft.y, bottomRight.y));

	return Rect(minX, minY, (maxX - minX), (maxY - minY));
}

NS_LAYOUT_END
