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
#include "SPDynamicBatchNode.h"
#include "SPDynamicAtlas.h"
#include "SPGLProgramSet.h"

#include "renderer/CCRenderer.h"
#include "renderer/CCTexture2D.h"

NS_SP_BEGIN

DynamicBatchNode::DynamicBatchNode() {
	_quads = Rc<DynamicQuadArray>::alloc();
}

DynamicBatchNode::~DynamicBatchNode() { }

bool DynamicBatchNode::init(cocos2d::Texture2D *tex, float density) {
	if (!BatchNodeBase::init(density)) {
		return false;
	}

	_texture = tex;
	if (_texture) {
		updateBlendFunc(_texture);
	}

	return true;
}

DynamicAtlas* DynamicBatchNode::getAtlas(void) {
	return _textureAtlas;
}

void DynamicBatchNode::updateColor() {
	if (!_quads->empty()) {
	    cocos2d::Color4B color4( _displayedColor.r, _displayedColor.g, _displayedColor.b, _displayedOpacity );

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

cocos2d::Texture2D* DynamicBatchNode::getTexture() const {
	return _texture;
}

void DynamicBatchNode::setTexture(cocos2d::Texture2D *texture) {
	if (texture != _texture) {
		if (_textureAtlas) {
		    _textureAtlas->setTexture(texture);
		}

		_texture = texture;
		_contentSizeDirty = true;

	    updateBlendFunc(_texture);
	}
}

void DynamicBatchNode::draw(cocos2d::Renderer *renderer, const cocos2d::Mat4 &transform, uint32_t flags, const ZPath &zPath) {
	if (!_textureAtlas) {
		if (auto atlas = construct<DynamicAtlas>(getTexture())) {
			_textureAtlas = atlas;
			_textureAtlas->addQuadArray(_quads);
		}
	}

	if (!_textureAtlas || !_texture) {
		return;
	}

	if (_normalized) {
		cocos2d::Mat4 newMV;
		newMV.m[12] = floorf(transform.m[12]);
		newMV.m[13] = floorf(transform.m[13]);
		newMV.m[14] = floorf(transform.m[14]);
		_batchCommand.init(_globalZOrder, getGLProgram(), _blendFunc, _textureAtlas, newMV, zPath, _normalized);
	} else {
		_batchCommand.init(_globalZOrder, getGLProgram(), _blendFunc, _textureAtlas, transform, zPath, _normalized);
	}

	renderer->addCommand(&_batchCommand);
}

DynamicQuadArray *DynamicBatchNode::getQuads() const {
	return _quads;
}

NS_SP_END
