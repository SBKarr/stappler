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
#include "SPDrawCanvasLibtess2.h"
#include "SPThreadManager.h"
#include "SPTextureCache.h"
#include "renderer/CCTexture2D.h"
#include "renderer/ccGLStateCache.h"

#if SP_LIBTESS2

NS_SP_EXT_BEGIN(draw)

void* CanvasLibtess2::staticPoolAlloc(void* userData, unsigned int size) {
	CanvasLibtess2 *canvas = (CanvasLibtess2 *)userData;
	size = (size+0x7) & ~0x7;
	return canvas->poolAlloc(size);
}

void* CanvasLibtess2::staticPoolRealloc(void* userData, void *ptr, unsigned int size) {
	CanvasLibtess2 *canvas = (CanvasLibtess2 *)userData;
	size = (size+0x7) & ~0x7;
	return canvas->poolRealloc(ptr, size);
}

void CanvasLibtess2::staticPoolFree(void * userData, void * ptr) {
	// empty
	TESS_NOTUSED(userData);
	TESS_NOTUSED(ptr);
}

CanvasLibtess2::~CanvasLibtess2() {
	if (_vbo[0] || _vbo[1]) {
		glDeleteBuffers(2, _vbo);
	}

	if (_rbo) {
		glDeleteRenderbuffers(1, &_rbo);
		_rbo = 0;
	}

	if (_fbo) {
		glDeleteFramebuffers(1, &_fbo);
		_fbo = 0;
	}
}

bool CanvasLibtess2::init() {
	glGenFramebuffers(1, &_fbo);
	glGenRenderbuffers(1, &_rbo);
	glGenBuffers(2, _vbo);
	if (_fbo != 0 && _vbo[0] != 0 && _vbo[1] != 0) {
		memset(&_tessAlloc, 0, sizeof(_tessAlloc));
		_tessAlloc.memalloc = &CanvasLibtess2::staticPoolAlloc;
		_tessAlloc.memfree = &CanvasLibtess2::staticPoolFree;
		_tessAlloc.memrealloc = &CanvasLibtess2::staticPoolRealloc;
		_tessAlloc.userData = (void*)this;

		return true;
	}

	return false;
}

