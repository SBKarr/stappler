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

#include "SPDynamicTriangleAtlas.h"
#include "base/CCConfiguration.h"
#include "base/ccMacros.h"
#include "renderer/CCGLProgram.h"
#include "renderer/ccGLStateCache.h"
#include "platform/CCGL.h"

NS_SP_BEGIN

static void DynamicTriangleAtlas_fillBuffer(const Set<Rc<DynamicTriangleArray>> &quads, size_t bufferSize) {
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
				memcpy((uint8_t *) buf + sizeof(V3F_C4B_T2F_Triangle) * offset, (void *) data,
						sizeof(V3F_C4B_T2F_Triangle) * size);

				offset += size;
			}
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
	} else {
		for (auto &it : quads) {
			size_t size = it->size();
			if (size > 0) {
				auto data = it->getData();
				glBufferSubData(GL_ARRAY_BUFFER, sizeof(V3F_C4B_T2F_Triangle) * offset,
						sizeof(V3F_C4B_T2F_Triangle) * size, (void *) data);
				offset += size;
			}
		}
	}

    LOG_GL_ERROR();
}

bool DynamicTriangleAtlas::init(cocos2d::Texture2D *tex) {
	if (!DynamicAtlas::init(tex, BufferParams{
		false, true, 3, [this] {
			DynamicTriangleAtlas_fillBuffer(_set, sizeof(V3F_C4B_T2F_Triangle) * transferBuffer().size);
		}, [this] {
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * transferBuffer().size * 3,
					_indices.data(), GL_DYNAMIC_DRAW);
		}
	})) {
		return false;
	}

	return true;
}

void DynamicTriangleAtlas::clear() {
	_set.clear();
	_dirty = true;
}

const DynamicTriangleAtlas::ArraySet &DynamicTriangleAtlas::getSet() const {
	return _set;
}
DynamicTriangleAtlas::ArraySet &DynamicTriangleAtlas::getSet() {
	return _set;
}

void DynamicTriangleAtlas::addArray(DynamicTriangleArray *arr) {
	_set.insert(arr);
	_dirty = true;
}

void DynamicTriangleAtlas::removeArray(DynamicTriangleArray *arr) {
	auto it = _set.find(arr);
	if (it != _set.end()) {
		_set.erase(it);
		_dirty = true;
	}
}

void DynamicTriangleAtlas::updateArrays(ArraySet &&set) {
	bool dirty = false;
	if (set.empty()) {
		_set.clear();
		_dirty = true;
		return;
	}

	if (_set.size() != set.size()) {
		dirty = true;
	} else {
		auto fit = _set.begin();
		auto sit = set.begin();

		while (fit != _set.end() && (*fit) == (*sit)) {
			fit ++;
			sit ++;
		}
		if (fit != _set.end()) {
			dirty = true;
		}
	}

	if (dirty) {
		_set = std::move(set);
		_dirty = true;
	}
}

void DynamicTriangleAtlas::setDirty() {
	DynamicAtlas::setDirty();
	for (auto it : _set) {
		it->setDirty();
	}
}

void DynamicTriangleAtlas::setup() {
    setupIndices();
    DynamicAtlas::setup();
}

void DynamicTriangleAtlas::setupIndices() {
    if (transferBuffer().size == 0) {
        return;
    }

    _indices.reserve(transferBuffer().size * 6);
    _indices.clear();

    for(size_t i= 0; i < transferBuffer().size; i++) {
        _indices.push_back( i*3 + 0 );
        _indices.push_back( i*3 + 1 );
        _indices.push_back( i*3 + 2 );
    }
}

void DynamicTriangleAtlas::onCapacityDirty(size_t newSize) {
	if (newSize > transferBuffer().size) {
		transferBuffer().size = newSize;
		setupIndices();
		if (transferBuffer().setup) {
			mapBuffers();
		} else {
			setup();
		}
		for (auto it : _set) {
			it->dropDirty();
		}
	} else {
		onQuadsDirty();
	}
}

void DynamicTriangleAtlas::onQuadsDirty() {
    CHECK_GL_ERROR_DEBUG();
	glBindBuffer(GL_ARRAY_BUFFER, transferBuffer().vbo[0]);

	DynamicTriangleAtlas_fillBuffer(_set, sizeof(V3F_C4B_T2F_Triangle) * transferBuffer().size);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

    CHECK_GL_ERROR_DEBUG();

	for (auto it : _set) {
		it->dropDirty();
	}

    swapBuffer();
}

void DynamicTriangleAtlas::visit() {
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

	for (auto &it : _set) {
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
		for (auto it : _set) {
			it->dropDirty();
		}
	}
}

size_t DynamicTriangleAtlas::calculateBufferSize() const {
	size_t ret = 0;
	for (auto &it : _set) {
		ret += it->capacity();
	}
	return ret;
}

size_t DynamicTriangleAtlas::calculateQuadsCount() const {
	size_t ret = 0;
	for (auto &it : _set) {
		ret += it->size();
	}
	return ret;
}

NS_SP_END
