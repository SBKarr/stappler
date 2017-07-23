/**
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

#ifndef LAYOUT_VG_SLIMAGE_H_
#define LAYOUT_VG_SLIMAGE_H_

#include "SLPath.h"
#include "SLSubscription.h"

NS_LAYOUT_BEGIN

/**
 * Vector graphics image with several paths
 * Paths in image can be modified with PathRef wrapper
 * Image can notify consumers about modifications via Subscription interface
 */

class Image : public Subscription {
public:
	class PathRef : public Ref {
	public:
		virtual ~PathRef();

		PathRef();
		PathRef(PathRef &&);
		PathRef(const PathRef &);
		PathRef(nullptr_t);

		PathRef & operator=(PathRef &&);
		PathRef & operator=(const PathRef &);
		PathRef & operator=(nullptr_t);

		size_t count() const;

		PathRef & moveTo(float x, float y);
		PathRef & moveTo(const Vec2 &point) {
			return this->moveTo(point.x, point.y);
		}

		PathRef & lineTo(float x, float y);
		PathRef & lineTo(const Vec2 &point) {
			return this->lineTo(point.x, point.y);
		}

		PathRef & quadTo(float x1, float y1, float x2, float y2);
		PathRef & quadTo(const Vec2& p1, const Vec2& p2) {
			return this->quadTo(p1.x, p1.y, p2.x, p2.y);
		}

		PathRef & cubicTo(float x1, float y1, float x2, float y2, float x3, float y3);
		PathRef & cubicTo(const Vec2& p1, const Vec2& p2, const Vec2& p3) {
			return this->cubicTo(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
		}

		// use _to_rad user suffix to convert from degrees to radians
		PathRef & arcTo(float rx, float ry, float rotation, bool largeFlag, bool sweepFlag, float x, float y);
		PathRef & arcTo(const Vec2 & r, float rotation, bool largeFlag, bool sweepFlag, const Vec2 &target) {
			return this->arcTo(r.x, r.y, rotation, largeFlag, sweepFlag, target.x, target.y);
		}

		PathRef & closePath();

		PathRef & addRect(const Rect& rect);
		PathRef & addOval(const Rect& oval);
		PathRef & addCircle(float x, float y, float radius);
		PathRef & addArc(const Rect& oval, float startAngleInRadians, float sweepAngleInRadians);

		PathRef & setFillColor(const Color4B &color);
		const Color4B &getFillColor() const;

		PathRef & setStrokeColor(const Color4B &color);
		const Color4B &getStrokeColor() const;

		PathRef & setFillOpacity(uint8_t value);
		uint8_t getFillOpacity() const;

		PathRef & setStrokeOpacity(uint8_t value);
		uint8_t getStrokeOpacity() const;

		PathRef & setStrokeWidth(float width);
		float getStrokeWidth() const;

		PathRef & setStyle(DrawStyle s);
		DrawStyle getStyle() const;

		PathRef & setTransform(const Mat4 &);
		PathRef & applyTransform(const Mat4 &);
		const Mat4 &getTransform() const;

		PathRef & clear();

		PathRef & setTag(uint32_t);
		uint32_t getTag() const;

		bool empty() const;
		bool valid() const;

		operator bool() const;

		void invalidate();

		Path *getPath() const;
	protected:
		friend class Image;
		PathRef(Image *, Path *, size_t);

		size_t index = 0;
		Path *path = nullptr;
		Image *image = nullptr;
	};

	virtual ~Image();

	static bool isSvg(const String &);
	static bool isSvg(const FilePath &);

	bool init(uint16_t width, uint16_t height, const String &);
	bool init(uint16_t width, uint16_t height, Path &&);
	bool init(uint16_t width, uint16_t height);
	bool init(const String &);
	bool init(FilePath &&);

	uint16_t getWidth() const;
	uint16_t getHeight() const;

	PathRef addPath(const Path &, uint32_t tag = 0);
	PathRef addPath(Path &&, uint32_t tag = 0);
	PathRef addPath();

	PathRef getPathByTag(uint32_t);
	PathRef getPath(size_t);

	void removePath(const PathRef &);
	void removePath(size_t);
	void removePathByTag(uint32_t);

	void clear();

	const Vector<Path> &getPaths() const;

	void setAntialiased(bool value);
	bool isAntialiased() const;

protected:
	void invalidateRefs();
	void clearRefs();

	void addRef(PathRef *);
	void removeRef(PathRef *);
	void replaceRef(PathRef *original, PathRef *target);
	void eraseRefs(size_t);

	Path *getPathByRef(const PathRef &) const;

	bool _isAntialiased = true;
	uint16_t _width = 0;
	uint16_t _height = 0;
	Vector<Path> _paths;
	Vector<PathRef *> _refs;
};

NS_LAYOUT_END

#endif /* LAYOUT_VG_SLIMAGE_H_ */
