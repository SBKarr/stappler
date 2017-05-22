/*
 * SPDrawGLCache.cpp
 *
 *  Created on: 13 февр. 2017 г.
 *      Author: sbkarr
 */

#include "SPDrawGLCacheNode.h"
#include "SPThreadManager.h"
#include "base/CCConfiguration.h"

#include "renderer/ccGLStateCache.h"
#include "base/CCDirector.h"

NS_SP_EXT_BEGIN(draw)

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

void GLCacheNode::cleanup() {
	bindTexture(0);
	useProgram(0);
	enableVertexAttribs(0);
	blendFunc(cocos2d::BlendFunc::DISABLE);

	if (ThreadManager::getInstance()->isMainThread()) {
		cocos2d::Director::getInstance()->setViewport();
		cocos2d::GL::blendResetToCache();
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
