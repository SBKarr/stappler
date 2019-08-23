// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPDynamicLabel.h"

#include "renderer/CCTexture2D.h"
#include "renderer/ccGLStateCache.h"
#include "RTDrawer.h"
#include "RTDrawerRequest.h"
#include "RTCommonSource.h"

#include "SPBitmap.h"
#include "SPString.h"
#include "SPTextureCache.h"
#include "SPThreadManager.h"
#include "SPDrawCanvas.h"
#include "SLImage.h"
#include "SLTesselator.h"

NS_SP_EXT_BEGIN(rich_text)

bool Drawer::init(StencilDepthFormat fmt) {
	auto &thread = TextureCache::thread();
	if (thread.isOnThisThread()) {
		initWithThread(fmt);
		_cacheUpdated = true;
		return true;
	} else {
		thread.perform([this, fmt] (const Task &) -> bool {
			initWithThread(fmt);
			return true;
		}, [this] (const Task &, bool) {
			_cacheUpdated = true;
		}, this);
		return true;
	}
}

void Drawer::initWithThread(StencilDepthFormat fmt) {
	TextureCache::getInstance()->performWithGL([&] {
		GLRenderSurface::initializeSurface(fmt);
		_vectorCanvas.init();
		_vectorCanvas.setFlushCallback([this] {
			GLRenderSurface::flushVectorCanvas(&_vectorCanvas);
		});
	});
}

void Drawer::free() {
	auto &thread = TextureCache::thread();
	if (thread.isOnThisThread()) {
		GLRenderSurface::finalizeSurface();
	} else {
		thread.perform([this] (const Task &) -> bool {
			TextureCache::getInstance()->performWithGL([&] {
				GLRenderSurface::finalizeSurface();
			});
			return true;
		}, nullptr, this);
	}
}

// draw normal texture
bool Drawer::draw(CommonSource *s, Result *res, const Rect &r, const Callback &cb, cocos2d::Ref *ref) {
	if (auto req = Rc<Request>::create(this, s, res, r, cb, ref)) {
		req->run();
		return true;
	}
	return false;
}

// draw thumbnail texture, where scale < 1.0f - resample coef
bool Drawer::thumbnail(CommonSource *s, Result *res, const Rect &r, float scale, const Callback &cb, cocos2d::Ref *ref) {
	if (auto req = Rc<Request>::create(this, s, res, r, scale, cb, ref)) {
		req->run();
		return true;
	}
	return false;
}

bool Drawer::begin(cocos2d::Texture2D * tex, const Color4B &clearColor, float density) {
	if (_fbo == 0) {
		return false;
	}

	if (GLRenderSurface::begin(tex, clearColor, true)) {
		_density = density;
		_vectorCanvas.save();
		_vectorCanvas.scale(_density, _density);

		return true;
	}

	return false;
}

void Drawer::end() {
	_vectorCanvas.restore();
	GLRenderSurface::end();
}

