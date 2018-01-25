/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef STAPPLER_SRC_NODES_LABEL_SPLAYEREDBATCHNODE_H_
#define STAPPLER_SRC_NODES_LABEL_SPLAYEREDBATCHNODE_H_

#include "SPBatchNodeBase.h"

#include "SPDynamicAtlas.h"
#include "SPDynamicQuadArray.h"
#include "SPDynamicBatchCommand.h"

NS_SP_BEGIN

class LayeredBatchNode : public BatchNodeBase {
public:
	struct TextureLayer {
		Rc<cocos2d::Texture2D> texture;
		Rc<DynamicAtlas> atlas;
		Rc<DynamicQuadArray> quads;
	};

	virtual ~LayeredBatchNode();

	virtual void setTextures(const Vector<Rc<cocos2d::Texture2D>> &);
	virtual void setTextures(const Vector<cocos2d::Texture2D *> &);
	virtual void setTextures(const Vector<Rc<cocos2d::Texture2D>> &, Vector<Rc<DynamicQuadArray>> &&newQuads);

	virtual void draw(cocos2d::Renderer *, const Mat4 &, uint32_t, const ZPath &zPath) override;

    virtual DynamicQuadArray *getQuads(cocos2d::Texture2D *);

protected:
	virtual void updateColor() override;

	struct CmdWrap : Ref {
		bool init() { return true; }
		DynamicBatchCommand cmd;
	};

	Vector<TextureLayer> _textures;
	Vector<Rc<CmdWrap>> _commands;
};

NS_SP_END

#endif /* STAPPLER_SRC_NODES_LABEL_SPLAYEREDBATCHNODE_H_ */
