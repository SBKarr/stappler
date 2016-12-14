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
#include "SPRichTextDrawer.h"
#include "SPDynamicLabel.h"

#include "renderer/ccGLStateCache.h"
#include "renderer/CCTexture2D.h"
#include "platform/CCImage.h"
#include "base/CCConfiguration.h"
#include "base/CCDirector.h"
#include "base/CCScheduler.h"

#include "SPBitmap.h"
#include "SPString.h"
#include "SPDrawCanvas.h"
#include "SPTextureCache.h"

NS_SP_EXT_BEGIN(rich_text)

class Request : public cocos2d::Ref {
public:
	using Callback = std::function<void(cocos2d::Texture2D *)>;

	// draw normal texture
	bool init(Drawer *, Source *, Result *, const Rect &, const Callback &, cocos2d::Ref *);

	// draw thumbnail texture, where scale < 1.0f - resample coef
	bool init(Drawer *, Source *, Result *, const Rect &, float, const Callback &, cocos2d::Ref *);

protected:
	void onAssetCaptured();

	void draw(cocos2d::Texture2D *);
	void onDrawed(cocos2d::Texture2D *);

	Rect getRect(const Rect &) const;

	void drawRef(const cocos2d::Rect &bbox);
	void drawOutline(const Rect &bbox, const Outline &);
	void drawBitmap(const Rect &bbox, cocos2d::Texture2D *bmp, const Background &bg);
	void drawBackgroundImage(const Rect &bbox, const Background &bg);
	void drawBackgroundColor(const Rect &bbox, const Background &bg);
	void drawBackground(const Rect &bbox, const Background &bg);
	void drawLabel(const Rect &bbox, const Label &l);
	void drawObject(const Object &obj);

	Rect _rect;
	float _scale = 1.0f;
	float _density = 1.0f;
	bool _isThumbnail = false;
	bool _highlightRefs = false;

	uint16_t _width = 0;
	uint16_t _height = 0;
	uint16_t _stride = 0;

	Rc<Drawer> _drawer;
	Rc<Result> _result;
	Rc<Source> _source;
	Rc<font::Source> _font;

	Callback _callback = nullptr;
	Rc<cocos2d::Ref> _ref = nullptr;
};

// draw normal texture
bool Request::init(Drawer *drawer, Source *source, Result *result, const Rect &rect, const Callback &cb, cocos2d::Ref *ref) {
	if (!cb) {
		return false;
	}

	_rect = rect;
	_density = result->getMedia().density;
	_drawer = drawer;
	_source = source;
	_result = result;
	_ref = ref;
	_callback = cb;

	_width = (uint16_t)ceilf(_rect.size.width * _result->getMedia().density);
	_height = (uint16_t)ceilf(_rect.size.height * _result->getMedia().density);
	_source->retainReadLock(this, std::bind(&Request::onAssetCaptured, this));
	return true;
}

// draw thumbnail texture, where scale < 1.0f - resample coef
bool Request::init(Drawer *drawer, Source *source, Result *result, const Rect &rect, float scale, const Callback &cb, cocos2d::Ref *ref) {
	if (!cb) {
		return false;
	}

	_rect = rect;
	_scale = scale;
	_density = result->getMedia().density * scale;
	_isThumbnail = true;
	_drawer = drawer;
	_source = source;
	_result = result;
	_ref = ref;
	_callback = cb;

	_width = (uint16_t)ceilf(_rect.size.width * _result->getMedia().density * _scale);
	_height = (uint16_t)ceilf(_rect.size.height * _result->getMedia().density * _scale);
	_source->retainReadLock(this, std::bind(&Request::onAssetCaptured, this));
	return true;
}

void Request::onAssetCaptured() {
	if (!_source->isActual()) {
		_callback(nullptr);
		_source->getAsset()->releaseReadLock(this);
		return;
	}

	_font = _source->getSource();
	_font->unschedule();

	Rc<cocos2d::Texture2D> *ptr = new Rc<cocos2d::Texture2D>(nullptr);

	TextureCache::thread().perform([this, ptr] (Ref *) -> bool {
		TextureCache::getInstance()->performWithGL([&] {
			//auto now = Time::now();
			auto tex = Rc<cocos2d::Texture2D>::create(cocos2d::Texture2D::PixelFormat::RGBA8888, _width, _height);
			draw( tex );
			*ptr = tex;
			//log::format("Profiling", "Texture rendering: %lu", (Time::now() - now).toMicroseconds());
		});
		return true;
	}, [this, ptr] (Ref *, bool) {
		onDrawed(*ptr);
		_source->releaseReadLock(this);
		delete ptr;
	}, this);
}

