/*
 * SPDynamicAtlas.cpp
 *
 *  Created on: 30 марта 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPDynamicAtlas.h"

#include "base/ccMacros.h"
#include "base/CCDirector.h"
#include "base/CCConfiguration.h"
#include "renderer/CCTextureCache.h"
#include "renderer/CCGLProgram.h"
#include "renderer/ccGLStateCache.h"
#include "renderer/CCRenderer.h"
#include "renderer/CCTexture2D.h"
#include "platform/CCGL.h"

#include "SPDynamicQuadArray.h"
#include "SPString.h"
#include "SPDevice.h"

NS_SP_BEGIN

static void DynamicAtlas_fillBuffer(const std::set<DynamicQuadArray *> &quads, size_t bufferSize) {
	if (bufferSize <= 0) {
		return;
	}

	bool vao = cocos2d::Configuration::getInstance()->supportsShareableVAO();
	void *buf = nullptr;

	if (vao) {
		glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
		buf = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	} else {
		buf = malloc(bufferSize);
	}

	size_t offset = 0;
	for (auto &it : quads) {
		size_t size = it->size();
		if (size > 0) {
			auto data = it->getData();
			memcpy((uint8_t *) buf + sizeof(cocos2d::V3F_C4B_T2F_Quad) * offset, (void *) data,
					sizeof(cocos2d::V3F_C4B_T2F_Quad) * size);

			offset += size;
		}
	}

	if (vao) {
		glUnmapBuffer(GL_ARRAY_BUFFER);
	} else {
		glBufferData(GL_ARRAY_BUFFER, bufferSize, buf, GL_DYNAMIC_DRAW);
	}
    CHECK_GL_ERROR_DEBUG();
}

DynamicAtlas::DynamicAtlas() {}

DynamicAtlas::~DynamicAtlas() {
	if (transferBuffer().setup) {
	    glDeleteBuffers(2, transferBuffer().vbo);

	    if (cocos2d::Configuration::getInstance()->supportsShareableVAO()) {
	        glDeleteVertexArrays(1, &transferBuffer().vao);
	        cocos2d::GL::bindVAO(0);
	    }
	    transferBuffer().setup = false;
	}

	if (drawBuffer().setup) {
	    glDeleteBuffers(2, drawBuffer().vbo);

	    if (cocos2d::Configuration::getInstance()->supportsShareableVAO()) {
	        glDeleteVertexArrays(1, &drawBuffer().vao);
	        cocos2d::GL::bindVAO(0);
	    }

	    drawBuffer().setup = false;
	}
}

ssize_t DynamicAtlas::getQuadsCount() const {
	return _quadsCount;
}

cocos2d::Texture2D* DynamicAtlas::getTexture() const {
    return _texture;
}

void DynamicAtlas::setTexture(cocos2d::Texture2D * var) {
    _texture = var;
}

const std::set<DynamicQuadArray *> &DynamicAtlas::getQuads() {
	return _quads;
}

void DynamicAtlas::clear() {
	_quads.clear();
	_dirty = true;
}

void DynamicAtlas::addQuadArray(DynamicQuadArray *arr) {
	_quads.insert(arr);
	_dirty = true;
}

void DynamicAtlas::removeQuadArray(DynamicQuadArray *arr) {
	auto it = _quads.find(arr);
	if (it != _quads.end()) {
		_quads.erase(it);
		_dirty = true;
	}
}

void DynamicAtlas::updateQuadArrays(QuadArraySet &&set) {
	bool dirty = false;
	if (set.empty()) {
		_quads.clear();
		_dirty = true;
		return;
	}

	if (_quads.size() != set.size()) {
		dirty = true;
	} else {
		auto fit = _quads.begin();
		auto sit = set.begin();

		while (fit != _quads.end() && (*fit) == (*sit)) {
			fit ++;
			sit ++;
		}
		if (fit != _quads.end()) {
			dirty = true;
		}
	}

	if (dirty) {
		_quads = std::move(set);
		_dirty = true;
	}
}

bool DynamicAtlas::init(cocos2d::Texture2D *texture, bool bufferSwapping) {
	_useBufferSwapping = bufferSwapping;
    _texture = texture;

#if CC_ENABLE_CACHE_TEXTURE_DATA
	onEvent(Device::onAndroidReset, [this] (const Event *ev) {
		listenRendererRecreated();
	});
#endif

    return true;
}

void DynamicAtlas::listenRendererRecreated() {
	drawBuffer().setup = false;
	drawBuffer().size = 0;
	transferBuffer().setup = false;
	transferBuffer().size = 0;

	_dirty = true;

	for (auto &it : _quads) {
		it->setDirty();
	}
}

size_t DynamicAtlas::calculateBufferSize() const {
	size_t ret = 0;
	for (auto &it : _quads) {
		ret += it->capacity();
	}
	return ret;
}
size_t DynamicAtlas::calculateQuadsCount() const {
	if (_quads.empty()) {
		return 0;
	}
	size_t ret = 0;
	for (auto &it : _quads) {
		ret += it->size();
	}
	return ret;
}


void DynamicAtlas::setup() {
    setupIndices();
    if (cocos2d::Configuration::getInstance()->supportsShareableVAO()) {
        setupVBOandVAO();
    } else {
        setupVBO();
    }

	transferBuffer().setup = true;
    swapBuffer();
}

void DynamicAtlas::setupIndices() {
    if (transferBuffer().size == 0)
        return;

    _indices.reserve(transferBuffer().size * 6);
    _indices.clear();

    for(size_t i= 0; i < transferBuffer().size; i++) {
        _indices.push_back( i*4 + 0 );
        _indices.push_back( i*4 + 1 );
        _indices.push_back( i*4 + 2 );

        // inverted index. issue #179
        _indices.push_back( i*4 + 3 );
        _indices.push_back( i*4 + 2 );
        _indices.push_back( i*4 + 1 );
    }
}

void DynamicAtlas::setupVBOandVAO() {
	glGenVertexArrays(1, &transferBuffer().vao);
	cocos2d::GL::bindVAO(transferBuffer().vao);

	glGenBuffers(2, &transferBuffer().vbo[0]);

	glBindBuffer(GL_ARRAY_BUFFER, transferBuffer().vbo[0]);

	DynamicAtlas_fillBuffer(_quads, sizeof(cocos2d::V3F_C4B_T2F_Quad) * transferBuffer().size);

	// vertices
	glEnableVertexAttribArray(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION);
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE,
			sizeof(cocos2d::V3F_C4B_T2F), (GLvoid*) offsetof(cocos2d::V3F_C4B_T2F, vertices));

	// colors
	glEnableVertexAttribArray(cocos2d::GLProgram::VERTEX_ATTRIB_COLOR);
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE,
			sizeof(cocos2d::V3F_C4B_T2F), (GLvoid*) offsetof(cocos2d::V3F_C4B_T2F, colors));

	// tex coords
	glEnableVertexAttribArray(cocos2d::GLProgram::VERTEX_ATTRIB_TEX_COORD);
	glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_TEX_COORD, 2, GL_FLOAT, GL_FALSE,
			sizeof(cocos2d::V3F_C4B_T2F), (GLvoid*) offsetof(cocos2d::V3F_C4B_T2F, texCoords));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transferBuffer().vbo[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * transferBuffer().size * 6,
			_indices.data(), GL_STATIC_DRAW);

	// Must unbind the VAO before changing the element buffer.
	cocos2d::GL::bindVAO(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	CHECK_GL_ERROR_DEBUG();
}

void DynamicAtlas::setupVBO() {
    glGenBuffers(2, &transferBuffer().vbo[0]);
    mapBuffers();
}

void DynamicAtlas::mapBuffers() {
    // Avoid changing the element buffer for whatever VAO might be bound.
	cocos2d::GL::bindVAO(0);

    CHECK_GL_ERROR_DEBUG();
    glBindBuffer(GL_ARRAY_BUFFER, transferBuffer().vbo[0]);

	DynamicAtlas_fillBuffer(_quads, sizeof(cocos2d::V3F_C4B_T2F_Quad) * transferBuffer().size);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transferBuffer().vbo[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (sizeof(GLushort) * transferBuffer().size * 6), (const GLvoid *)_indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    CHECK_GL_ERROR_DEBUG();

    swapBuffer();
}

void DynamicAtlas::onCapacityDirty(size_t newSize) {
	if (newSize > transferBuffer().size) {
		transferBuffer().size = newSize;
		setupIndices();
		if (transferBuffer().setup) {
			mapBuffers();
		} else {
			setup();
		}
		for (auto &it : _quads) {
			it->dropDirty();
		}
	} else {
		onQuadsDirty();
	}
}

void DynamicAtlas::onQuadsDirty() {
    CHECK_GL_ERROR_DEBUG();
	glBindBuffer(GL_ARRAY_BUFFER, transferBuffer().vbo[0]);

	DynamicAtlas_fillBuffer(_quads, sizeof(cocos2d::V3F_C4B_T2F_Quad) * transferBuffer().size);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

    CHECK_GL_ERROR_DEBUG();

	for (auto &it : _quads) {
		it->dropDirty();
	}

    swapBuffer();
}

void DynamicAtlas::visit() {
	if (_dirty) {
		_quadsCount = calculateQuadsCount();
		_quadsCapacity = calculateBufferSize();
		if (_quadsCapacity > 0) {
			if (_quadsCapacity > transferBuffer().size || !transferBuffer().setup) {
				onCapacityDirty(_quadsCapacity);
			} else {
				onQuadsDirty();
			}

			_dirty = false;
		}
		return;
	}

	bool isSizeDirty = false;
	bool isQuadsDirty = false;

	for (auto &it : _quads) {
		if (it->isCapacityDirty()) {
			isSizeDirty = true;
			break;
		}
		if (it->isQuadsDirty()) {
			isQuadsDirty = true;
		}
	}

	if (isSizeDirty || isQuadsDirty) {
		_quadsCount = calculateQuadsCount();
		_quadsCapacity = calculateBufferSize();
	} else {
		return;
	}

	if (_quadsCapacity > transferBuffer().size || !transferBuffer().setup) {
		onCapacityDirty(_quadsCapacity);
	} else if (isQuadsDirty || isSizeDirty) {
		onQuadsDirty();
	} else {
		for (auto &it : _quads) {
			it->dropDirty();
		}
	}
}

void DynamicAtlas::drawQuads() {
	if (_quads.empty()) {
		return;
	}

    if (!_useBufferSwapping) {
    	visit();
    }

    auto numberOfQuads = _quadsCount;
    if (!numberOfQuads) {
        if (_useBufferSwapping) {
            visit();
        }
        return;
    }

    if (drawBuffer().setup) {
        cocos2d::GL::bindTexture2D(_texture->getName());

        if (cocos2d::Configuration::getInstance()->supportsShareableVAO()) {
            cocos2d::GL::bindVAO(drawBuffer().vao);

    #if CC_REBIND_INDICES_BUFFER
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawBuffer().vbo[1]);
    #endif

            glDrawElements(GL_TRIANGLES, (GLsizei) numberOfQuads*6, GL_UNSIGNED_SHORT, (GLvoid*) (0) );

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

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawBuffer().vbo[1]);

            glDrawElements(GL_TRIANGLES, (GLsizei)numberOfQuads*6, GL_UNSIGNED_SHORT, (GLvoid*) (0));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }

        CC_INCREMENT_GL_DRAWN_BATCHES_AND_VERTICES(1,numberOfQuads*6);
        CHECK_GL_ERROR_DEBUG();
    }

    if (_useBufferSwapping) {
        visit();
    }
}

NS_SP_END
