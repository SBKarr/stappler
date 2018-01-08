// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPDrawGLCacheNode.h"
#include "SPThreadManager.h"
#include "base/CCConfiguration.h"

#include "renderer/ccGLStateCache.h"
#include "base/CCDirector.h"

NS_SP_EXT_BEGIN(draw)

GLBlending GLBlending::loadFromGL() {
	GLint blendSrcRgb, blendSrcAlpha, blendDstRgb, blendDstAlpha, blendEqRgb, blendEqAlpha;

	if (glIsEnabled(GL_BLEND)) {
		glGetIntegerv(GL_BLEND_SRC_RGB, (GLint *) &blendSrcRgb);
		glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint *) &blendSrcAlpha);
		glGetIntegerv(GL_BLEND_DST_RGB, (GLint *) &blendDstRgb);
		glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint *) &blendDstAlpha);
		glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint *) &blendEqRgb);
		glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint *) &blendEqAlpha);

		return GLBlending(BlendFunc{GLenum(blendSrcRgb), GLenum(blendDstRgb)}, BlendFunc{GLenum(blendSrcAlpha), GLenum(blendDstAlpha)},
				getEqForEnum(blendEqRgb), getEqForEnum(blendEqAlpha));
	} else {
		return GLBlending();
	}
}

GLenum GLBlending::getEnumForEq(Equation eq) {
	switch (eq) {
	case GLBlending::FuncAdd: return GL_FUNC_ADD; break;
	case GLBlending::FuncSubstract: return GL_FUNC_SUBTRACT; break;
	case GLBlending::FuncReverseSubstract: return GL_FUNC_REVERSE_SUBTRACT; break;
#ifdef GL_MIN
	case GLBlending::Min: return cocos2d::Configuration::getInstance()->supportsBlendMinMax()?GL_MIN:GL_FUNC_ADD; break;
#elif GL_MIN_EXT
	case GLBlending::Min: return cocos2d::Configuration::getInstance()->supportsBlendMinMax()?GL_MIN_EXT:GL_FUNC_ADD; break;
#endif
#ifdef GL_MAX
	case GLBlending::Max: return cocos2d::Configuration::getInstance()->supportsBlendMinMax()?GL_MAX:GL_FUNC_ADD; break;
#elif GL_MAX_EXT
	case GLBlending::Max: return cocos2d::Configuration::getInstance()->supportsBlendMinMax()?GL_MAX_EXT:GL_FUNC_ADD; break;
#endif
	}

	return GL_FUNC_ADD;
}

GLBlending::Equation GLBlending::getEqForEnum(GLenum eq) {
	switch (eq) {
	case GL_FUNC_ADD: return GLBlending::FuncAdd; break;
	case GL_FUNC_SUBTRACT: return GLBlending::FuncSubstract; break;
	case GL_FUNC_REVERSE_SUBTRACT: return GLBlending::FuncReverseSubstract; break;
#ifdef GL_MIN
	case GL_MIN: return GLBlending::Min; break;
#elif GL_MIN_EXT
	case GL_MIN_EXT: return GLBlending::Min; break;
#endif
#ifdef GL_MAX
	case GL_MAX: return GLBlending::Max; break;
#elif GL_MAX_EXT
	case GL_MAX_EXT: return GLBlending::Max; break;
#endif
	}

	return GLBlending::FuncAdd;
}

void GLCacheNode::load() {
	GLint currentTex, currentProg;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint *) &currentTex);
	glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *) &currentProg);

	_currentTexture = _origTexture = GLuint(currentTex);
	_currentProgram = _origProgram = GLuint(currentProg);
	_blending = _origBlending = GLBlending::loadFromGL();

	if (ThreadManager::getInstance()->isMainThread()) {
		cocos2d::GL::enableVertexAttribs(0);
	}
	_attributeFlags = 0;
}

void GLCacheNode::cleanup() {
	bindTexture(_origTexture);
	useProgram(_origProgram);
	enableVertexAttribs(0);
	setBlending(_origBlending);
	CHECK_GL_ERROR_DEBUG();

	if (ThreadManager::getInstance()->isMainThread()) {
		cocos2d::Director::getInstance()->setViewport();
	}
	CHECK_GL_ERROR_DEBUG();
}

void GLCacheNode::bindTexture(GLuint value) {
	if (_currentTexture != value) {
		_currentTexture = value;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, value);
	}
}

void GLCacheNode::useProgram(GLuint value) {
	if (_currentProgram != value) {
		_currentProgram = value;
		glUseProgram(value);
	}
}

void GLCacheNode::enableVertexAttribs(uint32_t flags) {
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

void GLCacheNode::setBlending(const GLBlending &b) {
	if (b != _blending) {
		bool funcDirty = b.separateFunc != _blending.separateFunc
				|| (b.separateFunc && (b.colorFunc != _blending.colorFunc || b.alphaFunc != _blending.alphaFunc))
				|| (!b.separateFunc && b.colorFunc != _blending.colorFunc);
		bool eqDirty = b.separateEq != _blending.separateEq
				|| (b.separateEq && (b.colorEq != _blending.colorEq || b.alphaEq != _blending.alphaEq))
				|| (!b.separateEq && b.colorEq != _blending.colorEq);

		if (b.empty()) {
			glDisable(GL_BLEND);
		} else {
			if (_blending.empty()) {
				glEnable(GL_BLEND);
			}
			if (funcDirty) {
				if (b.separateFunc) {
					glBlendFuncSeparate(b.colorFunc.src, b.colorFunc.dst, b.alphaFunc.src, b.alphaFunc.dst);
				} else {
					glBlendFunc(b.colorFunc.src, b.colorFunc.dst);
				}
			}
			if (eqDirty) {
				if (b.separateEq) {
					glBlendEquationSeparate(b.colorEqEnum(), b.alphaEqEnum());
				} else {
					glBlendEquation(b.colorEqEnum());
				}
			}
		}
		_blending = b;
	}
}

void GLCacheNode::blendFunc(GLenum sfactor, GLenum dfactor) {
	setBlending(GLBlending(BlendFunc{sfactor, dfactor}));
}

void GLCacheNode::blendFunc(const BlendFunc &func) {
	setBlending(GLBlending(func));
}

NS_SP_EXT_END(draw)