void CanvasLibtess2::begin(cocos2d::Texture2D *tex, const Color4B &color) {
	auto w = tex->getPixelsWide();
	auto h = tex->getPixelsHigh();
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_oldFbo);

	if (ThreadManager::getInstance()->isMainThread()) {
		cocos2d::GL::useProgram(0);
		cocos2d::GL::enableVertexAttribs(0);
		cocos2d::GL::bindTexture2D(0);
		cocos2d::GL::bindVAO(0);
	}


	if (uint32_t(w) != _width || uint32_t(h) != _height) {
		_width = w;
		_height = h;
		//glBindRenderbuffer(GL_RENDERBUFFER, _rbo);
		//glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA4, _width, _height);
		//glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _rbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->getName(), 0);

	auto check = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch (check) {
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		log::format("Framebuffer", "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		log::format("Framebuffer", "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
		break;
	case GL_FRAMEBUFFER_UNSUPPORTED:
		log::format("Framebuffer", "GL_FRAMEBUFFER_UNSUPPORTED");
		break;
#ifndef GL_ES_VERSION_2_0
	case GL_FRAMEBUFFER_UNDEFINED:
		log::format("Framebuffer", "GL_FRAMEBUFFER_UNDEFINED");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		log::format("Framebuffer", "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE");
		break;
#endif
	case GL_FRAMEBUFFER_COMPLETE:
		log::format("Framebuffer", "GL_FRAMEBUFFER_COMPLETE");
		break;
	case 0:
		log::format("Framebuffer", "Success");
		break;
	default:
		log::format("Framebuffer", "Undefined %d", check);
		break;
	}

	glViewport(0, 0, (GLsizei)_width, (GLsizei)_height);

	glClearColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT /* | GL_STENCIL_BUFFER_BIT */);

	int32_t gcd = sp_gcd(_width, _height);
	int32_t dw = (int32_t)_width / gcd;
	int32_t dh = (int32_t)_height / gcd;
	int32_t dwh = gcd * dw * dh;

	float mod = 1.0f;
	while (dwh * mod > 16384) {
		mod /= 2.0f;
	}

	_transform = Mat4::IDENTITY;
	_transform.scale(dh * mod, dw * mod, -1.0);
	_transform.m[12] = -dwh * mod / 2.0f;
	_transform.m[13] = -dwh * mod / 2.0f;
	_transform.m[14] = dwh * mod / 2.0f - 1;
	_transform.m[15] = dwh * mod / 2.0f + 1;
	_transform.m[11] = -1.0f;

	_verts.reserve(256);

	_program = TextureCache::getInstance()->getRawPrograms()->getProgram(GLProgramSet::RawRects);


	/*blendFunc(cocos2d::BlendFunc::ALPHA_NON_PREMULTIPLIED);
	bindTexture(0);
	useProgram(_program->getProgram());

	//_program->setUniformLocationWith4f(_program->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_AMBIENT_COLOR),
	//		fill.r / 255.0f, fill.g / 255.0f, fill.b / 255.0f, fill.a / 255.0f);
	_program->setUniformLocationWithMatrix4fv(_program->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_MVP_MATRIX),
			_transform.m, 1);
	_program->setUniformLocationWith4f(_program->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_AMBIENT_COLOR),
			0.0f, 0.0f, 0.0f, 1.0f);

	GLfloat vertices[] = { _width * 0.2f, _height * 0.2f, _width * 0.8f, _height * 0.2f, _width * 0.2f, _height * 0.8f, _width * 0.8f, _height * 0.8f};

	enableVertexAttribs(cocos2d::GL::VERTEX_ATTRIB_FLAG_POSITION);
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);*/
}

void CanvasLibtess2::end() {
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, _oldFbo);

	cleanup();
}

void CanvasLibtess2::scale(float sx, float sy) {
	//log::text("CanvasMonkVG", __FUNCTION__);
	_transform.scale(sx, sy, 1.0f);
}

void CanvasLibtess2::translate(float tx, float ty) {
	//log::text("CanvasMonkVG", __FUNCTION__);
	_transform.translate(tx, ty, 0.0f);
}
void CanvasLibtess2::transform(const Mat4 &t) {
	_transform *= t;
}

void CanvasLibtess2::save() {
	_states.push_back(_transform);

	//log::text("CanvasMonkVG", __FUNCTION__);
}

void CanvasLibtess2::restore() {
	if (!_states.empty()) {
		_transform = _states.back();
		_states.pop_back();
	}
	//log::text("CanvasMonkVG", __FUNCTION__);
}

void CanvasLibtess2::setAntialiasing(bool value) {
	//log::text("CanvasMonkVG", __FUNCTION__);

}

void CanvasLibtess2::setLineWidth(float value) {
	//log::text("CanvasMonkVG", __FUNCTION__);
	_lineWidth = value;
	//nvgStrokeWidth(_context, value);
}

void CanvasLibtess2::pathBegin() {
	//log::text("CanvasMonkVG", __FUNCTION__);
	_pathX = 0.0f; _pathY = 0.0f;
}

void CanvasLibtess2::pathClose() {
	if (!_contours.empty()) {
		auto &c = _contours.back();
		c.closed = true;
	}
	_pathX = 0; _pathY = 0;
	//log::format("CanvasMonkVG", "%s %d", __FUNCTION__, 1);
}

void CanvasLibtess2::pathMoveTo(float x, float y) {
	_contours.push_back(Contour{_verts.size(), 1, false});
	_verts.push_back(x);
	_verts.push_back(y);
	_pathX = x; _pathY = y;
	//log::format("CanvasMonkVG", "%s %d", __FUNCTION__, 1);
}

