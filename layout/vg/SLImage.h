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

#include "SPBitmap.h"
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
	struct PathXRef {
		String id;
		Vec2 pos;
	};

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

		StringView getId() const;

		bool empty() const;
		bool valid() const;

		operator bool() const;

		void invalidate();

		Path *getPath() const;

	protected:
		friend class Image;
		PathRef(Image *, Path *, const StringView &);

		StringView id;
		Path *path = nullptr;
		Image *image = nullptr;
	};

	virtual ~Image();

	static bool isSvg(const StringView &);
	static bool isSvg(const Bytes &);
	static bool isSvg(const FilePath &);

	bool init(uint16_t width, uint16_t height, const String &);
	bool init(uint16_t width, uint16_t height, Path &&);
	bool init(uint16_t width, uint16_t height);
	bool init(const StringView &);
	bool init(const Bytes &);
	bool init(FilePath &&);
	bool init(const Image &);

	uint16_t getWidth() const;
	uint16_t getHeight() const;

	Rect getViewBox() const;

	PathRef addPath(const Path &, const StringView & = StringView());
	PathRef addPath(Path &&, const StringView & = StringView());
	PathRef addPath(const StringView & = StringView());

	PathRef definePath(const Path &, const StringView & id = StringView());
	PathRef definePath(Path &&, const StringView & id = StringView());
	PathRef definePath(const StringView & = StringView());

	PathRef getPath(const StringView &);

	void removePath(const PathRef &);
	void removePath(const StringView &);

	void clear();

	const Map<String, Path> &getPaths() const;

	void setAntialiased(bool value);
	bool isAntialiased() const;

	Bitmap::PixelFormat detectFormat() const;

	const Vector<PathXRef> &getDrawOrder() const;
	void setDrawOrder(const Vector<PathXRef> &);
	void setDrawOrder(Vector<PathXRef> &&);

	PathXRef getDrawOrderPath(size_t) const;
	PathXRef addDrawOrderPath(const StringView &, const Vec2 & = Vec2::ZERO);

	void clearDrawOrder();

	Path *getPathById(const StringView &);

	void setViewBoxTransform(const Mat4 &);
	const Mat4 &getViewBoxTransform() const;

	Rc<Image> clone() const;

	// usage:
	// draw([&] (const Path &path, const Mat4 &transform) {
	//    // code here
	// });
	template <typename Callback>
	void draw(const Callback &cb) const {
		for (auto &it : _drawOrder) {
			auto pathIt = _paths.find(it.id);
			if (pathIt != _paths.end()) {
				cb(pathIt->second, it.pos);
			}
		}
	}

protected:
	void invalidateRefs();
	void clearRefs();

	void addRef(PathRef *);
	void removeRef(PathRef *);
	void replaceRef(PathRef *original, PathRef *target);
	void eraseRefs(const StringView &);

	bool _isAntialiased = true;
	uint16_t _nextId = 0;
	uint16_t _width = 0;
	uint16_t _height = 0;

	Rect _viewBox = Rect::ZERO;

	Mat4 _viewBoxTransform = Mat4::IDENTITY;
	Vector<PathXRef> _drawOrder;
	Vector<PathRef *> _refs;
	Map<String, Path> _paths;
};

NS_LAYOUT_END

#endif /* LAYOUT_VG_SLIMAGE_H_ */
