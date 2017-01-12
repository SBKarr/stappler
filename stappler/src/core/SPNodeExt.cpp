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

#include "base/ccMacros.h"
#include "platform/CCPlatformMacros.h"
#include "base/CCDirector.h"
#include "2d/CCNode.h"
#include "SPThreadManager.h"
#include "SPThread.h"

#include "SPString.h"
#include "SPData.h"

NS_SP_BEGIN

namespace node {
#ifndef SP_RESTRICT
	bool isTouched(cocos2d::Node *node, const cocos2d::Vec2 &location, float padding) {
		const cocos2d::Vec2 &point = node->convertToNodeSpace(location);
		if (point.x > -padding && point.y > -padding
				&& point.x < node->getContentSize().width + padding
				&& point.y < node->getContentSize().height + padding) {
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
#else
	bool isTouched(cocos2d::Node *node, const cocos2d::Vec2 &location, float padding) { return false; }

	bool isParent(cocos2d::Node *parent, cocos2d::Node *node) { return false; }
	Mat4 chainNodeToParent(cocos2d::Node *parent, cocos2d::Node *node, bool all) { return Mat4::IDENTITY; }
	Mat4 chainParentToNode(cocos2d::Node *parent, cocos2d::Node *node, bool all) { return Mat4::IDENTITY; }
#endif
}

NS_SP_END
