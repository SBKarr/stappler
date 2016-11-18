/*
 * SPDrawPrivate.cpp
 *
 *  Created on: 13 апр. 2016 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPDrawCanvasNanoVG.h"
#include "SPThreadManager.h"
#include "renderer/CCTexture2D.h"
#include "renderer/ccGLStateCache.h"
#include "math/CCAffineTransform.h"
#include "base/CCDirector.h"

#if SP_NANOVG

#define NANOVG_GLES2_IMPLEMENTATION
#include "nanovg.h"
#include "nanovg_gl.h"

NS_SP_EXT_BEGIN(draw)

CanvasNanoVG::~CanvasNanoVG() {
	if (_fbo) {
		glDeleteFramebuffers(1, &_fbo);
		_fbo = 0;
	}

	if (_rbo) {
		glDeleteRenderbuffers(1, &_rbo);
		_rbo = 0;
	}

	if (_context) {
		nvgDeleteGLES2(_context);
	}
}

bool CanvasNanoVG::init() {
	_context = nvgCreateGLES2(NVG_ANTIALIAS);
	glGenFramebuffers(1, &_fbo);
	glGenRenderbuffers(1, &_rbo);

	return (_fbo != 0 && _context != nullptr);
}

void CanvasNanoVG::begin(cocos2d::Texture2D *tex, const Color4B &color) {
	auto w = tex->getPixelsWide();
	auto h = tex->getPixelsHigh();
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_oldFbo);

	if (ThreadManager::getInstance()->isMainThread()) {
		cocos2d::GL::useProgram(0);
		cocos2d::GL::enableVertexAttribs(0);
		cocos2d::GL::bindTexture2D(0);
		cocos2d::GL::bindVAO(0);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

	if (uint32_t(w) != _width || uint32_t(h) != _height) {
		_width = w;
		_height = h;
		glBindRenderbuffer(GL_RENDERBUFFER, _rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, _width, _height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->getName(), 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _rbo);
	glViewport(0, 0, (GLsizei)_width, (GLsizei)_height);

	glClearColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	nvgBeginFrame(_context, _width, _height, 1.0f);
}

void CanvasNanoVG::end() {
	nvgEndFrame(_context);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, _oldFbo);

	if (ThreadManager::getInstance()->isMainThread()) {
		cocos2d::Director::getInstance()->setViewport();
		glUseProgram(0);
		glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
		cocos2d::GL::blendResetToCache();
	}
}

void CanvasNanoVG::scale(float sx, float sy) {
	nvgScale(_context, sx, sy);
}

void CanvasNanoVG::translate(float tx, float ty) {
	nvgTranslate(_context, tx, ty);
}

void CanvasNanoVG::save() {
	nvgSave(_context);
}

void CanvasNanoVG::restore() {
	nvgRestore(_context);
}

void CanvasNanoVG::setAntialiasing(bool value) { }

void CanvasNanoVG::setLineWidth(float value) {
	nvgStrokeWidth(_context, value);
}

void CanvasNanoVG::pathBegin() {
	_pathX = 0.0f; _pathY = 0.0f;
	nvgBeginPath(_context);
}

void CanvasNanoVG::pathClose() {
	_pathX = 0.0f; _pathY = 0.0f;
	nvgClosePath(_context);
}

void CanvasNanoVG::pathMoveTo(float x, float y) {
	_pathX = x; _pathY = y;
	nvgMoveTo(_context, x, y);
}

void CanvasNanoVG::pathLineTo(float x, float y) {
	_pathX = x; _pathY = y;
	nvgLineTo(_context, x, y);
}

void CanvasNanoVG::pathQuadTo(float x1, float y1, float x2, float y2) {
	_pathX = x2; _pathY = y2;
	//nvgQuadTo(_context, x1, y1, x2, y2);
	nvgLineTo(_context, x2, y2);
}

void CanvasNanoVG::pathCubicTo(float x1, float y1, float x2, float y2, float x3, float y3) {
	_pathX = x3; _pathY = y3;
	//nvgBezierTo(_context, x1, y1, x2, y2, x3, y3);
	nvgLineTo(_context, x3, y3);
}

void CanvasNanoVG::pathArcTo(float xc, float yc, float rx, float ry, float startAngle, float sweepAngle, float rotation) {
	// should be expressed as pathAltArcTo as in https://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes
	// (Conversion from center to endpoint parameterization)
}

static void nsvg__xformPoint(float* dx, float* dy, float x, float y, float* t) {
	*dx = x*t[0] + y*t[2] + t[4];
	*dy = x*t[1] + y*t[3] + t[5];
}

static void nsvg__xformVec(float* dx, float* dy, float x, float y, float* t) {
	*dx = x*t[0] + y*t[2];
	*dy = x*t[1] + y*t[3];
}

static float nsvg__sqr(float x) { return x*x; }
static float nsvg__vmag(float x, float y) { return sqrtf(x*x + y*y); }

static float nsvg__vecrat(float ux, float uy, float vx, float vy)
{
	return (ux*vx + uy*vy) / (nsvg__vmag(ux,uy) * nsvg__vmag(vx,vy));
}

static float nsvg__vecang(float ux, float uy, float vx, float vy)
{
	float r = nsvg__vecrat(ux,uy, vx,vy);
	if (r < -1.0f) r = -1.0f;
	if (r > 1.0f) r = 1.0f;
	return ((ux*vy < uy*vx) ? -1.0f : 1.0f) * acosf(r);
}

void CanvasNanoVG::pathAltArcTo(float rx, float ry, float rotx, bool fa, bool fs, float x2, float y2) {
	// Ported from nanosvg (https://github.com/memononen/nanosvg)
	// Originally ported from canvg (https://code.google.com/p/canvg/)
	float x1, y1, cx, cy, dx, dy, d;
	float x1p, y1p, cxp, cyp, s, sa, sb;
	float ux, uy, vx, vy, a1, da;
	float x, y, tanx, tany, a, px = 0, py = 0, ptanx = 0, ptany = 0, t[6];
	float sinrx, cosrx;
	int i, ndivs;
	float hda, kappa;

	x1 = _pathX; // start point
	y1 = _pathY;

	dx = x1 - x2;
	dy = y1 - y2;
	d = sqrtf(dx*dx + dy*dy);
	if (d < 1e-6f || rx < 1e-6f || ry < 1e-6f) {
		// The arc degenerates to a line
		pathLineTo(x2, y2);
		_pathX = x2;
		_pathY = y2;
		return;
	}

	sinrx = sinf(rotx);
	cosrx = cosf(rotx);

	// Convert to center point parameterization.
	// http://www.w3.org/TR/SVG11/implnote.html#ArcImplementationNotes
	// 1) Compute x1', y1'
	x1p = cosrx * dx / 2.0f + sinrx * dy / 2.0f;
	y1p = -sinrx * dx / 2.0f + cosrx * dy / 2.0f;
	d = nsvg__sqr(x1p)/nsvg__sqr(rx) + nsvg__sqr(y1p)/nsvg__sqr(ry);
	if (d > 1) {
		d = sqrtf(d);
		rx *= d;
		ry *= d;
	}
	// 2) Compute cx', cy'
	s = 0.0f;
	sa = nsvg__sqr(rx)*nsvg__sqr(ry) - nsvg__sqr(rx)*nsvg__sqr(y1p) - nsvg__sqr(ry)*nsvg__sqr(x1p);
	sb = nsvg__sqr(rx)*nsvg__sqr(y1p) + nsvg__sqr(ry)*nsvg__sqr(x1p);
	if (sa < 0.0f) sa = 0.0f;
	if (sb > 0.0f)
		s = sqrtf(sa / sb);
	if (fa == fs)
		s = -s;
	cxp = s * rx * y1p / ry;
	cyp = s * -ry * x1p / rx;

	// 3) Compute cx,cy from cx',cy'
	cx = (x1 + x2)/2.0f + cosrx*cxp - sinrx*cyp;
	cy = (y1 + y2)/2.0f + sinrx*cxp + cosrx*cyp;

	// 4) Calculate theta1, and delta theta.
	ux = (x1p - cxp) / rx;
	uy = (y1p - cyp) / ry;
	vx = (-x1p - cxp) / rx;
	vy = (-y1p - cyp) / ry;
	a1 = nsvg__vecang(1.0f,0.0f, ux,uy);	// Initial angle
	da = nsvg__vecang(ux,uy, vx,vy);		// Delta angle

//	if (vecrat(ux,uy,vx,vy) <= -1.0f) da = NSVG_PI;
//	if (vecrat(ux,uy,vx,vy) >= 1.0f) da = 0;

	if (fa) {
		// Choose large arc
		if (da > 0.0f)
			da = da - 2*M_PI;
		else
			da = 2*M_PI + da;
	}

	// Approximate the arc using cubic spline segments.
	t[0] = cosrx; t[1] = sinrx;
	t[2] = -sinrx; t[3] = cosrx;
	t[4] = cx; t[5] = cy;

	// Split arc into max 90 degree segments.
	// The loop assumes an iteration per end point (including start and end), this +1.
	ndivs = (int)(fabsf(da) / (M_PI*0.5f) + 1.0f);
	hda = (da / (float)ndivs) / 2.0f;
	kappa = fabsf(4.0f / 3.0f * (1.0f - cosf(hda)) / sinf(hda));
	if (da < 0.0f)
		kappa = -kappa;

	for (i = 0; i <= ndivs; i++) {
		a = a1 + da * (i/(float)ndivs);
		dx = cosf(a);
		dy = sinf(a);
		nsvg__xformPoint(&x, &y, dx*rx, dy*ry, t); // position
		nsvg__xformVec(&tanx, &tany, -dy*rx * kappa, dx*ry * kappa, t); // tangent
		if (i > 0)
			pathCubicTo(px+ptanx,py+ptany, x-tanx, y-tany, x, y);
		px = x;
		py = y;
		ptanx = tanx;
		ptany = tany;
	}

	_pathX = x2;
	_pathY = y2;
}

void CanvasNanoVG::pathFill(const Color4B &fill) {
	nvgFillColor(_context, nvgRGBA(fill.r, fill.g, fill.b, fill.a));
	nvgFill(_context);
}

void CanvasNanoVG::pathStroke(const Color4B &stroke) {
	nvgStrokeColor(_context, nvgRGBA(stroke.r, stroke.g, stroke.b, stroke.a));
	nvgStroke(_context);
}

void CanvasNanoVG::pathFillStroke(const Color4B &fill, const Color4B &stroke) {
	nvgFillColor(_context, nvgRGBA(fill.r, fill.g, fill.b, fill.a));
	nvgFill(_context);
	nvgStrokeColor(_context, nvgRGBA(stroke.r, stroke.g, stroke.b, stroke.a));
	nvgStroke(_context);
}

NS_SP_EXT_END(draw)

#endif // SP_NANOVG
