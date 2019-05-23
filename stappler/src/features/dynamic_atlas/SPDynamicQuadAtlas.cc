/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPDynamicQuadAtlas.h"
#include "base/CCConfiguration.h"
#include "base/ccMacros.h"
#include "renderer/CCGLProgram.h"
#include "renderer/ccGLStateCache.h"
#include "platform/CCGL.h"

NS_SP_BEGIN

static void DynamicQuadAtlas_fillBuffer(const Set<Rc<DynamicQuadArray>> &quads, size_t bufferSize) {
	if (bufferSize <= 0) {
		return;
	}

	bool vao = cocos2d::Configuration::getInstance()->supportsShareableVAO();
	void *buf = nullptr;

	size_t offset = 0;
	glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
	if (vao) {
		buf = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		for (auto &it : quads) {
			size_t size = it->size();
			if (size > 0) {
				auto data = it->getData();
				memcpy((uint8_t *) buf + sizeof(cocos2d::V3F_C4B_T2F_Quad) * offset, (void *) data,
						sizeof(cocos2d::V3F_C4B_T2F_Quad) * size);

				offset += size;
			}
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
	} else {
		for (auto &it : quads) {
			size_t size = it->size();
			if (size > 0) {
				auto data = it->getData();
				glBufferSubData(GL_ARRAY_BUFFER, sizeof(cocos2d::V3F_C4B_T2F_Quad) * offset,
						sizeof(cocos2d::V3F_C4B_T2F_Quad) * size, (void *) data);
				offset += size;
			}
		}
	}

    LOG_GL_ERROR();
}

bool DynamicQuadAtlas::init(cocos2d::Texture2D *tex) {
	if (!DynamicAtlas::init(tex, BufferParams{
		false, true, 6, [this] {
			DynamicQuadAtlas_fillBuffer(_quads, sizeof(cocos2d::V3F_C4B_T2F_Quad) * transferBuffer().size);
		}, [this] {
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * transferBuffer().size * 6,
					_indices.data(), GL_DYNAMIC_DRAW);
		}
	})) {
		return false;
	}

	return true;
}

void DynamicQuadAtlas::clear() {
	_quads.clear();
	_dirty = true;
}

const DynamicQuadAtlas::ArraySet &DynamicQuadAtlas::getSet() const {
	return _quads;
}
DynamicQuadAtlas::ArraySet &DynamicQuadAtlas::getSet() {
	return _quads;
}

void DynamicQuadAtlas::addArray(DynamicQuadArray *arr) {
	_quads.insert(arr);
	_dirty = true;
}

void DynamicQuadAtlas::removeArray(DynamicQuadArray *arr) {
	auto it = _quads.find(arr);
	if (it != _quads.end()) {
		_quads.erase(it);
		_dirty = true;
	}
}

void DynamicQuadAtlas::updateArrays(ArraySet &&set) {
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

void DynamicQuadAtlas::setDirty() {
	DynamicAtlas::setDirty();
	for (auto it : _quads) {
		it->setDirty();
	}
}

void DynamicQuadAtlas::setup() {
    setupIndices();
    DynamicAtlas::setup();
}

void DynamicQuadAtlas::setupIndices() {
    if (transferBuffer().size == 0) {
        return;
    }

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

void DynamicQuadAtlas::onCapacityDirty(size_t newSize) {
	if (newSize > transferBuffer().size) {
		transferBuffer().size = newSize;
		setupIndices();
		if (transferBuffer().setup) {
			mapBuffers();
		} else {
			setup();
		}
		for (auto it : _quads) {
			it->dropDirty();
		}
	} else {
		onQuadsDirty();
	}
}

void DynamicQuadAtlas::onQuadsDirty() {
    CHECK_GL_ERROR_DEBUG();
	glBindBuffer(GL_ARRAY_BUFFER, transferBuffer().vbo[0]);

	DynamicQuadAtlas_fillBuffer(_quads, sizeof(cocos2d::V3F_C4B_T2F_Quad) * transferBuffer().size);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

    CHECK_GL_ERROR_DEBUG();

	for (auto it : _quads) {
		it->dropDirty();
	}

    swapBuffer();
}

void DynamicQuadAtlas::visit() {
	if (_dirty) {
		_eltsCount = calculateQuadsCount();
		_eltsCapacity = calculateBufferSize();
		if (_eltsCapacity > 0) {
			if (_eltsCapacity > transferBuffer().size || !transferBuffer().setup) {
				onCapacityDirty(_eltsCapacity);
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
		_eltsCount = calculateQuadsCount();
		_eltsCapacity = calculateBufferSize();
	} else {
		return;
	}

	if (_eltsCapacity > transferBuffer().size || !transferBuffer().setup) {
		onCapacityDirty(_eltsCapacity);
	} else if (isQuadsDirty || isSizeDirty) {
		onQuadsDirty();
	} else {
		for (auto it : _quads) {
			it->dropDirty();
		}
	}
}

size_t DynamicQuadAtlas::calculateBufferSize() const {
	size_t ret = 0;
	for (auto &it : _quads) {
		ret += it->capacity();
	}
	return ret;
}

size_t DynamicQuadAtlas::calculateQuadsCount() const {
	size_t ret = 0;
	for (auto &it : _quads) {
		ret += it->size();
	}
	return ret;
}

NS_SP_END
