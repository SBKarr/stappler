/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SLFloatContext.h"

NS_LAYOUT_BEGIN

Pair<float, float> FloatContext::getAvailablePosition(float yPos, float height) const {
	float x = root->pos.position.x;
	float width = root->pos.size.width;

	float topStackWidth = 0;
	for (auto &stack : floatLeft) {
		if (stack.origin.y <= yPos + height && stack.origin.y + stack.size.height >= yPos) {
			if (topStackWidth < stack.origin.x + stack.size.width) {
				x = stack.origin.x + stack.size.width;
				width = root->pos.size.width - x;
				topStackWidth = stack.origin.x + stack.size.width;
			}
		}
	}

	topStackWidth = 0;
	for (auto &stack : floatRight) {
		if (stack.origin.y <= yPos + height && stack.origin.y + stack.size.height >= yPos) {
			if (topStackWidth < stack.size.width) {
				topStackWidth = root->pos.size.width - stack.origin.x;
			}
		}
	}
	width -= topStackWidth;
	return pair(x, width);
}

bool FloatContext::pushFloatingNode(Layout &origin, Layout &l, Vec2 &vec) {
	auto &stacks = (l.node.block.floating==style::Float::Left)?floatLeft:floatRight;
	auto bbox = l.getBoundingBox();

	if (l.node.block.clear == style::Clear::Both
			|| (l.node.block.floating == style::Float::Left && l.node.block.clear == style::Clear::Left)
			|| (l.node.block.floating == style::Float::Right && l.node.block.clear == style::Clear::Right) ) {
		return pushFloatingNodeToNewStack(origin, l, stacks, bbox, vec);
	}

	// can we use current stack?
	if (!stacks.empty()) {
		auto &stack = stacks.back();
		if (root->pos.size.width - stack.size.width > bbox.size.width) {
			if (pushFloatingNodeToStack(origin, l, stack, bbox, vec)) {
				return true;
			}
		}
	}
	return pushFloatingNodeToNewStack(origin, l, stacks, bbox, vec);
}

bool FloatContext::pushFloatingNodeToStack(Layout &origin, Layout &l, Rect &s, const Rect &bbox, Vec2 &vec) {
	auto &stacks = (l.node.block.floating==style::Float::Left)?floatRight:floatLeft;

	float newHeight = std::max(s.size.height, bbox.size.height);
	float oppositeWidth = 0;

	for (auto &stack : stacks) {
		if (stack.origin.y <= s.origin.y + newHeight && stack.origin.y + stack.size.height >= s.origin.y) {
			if (oppositeWidth < stack.size.width) {
				oppositeWidth = stack.size.width;
			}
		}
	}

	if (root->pos.size.width - s.size.width - oppositeWidth > bbox.size.width) {
		s.size.height = newHeight;
		l.setBoundPosition(Vec2(
				(l.node.block.floating==style::Float::Left)?(s.origin.x + s.size.width):(s.origin.x - s.size.width)
						, s.origin.y));
		s.size.width += bbox.size.width;

		if (fabsf(root->pos.size.width - s.size.width) < root->pos.size.width * 0.025) {
			vec.y = s.origin.y + s.size.height;
		}

		return true;
	}
	return false;
}

bool FloatContext::pushFloatingNodeToNewStack(Layout &lo, Layout &l, FloatStack &stacks, const Rect &bbox, Vec2 &vec) {
	auto &oppositeStacks = (l.node.block.floating==style::Float::Left)?floatRight:floatLeft;

	Vec2 origin = Vec2(
			(l.node.block.floating==style::Float::Left)?(lo.pos.position.x):(lo.pos.position.x + lo.pos.size.width - bbox.size.width),
			bbox.origin.y);

	Size bsize = bbox.size;

	for (auto &stack : stacks) {
		if (origin.y < stack.origin.y + stack.size.height) {
			origin.y = stack.origin.y + stack.size.height;
		}
	}

	bool oppositeClear = false;
	if (l.node.block.clear == style::Clear::Both
			|| (l.node.block.floating == style::Float::Left && l.node.block.clear == style::Clear::Right)
			|| (l.node.block.floating == style::Float::Right && l.node.block.clear == style::Clear::Left) ) {
		oppositeClear = true;
	}

	for (auto &stack : oppositeStacks) {
		if (stack.origin.y <= origin.y && stack.origin.y + stack.size.height >= origin.y) {
			if (oppositeClear || root->pos.size.width - stack.size.width < bsize.width) {
				origin.y = stack.origin.y + stack.size.height;
			}
		}
	}

	if (!isnan(pageHeight)) {
		uint32_t curr1 = (uint32_t)std::floor(origin.y / pageHeight);
		uint32_t curr2 = (uint32_t)std::floor((origin.y + bsize.height) / pageHeight);
		if (curr1 != curr2) {
			origin.y = curr2 * pageHeight;
		}
	}

	stacks.push_back(Rect(origin.x, origin.y, bsize.width, bsize.height));
	l.setBoundPosition(origin);

	if (fabsf(root->pos.size.width - bsize.width)  < root->pos.size.width * 0.025) {
		vec.y = origin.y + bsize.height;
	}

	return true;
}

NS_LAYOUT_END