void Drawer::drawResizeBuffer(size_t count) {
	if (count <= _indexBufferSize) {
		return;
	}

	_indexBufferSize = count;

    Vector<GLushort> indices;
    indices.reserve(count * 6);

    for (size_t i= 0; i < count; i++) {
        indices.push_back( i*4 + 0 );
        indices.push_back( i*4 + 1 );
        indices.push_back( i*4 + 2 );

        indices.push_back( i*4 + 3 );
        indices.push_back( i*4 + 2 );
        indices.push_back( i*4 + 1 );
    }

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (sizeof(GLushort) * count * 6), (const GLvoid *) indices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Drawer::resetSurface() {
	GLRenderSurface::resetSurface();

	_vertexBufferSize = 0;
	_indexBufferSize = 0;
	_cache.clear();
}

void Drawer::setLineWidth(float value) {
	if (value != _lineWidth) {
		_lineWidth = value;
	}
}

void Drawer::setColor(const Color4B &color) {
	if (color != _color) {
		_color = color;
	}
}

void Drawer::drawPath(const Rect &bbox, const layout::Path &path) {
	_vectorCanvas.save();
	_vectorCanvas.draw(path, bbox.origin.x, bbox.origin.y);
	_vectorCanvas.restore();
}

static void Drawer_makeRect(Vector<GLfloat> &vertices, float x, float y, float width, float height) {
	vertices.push_back(x);			vertices.push_back(y);
	vertices.push_back(x + width);	vertices.push_back(y);
	vertices.push_back(x);			vertices.push_back(y + height);
	vertices.push_back(x + width);	vertices.push_back(y + height);
}

void Drawer::drawTexture(const Rect &bbox, cocos2d::Texture2D *tex, const Rect &texRect) {
	auto programs = TextureCache::getInstance()->getPrograms();
	cocos2d::GLProgram * p = nullptr;

	bool colorize = tex->getReferenceFormat() == cocos2d::Texture2D::PixelFormat::A8
			|| tex->getReferenceFormat() == cocos2d::Texture2D::PixelFormat::AI88;

	if (colorize) {
		p = programs->getProgram(GLProgramDesc(GLProgramDesc::Attr::MatrixMVP | GLProgramDesc::Attr::AmbientColor
				| GLProgramDesc::Attr::TexCoords | GLProgramDesc::Attr::MediumP,
				tex->getPixelFormat(), tex->getReferenceFormat()));
	} else {
		p = programs->getProgram(GLProgramDesc(GLProgramDesc::Attr::MatrixMVP
				| GLProgramDesc::Attr::TexCoords | GLProgramDesc::Attr::MediumP,
				tex->getPixelFormat(), tex->getReferenceFormat()));
	}


	auto w = tex->getPixelsWide();
	auto h = tex->getPixelsHigh();

	Mat4 transform;
	transform.m[0] = 1.0f;
	transform.m[5] = 1.0f;
	transform.m[10] = 1.0f;
	transform.m[12] = bbox.origin.x;
	transform.m[13] = _height - bbox.origin.y - bbox.size.height;

	transform = _viewTransform * transform;

	bindTexture(tex->getName());
	useProgram(p->getProgram());
	if (colorize) {
		p->setUniformLocationWith4f(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_AMBIENT_COLOR),
			_color.r / 255.0f, _color.g / 255.0f, _color.b / 255.0f, _color.a / 255.0f);
	}
    p->setUniformLocationWithMatrix4fv(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_MVP_MATRIX), transform.m, 1);

    GLfloat coordinates[] = {
    		(texRect.origin.x / w) , ((texRect.origin.y + texRect.size.height) / h),
			((texRect.origin.x + texRect.size.width) / w), ((texRect.origin.y + texRect.size.height) / h),
			(texRect.origin.x / w), (texRect.origin.y / h),
			((texRect.origin.x + texRect.size.width) / w), (texRect.origin.y / h) };

    GLfloat vertices[] = {
		0.0f, 0.0f,
		bbox.size.width, 0.0f,
		0.0f, bbox.size.height,
		bbox.size.width, bbox.size.height,
    };
	if (!tex->hasPremultipliedAlpha()) {
		blendFunc(cocos2d::BlendFunc::ALPHA_NON_PREMULTIPLIED);
	} else {
		blendFunc(cocos2d::BlendFunc::ALPHA_PREMULTIPLIED);
	}

    enableVertexAttribs( cocos2d::GL::VERTEX_ATTRIB_FLAG_POSITION | cocos2d::GL::VERTEX_ATTRIB_FLAG_TEX_COORD );
    glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_TEX_COORD, 2, GL_FLOAT, GL_FALSE, 0, coordinates);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	CHECK_GL_ERROR_DEBUG();
}

void Drawer::drawCharRects(Font *f, const font::FormatSpec &format, const Rect & bbox, float scale) {
	DynamicLabel::makeLabelRects(f, &format, scale, [&] (const Vector<Rect> &rects) {
		drawRects(bbox, rects);
	});
}

void Drawer::drawRects(const Rect &bbox, const Vector<Rect> &rects) {
	Vector<GLfloat> vertices; vertices.reserve(rects.size() * 12);

	auto programs = TextureCache::getInstance()->getPrograms();
	auto p = programs->getProgram(GLProgramDesc::Default::RawRects);

	bindTexture(0);
	blendFunc(cocos2d::BlendFunc::ALPHA_NON_PREMULTIPLIED);

	Mat4 transform;
	transform.m[0] = 1.0f;
	transform.m[5] = 1.0f;
	transform.m[10] = 1.0f;
	transform.m[12] = bbox.origin.x;
	transform.m[13] = _height - bbox.origin.y - bbox.size.height;

	transform = _viewTransform * transform;

	useProgram(p->getProgram());
	p->setUniformLocationWith4f(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_AMBIENT_COLOR),
			_color.r / 255.0f, _color.g / 255.0f, _color.b / 255.0f, _color.a / 255.0f);
	p->setUniformLocationWithMatrix4fv(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_MVP_MATRIX),
			transform.m, 1);

	for (auto &it : rects) {
		Drawer_makeRect(vertices, it.origin.x, it.origin.y, it.size.width, it.size.height);
	}

	drawResizeBuffer(rects.size());

	glBindBuffer(GL_ARRAY_BUFFER, _vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

	enableVertexAttribs(cocos2d::GL::VERTEX_ATTRIB_FLAG_POSITION);
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo[1]);
	glDrawElements(GL_TRIANGLES, (GLsizei) rects.size() * 6, GL_UNSIGNED_SHORT, (GLvoid *) (0));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	CHECK_GL_ERROR_DEBUG();
}

void Drawer::drawChars(Font *f, const font::FormatSpec &format, const Rect & bbox) {
	DynamicLabel::makeLabelQuads(f, &format,
			[&] (const DynamicLabel::TextureVec &newTex, DynamicLabel::QuadVec &&newQuads, DynamicLabel::ColorMapVec &&cMap) {
		drawCharsQuads(newTex, std::move(newQuads), bbox);
	});
}

