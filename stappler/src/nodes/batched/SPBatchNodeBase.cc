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
#include "SPBatchNodeBase.h"

#include "SPGLProgramSet.h"
#include "SPTextureCache.h"

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

	if (density == 0.0f) {
		density = stappler::screen::density();
	}
	_density = density;
	_blendFunc = cocos2d::BlendFunc::ALPHA_PREMULTIPLIED;

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
		auto glProgramName = getGLProgram() ? getGLProgram()->getProgram() : 0;
		auto newState = acquireProgramState(tex);

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
	} else {
		cocos2d::GLProgramState *newState = acquireProgramState(nullptr);
		if (!getGLProgram() || newState->getGLProgram()->getProgram() != getGLProgram()->getProgram()) {
		    setGLProgramState(newState);
		}
        _blendFunc = cocos2d::BlendFunc::ALPHA_NON_PREMULTIPLIED;
		setOpacityModifyRGB(false);
	}
	_programDirty = false;
}

cocos2d::GLProgramState *BatchNodeBase::acquireProgramState(cocos2d::Texture2D *tex) const {
	auto attrs = GLProgramDesc::Attr::MatrixMVP | GLProgramDesc::Attr::Color;
	if (tex) {
		attrs |= GLProgramDesc::Attr::TexCoords;
	}
	if (_isHighPrecision) {
		attrs |=  GLProgramDesc::Attr::HighP;
	}

	switch (_alphaTest.state) {
	case AlphaTest::Disabled: break;
	case AlphaTest::LessThen: attrs |=  GLProgramDesc::Attr::AlphaTestLT; break;
	case AlphaTest::GreatherThen: attrs |=  GLProgramDesc::Attr::AlphaTestGT; break;
	}

	auto desc = tex ? GLProgramDesc(attrs, tex->getPixelFormat(), tex->getReferenceFormat()) : GLProgramDesc(attrs);
	auto prog = TextureCache::getInstance()->getPrograms()->getProgram(desc);
	return cocos2d::GLProgramState::getOrCreateWithGLProgram(prog);
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

void BatchNodeBase::setAlphaTest(AlphaTest::State s, uint8_t value) {
	AlphaTest newTest;
	newTest.state = s;
	if (newTest.state != AlphaTest::Disabled) {
		newTest.value = value;
	} else {
		newTest.value = 0;
	}

	if (newTest.state != _alphaTest.state) {
		_programDirty = true;
	}
	_alphaTest = newTest;
}
const AlphaTest &BatchNodeBase::getAlphaTest() const {
	return _alphaTest;
}

void BatchNodeBase::setHighPrecision(bool value) {
	if (_isHighPrecision != value) {
		_isHighPrecision = value;
		_programDirty = true;
	}
}
bool BatchNodeBase::isHighPrecision() const {
	return _isHighPrecision;
}

NS_SP_END
