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
#include "SPDrawPathNode.h"
#include "SPTime.h"
#include "SPEventListener.h"
#include "SPDevice.h"
#include "SPTextureCache.h"
#include "SPDrawCanvas.h"
#include "renderer/CCGLProgramCache.h"
#include "renderer/CCGLProgram.h"

NS_SP_EXT_BEGIN(draw)

PathNode::~PathNode() {
	clear();
}

bool PathNode::init(Format fmt) {
	return init(nullptr, fmt);
}

bool PathNode::init(uint16_t width, uint16_t height, Format fmt) {
	return init(Rc<Image>::create(width, height), fmt);
}

bool PathNode::init(Image *img, Format fmt) {
	if (!DynamicSprite::init()) {
		return false;
	}

	_image.setCallback(std::bind(&PathNode::updateCanvas, this, std::placeholders::_1));
	_image = img;

	auto l = Rc<EventListener>::create();
	l->onEvent(Device::onAndroidReset, [this] (const Event &) {
		if (_canvas) {
			_canvas->drop();
			_canvas = nullptr;
		}
		setTexture(nullptr);
		_pathsDirty = true;
	});
	addComponent(l);

	_baseWidth = (_image?_image->getWidth():0);
	_baseHeight = (_image?_image->getHeight():0);
	_format = fmt;

	setOpacity(255);

	return true;
}

void PathNode::visit(cocos2d::Renderer *r, const Mat4 &t, uint32_t f, ZPath &z) {
	if (isVisible() && (_contentSizeDirty || _pathsDirty)) {
		updateCanvas(_pathsDirty?data::Subscription::Initial:data::Subscription::Flags(0));
		_pathsDirty = false;
	}

	DynamicSprite::visit(r, t, f, z);
}

Image::PathRef PathNode::addPath() {
	return addPath(Path());
}
Image::PathRef PathNode::addPath(const Path &path) {
	if (_image) {
		return _image->addPath(path);
	}
	return Image::PathRef();
}
Image::PathRef PathNode::addPath(Path &&path) {
	if (_image) {
		return _image->addPath(std::move(path));
	}
	return Image::PathRef();
}

Image::PathRef PathNode::getPathByTag(uint32_t idx) {
	if (_image) {
		return _image->getPathByTag(idx);
	}
	return Image::PathRef();
}
Image::PathRef PathNode::getPath(size_t idx) {
	if (_image) {
		return _image->getPath(idx);
	}
	return Image::PathRef();
}

void PathNode::removePath(const Image::PathRef &path) {
	if (_image) {
		_image->removePath(path);
	}
}
void PathNode::removePath(size_t idx) {
	if (_image) {
		_image->removePath(idx);
	}
}
void PathNode::removePathByTag(uint32_t tag) {
	if (_image) {
		_image->removePathByTag(tag);
	}
}

bool PathNode::clear() {
	if (_image) {
		_image->clear();
		return true;
	}
	return false;
}

void PathNode::setAntialiased(bool value) {
	if (_image) {
		_image->setAntialiased(value);
	}
}

bool PathNode::isAntialiased() const {
	return (_image?_image->isAntialiased():false);
}

void PathNode::setImage(Image *img) {
	_image = img;
	_pathsDirty = true;
	if (_image) {
		_baseWidth = _image->getWidth();
		_baseHeight = _image->getHeight();
	}
}
Image * PathNode::getImage() const {
	return _image;
}

void PathNode::updateCanvas(layout::Subscription::Flags f) {
	if (_contentSize.equals(Size::ZERO)) {
		return;
	}
	if (!_image) {
		setTexture(nullptr);
		return;
	}

	_baseWidth = _image->getWidth();
	_baseHeight = _image->getHeight();

	if (_baseWidth == 0 || _baseHeight == 0) {
		return;
	}

	if (!_canvas) {
		_canvas.set(Rc<Canvas>::create());
		_canvas->setQuality(1.75f /*Canvas::QualityHigh*/);
	}

	auto &size = _contentSize;
	uint32_t width = ceilf(size.width * _density);
	uint32_t height = ceilf(size.height * _density);

	float scaleX = float(width) / float(_baseWidth);
	float scaleY = float(height) / float(_baseHeight);

	switch (_autofit) {
	case Autofit::Contain:
		scaleX = scaleY = std::min(scaleX, scaleY);
		break;
	case Autofit::Cover:
		scaleX = scaleY = std::max(scaleX, scaleY);
		break;
	case Autofit::Width:
		scaleY = scaleX;
		break;
	case Autofit::Height:
		scaleX = scaleY;
		break;
	case Autofit::None:
		break;
	}

	uint32_t nwidth = _baseWidth * scaleX;
	uint32_t nheight = _baseHeight * scaleY;

	float offsetX = 0.0f;
	float offsetY = 0.0f;

	if (nwidth > width) {
		offsetX = _autofitPos.x * (nwidth - width);
	} else {
		width = nwidth;
	}

	if (nheight > height) {
		offsetY = _autofitPos.y * (nheight - height);
	} else {
		height = nheight;
	}

	auto currentTex = getTexture();
	if (!f.empty() || !currentTex || (uint32_t)currentTex->getPixelsWide() != width || (uint32_t)currentTex->getPixelsHigh() != height) {
		auto tex = generateTexture(getTexture(), width, height, _format);
		if (tex) {
			_canvas->begin(tex, Color4B(0, 0, 0, 0));
			_canvas->save();
			_canvas->translate(offsetX, offsetY);
			_canvas->scale(scaleX, scaleY);

			auto &paths = _image->getPaths();
			for (auto & path : paths) {
				_canvas->draw(path);
			}

			_canvas->restore();
			_canvas->end();

			if (tex != getTexture()) {
				setTexture(tex);
			}
		}
	}
}

Rc<cocos2d::Texture2D> PathNode::generateTexture(cocos2d::Texture2D *tex, uint32_t w, uint32_t h, Format fmt) {
	if (tex && tex->getPixelsHigh() == int(h) && tex->getPixelsWide() == int(w) && fmt == tex->getReferenceFormat()) {
		return tex;
	}
	auto outtex = Rc<cocos2d::Texture2D>::create(fmt, w, h, cocos2d::Texture2D::InitAs::RenderTarget);
	if (outtex) {
		outtex->setAliasTexParameters();
	}
	return outtex;
}

uint32_t PathNode::getBaseWidth() {
	return _baseWidth;
}
uint32_t PathNode::getBaseHeight() {
	return _baseHeight;
}

NS_SP_EXT_END(draw)