void Drawer::drawCharsQuads(const Vector<Rc<cocos2d::Texture2D>> &tex, Vector<Rc<DynamicQuadArray>> &&quads, const Rect & bbox) {
	bindTexture(0);

	for (size_t i = 0; i < quads.size(); ++ i) {
		drawCharsQuads(tex[i], quads[i], bbox);
	}
}

void Drawer::drawCharsQuads(cocos2d::Texture2D *tex, DynamicQuadArray *quads, const Rect & bbox) {
	auto programs = TextureCache::getInstance()->getPrograms();
	auto p = programs->getProgram(GLProgramDesc::Default::DynamicBatchA8Highp);
	useProgram(p->getProgram());

	Mat4 transform;
	transform.m[0] = 1.0f;
	transform.m[5] = 1.0f;
	transform.m[10] = 1.0f;
	transform.m[12] = bbox.origin.x;
	transform.m[13] = _height - bbox.origin.y - bbox.size.height;

	transform = _viewTransform * transform;

    p->setUniformLocationWithMatrix4fv(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_MVP_MATRIX), transform.m, 1);

	bindTexture(tex->getName());

	auto count = quads->size();
	size_t bufferSize = sizeof(cocos2d::V3F_C4B_T2F_Quad) * count;

	drawResizeBuffer(count);

	glBindBuffer(GL_ARRAY_BUFFER, _vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, bufferSize, quads->getData(), GL_DYNAMIC_DRAW);

    blendFunc(cocos2d::BlendFunc::ALPHA_NON_PREMULTIPLIED);
	enableVertexAttribs(cocos2d::GL::VERTEX_ATTRIB_FLAG_POS_COLOR_TEX);

	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE,
			sizeof(cocos2d::V3F_C4B_T2F), (GLvoid*) offsetof(cocos2d::V3F_C4B_T2F, vertices));

	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE,
			sizeof(cocos2d::V3F_C4B_T2F), (GLvoid*) offsetof(cocos2d::V3F_C4B_T2F, colors));

	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_TEX_COORD, 2, GL_FLOAT, GL_FALSE,
			sizeof(cocos2d::V3F_C4B_T2F), (GLvoid*) offsetof(cocos2d::V3F_C4B_T2F, texCoords));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo[1]);

	glDrawElements(GL_TRIANGLES, (GLsizei) count * 6, GL_UNSIGNED_SHORT, (GLvoid *) (0));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	CHECK_GL_ERROR_DEBUG();
}

void Drawer::update() {
	if (!_cacheUpdated) {
		return;
	}

	if (Time::now() - _updated > TimeInterval::seconds(1)) {
		_cacheUpdated = false;
		auto &thread = TextureCache::thread();
		if (thread.isOnThisThread()) {
			TextureCache::getInstance()->performWithGL([&] {
				performUpdate();
			});
			_cacheUpdated = true;
			_updated = Time::now();
		} else {
			thread.perform([this] (const Task &) -> bool {
				TextureCache::getInstance()->performWithGL([&] {
					performUpdate();
				});
				return true;
			}, [this] (const Task &, bool) {
				_cacheUpdated = true;
				_updated = Time::now();
			});
		}
	}
}

void Drawer::clearCache() {
	auto &thread = TextureCache::thread();
	if (thread.isOnThisThread()) {
		TextureCache::getInstance()->performWithGL([&] {
			_cache.clear();
		});
	} else {
		thread.perform([this] (const Task &) -> bool {
			TextureCache::getInstance()->performWithGL([&] {
				_cache.clear();
			});
			return true;
		});
	}
}

void Drawer::performUpdate() {
	if (_cache.empty()) {
		return;
	}

	auto time = Time::now();
	Vector<String> keys;
	for (auto &it : _cache) {
		if (it.second.second - time > TimeInterval::seconds(30)) {
			keys.push_back(it.first);
		}
	}

	for (auto &it : keys) {
		_cache.erase(it);
	}
}

void Drawer::addBitmap(const StringView &str, cocos2d::Texture2D *bmp) {
	_cache.emplace(str.str(), pair(bmp, Time::now()));
}

void Drawer::addBitmap(const StringView &str, cocos2d::Texture2D *bmp, const Size &size) {
	addBitmap(toString(str, "?w=", int(roundf(size.width)), "&h=", int(roundf(size.height))), bmp);
}

cocos2d::Texture2D *Drawer::getBitmap(const StringView &key) {
	auto it = _cache.find(key);
	if (it != _cache.end()) {
		it->second.second = Time::now();
		return it->second.first;
	}
	return nullptr;
}

cocos2d::Texture2D *Drawer::getBitmap(const StringView &key, const Size &size) {
	return getBitmap(toString(key, "?w=", int(roundf(size.width)), "&h=", int(roundf(size.height))));
}

Thread &Drawer::thread() {
	return TextureCache::thread();
}

NS_SP_EXT_END(rich_text)
