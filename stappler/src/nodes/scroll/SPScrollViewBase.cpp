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
#include "SPScrollViewBase.h"
#include "SPScrollController.h"

#include "SPGestureListener.h"

#include "SPAccelerated.h"
#include "2d/CCActionInterval.h"
#include "2d/CCActionInstant.h"
#include "2d/CCLayer.h"

NS_SP_BEGIN

bool ScrollViewBase::init(Layout layout) {
	if (!StrictNode::init()) {
		return false;
	}

	_layout = layout;

    auto l = Rc<gesture::Listener>::create();
    l->setTapCallback([this] (gesture::Event ev, const gesture::Tap &s) {
    	onTap(s.count, s.location());
    	return false;
    });
    l->setPressCallback([this] (gesture::Event ev, const gesture::Press &s) -> bool {
    	if (ev == gesture::Event::Began) {
    		return onPressBegin(s.location());
    	} else if (ev == gesture::Event::Activated) {
    		return onLongPress(s.location(), s.time);
    	} else if (ev == gesture::Event::Ended) {
    		return onPressEnd(s.location(), s.time);
    	} else if (ev == gesture::Event::Cancelled) {
    		return onPressCancel(s.location(), s.time);
    	}
    	return false;
    });
    l->setSwipeCallback([this] (gesture::Event ev, const gesture::Swipe &s) -> bool {
    	if (ev == gesture::Event::Began) {
    		auto cs = (_layout == Vertical)?(_contentSize.height):(_contentSize.width);
    		auto length = getScrollLength();
			if (!isnan(length) && cs >= length) {
		        return false;
			}

			if (_layout == Vertical && fabsf(s.delta.y * 2.0f) <= fabsf(s.delta.x)) {
				return false;
			} else if (_layout == Horizontal && fabsf(s.delta.x * 2.0f) <= fabsf(s.delta.y)) {
				return false;
			}

			onSwipeBegin();

    		if (_layout == Vertical) {
        		return onSwipe(s.delta.y / _globalScale.y, s.velocity.y / _globalScale.y, false);
    		} else {
        		return onSwipe(- s.delta.x / _globalScale.x, - s.velocity.x / _globalScale.x, false);
    		}

			return true;
    	} else if (ev == gesture::Event::Activated) {
    		if (_layout == Vertical) {
        		return onSwipe(s.delta.y / _globalScale.y, s.velocity.y / _globalScale.y, false);
    		} else {
        		return onSwipe(- s.delta.x / _globalScale.x, - s.velocity.x / _globalScale.x, false);
    		}
    	} else {
			_movement = Movement::None;
    		if (_layout == Vertical) {
        		return onSwipe(0, s.velocity.y / _globalScale.y, true);
    		} else {
        		return onSwipe(0, - s.velocity.x / _globalScale.x, true);
    		}
    	}
    });
    l->setMouseWheelCallback([this] (gesture::Event ev, const gesture::Wheel &w) -> bool {
    	auto pos = getScrollPosition();
		if (_layout == Vertical) {
			onDelta(w.amount.y * 100.0f / _globalScale.y);
		} else {
			onDelta(- w.amount.x * 100.0f / _globalScale.x);
		}
		onScroll(getScrollPosition() - pos, false);
		return true;
    });
    addComponent(l);
    _listener = l;

    setCascadeOpacityEnabled(true);

    _root = construct<cocos2d::Node>();
    _root->setPosition(Vec2(0, 0));
    _root->setAnchorPoint((_layout == Vertical)?Vec2(0, 1):Vec2(0, 0));
    _root->setCascadeOpacityEnabled(true);
    _root->setOnContentSizeDirtyCallback(std::bind(&ScrollViewBase::onPosition, this));
    _root->setOnTransformDirtyCallback(std::bind(&ScrollViewBase::onPosition, this));
    addChild(_root, 0);

	return true;
}

void ScrollViewBase::setLayout(Layout l) {
	_layout = l;
	_contentSizeDirty = true;
}

void ScrollViewBase::visit(cocos2d::Renderer *r, const Mat4 &t, uint32_t f, ZPath &zPath) {
	if (_scrollDirty) {
		updateScrollBounds();
	}
	if (_animationDirty) {
		fixPosition();
	}
	StrictNode::visit(r, t, f, zPath);
	if (_animationDirty) {
		onPosition();
		onScroll(0, true);
		_animationDirty = false;
	}
}

