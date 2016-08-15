/*
 * SPPathNode.cpp
 *
 *  Created on: 09 дек. 2014 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPDrawPathNode.h"
#include "SPTime.h"
#include "SPEventListener.h"
#include "SPDevice.h"

#include "cairo.h"

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

void PathNode::acquireCache() {
	_useCache++;
}
void PathNode::releaseCache() {
	if (_useCache > 0) {
		_useCache--;
	}
	if (_useCache == 0) {
		if (_canvas) {
			_canvas = nullptr;
		}
	}
}

void PathNode::updateCanvas() {
	auto &size = _contentSize;
	uint32_t width = ceilf(size.width * _density);
	uint32_t height = ceilf(size.height * _density);

	auto currentTex = getTexture();
	if (!_pathsDirty && (uint32_t)currentTex->getPixelsWide() == width && (uint32_t)currentTex->getPixelsHigh() == height) {
		_pathsDirty = false;
		return;
	}

	auto cr = acquireDrawContext(width, height, _format);
	cairo_scale(cr, (float)width / (float)_baseWidth, (float)height / (float)_baseHeight);
	for (auto path : _paths) {
		path->_dirty = false;
		path->drawOn(cr);
	}

	cocos2d::Texture2D *tex = generateTexture(getTexture());
	if (tex != getTexture()) {
		setTexture(tex);
		setTextureRect(cocos2d::Rect(0, 0, width, height));

		tex->release();
	}

	_pathsDirty = false;

	releaseCanvasCache();
}

cairo_t *PathNode::acquireDrawContext(uint32_t w, uint32_t h, Format fmt) {
	if (_canvas && _canvas->match(w, h, fmt)) {
		return _canvas->acquireContext();
	}

	if (_canvas) {
		_canvas = nullptr;
	}

	_canvas = Rc<Canvas>::create(w, h, fmt);
	return _canvas->acquireContext();
}

cocos2d::Texture2D *PathNode::generateTexture(cocos2d::Texture2D *tex) {
	auto outtex = _canvas->generateTexture(tex);
	if (_isAntialiased) {
		outtex->setAntiAliasTexParameters();
	} else {
		outtex->setAliasTexParameters();
	}
	return outtex;
}

void PathNode::releaseCanvasCache() {
	if (_canvas) {
		_canvas->releaseContext();
		if (_useCache == 0) {
			_canvas = nullptr;
		}
	}
}

uint32_t PathNode::getBaseWidth() {
	return _baseWidth;
}
uint32_t PathNode::getBaseHeight() {
	return _baseWidth;
}

NS_SP_EXT_END(draw)
