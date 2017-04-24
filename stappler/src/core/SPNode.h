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

#ifndef STAPPLER_SRC_CORE_SPNODE_H_
#define STAPPLER_SRC_CORE_SPNODE_H_

#include "SPForward.h"

NS_SP_BEGIN

namespace node {
	struct Params {
		enum Mask {
			None = 0,
			Position = 1 << 0,
			ContentSize = 1 << 1,
			AnchorPoint = 1 << 2,
			Visibility = 1 << 3,
		};

		void setPosition(float x, float y);
		void setPosition(const Vec2 &pos);
		void setAnchorPoint(const Vec2 &pt);
		void setContentSize(const Size &size);
		void setVisible(bool value);
		void apply(cocos2d::Node *) const;

		Mask mask = None;
		Vec2 position;
		Vec2 anchorPoint;
		Size contentSize;
		bool visible = false;
	};

	bool isTouched(cocos2d::Node *node, const Vec2 &point, float padding = 0);
	bool isParent(cocos2d::Node *parent, cocos2d::Node *node);
	Mat4 chainNodeToParent(cocos2d::Node *parent, cocos2d::Node *node, bool full = true);
	Mat4 chainParentToNode(cocos2d::Node *parent, cocos2d::Node *node, bool full = true);
	void dump(cocos2d::Node *node, int depth = -1);
	void apply(cocos2d::Node *node, const Params &);
}

NS_SP_END

#endif /* STAPPLER_SRC_CORE_SPNODE_H_ */
