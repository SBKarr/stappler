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
#include "SPStrictNode.h"
#include "SPDevice.h"
#include "SPScreen.h"
#include "platform/CCGLView.h"
#include "base/CCDirector.h"
#include "platform/CCGLView.h"
#include "renderer/CCRenderer.h"
#include "2d/CCComponentContainer.h"

USING_NS_CC;

namespace stappler {

bool StrictNode::isClippingEnabled() {
	return _isClippingEnabled;
}

void StrictNode::setClippingEnabled(bool value) {
	_isClippingEnabled = value;
}

void StrictNode::setClippingOutline(float top, float left, float bottom, float right) {
	_outlineTop = top;
	_outlineLeft = left;
	_outlineBottom = bottom;
	_outlineRight = right;
}
void StrictNode::setClippingOutlineTop(float value) {
	_outlineTop = value;
}
void StrictNode::setClippingOutlineLeft(float value) {
	_outlineLeft = value;
}
void StrictNode::setClippingOutlineBottom(float value) {
	_outlineBottom = value;
}
void StrictNode::setClippingOutlineRight(float value) {
	_outlineRight = value;
}

void StrictNode::setClipOnlyNodesBelow(bool value) {
	_clipOnlyNodesBelow = value;
}

void StrictNode::onBeforeDraw() {
	_scissorRestored = false;
	Rect frame = getViewRect();
	auto view = Director::getInstance()->getOpenGLView();
	if (view->isScissorEnabled()) {
		_scissorRestored = true;
		_parentScissorRect = view->getScissorRect();
            //set the intersection of m_tParentScissorRect and frame as the new scissor rect
		if (frame.intersectsRect(_parentScissorRect)) {
			float x = MAX(frame.origin.x, _parentScissorRect.origin.x);
			float y = MAX(frame.origin.y, _parentScissorRect.origin.y);
			float xx = MIN(frame.origin.x + frame.size.width,
						   _parentScissorRect.origin.x + _parentScissorRect.size.width);
			float yy = MIN(frame.origin.y + frame.size.height,
						   _parentScissorRect.origin.y + _parentScissorRect.size.height);
			view->setScissorInPoints(x, y, xx-x, yy-y);
		}
	} else {
		glEnable(GL_SCISSOR_TEST);
		view->setScissorInPoints(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
	}
}

void StrictNode::onAfterDraw() {
	if (_scissorRestored) {//restore the parent's scissor rect
		auto view = Director::getInstance()->getOpenGLView();
		view->setScissorInPoints(_parentScissorRect.origin.x, _parentScissorRect.origin.y, _parentScissorRect.size.width, _parentScissorRect.size.height);
	} else {
		glDisable(GL_SCISSOR_TEST);
	}

	_scissorRestored = false;
}

bool StrictNode::init() {
	if (!Node::init()) {
		return false;
	}
	_isClippingEnabled = true;
	setCascadeOpacityEnabled(true);
	setClippingOutline(0, 0, 0, 0);
	return true;
}

void StrictNode::visit(cocos2d::Renderer *renderer, const cocos2d::Mat4 &parentTransform, uint32_t parentFlags, ZPath &zPath) {
	// quick return if not visible. children won't be drawn.
	if (!_isClippingEnabled || !Device::getInstance()->isScissorAvailable() || Screen::getInstance()->isInTransition()) {
		Node::visit(renderer, parentTransform, parentFlags, zPath);
		return;
	}

    if (!_visible) {
        return;
    }

    uint32_t flags = processParentFlags(parentTransform, parentFlags);

    Director* director = Director::getInstance();
    director->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
    director->loadMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, _modelViewTransform);

    int i = 0;

    zPath.push_back(getLocalZOrder());
    if (_componentContainer) {
    	_componentContainer->onVisit(renderer, _modelViewTransform, flags, zPath);
    }

    if(!_children.empty()) {
		_beforeVisitCmd.init(_globalZOrder, zPath);
		_beforeVisitCmd.func = CC_CALLBACK_0(StrictNode::onBeforeDraw, this);
		renderer->addCommand(&_beforeVisitCmd);

		_groupCommand.init(_globalZOrder, zPath);
		renderer->pushGroup(_groupCommand.getRenderQueueID());

        sortAllChildren();
        // draw children zOrder < 0
        for( ; i < _children.size(); i++ ) {
            auto node = _children.at(i);
            if ( node && node->getLocalZOrder() < 0 ) {
				node->visit(renderer, _modelViewTransform, flags, zPath);
            } else {
				break;
			}
        }

		if (_clipOnlyNodesBelow) {
			renderer->popGroup();
			renderer->addCommand(&_groupCommand);

			_afterVisitCmd.init(_globalZOrder, zPath);
			_afterVisitCmd.func = CC_CALLBACK_0(StrictNode::onAfterDraw, this);
			renderer->addCommand(&_afterVisitCmd);
		}
        // self draw
        this->draw(renderer, _modelViewTransform, flags, zPath);

        for(auto it=_children.cbegin()+i; it != _children.cend(); ++it) {
			(*it)->visit(renderer, _modelViewTransform, flags, zPath);
		}

		if (!_clipOnlyNodesBelow) {
			renderer->popGroup();
			renderer->addCommand(&_groupCommand);

			_afterVisitCmd.init(_globalZOrder, zPath);
			_afterVisitCmd.func = CC_CALLBACK_0(StrictNode::onAfterDraw, this);
			renderer->addCommand(&_afterVisitCmd);
		}
    } else {
        this->draw(renderer, _modelViewTransform, flags, zPath);
    }
    zPath.pop_back();

    director->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
}

Rect StrictNode::getViewRect() {
    Vec2 bottomLeft = convertToWorldSpace(Vec2(-_outlineLeft, -_outlineBottom));
	Vec2 topRight = convertToWorldSpace(Vec2(_contentSize.width + _outlineRight, _contentSize.height + _outlineTop));

	if (bottomLeft.x > topRight.x) {
		float b = topRight.x;
		topRight.x = bottomLeft.x;
		bottomLeft.x = b;
	}

	if (bottomLeft.y > topRight.y) {
		float b = topRight.y;
		topRight.y = bottomLeft.y;
		bottomLeft.y = b;
	}

	return Rect(bottomLeft.x, bottomLeft.y, topRight.x - bottomLeft.x, topRight.y - bottomLeft.y);
}

bool AlwaysDrawedNode::init() {
	if (!cocos2d::Node::init()) {
		return false;
	}

	setCascadeColorEnabled(true);
	setCascadeOpacityEnabled(true);

	return true;
}

void AlwaysDrawedNode::visit(cocos2d::Renderer *renderer, const cocos2d::Mat4 &parentTransform, uint32_t parentFlags, ZPath &zPath) {
    // quick return if not visible. children won't be drawn.
    if (!_visible) {
        return;
    }

    zPath.push_back(getLocalZOrder());
	_beforeVisitCmd.init(_globalZOrder, zPath);
	_groupCommand.init(_globalZOrder, zPath);
	_afterVisitCmd.init(_globalZOrder, zPath);
	zPath.pop_back();

	_beforeVisitCmd.func = CC_CALLBACK_0(AlwaysDrawedNode::onBeforeDraw, this);
	renderer->addCommand(&_beforeVisitCmd);
	renderer->pushGroup(_groupCommand.getRenderQueueID());

	Node::visit(renderer, parentTransform, parentFlags, zPath);

	renderer->popGroup();
	renderer->addCommand(&_groupCommand);

	_afterVisitCmd.func = CC_CALLBACK_0(AlwaysDrawedNode::onAfterDraw, this);
	renderer->addCommand(&_afterVisitCmd);
}

void AlwaysDrawedNode::onBeforeDraw() {
	auto view = Director::getInstance()->getOpenGLView();
	_scissorEnabled = view->isScissorEnabled();
	if (_scissorEnabled) {
		_scissorRect = view->getScissorRect();
		glDisable(GL_SCISSOR_TEST);
	}
}

void AlwaysDrawedNode::onAfterDraw() {
	auto view = Director::getInstance()->getOpenGLView();
	if (_scissorEnabled) {
		glEnable(GL_SCISSOR_TEST);
		view->setScissorInPoints(_scissorRect.origin.x, _scissorRect.origin.y, _scissorRect.size.width, _scissorRect.size.height);
	}
	_scissorEnabled = false;
}

}

