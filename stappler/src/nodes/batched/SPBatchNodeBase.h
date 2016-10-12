/*
 * SPBatchNodeBase.h
 *
 *  Created on: 26 окт. 2016 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_NODES_BATCHED_SPBATCHNODEBASE_H_
#define STAPPLER_SRC_NODES_BATCHED_SPBATCHNODEBASE_H_

#include "SPDefine.h"
#include "base/CCProtocols.h"
#include "2d/CCNode.h"

NS_SP_BEGIN

class BatchNodeBase : public cocos2d::Node {
public:
	virtual ~BatchNodeBase();

	virtual bool init(float density = 0.0f);

	void setBlendFunc(const cocos2d::BlendFunc &blendFunc);
	const cocos2d::BlendFunc &getBlendFunc() const;

	virtual void setOpacityModifyRGB(bool modify) override;
	virtual bool isOpacityModifyRGB(void) const override;

	virtual bool isNormalized() const;
	virtual void setNormalized(bool value);

    virtual void setDensity(float density);
    virtual float getDensity() const;

protected:
	void updateBlendFunc(cocos2d::Texture2D *);

	virtual cocos2d::GLProgramState *getProgramStateA8() const;
	virtual cocos2d::GLProgramState *getProgramStateI8() const;
	virtual cocos2d::GLProgramState *getProgramStateAI88() const;
	virtual cocos2d::GLProgramState *getProgramStateFullColor() const;

	bool _opacityModifyRGB = false;
	bool _normalized = false;

	cocos2d::BlendFunc _blendFunc;
    float _density = 1.0f;
};

NS_SP_END

#endif /* STAPPLER_SRC_NODES_BATCHED_SPBATCHNODEBASE_H_ */
