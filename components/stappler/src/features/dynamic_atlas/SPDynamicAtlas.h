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

	struct BufferParams {
	    bool bufferSwapping = false;
	    bool indexBuffer = false;

	    size_t pointsPerElt = 3;

	    Function<void()> fillBufferCallback;
	    Function<void()> fillIndexesCallback;
	};

	DynamicAtlas();
	virtual ~DynamicAtlas();

	virtual bool init(cocos2d::Texture2D *texture, BufferParams);

	virtual void draw(bool update = true);

	void listenRendererRecreated();

	ssize_t getQuadsCount() const;

	cocos2d::Texture2D* getTexture() const;
	void setTexture(cocos2d::Texture2D* texture);

	virtual void clear() = 0;

protected:
    virtual void visit() = 0;

	virtual void setDirty();
	virtual void setup();

    void mapBuffers();
	void setupVBOandVAO();
	void setupVBO();

    struct Buffer {
        GLuint vao = 0;
        GLuint vbo[2] = { 0, 0 };
        bool setup = false;
        size_t size = 0;
    };

    inline Buffer &drawBuffer() { return _buffers[_params.bufferSwapping?_index:0]; }
    inline Buffer &transferBuffer() { return _buffers[_params.bufferSwapping?((_index + 1) % 2):0]; }
    inline void swapBuffer() { _index = _params.bufferSwapping?((_index + 1) % 2):_index; }

    BufferParams _params;
    Buffer _buffers[2];

    uint8_t _index = 0;
    bool _dirty = false;

    size_t _eltsCapacity = 0;
    size_t _eltsCount = 0;
    Rc<cocos2d::Texture2D> _texture;
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICATLAS_H_ */
