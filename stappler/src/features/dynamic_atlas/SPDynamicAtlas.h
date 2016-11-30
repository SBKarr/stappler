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

#ifndef LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICATLAS_H_
#define LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICATLAS_H_

#include "SPDefine.h"
#include "base/CCRef.h"
#include "base/ccTypes.h"
#include "renderer/CCTexture2D.h"
#include "SPEventHandler.h"

NS_SP_BEGIN

class DynamicAtlas : public cocos2d::Ref, EventHandler {
public:
	using QuadArraySet = Set<Rc<DynamicQuadArray>>;

	DynamicAtlas();
    virtual ~DynamicAtlas();

    bool init(cocos2d::Texture2D *texture, bool bufferSwapping = false);

    void drawQuads();
    void listenRendererRecreated();

    ssize_t getQuadsCount() const;

    cocos2d::Texture2D* getTexture() const;
    void setTexture(cocos2d::Texture2D* texture);

    const QuadArraySet &getQuads() const;
    QuadArraySet &getQuads();

    void clear();
    void addQuadArray(DynamicQuadArray *);
    void removeQuadArray(DynamicQuadArray *);
    void updateQuadArrays(QuadArraySet &&);

protected:
    size_t calculateBufferSize() const;
    size_t calculateQuadsCount() const;

    void visit();
    void setupIndices();
    void mapBuffers();
    void setup();
    void setupVBOandVAO();
    void setupVBO();

    void onCapacityDirty(size_t newSize);
    void onQuadsDirty();

    struct Buffer {
        GLuint vao = 0;
        GLuint vbo[2] = { 0, 0 };
        bool setup = false;
        size_t size = 0;
    };

    inline Buffer &drawBuffer() { return _buffers[_useBufferSwapping?_index:0]; }
    inline Buffer &transferBuffer() { return _buffers[_useBufferSwapping?((_index + 1) % 2):0]; }
    inline void swapBuffer() { _index = _useBufferSwapping?((_index + 1) % 2):_index; }

    bool _useBufferSwapping = true;
    Vector<GLushort> _indices;
    QuadArraySet _quads;
    Buffer _buffers[2];

    uint8_t _index = 0;
    bool _dirty = false;

    size_t _quadsCapacity = 0;
    size_t _quadsCount = 0;
    Rc<cocos2d::Texture2D> _texture;
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICATLAS_H_ */
