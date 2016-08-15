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
#include "renderer/CCTextureCache.h"
#include "renderer/CCGLProgram.h"
#include "renderer/CCGLProgramState.h"
#include "renderer/CCGLProgramCache.h"

NS_SP_BEGIN

DynamicBatchNode::~DynamicBatchNode() { }

bool DynamicBatchNode::init(cocos2d::Texture2D *tex, float density) {
	if (!Node::init()) {
		return false;
	}

	if (density == 0) {
		density = stappler::screen::density();
	}
	_density = density;

	_texture = tex;
    _blendFunc = cocos2d::BlendFunc::ALPHA_PREMULTIPLIED;

    setGLProgramState(getProgramStateA8());

	setColor(cocos2d::Color3B(255, 255, 255));

	setCascadeOpacityEnabled(true);

	if (_texture) {
		updateBlendFunc();
	}

	return true;
}

DynamicAtlas* DynamicBatchNode::getAtlas(void) {
	return _textureAtlas;
}

void DynamicBatchNode::setBlendFunc(const cocos2d::BlendFunc &blendFunc) {
    _blendFunc = blendFunc;
}
const cocos2d::BlendFunc& DynamicBatchNode::getBlendFunc() const {
	return _blendFunc;
}

void DynamicBatchNode::updateBlendFunc() {
	if (_texture) {
		auto glProgramName = getGLProgram()->getProgram();
		auto pixelFormat = getTexture()->getPixelFormat();
		cocos2d::GLProgramState *newState = nullptr;
		if (pixelFormat == cocos2d::Texture2D::PixelFormat::A8) {
			newState = getProgramStateA8();
		} else if (pixelFormat == cocos2d::Texture2D::PixelFormat::I8) {
			newState = getProgramStateI8();
		} else if (pixelFormat == cocos2d::Texture2D::PixelFormat::AI88) {
			newState = getProgramStateAI88();
		} else {
			newState = getProgramStateFullColor();
		}

		if (newState->getGLProgram()->getProgram() != glProgramName) {
		    setGLProgramState(newState);
		}

	    if (! getTexture()->hasPremultipliedAlpha()) {
	        _blendFunc = cocos2d::BlendFunc::ALPHA_NON_PREMULTIPLIED;
	        setOpacityModifyRGB(false);
	    } else {
	        _blendFunc = cocos2d::BlendFunc::ALPHA_PREMULTIPLIED;
	        setOpacityModifyRGB(true);
	    }
	}
}

cocos2d::GLProgramState *DynamicBatchNode::getProgramStateA8() const {
	return cocos2d::GLProgramState::getOrCreateWithGLProgramName(cocos2d::GLProgram::SHADER_NAME_POSITION_TEXTURE_A8_COLOR);
}
cocos2d::GLProgramState *DynamicBatchNode::getProgramStateI8() const {
	return cocos2d::GLProgramState::getOrCreateWithGLProgram(GLProgramSet::getInstance()->getProgram(GLProgramSet::DynamicBatchI8));
}
cocos2d::GLProgramState *DynamicBatchNode::getProgramStateAI88() const {
	return cocos2d::GLProgramState::getOrCreateWithGLProgram(GLProgramSet::getInstance()->getProgram(GLProgramSet::DynamicBatchAI88));
}
cocos2d::GLProgramState *DynamicBatchNode::getProgramStateFullColor() const {
	return cocos2d::GLProgramState::getOrCreateWithGLProgramName(cocos2d::GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR);
}

void DynamicBatchNode::updateColor() {
	if (!_quads.empty()) {
	    cocos2d::Color4B color4( _displayedColor.r, _displayedColor.g, _displayedColor.b, _displayedOpacity );

	    // special opacity for premultiplied textures
		if (_opacityModifyRGB) {
			color4.r *= _displayedOpacity/255.0f;
			color4.g *= _displayedOpacity/255.0f;
			color4.b *= _displayedOpacity/255.0f;
	    }

		for (size_t i = 0; i < _quads.size(); i++) {
			_quads.setColor(i, color4);
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

	    updateBlendFunc();
	}
}

void DynamicBatchNode::setOpacityModifyRGB(bool modify) {
    if (_opacityModifyRGB != modify) {
        _opacityModifyRGB = modify;
        updateColor();
    }
}

bool DynamicBatchNode::isOpacityModifyRGB(void) const {
    return _opacityModifyRGB;
}

void DynamicBatchNode::draw(cocos2d::Renderer *renderer, const cocos2d::Mat4 &transform, uint32_t flags, const ZPath &zPath) {
	if (!_textureAtlas) {
		if (auto atlas = construct<DynamicAtlas>(getTexture())) {
			_textureAtlas = atlas;
			_textureAtlas->addQuadArray(&_quads);
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

bool DynamicBatchNode::isNormalized() const {
	return _normalized;
}

void DynamicBatchNode::setNormalized(bool value) {
	if (_normalized != value) {
		_normalized = value;
		_transformUpdated = _transformDirty = _inverseDirty = true;
	}
}

void DynamicBatchNode::setDensity(float density) {
	if (density != _density) {
		_density = density;
		_contentSizeDirty = true;
	}
}

float DynamicBatchNode::getDensity() const {
	return _density;
}

const DynamicQuadArray &DynamicBatchNode::getQuads() const {
	return _quads;
}

DynamicQuadArray *DynamicBatchNode::getQuadsPtr() {
	return &_quads;
}

NS_SP_END
