/*
 * SPDynamicBatchNode.h
 *
 *  Created on: 02 апр. 2015 г.
 *      Author: sbkarr
 */

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
