/*
 * SPDynamicBatchNode.cpp
 *
 *  Created on: 02 апр. 2015 г.
 *      Author: sbkarr
 */

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
