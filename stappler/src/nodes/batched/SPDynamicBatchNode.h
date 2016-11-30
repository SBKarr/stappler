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

#ifndef LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICBATCHNODE_H_
#define LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICBATCHNODE_H_

#include "SPBatchNodeBase.h"
#include "SPDynamicAtlas.h"
#include "SPDynamicQuadArray.h"
#include "SPDynamicBatchCommand.h"

NS_SP_BEGIN

class DynamicBatchNode: public BatchNodeBase {
public:
	DynamicBatchNode();
	virtual ~DynamicBatchNode();

	virtual bool init(cocos2d::Texture2D * = nullptr, float density = 0.0f);

	virtual cocos2d::Texture2D* getTexture() const;
	virtual void setTexture(cocos2d::Texture2D *texture);

	virtual void draw(cocos2d::Renderer *, const cocos2d::Mat4 &, uint32_t, const ZPath &zPath) override;

    virtual DynamicQuadArray *getQuads() const;

protected:
	virtual void updateColor() override;

	DynamicAtlas* getAtlas(void);

	Rc<cocos2d::Texture2D> _texture = nullptr;
	Rc<DynamicAtlas> _textureAtlas = nullptr;
	Rc<DynamicQuadArray> _quads;
	DynamicBatchCommand _batchCommand;     // render command
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICBATCHNODE_H_ */
