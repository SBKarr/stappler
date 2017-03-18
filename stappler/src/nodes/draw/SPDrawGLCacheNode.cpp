/*
 * SPDrawGLCache.cpp
 *
 *  Created on: 13 февр. 2017 г.
 *      Author: sbkarr
 */

#include "SPDrawGLCacheNode.h"
#include "SPThreadManager.h"

#include "renderer/ccGLStateCache.h"
#include "base/CCDirector.h"

NS_SP_EXT_BEGIN(draw)

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

void GLCacheNode::blendFunc(GLenum sfactor, GLenum dfactor) {
	if (sfactor != _blendingSource || dfactor != _blendingDest) {
		_blendingSource = sfactor;
		_blendingDest = dfactor;
		if (sfactor == GL_ONE && dfactor == GL_ZERO) {
			glDisable(GL_BLEND);
		} else {
			glEnable(GL_BLEND);
			glBlendFunc(sfactor, dfactor);
		}
	}
}

void GLCacheNode::blendFunc(const cocos2d::BlendFunc &func) {
	blendFunc(func.src, func.dst);
}

NS_SP_EXT_END(draw)
