/*
 * SPDynamicBatchCommand.h
 *
 *  Created on: 01 апр. 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICBATCHCOMMAND_H_
#define LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICBATCHCOMMAND_H_

#include "SPDefine.h"
#include "renderer/CCCustomCommand.h"

NS_SP_BEGIN

class DynamicBatchCommand : public cocos2d::CustomCommand {
public:
	using GLProgram = cocos2d::GLProgram;
	using BlendFunc = cocos2d::BlendFunc;

	DynamicBatchCommand(bool b = false);

    void init(float depth, GLProgram*, BlendFunc, DynamicAtlas *, const Mat4& mv, const std::vector<int> &, bool n);
    void execute();

    GLuint getTextureId() const;
    cocos2d::GLProgram *getProgram() const;
    const cocos2d::BlendFunc &getBlendFunc() const;
    DynamicAtlas *getAtlas() const;

    const Mat4 &getTransform() const;

    bool isNormalized() const;

    uint32_t getMaterialId(int32_t groupId) const;

protected:
    //Material
    GLuint _textureID;
    cocos2d::GLProgram* _shader;
    cocos2d::BlendFunc _blendType;

    DynamicAtlas *_textureAtlas;

    // ModelView transform
    cocos2d::Mat4 _mv;
    bool _normalized;
    bool _batch;
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICBATCHCOMMAND_H_ */