void ScrollViewBase::onEnter() {
	StrictNode::onEnter();
	onPosition();
}

void ScrollViewBase::onContentSizeDirty() {
	if (isVertical()) {
		auto pos = _root->getPositionY() - _scrollSize;
		_scrollSize = _contentSize.height;
	    _root->setAnchorPoint(Vec2(0, 1));
		_root->setContentSize(Size(_contentSize.width - _paddingGlobal.left - _paddingGlobal.right, 0));
		_root->setPositionY(pos + _scrollSize);
		_root->setPositionX(_paddingGlobal.left);
	} else {
		_scrollSize = _contentSize.width;
	    _root->setAnchorPoint(Vec2(0, 0));
		_root->setContentSize(Size(0, _contentSize.height - _paddingGlobal.top - _paddingGlobal.bottom));
		_root->setPositionY(_paddingGlobal.bottom);
	}

	StrictNode::onContentSizeDirty();
	updateScrollBounds();
	fixPosition();
}

void ScrollViewBase::onTransformDirty() {
	StrictNode::onTransformDirty();
	Vec3 scale;
	getNodeToWorldTransform().getScale(&scale);
	_globalScale = Vec2(scale.x, scale.y);
}

void ScrollViewBase::setEnabled(bool value) {
	_listener->setEnabled(value);
}
bool ScrollViewBase::isEnabled() const {
	return _listener->isEnabled();
}
bool ScrollViewBase::isTouched() const {
	return _movement == Movement::Manual;
}
bool ScrollViewBase::isMoved() const {
	return _movement != Movement::None;
}

void ScrollViewBase::setScrollCallback(const ScrollCallback & cb) {
	_scrollCallback = cb;
}
const ScrollViewBase::ScrollCallback &ScrollViewBase::getScrollCallback() const {
	return _scrollCallback;
}

void ScrollViewBase::setOverscrollCallback(const OverscrollCallback & cb) {
	_overscrollCallback = cb;
}
const ScrollViewBase::OverscrollCallback &ScrollViewBase::getOverscrollCallback() const {
	return _overscrollCallback;
}

cocos2d::Node *ScrollViewBase::getRoot() const {
	return _root;
}
gesture::Listener *ScrollViewBase::getGestureListener() const {
	return _listener;
}

bool ScrollViewBase::addComponent(cocos2d::Component *cmp) {
	if (auto c = dynamic_cast<ScrollController *>(cmp)) {
		setController(c);
		return true;
	} else {
		return StrictNode::addComponent(cmp);
	}
}
void ScrollViewBase::setController(ScrollController *c) {
	if (c != _controller) {
		if (_controller) {
			StrictNode::removeComponent(_controller);
			_controller = nullptr;
		}
		_controller = c;
		if (_controller) {
			StrictNode::addComponent(_controller);
		}
	}
}
ScrollController *ScrollViewBase::getController() {
	return _controller;
}

void ScrollViewBase::setPadding(const Padding &p) {
	if (p != _paddingGlobal) {
		_paddingGlobal = p;
		_contentSizeDirty = true;
	}
}
const Padding &ScrollViewBase::getPadding() const {
	return _paddingGlobal;
}

float ScrollViewBase::getScrollableAreaOffset() const {
	if (_controller) {
		return _controller->getScrollableAreaOffset();
	}
	return std::numeric_limits<float>::quiet_NaN();
}

float ScrollViewBase::getScrollableAreaSize() const {
	if (_controller) {
		return _controller->getScrollableAreaSize();
	}
	return std::numeric_limits<float>::quiet_NaN();
}

Vec2 ScrollViewBase::getPositionForNode(float scrollPos) const {
	if (isVertical()) {
		return Vec2(0.0f, scrollPos);
	} else {
		return Vec2(scrollPos, 0.0f);
	}
}
Size ScrollViewBase::getContentSizeForNode(float size) const {
	if (isVertical()) {
		return Size(nan(), size);
	} else {
		return Size(size, nan());
	}
}
Vec2 ScrollViewBase::getAnchorPointForNode() const {
	if (isVertical()) {
		return Vec2(0, 1.0f);
	} else {
		return Vec2(0, 0);
	}
}

