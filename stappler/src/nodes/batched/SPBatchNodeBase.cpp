/*
 * SPBatchNodeBase.cpp
 *
 *  Created on: 26 окт. 2016 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPBatchNodeBase.h"

#include "SPGLProgramSet.h"

#include "renderer/CCRenderer.h"
#include "renderer/CCTexture2D.h"
#include "renderer/CCTextureCache.h"
#include "renderer/CCGLProgram.h"
#include "renderer/CCGLProgramState.h"
#include "renderer/CCGLProgramCache.h"

NS_SP_BEGIN

BatchNodeBase::~BatchNodeBase() { }

bool BatchNodeBase::init(float density) {
	if (!Node::init()) {
		return false;
	}

	if (density == 0) {
		density = stappler::screen::density();
	}
	_density = density;
    _blendFunc = cocos2d::BlendFunc::ALPHA_PREMULTIPLIED;

    setGLProgramState(getProgramStateA8());
	setColor(cocos2d::Color3B(255, 255, 255));
	setCascadeOpacityEnabled(true);

	return true;
}

void BatchNodeBase::setBlendFunc(const cocos2d::BlendFunc &blendFunc) {
    _blendFunc = blendFunc;
}
const cocos2d::BlendFunc& BatchNodeBase::getBlendFunc() const {
	return _blendFunc;
}

void BatchNodeBase::updateBlendFunc(cocos2d::Texture2D *tex) {
	if (tex) {
		auto glProgramName = getGLProgram()->getProgram();
		auto pixelFormat = tex->getPixelFormat();
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

	    if (!tex->hasPremultipliedAlpha()) {
	        _blendFunc = cocos2d::BlendFunc::ALPHA_NON_PREMULTIPLIED;
	        setOpacityModifyRGB(false);
	    } else {
	        _blendFunc = cocos2d::BlendFunc::ALPHA_PREMULTIPLIED;
	        setOpacityModifyRGB(true);
	    }
	}
}

cocos2d::GLProgramState *BatchNodeBase::getProgramStateA8() const {
	return cocos2d::GLProgramState::getOrCreateWithGLProgramName(cocos2d::GLProgram::SHADER_NAME_POSITION_TEXTURE_A8_COLOR);
}
cocos2d::GLProgramState *BatchNodeBase::getProgramStateI8() const {
	return cocos2d::GLProgramState::getOrCreateWithGLProgram(GLProgramSet::getInstance()->getProgram(GLProgramSet::DynamicBatchI8));
}
cocos2d::GLProgramState *BatchNodeBase::getProgramStateAI88() const {
	return cocos2d::GLProgramState::getOrCreateWithGLProgram(GLProgramSet::getInstance()->getProgram(GLProgramSet::DynamicBatchAI88));
}
cocos2d::GLProgramState *BatchNodeBase::getProgramStateFullColor() const {
	return cocos2d::GLProgramState::getOrCreateWithGLProgramName(cocos2d::GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR);
}

void BatchNodeBase::setOpacityModifyRGB(bool modify) {
    if (_opacityModifyRGB != modify) {
        _opacityModifyRGB = modify;
        updateColor();
    }
}

bool BatchNodeBase::isOpacityModifyRGB(void) const {
    return _opacityModifyRGB;
}

bool BatchNodeBase::isNormalized() const {
	return _normalized;
}

void BatchNodeBase::setNormalized(bool value) {
	if (_normalized != value) {
		_normalized = value;
		_transformUpdated = _transformDirty = _inverseDirty = true;
	}
}

void BatchNodeBase::setDensity(float density) {
	if (density != _density) {
		_density = density;
		_contentSizeDirty = true;
	}
}

float BatchNodeBase::getDensity() const {
	return _density;
}

NS_SP_END