void Request::draw(cocos2d::Texture2D *data) {
	if (!_isThumbnail) {
		auto bg = _result->getBackgroundColor();
		if (bg.a == 0) {
			bg.r = 255;
			bg.g = 255;
			bg.b = 255;
		}
		if (!_drawer->begin(data, bg)) {
			return;
		}
	} else {
		if (!_drawer->begin(data, Color4B(0, 0, 0, 0))) {
			return;
		}
	}

	Vector<const Object *> drawObjects;
	auto &objs = _result->getObjects();
	for (auto &obj : objs) {
		if (obj.bbox.intersectsRect(_rect)) {
			drawObjects.push_back(&obj);
			if (obj.type == Object::Type::Label) {
				const Label &l = obj.value.label;
				for (auto &it : l.format.ranges) {
					_font->addTextureChars(it.layout->getName(), l.format.chars, it.start, it.count);
				}
			} else if (obj.type == Object::Type::Background) {
				const Background &bg = obj.value.background;
				if (!bg.backgroundImage.empty()) {
					auto &src = bg.backgroundImage;
					auto document = _source->getDocument();
					if (document->hasImage(src)) {
						auto bmp = _drawer->getBitmap(src);
						if (!bmp) {
							Bitmap bmpSource(_source->getDocument()->getImageBitmap(src));
							if (bmpSource) {
								auto tex = TextureCache::uploadTexture(bmpSource);
								_drawer->addBitmap(src, tex);
							}
						}
					}
				}
			}
		}
	}

	if (_font->isDirty()) {
		_font->update(0.0f);
	}

	for (auto &obj : drawObjects) {
		drawObject(*obj);
	}

	_drawer->end();
}

Rect Request::getRect(const Rect &rect) const {
	return Rect(
			(rect.origin.x - _rect.origin.x) * _density,
			(rect.origin.y - _rect.origin.y) * _density,
			rect.size.width * _density,
			rect.size.height * _density);
}

void Request::drawRef(const Rect &bbox) {
	if (_isThumbnail) {
		return;
	}
	if (_highlightRefs) {
		_drawer->setColor(Color4B(127, 255, 0, 64));
		_drawer->drawRectangle(getRect(bbox), draw::Style::Fill);
	}
}

static void DrawerCanvas_prepareOutline(Drawer *canvas, const Outline::Params &outline, float density) {
	canvas->setColor(outline.color);
	canvas->setLineWidth(std::max(outline.width * density, 1.0f));
}

void Request::drawOutline(const Rect &bbox, const Outline &outline) {
	if (_isThumbnail) {
		return;
	}
	if (outline.isMono()) {
		DrawerCanvas_prepareOutline(_drawer, outline.top, _density);
		_drawer->drawRectangleOutline(getRect(bbox),
				outline.hasTopLine(), outline.hasRightLine(), outline.hasBottomLine(), outline.hasLeftLine());
	} else {
		if (outline.hasTopLine()) {
			DrawerCanvas_prepareOutline(_drawer, outline.top, _density);
			_drawer->drawRectangleOutline(getRect(bbox), true, false, false, false);
		}
		if (outline.hasRightLine()) {
			DrawerCanvas_prepareOutline(_drawer, outline.right, _density);
			_drawer->drawRectangleOutline(getRect(bbox), false, true, false, false);
		}
		if (outline.hasBottomLine()) {
			DrawerCanvas_prepareOutline(_drawer, outline.bottom, _density);
			_drawer->drawRectangleOutline(getRect(bbox), false, false, true, false);
		}
		if (outline.hasLeftLine()) {
			DrawerCanvas_prepareOutline(_drawer, outline.left, _density);
			_drawer->drawRectangleOutline(getRect(bbox), false, false, false, true);
		}
	}
}