float ScrollViewBase::getNodeScrollSize(const Size &size) const {
	return (isVertical())?(size.height):(size.width);
}

float ScrollViewBase::getNodeScrollPosition(const Vec2 &pos) const {
	return (isVertical())?(pos.y):(pos.x);
}

bool ScrollViewBase::addScrollNode(cocos2d::Node *node, const Vec2 & pos, const Size & size, int z) {
	updateScrollNode(node, pos, size, z);
	if (z) {
		_root->addChild(node, z);
	} else {
		_root->addChild(node);
	}
	return true;
}

void ScrollViewBase::updateScrollNode(cocos2d::Node *node, const Vec2 & pos, const Size & size, int z) {
	auto p = node->getParent();
	if (p == _root || p == nullptr) {
		auto cs = Size(isnan(size.width)?_root->getContentSize().width:size.width,
				isnan(size.height)?_root->getContentSize().height:size.height);

		node->setContentSize(cs);
		node->setPosition((isVertical()?Vec2(pos.x,-pos.y):pos));
		node->setAnchorPoint(getAnchorPointForNode());
		if (z) {
			node->setLocalZOrder(z);
		}
	}
}

bool ScrollViewBase::removeScrollNode(cocos2d::Node *node) {
	if (node && node->getParent() == _root) {
		node->removeFromParent();
		return true;
	}
	return false;
}

float ScrollViewBase::getDistanceFromStart() const {
	auto min = getScrollMinPosition();
	if (!isnan(min)) {
		return fabsf(getScrollPosition() - min);
	} else {
		return std::numeric_limits<float>::quiet_NaN();
	}
}

void ScrollViewBase::setScrollMaxVelocity(float value) {
	_maxVelocity = value;
}
float ScrollViewBase::getScrollMaxVelocity() const {
	return _maxVelocity;
}


cocos2d::Node * ScrollViewBase::getFrontNode() const {
	if (_controller) {
		return _controller->getFrontNode();
	}
	return nullptr;
}
cocos2d::Node * ScrollViewBase::getBackNode() const {
	if (_controller) {
		return _controller->getBackNode();
	}
	return nullptr;
}

float ScrollViewBase::getScrollMinPosition() const {
	auto pos = getScrollableAreaOffset();
	if (!isnan(pos)) {
		return pos - (isVertical()?_paddingGlobal.top:_paddingGlobal.left);
	}
	if (_controller) {
		float min = _controller->getScrollMin();
		if (!isnan(min)) {
			return min - (isVertical()?_paddingGlobal.top:_paddingGlobal.left);
		}
	}
	return std::numeric_limits<float>::quiet_NaN();
}

float ScrollViewBase::getScrollMaxPosition() const {
	auto pos = getScrollableAreaOffset();
	auto size = getScrollableAreaSize();
	if (!isnan(pos) && !isnan(size)) {
		pos -= (isVertical()?_paddingGlobal.top:_paddingGlobal.left);
		size += (isVertical()?(_paddingGlobal.top + _paddingGlobal.bottom):(_paddingGlobal.left + _paddingGlobal.right));
		if (size > _scrollSize) {
			return pos + size - _scrollSize;
		} else {
			return pos;
		}
	}
	if (_controller) {
		float min = _controller->getScrollMin();
		float max = _controller->getScrollMax();
		if (!isnan(max) && !isnan(min)) {
			return MAX(min, max - _scrollSize + (isVertical()?_paddingGlobal.bottom:_paddingGlobal.right));
		} else if (!isnan(max)) {
			return max - _scrollSize + (isVertical()?_paddingGlobal.bottom:_paddingGlobal.right);
		}
	}

	return std::numeric_limits<float>::quiet_NaN();
}

float ScrollViewBase::getScrollLength() const {
	float size = getScrollableAreaSize();
	if (!isnan(size)) {
		return size + (isVertical()?(_paddingGlobal.top + _paddingGlobal.bottom):(_paddingGlobal.left + _paddingGlobal.right));
	}

	float min = getScrollMinPosition();
	float max = getScrollMaxPosition();

	if (!isnan(min) && !isnan(max)) {
		float trueMax = max - (isVertical()?_paddingGlobal.bottom:_paddingGlobal.right);
		float trueMin = min + (isVertical()?_paddingGlobal.top:_paddingGlobal.left);
		if (trueMax > trueMin) {
			return max  - min + _scrollSize;
		} else {
			return _scrollSize;
		}
	} else {
		return std::numeric_limits<float>::quiet_NaN();
	}
}

