/*
 * SPClippingNode.h
 *
 *  Created on: 16 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_NODES_SPCLIPPINGNODE_H_
#define LIBS_STAPPLER_NODES_SPCLIPPINGNODE_H_

#include "SPDefine.h"
#include "2d/CCClippingNode.h"

NS_SP_BEGIN

class ClippingNode : public cocos2d::ClippingNode {
public:
    virtual bool init(Node *stencil = nullptr) override;

    virtual void setEnabled(bool value);
    virtual bool isEnabled() const;

    virtual void visit(cocos2d::Renderer *, const cocos2d::Mat4 &, uint32_t, ZPath &zPath) override;

protected:
	bool _enabled = true;
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_SPCLIPPINGNODE_H_ */
