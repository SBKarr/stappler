/**
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2013-2014 Chukong Technologies
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef LAYOUT_TYPES_SLGEOMETRY_H_
#define LAYOUT_TYPES_SLGEOMETRY_H_

#include "SPLayout.h"

NS_LAYOUT_BEGIN

/*
 * Based on cocos2d-x sources
 * stappler-cocos2d-x fork use this sources as a replacement of original
 */

struct Size2 {
	static const Size2 ZERO;

	float width = 0.0f;
	float height = 0.0f;

	constexpr Size2() = default;
	constexpr Size2(float w, float h) : width(w), height(h) { }
	constexpr Size2(const Size2 &other) = default;
	constexpr explicit Size2(const Vec2 &point) : width(point.x), height(point.y) { }

	operator Vec2() const {return Vec2(width, height);}

	bool operator==(const Size2 &s) const { return equals(s); }
	bool operator!=(const Size2 &s) const { return !equals(s); }

	Size2& operator= (const Size2 &other) = default;
	Size2& operator= (const Vec2 &point);

	Size2 operator+(const Size2 &right) const;
	Size2 operator-(const Size2 &right) const;
	Size2 operator*(float a) const;
	Size2 operator/(float a) const;

	void setSize(float width, float height);

	bool equals(const Size2 &target) const;
};

inline constexpr Size2 Size2::ZERO(0.0f, 0.0f);


struct Size3 {
	static const Size3 ZERO;

	float width = 0.0f;
	float height = 0.0f;
	float depth = 0.0f;

	constexpr Size3() = default;
	constexpr Size3(float w, float h, float d) : width(w), height(h), depth(d) { }
	constexpr Size3(const Size3& other) = default;
	constexpr explicit Size3(const Vec3& point) : width(point.x), height(point.y), depth(point.z) { }

	operator Vec3() const { return Vec3(width, height, depth); }

	bool operator==(const Size3 &s) const { return equals(s); }
	bool operator!=(const Size3 &s) const { return !equals(s); }

	Size3& operator= (const Size3& other) = default;
	Size3& operator= (const Vec3& point);

	Size3 operator+(const Size3& right) const;
	Size3 operator-(const Size3& right) const;
	Size3 operator*(float a) const;
	Size3 operator/(float a) const;

	bool equals(const Size3& target) const;
};

inline constexpr Size3 Size3::ZERO = Size3(0.0f, 0.0f, 0.0f);


struct Extent2 {
	static const Extent2 ZERO;

	uint32_t width = 0;
	uint32_t height = 0;

	constexpr Extent2() = default;
	constexpr Extent2(uint32_t w, uint32_t h) : width(w), height(h) { }

	constexpr Extent2(const Extent2 &other) = default;
	Extent2& operator= (const Extent2 &other) = default;

	constexpr explicit Extent2(const Size2 &size) : width(size.width), height(size.height) { }
	constexpr explicit Extent2(const Vec2 &point) : width(point.x), height(point.y) { }

	Extent2& operator= (const Size2 &size) { width = size.width; height = size.width; return *this; }
	Extent2& operator= (const Vec2 &other) { width = other.x; height = other.y; return *this; }

	bool operator==(const Extent2 &other) const { return width == other.width && height == other.height; }
	bool operator!=(const Extent2 &other) const { return width != other.width || height != other.height; }

	operator Size2 () const { return Size2(width, height); }
};

inline constexpr Extent2 Extent2::ZERO = Extent2(0, 0);


struct Extent3 {
	static const Extent3 ZERO;

	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t depth = 0;

	constexpr Extent3() = default;
	constexpr Extent3(uint32_t w, uint32_t h, uint32_t d) : width(w), height(h), depth(d) { }

	constexpr Extent3(const Extent3& other) = default;
	Extent3& operator= (const Extent3& other) = default;

	constexpr Extent3(const Extent2& other) : width(other.width), height(other.height), depth(1) { }
	Extent3& operator= (const Extent2& other) { width = other.width; height = other.height; depth = 1; return *this; }

	constexpr explicit Extent3(const Size3 &size) : width(size.width), height(size.height), depth(size.depth) { }
	constexpr explicit Extent3(const Vec3 &point) : width(point.x), height(point.y), depth(point.z) { }

	Extent3& operator= (const Size3 &size) { width = size.width; height = size.width; depth = size.depth; return *this; }
	Extent3& operator= (const Vec3 &other) { width = other.x; height = other.y; depth = other.z; return *this; }

	bool operator==(const Extent3 &other) const { return width == other.width && height == other.height && depth == other.depth; }
	bool operator!=(const Extent3 &other) const { return width != other.width || height != other.height || depth != other.depth; }

	operator Size3 () const { return Size3(width, height, depth); }
};

inline constexpr Extent3 Extent3::ZERO = Extent3(0, 0, 0);


struct Rect {
	static const Rect ZERO;

	Vec2 origin;
	Size2 size;

	constexpr Rect() = default;
	constexpr Rect(float x, float y, float width, float height) : origin(x, y), size(width, height) { }
	constexpr Rect(const Vec2 &origin, const Size2 &size) : origin(origin), size(size) { }
	constexpr Rect(const Rect& other) = default;

	Rect& operator= (const Rect& other) = default;

	void setRect(float x, float y, float width, float height);

	float getMinX() const; /// return the leftmost x-value of current rect
	float getMidX() const; /// return the midpoint x-value of current rect
	float getMaxX() const; /// return the rightmost x-value of current rect
	float getMinY() const; /// return the bottommost y-value of current rect
	float getMidY() const; /// return the midpoint y-value of current rect
	float getMaxY() const; /// return the topmost y-value of current rect

	bool equals(const Rect& rect) const;

	bool containsPoint(const Vec2& point) const;
	bool intersectsRect(const Rect& rect) const;
	bool intersectsCircle(const Vec2& center, float radius) const;

	/** Get the min rect which can contain this and rect. */
	Rect unionWithRect(const Rect & rect) const;

	/** Compute the min rect which can contain this and rect, assign it to this. */
	void merge(const Rect& rect);
};

inline constexpr Rect Rect::ZERO = Rect(0.0f, 0.0f, 0.0f, 0.0f);


Rect TransformRect(const Rect& rect, const Mat4& transform);
Vec2 TransformPoint(const Vec2& point, const Mat4& transform);

inline std::ostream & operator<<(std::ostream & stream, const Rect & obj) {
	stream << "Rect(x:" << obj.origin.x << " y:" << obj.origin.y
			<< " width:" << obj.size.width << " height:" << obj.size.height << ");";
	return stream;
}

inline std::ostream & operator<<(std::ostream & stream, const Size2 & obj) {
	stream << "Size(width:" << obj.width << " height:" << obj.height << ");";
	return stream;
}

using Size = Size2;

NS_LAYOUT_END

#endif /* LAYOUT_TYPES_SLGEOMETRY_H_ */
