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
#include "SPScrollView.h"
#include "SPActions.h"

#include "SPOverscroll.h"
#include "SPRoundedSprite.h"
#include "SPScrollController.h"

#include "2d/CCActionInterval.h"

NS_SP_BEGIN

bool ScrollView::init(Layout l) {
	if (!ScrollViewBase::init(l)) {
		return false;
	}

	_indicator = construct<RoundedSprite>(2, screen::density());
	_indicator->setOpacity(0);
	_indicator->setColor(Color3B(127, 127, 127));
	_indicator->setAnchorPoint(Vec2(1, 0));
	addChild(_indicator, 11);

	_overflowFront = construct<Overscroll>();
	addChild(_overflowFront, 12);

	_overflowBack = construct<Overscroll>();
	addChild(_overflowBack, 12);

	//setOverscrollColor(Color3B(0x03, 0xa9, 0xf4));
	setOverscrollColor(Color3B(127, 127, 127));
	setOverscrollVisible(!_bounce);

	return true;
}

void ScrollView::onContentSizeDirty() {
	ScrollViewBase::onContentSizeDirty();
	if (isVertical()) {
		_overflowFront->setAnchorPoint(Vec2(0, 1)); // top
		_overflowFront->setDirection(Overscroll::Direction::Top);
		_overflowFront->setPosition(0, _contentSize.height - _overscrollFrontOffset);
		_overflowFront->setContentSize(Size(_contentSize.width,
				MIN(_contentSize.width * Overscroll::OverscrollScale(), Overscroll::OverscrollMaxHeight())));

		_overflowBack->setAnchorPoint(Vec2(0, 0)); // bottom
		_overflowBack->setDirection(Overscroll::Direction::Bottom);
		_overflowBack->setPosition(0, _overscrollBackOffset);
		_overflowBack->setContentSize(Size(_contentSize.width,
				MIN(_contentSize.width * Overscroll::OverscrollScale(), Overscroll::OverscrollMaxHeight())));

	} else {
		_overflowFront->setAnchorPoint(Vec2(0, 0)); // left
		_overflowFront->setDirection(Overscroll::Direction::Left);
		_overflowFront->setPosition(_overscrollFrontOffset, 0);
		_overflowFront->setContentSize(Size(
				MIN(_contentSize.height * Overscroll::OverscrollScale(), Overscroll::OverscrollMaxHeight()),
				_contentSize.height));

		_overflowBack->setAnchorPoint(Vec2(1, 0)); // right
		_overflowBack->setDirection(Overscroll::Direction::Right);
		_overflowBack->setPosition(_contentSize.width - _overscrollBackOffset, 0);
		_overflowBack->setContentSize(Size(
				MIN(_contentSize.height * Overscroll::OverscrollScale(), Overscroll::OverscrollMaxHeight()),
				_contentSize.height));
	}
    updateIndicatorPosition();
}

void ScrollView::setOverscrollColor(const Color3B &val) {
	_overflowFront->setColor(val);
	_overflowBack->setColor(val);
}
const Color3B & ScrollView::getOverscrollColor() const {
	return _overflowFront->getColor();
}

void ScrollView::setOverscrollOpacity(uint8_t val) {
	_overflowFront->setOverscrollOpacity(val);
	_overflowBack->setOverscrollOpacity(val);
}
uint8_t ScrollView::getOverscrollOpacity() const {
	return _overflowFront->getOverscrollOpacity();
}

void ScrollView::setOverscrollVisible(bool value) {
	_overflowFront->setVisible(value);
	_overflowBack->setVisible(value);
}
bool ScrollView::isOverscrollVisible() const {
	return _overflowFront->isVisible();
}

void ScrollView::setIndicatorColor(const Color3B &val) {
	_indicator->setColor(val);
}
const Color3B & ScrollView::getIndicatorColor() const {
	return _indicator->getColor();
}

void ScrollView::setIndicatorOpacity(uint8_t val) {
	_indicator->setOpacity(val);
}
uint8_t ScrollView::getIndicatorOpacity() const {
	return _indicator->getOpacity();
}

void ScrollView::setIndicatorVisible(bool value) {
	_indicatorVisible = value;
	if (!isnan(getScrollLength())) {
		_indicator->setVisible(value);
	} else {
		_indicator->setVisible(false);
	}
}
bool ScrollView::isIndicatorVisible() const {
	return _indicatorVisible;
}

