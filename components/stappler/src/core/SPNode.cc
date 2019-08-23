// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPNode.h"
#include "base/ccMacros.h"
#include "platform/CCPlatformMacros.h"
#include "base/CCDirector.h"
#include "2d/CCNode.h"
#include "SPThreadManager.h"
#include "SPThread.h"

#include "SPString.h"
#include "SPData.h"

NS_SP_BEGIN

namespace Anchor {
const Vec2 Middle(0.5f, 0.5f); /** equals to Vec2(0.5, 0.5) */
const Vec2 BottomLeft(0.0f, 0.0f); /** equals to Vec2(0, 0) */
const Vec2 TopLeft(0.0f, 1.0f); /** equals to Vec2(0, 1) */
const Vec2 BottomRight(1.0f, 0.0f); /** equals to Vec2(1, 0) */
const Vec2 TopRight(1.0f, 1.0f); /** equals to Vec2(1, 1) */
const Vec2 MiddleRight(1.0f, 0.5f); /** equals to Vec2(1, 0.5) */
const Vec2 MiddleLeft(0.0f, 0.5f); /** equals to Vec2(0, 0.5) */
const Vec2 MiddleTop(0.5f, 1.0f); /** equals to Vec2(0.5, 1) */
const Vec2 MiddleBottom(0.5f, 0.0f); /** equals to Vec2(0.5, 0) */
}

namespace node {

void Params::setPosition(float x, float y) {
	setPosition(Vec2(x, y));
}

void Params::setPosition(const Vec2 &pos) {
	position = pos;
	mask = Mask(mask | Mask::Position);
}

void Params::setAnchorPoint(const Vec2 &pt) {
	anchorPoint = pt;
	mask = Mask(mask | Mask::AnchorPoint);
}

void Params::setContentSize(const Size &size) {
	contentSize = size;
	mask = Mask(mask | Mask::ContentSize);
}

void Params::setVisible(bool value) {
	visible = value;
	mask = Mask(mask | Mask::Visibility);
}

#ifndef SP_RESTRICT
	bool isTouched(cocos2d::Node *node, const Vec2 &location, float padding) {
		const Vec2 &point = node->convertToNodeSpace(location);
		const Size &size = node->getContentSize();
		if (point.x > -padding && point.y > -padding
				&& point.x < size.width + padding
				&& point.y < size.height + padding) {
			return true;
		} else {
			return false;
		}
	}

	bool isParent(cocos2d::Node *parent, cocos2d::Node *node) {
		if (!node) {
			return false;
		}
		auto p = node->getParent();
		while (p) {
			if (p == parent) {
				return true;
			}
			p = p->getParent();
		}
		return false;
	}
	Mat4 chainNodeToParent(cocos2d::Node *parent, cocos2d::Node *node, bool all) {
		if (!isParent(parent, node)) {
			return Mat4::IDENTITY;
		}

		Mat4 ret = node->getNodeToParentTransform();
		auto p = node->getParent();
		for (; p != parent; p = p->getParent()) {
			ret = ret * p->getNodeToParentTransform();
		}
		if (all && p == parent) {
			ret = ret * p->getNodeToParentTransform();
		}
		return ret;
	}
	Mat4 chainParentToNode(cocos2d::Node *parent, cocos2d::Node *node, bool all) {
		if (!isParent(parent, node)) {
			return Mat4::IDENTITY;
		}

		Mat4 ret = node->getParentToNodeTransform();
		auto p = node->getParent();
		for (; p != parent; p = p->getParent()) {
			ret = p->getParentToNodeTransform() * ret;
		}
		if (all && p == parent) {
			ret = p->getParentToNodeTransform() * ret;
		}
		return ret;
	}

	void Params::apply(cocos2d::Node *node) const {
		if (mask & Mask::AnchorPoint) {
			node->setAnchorPoint(anchorPoint);
		}
		if (mask & Mask::Position) {
			node->setPosition(position);
		}
		if (mask & Mask::ContentSize) {
			node->setContentSize(contentSize);
		}
		if (mask & Mask::Visibility) {
			node->setVisible(visible);
		}
	}

#else
	void Params::apply(cocos2d::Node *) const { }

	bool isTouched(cocos2d::Node *node, const cocos2d::Vec2 &location, float padding) { return false; }

	bool isParent(cocos2d::Node *parent, cocos2d::Node *node) { return false; }
	Mat4 chainNodeToParent(cocos2d::Node *parent, cocos2d::Node *node, bool all) { return Mat4::IDENTITY; }
	Mat4 chainParentToNode(cocos2d::Node *parent, cocos2d::Node *node, bool all) { return Mat4::IDENTITY; }
#endif

	void apply(cocos2d::Node *node, const Params &params) {
		params.apply(node);
	}
}

NS_SP_END