float ScrollViewBase::getScrollSize() const {
	return _scrollSize;
}

void ScrollViewBase::setScrollRelativePosition(float value) {
	if (!isnan(value)) {
		if (value < 0.0f) {
			value = 0.0f;
		} else if (value > 1.0f) {
			value = 1.0f;
		}
	} else {
		value = 0.0f;
	}

	float areaSize = getScrollableAreaSize();
	float areaOffset = getScrollableAreaOffset();
	float size = getScrollSize();

	auto &padding = getPadding();
	auto paddingFront = (isVertical())?padding.top:padding.left;
	auto paddingBack = (isVertical())?padding.bottom:padding.right;

	if (!isnan(areaSize) && !isnan(areaOffset) && areaSize > 0) {
		float liveSize = areaSize + paddingFront + paddingBack - size;
		float pos = (value * liveSize) - paddingFront + areaOffset;

		setScrollPosition(pos);
	} else {
		_savedRelativePosition = value;
	}
}

float ScrollViewBase::getScrollRelativePosition() const {
	if (!isnan(_savedRelativePosition)) {
		return _savedRelativePosition;
	}

	return getScrollRelativePosition(getScrollPosition());
}

float ScrollViewBase::getScrollRelativePosition(float pos) const {
	float areaSize = getScrollableAreaSize();
	float areaOffset = getScrollableAreaOffset();
	float size = getScrollSize();

	auto &padding = getPadding();
	auto paddingFront = (isVertical())?padding.top:padding.left;
	auto paddingBack = (isVertical())?padding.bottom:padding.right;

	if (!isnan(areaSize) && !isnan(areaOffset)) {
		float liveSize = areaSize + paddingFront + paddingBack - size;
		return (pos - areaOffset + paddingFront) / liveSize;
	}

	return 0.0f;
}

void ScrollViewBase::setScrollPosition(float pos) {
	if (pos != _scrollPosition) {
		if (isVertical()) {
			_root->setPositionY(pos + _scrollSize);
		} else {
			_root->setPositionX(-pos);
		}
	}
}


float ScrollViewBase::getScrollPosition() const {
	if (isVertical()) {
		return _root->getPositionY() - _scrollSize;
	} else {
		return -_root->getPositionX();
	}
}

Vec2 ScrollViewBase::getPointForScrollPosition(float pos) {
	return (isVertical())?Vec2(_root->getPositionX(), pos + _scrollSize):Vec2(-pos, _root->getPositionY());
}

void ScrollViewBase::onDelta(float delta) {
	if (delta < 0) {
		if (!isnan(_scrollMin) && _scrollPosition + delta < _scrollMin) {
			if (_bounce) {
				float mod = 1.0f / (1.0f + (_scrollMin - (_scrollPosition + delta)) / 5.0f);
				setScrollPosition(_scrollPosition + delta * mod);
				return;
			} else {
				onOverscroll(delta);
				setScrollPosition(_scrollMin);
				return;
			}
		}
	} else if (delta > 0) {
		if (!isnan(_scrollMax) && _scrollPosition + delta > _scrollMax) {
			if (_bounce) {
				float mod = 1.0f / (1.0f + ((_scrollPosition + delta) - _scrollMax) / 5.0f);
				setScrollPosition(_scrollPosition + delta * mod);
				return;
			} else {
				onOverscroll(delta);
				setScrollPosition(_scrollMax);
				return;
			}
		}
	}
	setScrollPosition(_scrollPosition + delta);
}