void Request::drawBitmap(const Rect &origBbox, cocos2d::Texture2D *bmp, const Background &bg) {
	Rect bbox = origBbox;
	float coverRatio, containRatio;
	Size coverSize, containSize;

	auto w = bmp->getPixelsWide();
	auto h = bmp->getPixelsHigh();

	coverRatio = MAX(bbox.size.width / w, bbox.size.height / h);
	containRatio = MIN(bbox.size.width / w, bbox.size.height / h);

	coverSize = Size(w * coverRatio, h * coverRatio);
	containSize = Size(w * containRatio, h * containRatio);

	float boxWidth = 0.0f, boxHeight = 0.0f;
	switch (bg.backgroundSizeWidth.metric) {
	case style::Size::Metric::Contain: boxWidth = containSize.width; break;
	case style::Size::Metric::Cover: boxWidth = coverSize.width; break;
	case style::Size::Metric::Percent: boxWidth = bbox.size.width * bg.backgroundSizeWidth.value; break;
	case style::Size::Metric::Px: boxWidth = bg.backgroundSizeWidth.value; break;
	default: boxWidth = bbox.size.width; break;
	}

	switch (bg.backgroundSizeHeight.metric) {
	case style::Size::Metric::Contain: boxHeight = containSize.height; break;
	case style::Size::Metric::Cover: boxHeight = coverSize.height; break;
	case style::Size::Metric::Percent: boxHeight = bbox.size.height * bg.backgroundSizeHeight.value; break;
	case style::Size::Metric::Px: boxHeight = bg.backgroundSizeHeight.value; break;
	default: boxHeight = bbox.size.height; break;
	}

	if (bg.backgroundSizeWidth.metric == style::Size::Metric::Auto
			&& bg.backgroundSizeHeight.metric == style::Size::Metric::Auto) {
		boxWidth = w;
		boxHeight = h;
	} else if (bg.backgroundSizeWidth.metric == style::Size::Metric::Auto) {
		boxWidth = boxHeight * ((float)w / (float)h);
	} else if (bg.backgroundSizeHeight.metric == style::Size::Metric::Auto) {
		boxHeight = boxWidth * ((float)h / (float)w);
	}

	float availableWidth = bbox.size.width - boxWidth, availableHeight = bbox.size.height - boxHeight;
	float xOffset = 0.0f, yOffset = 0.0f;

	switch (bg.backgroundPositionX.metric) {
	case style::Size::Metric::Percent: xOffset = availableWidth * bg.backgroundPositionX.value; break;
	case style::Size::Metric::Px: xOffset = bg.backgroundPositionX.value; break;
	default: xOffset = availableWidth / 2.0f; break;
	}

	switch (bg.backgroundPositionY.metric) {
	case style::Size::Metric::Percent: yOffset = availableHeight * bg.backgroundPositionY.value; break;
	case style::Size::Metric::Px: yOffset = bg.backgroundPositionY.value; break;
	default: yOffset = availableHeight / 2.0f; break;
	}

	Rect contentBox(0, 0, w, h);

	if (boxWidth < bbox.size.width) {
		bbox.size.width = boxWidth;
		bbox.origin.x += xOffset;
	} else if (boxWidth > bbox.size.width) {
		contentBox.size.width = bbox.size.width * w / boxWidth;
		contentBox.origin.x -= xOffset * (w / boxWidth);
	}

	if (boxHeight < bbox.size.height) {
		bbox.size.height = boxHeight;
		bbox.origin.y += yOffset;
	} else if (boxHeight > bbox.size.height) {
		contentBox.size.height = bbox.size.height * h / boxHeight;
		contentBox.origin.y -= yOffset * (h / boxHeight);
	}

	bbox = getRect(bbox);
	_drawer->drawTexture(bbox, bmp, contentBox);
}

void Request::drawBackgroundImage(const cocos2d::Rect &bbox, const Background &bg) {
	auto src = bg.backgroundImage;
	auto document = _source->getDocument();
	if (!document->hasImage(src)) {
		return;
	}

	auto bmp = _drawer->getBitmap(src);
	if (bmp) {
		drawBitmap(bbox, bmp, bg);
	}
}

void Request::drawBackgroundColor(const Rect &bbox, const Background &bg) {
	auto &color = bg.backgroundColor;
	_drawer->setColor(color);
	_drawer->drawRectangle(getRect(bbox), draw::Style::Fill);
}

void Request::drawBackground(const Rect &bbox, const Background &bg) {
	if (_isThumbnail) {
		_drawer->setColor(Color4B(127, 127, 127, 127));
		_drawer->drawRectangle(getRect(bbox), draw::Style::Fill);
		return;
	}
	if (bg.backgroundColor.a != 0) {
		drawBackgroundColor(bbox, bg);
	}
	if (!bg.backgroundImage.empty()) {
		drawBackgroundImage(bbox, bg);
	}
}