void ScrollView::onOverscroll(float delta) {
	ScrollViewBase::onOverscroll(delta);
	if (isOverscrollVisible()) {
		if (delta > 0.0f) {
			_overflowBack->incrementProgress(delta / 50.0f);
		} else {
			_overflowFront->incrementProgress(-delta / 50.0f);
		}
	}
}

void ScrollView::onScroll(float delta, bool finished) {
	ScrollViewBase::onScroll(delta, finished);
	if (!finished) {
		updateIndicatorPosition();
	}
}

void ScrollView::onTap(int count, const Vec2 &loc) {
	if (_tapCallback) {
		_tapCallback(count, loc);
	}
}

void ScrollView::onAnimationFinished() {
	ScrollViewBase::onAnimationFinished();
	if (_animationCallback) {
		_animationCallback();
	}
}

void ScrollView::updateIndicatorPosition() {
	if (!_indicatorVisible) {
		return;
	}

	const float scrollWidth = _contentSize.width;
	const float scrollHeight = _contentSize.height;
	const float scrollLength = getScrollLength();

	updateIndicatorPosition(_indicator, (isVertical()?scrollHeight:scrollWidth) / scrollLength,
			(_scrollPosition - getScrollMinPosition()) / (getScrollMaxPosition() - getScrollMinPosition()), true, 20.0f);
}

void ScrollView::updateIndicatorPosition(cocos2d::Node *indicator, float size, float value, bool actions, float min) {
	if (!_indicatorVisible) {
		return;
	}

	float scrollWidth = _contentSize.width;
	float scrollHeight = _contentSize.height;

	float scrollLength = getScrollLength();
	if (isnan(scrollLength)) {
		indicator->setVisible(false);
	} else {
		indicator->setVisible(_indicatorVisible);
	}

	auto paddingLocal = _paddingGlobal;
	if (_indicatorIgnorePadding) {
		if (isVertical()) {
			paddingLocal.top = 0;
			paddingLocal.bottom = 0;
		} else {
			paddingLocal.left = 0;
			paddingLocal.right = 0;
		}
	}

	if (scrollLength > _scrollSize) {
		if (isVertical()) {
			float h = (scrollHeight - 4 - paddingLocal.top - paddingLocal.bottom) * size;
			if (h < min) {
				h = min;
			}
			float r = scrollHeight - h - 4 - paddingLocal.top - paddingLocal.bottom;

			indicator->setContentSize(Size(3, h));
			indicator->setPosition(Vec2(scrollWidth - 2, paddingLocal.bottom + 2 + r * (1.0f - value)));
			indicator->setAnchorPoint(Vec2(1, 0));
		} else {
			float h = (scrollWidth - 4 - paddingLocal.left - paddingLocal.right) * size;
			if (h < min) {
				h = min;
			}
			float r = scrollWidth - h - 4 - paddingLocal.left - paddingLocal.right;

			indicator->setContentSize(Size(h, 3));
			indicator->setPosition(Vec2(paddingLocal.left + 2 + r * (value), 2));
			indicator->setAnchorPoint(Vec2(0, 0));
		}
		if (actions) {
			if (indicator->getOpacity() != 255) {
				cocos2d::Action *a = indicator->getActionByTag(19);
				if (!a) {
					a = cocos2d::FadeTo::create(progress(0.1f, 0.0f, indicator->getOpacity() / 255.0f), 255);
					indicator->runAction(a, 19);
				}
			}

			indicator->stopActionByTag(18);
			auto fade = action::sequence(2.0f, cocos2d::FadeOut::create(0.25f));
			indicator->runAction(fade, 18);
		}
	} else {
		indicator->setVisible(false);
	}
}

void ScrollView::setPadding(const Padding &p) {
	if (p != _paddingGlobal) {
		float offset = (isVertical()?_paddingGlobal.top:_paddingGlobal.left);
		float newOffset = (isVertical()?p.top:p.left);
		ScrollViewBase::setPadding(p);

		if (offset != newOffset) {
			setScrollPosition(getScrollPosition() + (offset - newOffset));
		}
	}
}

void ScrollView::setOverscrollFrontOffset(float value) {
	if (_overscrollFrontOffset != value) {
		_overscrollFrontOffset = value;
		_contentSizeDirty = true;
	}
}
float ScrollView::getOverscrollFrontOffset() const {
	return _overscrollFrontOffset;
}

