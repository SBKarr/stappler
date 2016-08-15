/*
 * SPClippingNode.cpp
 *
 *  Created on: 16 мая 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPClippingNode.h"

NS_SP_BEGIN

bool ClippingNode::init(cocos2d::Node *stencil) {
	if (!cocos2d::ClippingNode::init(stencil)) {
		return false;
	}

	setCascadeOpacityEnabled(true);

	return true;
}

void ClippingNode::setEnabled(bool value) {
	_enabled = value;
}
bool ClippingNode::isEnabled() const {
	return _enabled;
}

void ClippingNode::visit(cocos2d::Renderer *r, const cocos2d::Mat4 &t, uint32_t f, ZPath &zPath) {
	if (_enabled) {
		cocos2d::ClippingNode::visit(r, t, f, zPath);
	} else {
		cocos2d::Node::visit(r, t, f, zPath);
	}
}
NS_SP_END
