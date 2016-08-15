/*
 * SPDynamicBatchNode.h
 *
 *  Created on: 02 апр. 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICBATCHNODE_H_
#define LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICBATCHNODE_H_

#include "base/CCProtocols.h"
#include "2d/CCNode.h"

#include "SPDynamicAtlas.h"
#include "SPDynamicQuadArray.h"
#include "SPDynamicBatchCommand.h"

NS_SP_BEGIN

class DynamicBatchNode: public cocos2d::Node, public cocos2d::TextureProtocol {
public:
	virtual ~DynamicBatchNode();

	virtual bool init(cocos2d::Texture2D * = nullptr, float density = 0.0f);

	void setBlendFunc(const cocos2d::BlendFunc &blendFunc) override;
	const cocos2d::BlendFunc& getBlendFunc() const override;

	virtual void setOpacityModifyRGB(bool modify) override;
	virtual bool isOpacityModifyRGB(void) const override;

	virtual cocos2d::Texture2D* getTexture() const override;
	virtual void setTexture(cocos2d::Texture2D *texture) override;

	virtual void draw(cocos2d::Renderer *, const cocos2d::Mat4 &, uint32_t, const ZPath &zPath) override;

	virtual bool isNormalized() const;
	virtual void setNormalized(bool value);

    virtual void setDensity(float density);
    virtual float getDensity() const;

    virtual const DynamicQuadArray &getQuads() const;
    virtual DynamicQuadArray *getQuadsPtr();

protected:
	virtual void updateColor() override;

	void updateBlendFunc();

	DynamicAtlas* getAtlas(void);

	virtual cocos2d::GLProgramState *getProgramStateA8() const;
	virtual cocos2d::GLProgramState *getProgramStateI8() const;
	virtual cocos2d::GLProgramState *getProgramStateAI88() const;
	virtual cocos2d::GLProgramState *getProgramStateFullColor() const;

	bool _opacityModifyRGB = false;
	bool _normalized = false;

	cocos2d::BlendFunc _blendFunc;
	Rc<cocos2d::Texture2D> _texture = nullptr;

	Rc<DynamicAtlas> _textureAtlas = nullptr;
	DynamicQuadArray _quads;
	DynamicBatchCommand _batchCommand;     // render command

    float _density = 1.0f;
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICBATCHNODE_H_ */
