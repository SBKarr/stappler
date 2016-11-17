/*
 * SPDymanicBatchScene.h
 *
 *  Created on: 12 окт. 2015 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_NODES_SPDYNAMICBATCHSCENE_H_
#define STAPPLER_SRC_NODES_SPDYNAMICBATCHSCENE_H_

#include "2d/CCScene.h"
#include "base/CCMap.h"
#include "SPDefine.h"

#include "SPDynamicAtlas.h"
#include "SPDynamicBatchCommand.h"

NS_SP_BEGIN

class DynamicBatchScene : public cocos2d::Scene {
public:
	static EventHeader onFrameBegin;
	static EventHeader onFrameEnd;

	virtual bool init() override;
    virtual void visit(cocos2d::Renderer *, const Mat4 &, uint32_t, ZPath &zPath) override;

    virtual void setBatchingEnabled(bool value);
    virtual bool isBatchingEnabled() const;

    virtual void clearCachedMaterials(bool force = false);

protected:
    friend class DynamicBatchSceneRenderer;

	struct AtlasCacheNode {
		Rc<DynamicAtlas> atlas;
		DynamicBatchCommand cmd;
		Set<Rc<DynamicQuadArray>> set;
		bool cmdInit;

		AtlasCacheNode(Rc<DynamicAtlas> &&atlas);
	};

	virtual AtlasCacheNode &getAtlasForMaterial(uint32_t id, DynamicBatchCommand *cmd);

	bool _batchingEnabled = true;
	size_t _clearDelay = 0;
	bool _shouldClear = false;
	std::map<uint32_t, AtlasCacheNode> _map;
};

NS_SP_END

#endif /* STAPPLER_SRC_NODES_SPDYNAMICBATCHSCENE_H_ */
