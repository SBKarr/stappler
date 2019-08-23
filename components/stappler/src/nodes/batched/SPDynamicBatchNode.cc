// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPDynamicBatchNode.h"
#include "SPDynamicQuadAtlas.h"
#include "SPDynamicTriangleAtlas.h"
#include "SPGLProgramSet.h"

#include "renderer/CCRenderer.h"
#include "renderer/CCTexture2D.h"

NS_SP_BEGIN

DynamicBatchAtlasNode::~DynamicBatchAtlasNode() { }

bool DynamicBatchAtlasNode::init(cocos2d::Texture2D *tex, float density) {
	if (!BatchNodeBase::init(density)) {
		return false;
	}

	_texture = tex;
	if (_texture) {
		_programDirty = true;
	}

	return true;
}

DynamicAtlas* DynamicBatchAtlasNode::getAtlas(void) {
	return _textureAtlas;
}

cocos2d::Texture2D* DynamicBatchAtlasNode::getTexture() const {
	return _texture;
}

void DynamicBatchAtlasNode::setTexture(cocos2d::Texture2D *texture) {
	if (texture != _texture) {
		if (_textureAtlas) {
		    _textureAtlas->setTexture(texture);
		}

		_texture = texture;
		_contentSizeDirty = true;
		_programDirty = true;
	}
}

void DynamicBatchAtlasNode::draw(cocos2d::Renderer *renderer, const Mat4 &transform, uint32_t flags, const ZPath &zPath) {
	if (_programDirty) {
		updateBlendFunc(_texture);
	}

	if (_gradientObject) {
		if (_gradientObject->isAbsolute()) {
			Vec2 pos = transform.transformPoint(Vec2::ZERO);
			Vec2 size = transform.transformPoint(Vec2(_contentSize.width, _contentSize.height)) - pos;
			_gradientObject->updateWithSize(Size(size.x, size.y), pos);
		} else {
			_gradientObject->updateWithSize(_contentSize, Vec2::ZERO);
		}
	}

	if (!_textureAtlas) {
		if (auto a = makeAtlas()) {
			_textureAtlas = a;
		}
	}

	if (!_textureAtlas || !getGLProgram()) {
		return;
	}

	bool asStencil = (_displayedOpacity == 255) ? _stencil : false;

	if (_normalized) {
		Mat4 newMV;
		newMV.m[12] = floorf(transform.m[12]);
		newMV.m[13] = floorf(transform.m[13]);
		newMV.m[14] = floorf(transform.m[14]);
		_batchCommand.init(_globalZOrder, getGLProgramState(), _blendFunc, _textureAtlas, newMV, zPath, _normalized, asStencil, _alphaTest, _gradientObject);
	} else {
		_batchCommand.init(_globalZOrder, getGLProgramState(), _blendFunc, _textureAtlas, transform, zPath, _normalized, asStencil, _alphaTest, _gradientObject);
	}

	renderer->addCommand(&_batchCommand);
}

DynamicBatchNode::DynamicBatchNode() {
	_quads = Rc<DynamicQuadArray>::alloc();
}

DynamicBatchNode::~DynamicBatchNode() { }

void DynamicBatchNode::updateColor() {
	if (_quads && !_quads->empty()) {
	    Color4B color4( _displayedColor.r, _displayedColor.g, _displayedColor.b, _displayedOpacity );

	    // special opacity for premultiplied textures
		if (_opacityModifyRGB) {
			color4.r *= _displayedOpacity/255.0f;
			color4.g *= _displayedOpacity/255.0f;
			color4.b *= _displayedOpacity/255.0f;
	    }

		for (size_t i = 0; i < _quads->size(); i++) {
			_quads->setColor(i, color4);
		}
	}
}

Rc<DynamicAtlas> DynamicBatchNode::makeAtlas() const {
	if (auto atlas = Rc<DynamicQuadAtlas>::create(getTexture())) {
		atlas->addArray(_quads);
		return Rc<DynamicAtlas>(static_cast<DynamicAtlas *>(atlas));
	}
	return nullptr;
}

DynamicQuadArray *DynamicBatchNode::getArray() const {
	return _quads;
}


DynamicBatchTriangleNode::DynamicBatchTriangleNode() {
	_array = Rc<DynamicTriangleArray>::alloc();
}

DynamicBatchTriangleNode::~DynamicBatchTriangleNode() { }

DynamicTriangleArray *DynamicBatchTriangleNode::getArray() const {
	return _array;
}

void DynamicBatchTriangleNode::updateColor() {
	if (_array && !_array->empty()) {
	    Color4B color4( _displayedColor.r, _displayedColor.g, _displayedColor.b, _displayedOpacity );

	    // special opacity for premultiplied textures
		if (_opacityModifyRGB) {
			color4.r *= _displayedOpacity/255.0f;
			color4.g *= _displayedOpacity/255.0f;
			color4.b *= _displayedOpacity/255.0f;
	    }

		for (size_t i = 0; i < _array->size(); i++) {
			_array->setColor(i, color4);
		}
	}
}

Rc<DynamicAtlas> DynamicBatchTriangleNode::makeAtlas() const {
	if (auto atlas = Rc<DynamicTriangleAtlas>::create(getTexture())) {
		atlas->addArray(_array);
		return Rc<DynamicAtlas>(static_cast<DynamicAtlas *>(atlas));
	}
	return nullptr;
}

NS_SP_END