void ScrollViewBase::onOverscrollPerformed(float velocity, float pos, float boundary) {
	if (_movement == Movement::Auto) {
		if (_movementAction) {
			auto n = (pos < boundary)?1.0f:-1.0f;

			auto vel = _movementAction->getCurrentVelocity();
			auto normal = _movementAction->getNormal();
			if (n * (isVertical()?normal.y:-normal.x) > 0) {
				velocity = vel;
			} else {
				velocity = -vel;
			}
		}
	}

	if (_animationAction) {
		_root->stopAction(_animationAction);
		_animationAction = nullptr;
		_movementAction = nullptr;
	}

	if (_movement == Movement::Manual || _movement == Movement::None) {
		if (!_bounce && pos == boundary) {
			return;
		}
		if ((pos < boundary && velocity < 0) || (pos > boundary && velocity > 0)) {
			velocity = - fabs(velocity);
		} else {
			velocity = fabs(velocity);
		}
	}

	if (_movement != Movement::Overscroll) {
		Vec2 boundaryPos = getPointForScrollPosition(boundary);
		Vec2 currentPos = getPointForScrollPosition(pos);

		auto a = Accelerated::createBounce(5000, currentPos, boundaryPos, velocity, MAX(25000, fabsf(velocity) * 50));
		if (a) {
			auto b = cocos2d::CallFunc::create(std::bind(&ScrollViewBase::onAnimationFinished, this));
			_movement = Movement::Overscroll;
			_animationAction = cocos2d::Sequence::createWithTwoActions(a, b);
			_root->runAction(_animationAction);
		}
	}
}

void ScrollViewBase::onSwipeBegin() {
	_root->stopAllActions();
	_movementAction = nullptr;
	_animationAction = nullptr;
	_movement = Movement::Manual;
}

bool ScrollViewBase::onSwipe(float delta, float velocity, bool ended) {
	if (!ended) {
		if (_scrollFilter) {
			delta = _scrollFilter(delta);
		}
		onDelta(delta);
	} else {
		float pos = getScrollPosition();

		if (!isnan(_scrollMin)) {
			if (pos < _scrollMin) {
				float mod = 1.0f / (1.0f + fabsf(_scrollMin - pos) / 5.0f);
				onOverscrollPerformed(velocity * mod, pos, _scrollMin);
				return true;
			}
		}

		if (!isnan(_scrollMax)) {
			if (pos > _scrollMax) {
				float mod = 1.0f / (1.0f + fabsf(_scrollMax - pos) / 5.0f);
				onOverscrollPerformed(velocity * mod, pos, _scrollMax);
				return true;
			}
		}

		if (auto a = onSwipeFinalizeAction(velocity)) {
			auto b = cocos2d::CallFunc::create(std::bind(&ScrollViewBase::onAnimationFinished, this));
			_movement = Movement::Auto;
			_animationAction = cocos2d::Sequence::createWithTwoActions(a, b);
			_root->runAction(_animationAction);
		} else {
			onScroll(0, true);
		}
	}
	return true;
}

cocos2d::ActionInterval *ScrollViewBase::onSwipeFinalizeAction(float velocity) {
	if (velocity == 0) {
		return nullptr;
	}

	float acceleration = (velocity > 0)?-5000.0f:5000.0f;
	float boundary = (velocity > 0)?_scrollMax:_scrollMin;

	Vec2 normal = (isVertical())
			?(Vec2(0.0f, (velocity > 0)?1.0f:-1.0f))
			:(Vec2((velocity > 0)?-1.0f:1.0f, 0.0f));

	cocos2d::ActionInterval *a = nullptr;

	if (!isnan(_maxVelocity)) {
		if (velocity > fabs(_maxVelocity)) {
			velocity = fabs(_maxVelocity);
		} else if (velocity < -fabs(_maxVelocity)) {
			velocity = -fabs(_maxVelocity);
		}
	}

	if (!isnan(boundary)) {
		float pos = getScrollPosition();
    	float duration = fabsf(velocity / acceleration);
		float path = velocity * duration + acceleration * duration * duration * 0.5f;

		auto from = _root->getPosition();
		auto to = getPointForScrollPosition(boundary);

		float distance = from.distance(to);
		if (distance < 2.0f) {
			setScrollPosition(boundary);
			return nullptr;
		}

		if ((velocity > 0 && pos + path > boundary) || (velocity < 0 && pos + path < boundary)) {
			_movementAction = Accelerated::createAccelerationTo(from, to, fabsf(velocity), -fabsf(acceleration));

			auto overscrollPath = path + ((velocity < 0)?(distance):(-distance));
			if (overscrollPath) {
				auto cb = cocos2d::CallFunc::create(std::bind(&ScrollViewBase::onOverscroll, this, overscrollPath));
				a = cocos2d::Sequence::createWithTwoActions(_movementAction, cb);
			}
		}
	}

	if (!_movementAction) {
		_movementAction = Accelerated::createDecceleration(normal, _root->getPosition(), fabs(velocity), fabsf(acceleration));
	}

	if (!a) {
		a = _movementAction;
	}

	return a;
}