void Request::drawLabel(const cocos2d::Rect &bbox, const Label &l) {
	if (l.format.chars.empty()) {
		return;
	}

	if (_isThumbnail) {
		_drawer->setColor(Color4B(127, 127, 127, 127));
		_drawer->drawCharRects(_font, l.format, getRect(bbox), _scale);
	} else {
		_drawer->drawChars(_font, l.format, getRect(bbox));
	}
}

void Request::drawObject(const Object &obj) {
	switch (obj.type) {
	case Object::Type::Background: drawBackground(obj.bbox, obj.value.background); break;
	case Object::Type::Label: drawLabel(obj.bbox, obj.value.label); break;
	case Object::Type::Outline: drawOutline(obj.bbox, obj.value.outline); break;
	case Object::Type::Ref: drawRef(obj.bbox); break;
	default: break;
	}
}

void Request::onDrawed(cocos2d::Texture2D *data) {
	if (data) {
		_callback(data);
	}
}

bool Drawer::init() {
	TextureCache::thread().perform([this] (stappler::Ref *) -> bool {
		TextureCache::getInstance()->performWithGL([&] {
			glGenFramebuffers(1, &_fbo);
			glGenBuffers(2, _drawBufferVBO);
		});
		return true;
	}, nullptr, this);
	return true;
}

void Drawer::free() {
	TextureCache::thread().perform([this] (stappler::Ref *) -> bool {
		TextureCache::getInstance()->performWithGL([&] {
			cleanup();
			if (_drawBufferVBO[0] != 0) {
				glDeleteBuffers(1, &_drawBufferVBO[0]);
			}
			if (_drawBufferVBO[1] != 0) {
				glDeleteBuffers(1, &_drawBufferVBO[1]);
			}
			if (_fbo != 0) {
			    glDeleteFramebuffers(1, &_fbo);
			}
		});
		return true;
	}, nullptr, this);
}

// draw normal texture
bool Drawer::draw(Source *s, Result *res, const Rect &r, const Callback &cb, cocos2d::Ref *ref) {
	return construct<Request>(this, s, res, r, cb, ref);
}

// draw thumbnail texture, where scale < 1.0f - resample coef
bool Drawer::thumbnail(Source *s, Result *res, const Rect &r, float scale, const Callback &cb, cocos2d::Ref *ref) {
	return construct<Request>(this, s, res, r, scale, cb, ref);
}

static inline int32_t sp_gcd (int16_t a, int16_t b) {
	int32_t c;
	while ( a != 0 ) {
		c = a; a = b%a;  b = c;
	}
	return b;
}

bool Drawer::begin(cocos2d::Texture2D * tex, const Color4B &clearColor) {
	if (_fbo == 0) {
		return false;
	}
	_width = tex->getPixelsWide();
	_height = tex->getPixelsHigh();

	tex->setAliasTexParameters();

	int32_t gcd = sp_gcd(_width, _height);
	int32_t dw = (int32_t)_width / gcd;
	int32_t dh = (int32_t)_height / gcd;
	int32_t dwh = gcd * dw * dh;

	float mod = 1.0f;
	while (dwh * mod > 16384) {
		mod /= 2.0f;
	}

	_projection = Mat4::IDENTITY;
	_projection.scale(dh * mod, dw * mod, -1.0);
	_projection.m[12] = -dwh * mod / 2.0f;
	_projection.m[13] = -dwh * mod / 2.0f;
	_projection.m[14] = dwh * mod / 2.0f - 1;
	_projection.m[15] = dwh * mod / 2.0f + 1;
	_projection.m[11] = -1.0f;

	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->getName(), 0);
	glViewport(0, 0, (GLsizei)_width, (GLsizei)_height);

	glClearColor(clearColor.r / 255.0f, clearColor.g / 255.0f, clearColor.b / 255.0f, clearColor.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	return true;
}

void Drawer::end() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	cleanup();
}

void Drawer::cleanup() {
	bindTexture(0);
	useProgram(0);
	enableVertexAttribs(0);
}

void Drawer::bindTexture(GLuint value) {
	if (_currentTexture != value) {
		_currentTexture = value;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, value);
	}
}

void Drawer::useProgram(GLuint value) {
	if (_currentProgram != value) {
		_currentProgram = value;
		glUseProgram(value);
	}
}

