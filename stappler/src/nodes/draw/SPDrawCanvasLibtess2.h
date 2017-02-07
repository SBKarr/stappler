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

#ifndef STAPPLER_SRC_NODES_DRAW_SPDRAWCANVASLIBTESS2_H_
#define STAPPLER_SRC_NODES_DRAW_SPDRAWCANVASLIBTESS2_H_

#include "SPDrawPathNode.h"
#include "SPDrawGLCacheNode.h"
#include "SPBuffer.h"
#include "SLMemPool.h"

#define SP_LIBTESS2 0
#if SP_LIBTESS2

#include "tesselator.h"

NS_SP_EXT_BEGIN(draw)

class CanvasLibtess2 : public Canvas, public GLCacheNode {
public:
	static void * staticPoolAlloc(void* userData, unsigned int size);
	static void * staticPoolRealloc(void * userData, void * ptr, unsigned int size);
	static void staticPoolFree(void * userData, void * ptr);

	virtual ~CanvasLibtess2();

	virtual bool init() override;

	virtual void begin(cocos2d::Texture2D *, const Color4B &) override;
	virtual void end() override;

	virtual void scale(float sx, float sy) override;
	virtual void translate(float tx, float ty) override;
	virtual void transform(const Mat4 &) override;

	virtual void save() override;
	virtual void restore() override;

	virtual void setAntialiasing(bool) override;
	virtual void setLineWidth(float) override;

	virtual void pathBegin() override;
	virtual void pathClose() override;
	virtual void pathMoveTo(float x, float y) override;
	virtual void pathLineTo(float x, float y) override;
	virtual void pathQuadTo(float x1, float y1, float x2, float y2) override;
	virtual void pathCubicTo(float x1, float y1, float x2, float y2, float x3, float y3) override;
	virtual void pathArcTo(float rx, float ry, float angle, bool largeArc, bool sweep, float x, float y) override;

	virtual void pathFill(const Color4B &) override;
	virtual void pathStroke(const Color4B &) override;
	virtual void pathFillStroke(const Color4B &fill, const Color4B &stroke) override;

protected:
	void *poolAlloc(uint32_t);
	void *poolRealloc(void *, uint32_t);
	size_t poolGetAllocated() const;
	void poolClear();

	void poolSaveRec(void *, uint32_t);
	uint32_t poolGetRec(void *);

	TESSalloc _tessAlloc;
	TESStesselator* _tess = nullptr;
	cocos2d::GLProgram *_program = nullptr;

	GLint _oldFbo = 0;
	GLuint _fbo = 0;
	GLuint _rbo = 0;
	GLuint _vbo[2] = { 0, 0 };

	struct Contour {
		size_t start;
		size_t count;
		bool closed;
	};

	Mat4 _transform;
	Vector<Mat4> _states;

	Vector<float> _verts;
	Vector<Contour> _contours;

	// reallocatable memory pool
	layout::MemPool<256_KiB, true> _pool;

	float _lineWidth = 1.0f;
	float _pathX = 0, _pathY = 0;
};

NS_SP_EXT_END(draw)

#endif // SP_LIBTESS2

#endif /* STAPPLER_SRC_NODES_DRAW_SPDRAWCANVASLIBTESS2_H_ */