void CanvasLibtess2::pathLineTo(float x, float y) {
	if (_contours.empty() || _contours.back().closed) {
		_contours.push_back(Contour{_verts.size(), 1, false});
		_verts.push_back(0);
		_verts.push_back(0);
	}

	++ _contours.back().count;
	_verts.push_back(x);
	_verts.push_back(y);
	_pathX = x; _pathY = y;
	//log::format("CanvasMonkVG", "%s %d", __FUNCTION__, 1);
}

void CanvasLibtess2::pathQuadTo(float x1, float y1, float x2, float y2) {
	if (_contours.empty() || _contours.back().closed) {
		_contours.push_back(Contour{_verts.size(), 1, false});
		_verts.push_back(0);
		_verts.push_back(0);
	}

	Vec3 scale; _transform.getScale(&scale);
	_contours.back().count += layout::drawQuadBezier(_verts, std::max(scale.x, scale.y) * 0.5f, 0.0f, _pathX, _pathY, x1, y1, x2, y2);
	_pathX = x2; _pathY = y2;
	//log::format("CanvasMonkVG", "%s %d", __FUNCTION__, 1);
}

void CanvasLibtess2::pathCubicTo(float x1, float y1, float x2, float y2, float x3, float y3) {
	if (_contours.empty() || _contours.back().closed) {
		_contours.push_back(Contour{_verts.size(), 1, false});
		_verts.push_back(0);
		_verts.push_back(0);
	}

	Vec3 scale; _transform.getScale(&scale);
	_contours.back().count += layout::drawCubicBezier(_verts, std::max(scale.x, scale.y) * 0.5f, 0.0f, _pathX, _pathY, x1, y1, x2, y2, x3, y3);
	_pathX = x3; _pathY = y3;
	//log::format("CanvasMonkVG", "%s %d", __FUNCTION__, 1);
}

void CanvasLibtess2::pathArcTo(float rx, float ry, float rot, bool fa, bool fs, float x2, float y2) {
	if (_contours.empty() || _contours.back().closed) {
		_contours.push_back(Contour{_verts.size(), 1, false});
		_verts.push_back(0);
		_verts.push_back(0);
	}

	Vec3 scale; _transform.getScale(&scale);
	_contours.back().count += layout::drawArc(_verts, std::max(scale.x, scale.y) * 0.5f, 0.0f, _pathX, _pathY, rx, ry, rot, fa, fs, x2, y2);
	_pathX = x2; _pathY = y2;
	//log::format("CanvasMonkVG", "%s %d", __FUNCTION__, 1);
}

void CanvasLibtess2::pathFill(const Color4B &fill) {
	bool sw = false;
	for (auto &it : _verts) {
		//std::cout << it;
		if (sw) {
			//std::cout << '\n';
			sw = false;
		} else {
			//std::cout << ", ";
			sw = true;
		}
	}
	// std::cout << '\n';

	_tess = tessNewTess(&_tessAlloc);

	for (auto &it : _contours) {
		tessAddContour(_tess, 2, _verts.data() + it.start, sizeof(float) * 2, it.count);
	}

	if (tessTesselate(_tess, _currentPath->getWindingRule()==layout::Path::Winding::NonZero?TESS_WINDING_NONZERO:TESS_WINDING_ODD,
			TESS_POLYGONS, 3, 2, nullptr)) {
		const float* verts = tessGetVertices(_tess);
		const int nverts = tessGetVertexCount(_tess);

		auto elts = tessGetElements(_tess);
		auto nelts = tessGetElementCount(_tess);

		/*for (int i = 0; i < nverts; ++ i) {
			//log::format("vert", "%f %f", (verts + (i * 2))[0], (verts + (i * 2))[1] );
		}

		for (int i = 0; i < nelts; ++ i) {
			//log::text("CanvasLibtess2", "begin elt");
			for (int j = 0; j < 3; j ++) {
				//log::format("elts", "%d", elts[i * 3 + j] );
			}
		}*/

		blendFunc(cocos2d::BlendFunc::ALPHA_NON_PREMULTIPLIED);
		bindTexture(0);
		useProgram(_program->getProgram());

		_program->setUniformLocationWith4f(_program->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_AMBIENT_COLOR),
				fill.r / 255.0f, fill.g / 255.0f, fill.b / 255.0f, fill.a / 255.0f);
		_program->setUniformLocationWithMatrix4fv(_program->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_MVP_MATRIX),
				_transform.m, 1);
		//_program->setUniformLocationWith4f(_program->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_AMBIENT_COLOR),
		//		0.0f, 0.0f, 0.0f, 1.0f);

		glBindBuffer(GL_ARRAY_BUFFER, _vbo[0]);
		enableVertexAttribs(cocos2d::GL::VERTEX_ATTRIB_FLAG_POSITION);
		glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBufferData(GL_ARRAY_BUFFER, nverts * sizeof(float) * 2, verts, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo[1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (sizeof(GLuint) * nelts * 3), elts, GL_STATIC_DRAW);

		glDrawElements(GL_TRIANGLES, (GLsizei) nelts * 3, GL_UNSIGNED_INT, (GLvoid *) (0));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//glBufferData(GL_ARRAY_BUFFER, _verts.size() * sizeof(float), _verts.data(), GL_DYNAMIC_DRAW);

		//glColor4b(0, 0, 0, 255);
		//glPointSize(2.0f);
		// glDrawArrays(GL_POINTS, 0, _verts.size() / 2);

	}

	tessDeleteTess(_tess);
	poolClear();

	_contours.clear();
	_verts.clear();
	//log::format("CanvasMonkVG", "%s %d", __FUNCTION__, 1);
}