void Drawer::enableVertexAttribs(uint32_t flags) {
	for (int i = 0; i < 16; i++) {
		unsigned int bit = 1 << i;
		bool enabled = (flags & bit) != 0;
		bool enabledBefore = (_attributeFlags & bit) != 0;
		if (enabled != enabledBefore) {
			if (enabled) {
				glEnableVertexAttribArray(i);
			} else {
				glDisableVertexAttribArray(i);
			}
		}
	}
    _attributeFlags = flags;
}

void Drawer::blendFunc(GLenum sfactor, GLenum dfactor) {
	if (sfactor != _blendingSource || dfactor != _blendingDest) {
		_blendingSource = sfactor;
		_blendingDest = dfactor;
		if (sfactor == GL_ONE && dfactor == GL_ZERO) {
			glDisable(GL_BLEND);
		} else {
			glEnable(GL_BLEND);
			glBlendFunc(sfactor, dfactor);
		}
	}
}

void Drawer::blendFunc(const cocos2d::BlendFunc &func) {
	blendFunc(func.src, func.dst);
}

void Drawer::drawResizeBuffer(size_t count) {
	if (count <= _drawBufferSize) {
		return;
	}

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

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _drawBufferVBO[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (sizeof(GLushort) * count * 6), (const GLvoid *) indices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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

void Drawer::drawRectangle(const Rect &bbox, draw::Style style) {
	switch (style) {
	case draw::Style::Fill:
		drawRectangleFill(bbox);
		break;
	case draw::Style::Stroke:
		drawRectangleOutline(bbox);
		break;
	case draw::Style::FillAndStroke:
		drawRectangleFill(bbox);
		drawRectangleOutline(bbox);
		break;
	}
}

void Drawer::drawRectangleFill(const Rect &bbox) {
	auto c = TextureCache::getInstance();
	auto programs = c->getRawPrograms();
	auto p = programs->getProgram(GLProgramSet::RawRect);

	bindTexture(0);
	blendFunc(cocos2d::BlendFunc::ALPHA_NON_PREMULTIPLIED);

	Mat4 transform;
	transform.m[0] = 1.0f;
	transform.m[5] = 1.0f;
	transform.m[10] = 1.0f;
	transform.m[12] = bbox.origin.x;
	transform.m[13] = _height - bbox.origin.y - bbox.size.height;

	transform = _projection * transform;

	useProgram(p->getProgram());
	p->setUniformLocationWith4f(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_AMBIENT_COLOR),
			_color.r / 255.0f, _color.g / 255.0f, _color.b / 255.0f, _color.a / 255.0f);
	p->setUniformLocationWithMatrix4fv(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_MVP_MATRIX),
			transform.m, 1);
	p->setUniformLocationWith2f(p->getUniformLocationForName("u_size"), GLfloat(bbox.size.width / 2.0f + 0.5f), GLfloat(bbox.size.height / 2.0f + 0.5f));
	p->setUniformLocationWith2f(p->getUniformLocationForName("u_position"), GLfloat(bbox.origin.x - 0.5f), GLfloat(_height - bbox.origin.y - bbox.size.height - 0.5f));

	GLfloat vertices[] = { -1.5f, -1.5f, bbox.size.width + 1.5f, -1.5f, -1.5f, bbox.size.height + 1.5f, bbox.size.width + 1.5f, bbox.size.height + 1.5f};

	enableVertexAttribs(cocos2d::GL::VERTEX_ATTRIB_FLAG_POSITION);
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	CHECK_GL_ERROR_DEBUG();
}

void Drawer::drawRectangleOutline(const Rect &bbox) {
	drawRectangleOutline(bbox, true, true, true, true);
}