void ScrollViewBase::onAnimationFinished() {
	_animationDirty = true;
	_movement = Movement::None;
	_movementAction = nullptr;
	_animationAction = nullptr;
}

void ScrollViewBase::fixPosition() {
	if (_movement == Movement::None) {
		auto pos = getScrollPosition();
		if (!isnan(_scrollMin)) {
			if (pos < _scrollMin) {
				setScrollPosition(_scrollMin);
				return;
			}
		}

		if (!isnan(_scrollMax)) {
			if (pos > _scrollMax) {
				setScrollPosition(_scrollMax);
				return;
			}
		}
	}
}

void ScrollViewBase::onPosition() {
	float oldPos = _scrollPosition;
	float newPos;
	if (isVertical()) {
		newPos = _root->getPositionY() - _scrollSize;
	} else {
		newPos = -_root->getPositionX();
	}

	_scrollPosition = newPos;

	if (_controller) {
		_controller->onScrollPosition();
	}

	if (_movement == Movement::Auto) {
		if (!isnan(_scrollMin)) {
			if (newPos < _scrollMin) {
				onOverscrollPerformed(0, newPos, _scrollMin);
				return;
			}
		}

		if (!isnan(_scrollMax)) {
			if (newPos > _scrollMax) {
				onOverscrollPerformed(0, newPos, _scrollMax);
				return;
			}
		}
	}

	if (_movement != Movement::None && _movement != Movement::Overscroll && newPos - oldPos != 0) {
		onScroll(newPos - oldPos, false);
	} else if (_movement == Movement::Overscroll) {
		if (!isnan(_scrollMin)) {
			if (_scrollPosition < _scrollMin) {
				if (newPos - oldPos < 0) {
					onOverscroll(newPos - oldPos);
				}
				return;
			}
		}

		if (!isnan(_scrollMax)) {
			if (newPos > _scrollMax) {
				if (newPos - oldPos > 0) {
					onOverscroll(newPos - oldPos);
				}
				return;
			}
		}
	}
}

void ScrollViewBase::updateScrollBounds() {
	if ((isVertical() && _contentSize.width == 0) || (isHorizontal() && _contentSize.height == 0)) {
		return;
	}

	_scrollMin = getScrollMinPosition();
	_scrollMax = getScrollMaxPosition();

	_scrollDirty = false;

	fixPosition();

	_scrollMin = getScrollMinPosition();
	_scrollMax = getScrollMaxPosition();

	if (!isnan(_savedRelativePosition)) {
		float value = _savedRelativePosition;
		_savedRelativePosition = nan();
		setScrollRelativePosition(value);
	}

	_root->setPositionZ(1.0f);
	_root->setPositionZ(0.0f);
}

void ScrollViewBase::onScroll(float delta, bool finished) {
	if (_controller) {
		_controller->onScroll(delta, finished);
	}
	if (_scrollCallback) {
		_scrollCallback(delta, finished);
	}
}

void ScrollViewBase::onOverscroll(float delta) {
	if (_controller) {
		_controller->onOverscroll(delta);
	}
	if (_overscrollCallback) {
		_overscrollCallback(delta);
	}
}

void ScrollViewBase::setScrollDirty(bool value) {
	_scrollDirty = value;
}

bool ScrollViewBase::onPressBegin(const Vec2 &) {
	_root->stopAllActions();
	onAnimationFinished();
	return false;
}
bool ScrollViewBase::onLongPress(const Vec2 &, const TimeInterval &) {
	return true;
}
bool ScrollViewBase::onPressEnd(const Vec2 &, const TimeInterval &) {
	return true;
}
bool ScrollViewBase::onPressCancel(const Vec2 &, const TimeInterval &) {
	return true;
}

void ScrollViewBase::onTap(int count, const Vec2 &loc) {

}

NS_SP_END
