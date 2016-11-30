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