void Drawer::drawRectangleOutline(const Rect &bbox, bool top, bool right, bool bottom, bool left) {
	Vector<GLfloat> vertices; vertices.reserve(6 * 8 * 2);

	auto c = TextureCache::getInstance();
	auto programs = c->getRawPrograms();
	auto p = programs->getProgram(GLProgramSet::RawRectBorder);

	bindTexture(0);
	blendFunc(cocos2d::BlendFunc::ALPHA_NON_PREMULTIPLIED);

	Mat4 transform;
	transform.m[0] = 1.0f;
	transform.m[5] = 1.0f;
	transform.m[10] = 1.0f;
	transform.m[12] = bbox.origin.x + 1.0f;
	transform.m[13] = _height - bbox.origin.y - bbox.size.height + 1.0f;

	transform = _projection * transform;

	useProgram(p->getProgram());
	p->setUniformLocationWith4f(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_AMBIENT_COLOR),
			_color.r / 255.0f, _color.g / 255.0f, _color.b / 255.0f, _color.a / 255.0f);
	p->setUniformLocationWithMatrix4fv(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_MVP_MATRIX),
			transform.m, 1);
	p->setUniformLocationWith2f(p->getUniformLocationForName("u_size"), GLfloat(bbox.size.width / 2.0f + 0.5f), GLfloat(bbox.size.height / 2.0f + 0.5f));
	p->setUniformLocationWith2f(p->getUniformLocationForName("u_position"), GLfloat(bbox.origin.x - 0.5f), GLfloat(_height - bbox.origin.y - bbox.size.height - 0.5f));
	p->setUniformLocationWith1f(p->getUniformLocationForName("u_border"), GLfloat(_lineWidth / 2.0f));

    const float inc = _lineWidth/2.0f + 2.0f;


    if (left && bottom) {
    	vertices.push_back(-inc);	vertices.push_back(-inc);
    	vertices.push_back(inc);	vertices.push_back(-inc);
    	vertices.push_back(-inc);	vertices.push_back(inc);
    	vertices.push_back(inc);	vertices.push_back(-inc);
    	vertices.push_back(-inc);	vertices.push_back(inc);
    	vertices.push_back(inc);	vertices.push_back(inc);
    }

    if (left) {
    	vertices.push_back(-inc);	vertices.push_back(inc);
    	vertices.push_back(inc);	vertices.push_back(inc);
    	vertices.push_back(-inc);	vertices.push_back(bbox.size.height - inc);
    	vertices.push_back(inc);	vertices.push_back(inc);
    	vertices.push_back(-inc);	vertices.push_back(bbox.size.height - inc);
    	vertices.push_back(inc);	vertices.push_back(bbox.size.height - inc);
    }

    if (left && top) {
    	vertices.push_back(-inc);	vertices.push_back(bbox.size.height - inc);
    	vertices.push_back(inc);	vertices.push_back(bbox.size.height - inc);
    	vertices.push_back(-inc);	vertices.push_back(bbox.size.height + inc);
    	vertices.push_back(inc);	vertices.push_back(bbox.size.height - inc);
    	vertices.push_back(-inc);	vertices.push_back(bbox.size.height + inc);
    	vertices.push_back(inc);	vertices.push_back(bbox.size.height + inc);
    }

    if (top) {
    	vertices.push_back(inc);					vertices.push_back(bbox.size.height - inc);
    	vertices.push_back(inc);					vertices.push_back(bbox.size.height + inc);
    	vertices.push_back(bbox.size.width - inc);	vertices.push_back(bbox.size.height + inc);
    	vertices.push_back(inc);					vertices.push_back(bbox.size.height - inc);
    	vertices.push_back(bbox.size.width - inc);	vertices.push_back(bbox.size.height + inc);
    	vertices.push_back(bbox.size.width - inc);	vertices.push_back(bbox.size.height - inc);
    }

    if (top && right) {
    	vertices.push_back(bbox.size.width - inc);	vertices.push_back(bbox.size.height + inc);
    	vertices.push_back(bbox.size.width - inc);	vertices.push_back(bbox.size.height - inc);
    	vertices.push_back(bbox.size.width + inc);	vertices.push_back(bbox.size.height + inc);
    	vertices.push_back(bbox.size.width - inc);	vertices.push_back(bbox.size.height - inc);
    	vertices.push_back(bbox.size.width + inc);	vertices.push_back(bbox.size.height + inc);
    	vertices.push_back(bbox.size.width + inc);	vertices.push_back(bbox.size.height - inc);
    }

    if (right) {
    	vertices.push_back(bbox.size.width - inc);	vertices.push_back(bbox.size.height - inc);
    	vertices.push_back(bbox.size.width + inc);	vertices.push_back(bbox.size.height - inc);
    	vertices.push_back(bbox.size.width - inc);	vertices.push_back(inc);
    	vertices.push_back(bbox.size.width + inc);	vertices.push_back(bbox.size.height - inc);
    	vertices.push_back(bbox.size.width - inc);	vertices.push_back(inc);
    	vertices.push_back(bbox.size.width + inc);	vertices.push_back(inc);
    }

    if (right && bottom) {
    	vertices.push_back(bbox.size.width - inc);	vertices.push_back(inc);
    	vertices.push_back(bbox.size.width + inc);	vertices.push_back(inc);
    	vertices.push_back(bbox.size.width + inc);	vertices.push_back(-inc);
    	vertices.push_back(bbox.size.width - inc);	vertices.push_back(inc);
    	vertices.push_back(bbox.size.width + inc);	vertices.push_back(-inc);
    	vertices.push_back(bbox.size.width - inc);	vertices.push_back(-inc);
    }

    if (bottom) {
    	vertices.push_back(bbox.size.width - inc);	vertices.push_back(inc);
    	vertices.push_back(bbox.size.width - inc);	vertices.push_back(-inc);
    	vertices.push_back(inc);					vertices.push_back(-inc);
    	vertices.push_back(bbox.size.width - inc);	vertices.push_back(inc);
    	vertices.push_back(inc);					vertices.push_back(-inc);
    	vertices.push_back(inc);					vertices.push_back(inc);
    }

	enableVertexAttribs(cocos2d::GL::VERTEX_ATTRIB_FLAG_POSITION);
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0, vertices.data());
	glDrawArrays(GL_TRIANGLES, 0, GLint(vertices.size()));
	CHECK_GL_ERROR_DEBUG();
}

