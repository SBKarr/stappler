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
#include "SPDrawCanvasCairo.h"

NS_SP_EXT_BEGIN(draw)

PathNode::~PathNode() {
	clear();
}

bool PathNode::init(uint32_t width, uint32_t height, Format fmt) {
	if (!DynamicSprite::init()) {
		return false;
	}

	auto l = construct<EventListener>();
	l->onEvent(Device::onAndroidReset, [this] (const Event *) {
		setTexture(nullptr);
		_pathsDirty = true;
	});
	addComponent(l);

	if (fmt == Format::A8) {
		setGLProgram(getA8Program());
	} else if (fmt == Format::RGBA8888) {
		setGLProgram(getRGBA8888Program());
	}

	_baseWidth = width;
	_baseHeight = height;
	_format = fmt;

	setOpacity(255);

	return true;
}

void PathNode::visit(cocos2d::Renderer *renderer, const cocos2d::Mat4 &parentTransform, uint32_t parentFlags, ZPath &zPath) {
	if (_contentSizeDirty || _pathsDirty) {
		updateCanvas();
	}

	DynamicSprite::visit(renderer, parentTransform, parentFlags, zPath);
}

void PathNode::addPath(Path *path) {
	_pathsDirty = true;
	_paths.pushBack(path);
	path->addNode(this);
}

void PathNode::removePath(Path *path) {
	auto it = _paths.find(path);
	if (it != _paths.end()) {
		path->removeNode(this);
		_paths.erase(it);
	}
}

void PathNode::clear() {
	for (auto path : _paths) {
		path->removeNode(this);
	}
	_paths.clear();
}

void PathNode::setAntialiased(bool value) {
	if (value != _isAntialiased) {
		_isAntialiased = value;
		_pathsDirty = true;
	}
}

bool PathNode::isAntialiased() const {
	return _isAntialiased;
}

const cocos2d::Vector<Path *> &PathNode::getPaths() const {
	return _paths;
}

void PathNode::updateCanvas() {
	if (!_canvas) {
		_canvas.set(Rc<CanvasCairo>::create());
	}

	auto &size = _contentSize;
	uint32_t width = ceilf(size.width * _density);
	uint32_t height = ceilf(size.height * _density);

	auto currentTex = getTexture();
	if (!_pathsDirty && (uint32_t)currentTex->getPixelsWide() == width && (uint32_t)currentTex->getPixelsHigh() == height) {
		_pathsDirty = false;
		return;
	}

	auto tex = generateTexture(getTexture(), width, height, _format);
	_canvas->begin(tex, Color4B(0, 0, 0, 0));
	_canvas->save();
	_canvas->scale((float)width / (float)_baseWidth, (float)height / (float)_baseHeight);
	for (auto path : _paths) {
		path->_dirty = false;
		path->drawOn(_canvas);
	}

	_canvas->restore();
	_canvas->end();

	if (tex != getTexture()) {
		setTexture(tex);
		setTextureRect(Rect(0, 0, width, height));
	}

	_pathsDirty = false;
}

Rc<cocos2d::Texture2D> PathNode::generateTexture(cocos2d::Texture2D *tex, uint32_t w, uint32_t h, Format fmt) {
	if (tex && tex->getPixelsHigh() == int(h) && tex->getPixelsWide() == int(w) &&
			((fmt == Format::A8 && tex->getPixelFormat() == cocos2d::Texture2D::PixelFormat::A8)
					|| (fmt == Format::RGBA8888 && tex->getPixelFormat() == cocos2d::Texture2D::PixelFormat::RGBA8888))) {
		return tex;
	}
	auto outtex = Rc<cocos2d::Texture2D>::create(
			(fmt == Format::A8?cocos2d::Texture2D::PixelFormat::A8:cocos2d::Texture2D::PixelFormat::RGBA8888), w, h);
	if (_isAntialiased) {
		outtex->setAntiAliasTexParameters();
	} else {
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
