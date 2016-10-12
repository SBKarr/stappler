/*
 * SPLayeredBatchNode.h
 *
 *  Created on: 26 окт. 2016 г.
 *      Author: sbkarr
 */

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

	Vector<TextureLayer> _textures;
	Vector<Rc<DynamicBatchCommand>> _commands;
};

NS_SP_END

#endif /* STAPPLER_SRC_NODES_LABEL_SPLAYEREDBATCHNODE_H_ */