void Drawer::drawTexture(const Rect &bbox, cocos2d::Texture2D *tex, const Rect &texRect) {
	auto c = TextureCache::getInstance();
	auto programs = c->getRawPrograms();
	auto p = programs->getProgram(GLProgramSet::RawTexture);

	auto w = tex->getPixelsWide();
	auto h = tex->getPixelsHigh();

	Mat4 transform;
	transform.m[0] = 1.0f;
	transform.m[5] = 1.0f;
	transform.m[10] = 1.0f;
	transform.m[12] = bbox.origin.x;
	transform.m[13] = _height - bbox.origin.y - bbox.size.height;

	transform = _projection * transform;

	bindTexture(tex->getName());
	useProgram(p->getProgram());
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

	auto c = TextureCache::getInstance();
	auto programs = c->getRawPrograms();
	auto p = programs->getProgram(GLProgramSet::RawRects);

	bindTexture(0);
	blendFunc(cocos2d::BlendFunc::ALPHA_NON_PREMULTIPLIED);

	Mat4 transform;
	transform.m[0] = 1.0f;
	transform.m[5] = 1.0f;
	transform.m[10] = 1.0f;
	transform.m[12] = bbox.origin.x;
	transform.m[13] = _height - bbox.origin.y - bbox.size.height;

	transform = _projection * transform;

	useProgram(p->getProgram());
	p->setUniformLocationWith4f(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_AMBIENT_COLOR),
			_color.r / 255.0f, _color.g / 255.0f, _color.b / 255.0f, _color.a / 255.0f);
	p->setUniformLocationWithMatrix4fv(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_MVP_MATRIX),
			transform.m, 1);

	for (auto &it : rects) {
    	vertices.push_back(it.origin.x);					vertices.push_back(it.origin.y);
    	vertices.push_back(it.origin.x + it.size.width);	vertices.push_back(it.origin.y);
    	vertices.push_back(it.origin.x);					vertices.push_back(it.origin.y + it.size.height);
    	vertices.push_back(it.origin.x + it.size.width);	vertices.push_back(it.origin.y + it.size.height);
	}

	drawResizeBuffer(rects.size());

	glBindBuffer(GL_ARRAY_BUFFER, _drawBufferVBO[0]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

	enableVertexAttribs(cocos2d::GL::VERTEX_ATTRIB_FLAG_POSITION);
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _drawBufferVBO[1]);
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

	drawCharsEffects(f, format, bbox);
}

void Drawer::drawCharsQuads(const Vector<Rc<cocos2d::Texture2D>> &tex, Vector<Rc<DynamicQuadArray>> &&quads, const Rect & bbox) {
	bindTexture(0);

	for (size_t i = 0; i < quads.size(); ++ i) {
		drawCharsQuads(tex[i], quads[i], bbox);
	}
}