void CanvasLibtess2::pathStroke(const Color4B &stroke) {
	/*auto paint = vgCreatePaint();
	VGfloat color[4] = { stroke.r / 255.0f, stroke.g / 255.0f, stroke.b / 255.0f, stroke.r / 255.0f };
	vgSetParameterfv(paint, VG_PAINT_COLOR, 4, &color[0]);
	vgSetPaint(paint, VG_STROKE_PATH );

	//vgDrawPath(_path, VG_STROKE_PATH);

	vgDestroyPaint(paint);
	vgDestroyPath(_path);
	_path = 0;*/

	_contours.clear();
	_verts.clear();
	//log::format("CanvasMonkVG", "%s %d", __FUNCTION__, 1);
}

void CanvasLibtess2::pathFillStroke(const Color4B &fill, const Color4B &stroke) {
	/*auto fillPaint = vgCreatePaint();
	VGfloat fillColor[4] = { fill.r / 255.0f, fill.g / 255.0f, fill.b / 255.0f, fill.r / 255.0f };
	vgSetParameterfv(fillPaint, VG_PAINT_COLOR, 4, &fillColor[0]);
	vgSetPaint(fillPaint, VG_FILL_PATH );

	auto strokePaint = vgCreatePaint();
	VGfloat strokeColor[4] = { stroke.r / 255.0f, stroke.g / 255.0f, stroke.b / 255.0f, stroke.r / 255.0f };
	vgSetParameterfv(strokePaint, VG_PAINT_COLOR, 4, &strokeColor[0]);
	vgSetPaint(strokePaint, VG_STROKE_PATH );

	//vgDrawPath(_path, VG_FILL_PATH | VG_STROKE_PATH);

	vgDestroyPaint(fillPaint);
	vgDestroyPaint(strokePaint);
	vgDestroyPath(_path);
	_path = 0;*/

	_contours.clear();
	_verts.clear();
	//log::format("CanvasMonkVG", "%s %d", __FUNCTION__, 1);
}

void *CanvasLibtess2::poolAlloc(uint32_t size) {
	return _pool.allocate(size);
}
void *CanvasLibtess2::poolRealloc(void *ptr, uint32_t newSize) {
	return _pool.reallocate(ptr, newSize);
}

size_t CanvasLibtess2::poolGetAllocated() const {
	return _pool.allocated();
}
void CanvasLibtess2::poolClear() {
	_pool.clear();
}

NS_SP_EXT_END(draw)

#endif // SP_LIBTESS2