void ScrollView::setOverscrollBackOffset(float value) {
	if (_overscrollBackOffset != value) {
		_overscrollBackOffset = value;
		_contentSizeDirty = true;
	}
}
float ScrollView::getOverscrollBackOffset() const {
	return _overscrollBackOffset;
}

void ScrollView::setIndicatorIgnorePadding(bool value) {
	if (_indicatorIgnorePadding != value) {
		_indicatorIgnorePadding = value;
	}
}
bool ScrollView::isIndicatorIgnorePadding() const {
	return _indicatorIgnorePadding;
}

void ScrollView::setTapCallback(const TapCallback &cb) {
	_tapCallback = cb;
}

const ScrollView::TapCallback &ScrollView::getTapCallback() const {
	return _tapCallback;
}

void ScrollView::setAnimationCallback(const AnimationCallback &cb) {
	_animationCallback = cb;
}

const ScrollView::AnimationCallback &ScrollView::getAnimationCallback() const {
	return _animationCallback;
}

void ScrollView::update(float dt) {
	auto newpos = getScrollPosition();
	auto factor = std::min(64.0f, _adjustValue);

	switch (_adjust) {
	case Adjust::Front:
		newpos += (45.0f + progress(0.0f, 200.0f, factor / 32.0f)) * dt;
		break;
	case Adjust::Back:
		newpos -= (45.0f + progress(0.0f, 200.0f, factor / 32.0f)) * dt;
		break;
	default:
		break;
	}

	if (newpos != getScrollPosition()) {
		if (newpos < getScrollMinPosition()) {
			newpos = getScrollMinPosition();
		} else if (newpos > getScrollMaxPosition()) {
			newpos = getScrollMaxPosition();
		}
		_root->stopAllActionsByTag("ScrollViewAdjust"_tag);
		setScrollPosition(newpos);
	}
}

void ScrollView::runAdjust(float pos, float factor) {
	auto scrollPos = getScrollPosition();
	auto scrollSize = getScrollSize();

	float newPos = nan();
	if (scrollSize < 64.0f +  48.0f) {
		newPos = ((pos - 64.0f) + (pos - scrollSize + 48.0f)) / 2.0f;
	} else if (pos < scrollPos + 64.0f) {
		newPos = pos - 64.0f;
	} else if (pos > scrollPos + scrollSize - 48.0f) {
		newPos = pos - scrollSize + 48.0f;
	}

	if (!isnan(newPos)) {
		if (newPos < getScrollMinPosition()) {
			newPos = getScrollMinPosition();
		} else if (newPos > getScrollMaxPosition()) {
			newPos = getScrollMaxPosition();
		}
		if (_adjustValue != newPos) {
			_adjustValue = newPos;
			auto dist = fabsf(newPos - scrollPos);

			auto t = 0.15f;
			if (dist < 20.0f) {
				t = 0.15f;
			} else if (dist > 220.0f) {
				t = 0.45f;
			} else {
				t = progress(0.15f, 0.45f, (dist - 20.0f) / 200.0f);
			}
			_root->stopAllActionsByTag("ScrollViewAdjust"_tag);
			auto a = action::sequence(cocos2d::EaseQuadraticActionInOut::create(cocos2d::MoveTo::create(t,
					isVertical()?Vec2(_root->getPositionX(), newPos + _scrollSize):Vec2(-newPos, _root->getPositionY()))),
					[this] { _adjustValue = nan(); });
			_root->runAction(a, "ScrollViewAdjust"_tag);
		}
	}
}

void ScrollView::scheduleAdjust(Adjust a, float val) {
	_adjustValue = val;
	if (a != _adjust) {
		_adjust = a;
		switch (_adjust) {
		case Adjust::None:
			unscheduleUpdate();
			_adjustValue = nan();
			break;
		default:
			scheduleUpdate();
			break;
		}
	}
}

data::Value ScrollView::save() const {
	data::Value ret;
	ret.setDouble(getScrollRelativePosition(), "value");
	return ret;
}

void ScrollView::load(const data::Value &d) {
	if (d.isDictionary()) {
		_savedRelativePosition = d.getDouble("value");
		if (_controller) {
			_controller->onScrollPosition(true);
		}
	}
}

NS_SP_END