void Drawer::drawCharsQuads(cocos2d::Texture2D *tex, DynamicQuadArray *quads, const Rect & bbox) {
	auto c = TextureCache::getInstance();
	auto programs = c->getRawPrograms();
	auto p = programs->getProgram(GLProgramSet::DynamicBatchA8Highp);
	useProgram(p->getProgram());

	Mat4 transform;
	transform.m[0] = 1.0f;
	transform.m[5] = 1.0f;
	transform.m[10] = 1.0f;
	transform.m[12] = bbox.origin.x;
	transform.m[13] = _height - bbox.origin.y - bbox.size.height;

	transform = _projection * transform;

    p->setUniformLocationWithMatrix4fv(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_MVP_MATRIX), transform.m, 1);

	bindTexture(tex->getName());

	auto count = quads->size();
	size_t bufferSize = sizeof(cocos2d::V3F_C4B_T2F_Quad) * count;

	drawResizeBuffer(count);

	glBindBuffer(GL_ARRAY_BUFFER, _drawBufferVBO[0]);
	glBufferData(GL_ARRAY_BUFFER, bufferSize, quads->getData(), GL_DYNAMIC_DRAW);

    blendFunc(cocos2d::BlendFunc::ALPHA_NON_PREMULTIPLIED);
	enableVertexAttribs(cocos2d::GL::VERTEX_ATTRIB_FLAG_POS_COLOR_TEX);

	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE,
			sizeof(cocos2d::V3F_C4B_T2F), (GLvoid*) offsetof(cocos2d::V3F_C4B_T2F, vertices));

	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE,
			sizeof(cocos2d::V3F_C4B_T2F), (GLvoid*) offsetof(cocos2d::V3F_C4B_T2F, colors));

	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_TEX_COORD, 2, GL_FLOAT, GL_FALSE,
			sizeof(cocos2d::V3F_C4B_T2F), (GLvoid*) offsetof(cocos2d::V3F_C4B_T2F, texCoords));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _drawBufferVBO[1]);

	glDrawElements(GL_TRIANGLES, (GLsizei) count * 6, GL_UNSIGNED_SHORT, (GLvoid *) (0));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	CHECK_GL_ERROR_DEBUG();
}

void Drawer::drawCharsEffects(Font *font, const font::FormatSpec &format, const Rect &bbox) {
	for (auto it = format.begin(); it != format.end(); ++ it) {
		if (it.count() > 0 && it.range->underline > 0) {
			const font::CharSpec &firstChar = format.chars[it.start()];
			const font::CharSpec &lastChar = format.chars[it.start() + it.count() - 1];

			auto color = it.range->color;
			color.a = uint8_t(0.75f * color.a);
			setColor(color);
			Arc<font::FontLayout> layout(it.range->layout);
			Arc<font::FontData> data(layout->getData());
			drawRectangleFill(Rect(bbox.origin.x + firstChar.pos, bbox.origin.y + it.line->pos - data->metrics.height / 8.0f,
					lastChar.pos + lastChar.advance - firstChar.pos, data->metrics.height / 16.0f));
		}
	}
}

void Drawer::update() {
	if (!_cacheUpdated) {
		return;
	}

	if (Time::now() - _updated > TimeInterval::seconds(1)) {
		_cacheUpdated = false;
		TextureCache::thread().perform([this] (stappler::Ref *) -> bool {
			TextureCache::getInstance()->performWithGL([&] {
				performUpdate();
			});
			return true;
		}, [this] (stappler::Ref *, bool) {
			_cacheUpdated = true;
			_updated = Time::now();
		});
	}
}

void Drawer::clearCache() {
	TextureCache::thread().perform([this] (stappler::Ref *) -> bool {
		TextureCache::getInstance()->performWithGL([&] {
			_cache.clear();
		});
		return true;
	});
}

void Drawer::performUpdate() {
	if (_cache.empty()) {
		return;
	}

	auto time = Time::now();
	Vector<String> keys;
	for (auto &it : _cache) {
		if (it.second.second - time > TimeInterval::seconds(6)) {
			keys.push_back(it.first);
		}
	}

	for (auto &it : keys) {
		_cache.erase(it);
	}
}

void Drawer::addBitmap(const String &str, cocos2d::Texture2D *bmp) {
	_cache.emplace(str, pair(bmp, Time::now()));
}

cocos2d::Texture2D *Drawer::getBitmap(const std::string &key) {
	auto it = _cache.find(key);
	if (it != _cache.end()) {
		it->second.second = Time::now();
		return it->second.first;
	}
	return nullptr;
}

Thread &Drawer::thread() {
	return TextureCache::thread();
}

NS_SP_EXT_END(rich_text)
