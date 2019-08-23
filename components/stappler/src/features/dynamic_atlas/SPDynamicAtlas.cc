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
#include "SPDynamicAtlas.h"

#include "base/CCDirector.h"
#include "base/CCConfiguration.h"
#include "renderer/CCTextureCache.h"
#include "renderer/CCGLProgram.h"
#include "renderer/CCRenderer.h"
#include "renderer/CCTexture2D.h"
#include "renderer/ccGLStateCache.h"
#include "platform/CCGL.h"

#include "SPDynamicQuadArray.h"
#include "SPStencilCache.h"
#include "SPString.h"
#include "SPDevice.h"

NS_SP_BEGIN

DynamicAtlas::DynamicAtlas() {}

DynamicAtlas::~DynamicAtlas() {
	if (transferBuffer().setup) {
	    glDeleteBuffers(2, transferBuffer().vbo);

	    if (cocos2d::Configuration::getInstance()->supportsShareableVAO()) {
	        glDeleteVertexArrays(1, &transferBuffer().vao);
	    }
	    transferBuffer().setup = false;
	}

	if (drawBuffer().setup) {
	    glDeleteBuffers(2, drawBuffer().vbo);

	    if (cocos2d::Configuration::getInstance()->supportsShareableVAO()) {
	        glDeleteVertexArrays(1, &drawBuffer().vao);
	    }

	    drawBuffer().setup = false;
	}
}

ssize_t DynamicAtlas::getQuadsCount() const {
	return _eltsCount;
}

cocos2d::Texture2D* DynamicAtlas::getTexture() const {
    return _texture;
}

void DynamicAtlas::setTexture(cocos2d::Texture2D * var) {
    _texture = var;
}

bool DynamicAtlas::init(cocos2d::Texture2D *texture, BufferParams buf) {
	_params = buf;
    _texture = texture;

#if CC_ENABLE_CACHE_TEXTURE_DATA
	onEvent(Device::onRegenerateTextures, [this] (const Event &ev) {
		listenRendererRecreated();
	});
#endif

    return true;
}

void DynamicAtlas::draw(bool update) {
	if (!_params.bufferSwapping) {
		visit();
	}

	auto nelts = _eltsCount;
	if (!nelts) {
		if (update && _params.bufferSwapping) {
			visit();
		}
		return;
	}

	if (drawBuffer().setup) {
		if (_texture) {
			cocos2d::GL::bindTexture2D(_texture->getName());
		}

		if (cocos2d::Configuration::getInstance()->supportsShareableVAO()) {
			cocos2d::GL::bindVAO(drawBuffer().vao);

#if CC_REBIND_INDICES_BUFFER
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawBuffer().vbo[1]);
#endif

			glDrawElements(GL_TRIANGLES, (GLsizei) nelts * _params.pointsPerElt, GL_UNSIGNED_SHORT, (GLvoid*) (0));

#if CC_REBIND_INDICES_BUFFER
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#endif
		} else {
			glBindBuffer(GL_ARRAY_BUFFER, drawBuffer().vbo[0]);

			cocos2d::GL::enableVertexAttribs(cocos2d::GL::VERTEX_ATTRIB_FLAG_POS_COLOR_TEX);

			glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE,
					sizeof(cocos2d::V3F_C4B_T2F), (GLvoid*) offsetof(cocos2d::V3F_C4B_T2F, vertices));

			glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE,
					sizeof(cocos2d::V3F_C4B_T2F), (GLvoid*) offsetof(cocos2d::V3F_C4B_T2F, colors));

			glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_TEX_COORD, 2, GL_FLOAT, GL_FALSE,
					sizeof(cocos2d::V3F_C4B_T2F), (GLvoid*) offsetof(cocos2d::V3F_C4B_T2F, texCoords));

			if (_params.indexBuffer) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawBuffer().vbo[1]);
			}

			glDrawElements(GL_TRIANGLES, (GLsizei) nelts * _params.pointsPerElt, GL_UNSIGNED_SHORT, (GLvoid*) (0));

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			if (_params.indexBuffer) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
		}

		CC_INCREMENT_GL_DRAWN_BATCHES_AND_VERTICES(1, nelts * 6);
		LOG_GL_ERROR();
	}

	if (update && _params.bufferSwapping) {
		visit();
	}
}

void DynamicAtlas::listenRendererRecreated() {
	drawBuffer().setup = false;
	drawBuffer().size = 0;
	transferBuffer().setup = false;
	transferBuffer().size = 0;

	setDirty();
}

void DynamicAtlas::setDirty() {
	_dirty = true;
}

void DynamicAtlas::mapBuffers() {
    // Avoid changing the element buffer for whatever VAO might be bound.
	cocos2d::GL::bindVAO(0);

    CHECK_GL_ERROR_DEBUG();
    glBindBuffer(GL_ARRAY_BUFFER, transferBuffer().vbo[0]);
    if (_params.fillBufferCallback) {
        _params.fillBufferCallback();
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (_params.indexBuffer && _params.fillIndexesCallback) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transferBuffer().vbo[1]);
        _params.fillIndexesCallback();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    CHECK_GL_ERROR_DEBUG();

    swapBuffer();
}

void DynamicAtlas::setupVBOandVAO() {
	glGenVertexArrays(1, &transferBuffer().vao);
	cocos2d::GL::bindVAO(transferBuffer().vao);

	glGenBuffers(2, &transferBuffer().vbo[0]);

	glBindBuffer(GL_ARRAY_BUFFER, transferBuffer().vbo[0]);

    if (_params.fillBufferCallback) {
        _params.fillBufferCallback();
    }

	glEnableVertexAttribArray(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION);
	glEnableVertexAttribArray(cocos2d::GLProgram::VERTEX_ATTRIB_COLOR);
	glEnableVertexAttribArray(cocos2d::GLProgram::VERTEX_ATTRIB_TEX_COORD);

	// vertices
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE,
			sizeof(cocos2d::V3F_C4B_T2F), (GLvoid*) offsetof(cocos2d::V3F_C4B_T2F, vertices));

	// colors
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE,
			sizeof(cocos2d::V3F_C4B_T2F), (GLvoid*) offsetof(cocos2d::V3F_C4B_T2F, colors));

	// tex coords
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_TEX_COORD, 2, GL_FLOAT, GL_FALSE,
			sizeof(cocos2d::V3F_C4B_T2F), (GLvoid*) offsetof(cocos2d::V3F_C4B_T2F, texCoords));

    if (_params.indexBuffer && _params.fillIndexesCallback) {
    	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transferBuffer().vbo[1]);
        _params.fillIndexesCallback();
    }

	// Must unbind the VAO before changing the element buffer.
	cocos2d::GL::bindVAO(0);
    if (_params.indexBuffer && _params.fillIndexesCallback) {
    	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
	glBindBuffer(GL_ARRAY_BUFFER, 0);

    LOG_GL_ERROR();
}

void DynamicAtlas::setupVBO() {
    glGenBuffers(2, &transferBuffer().vbo[0]);
    mapBuffers();
}

void DynamicAtlas::setup() {
    if (cocos2d::Configuration::getInstance()->supportsShareableVAO()) {
        setupVBOandVAO();
    } else {
        setupVBO();
    }

	transferBuffer().setup = true;
    swapBuffer();
}

NS_SP_END
