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

#include "SPOverscroll.h"
#include "SPRoundedSprite.h"

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
	updateIndicatorPosition();
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

	float scrollWidth = _contentSize.width;
	float scrollHeight = _contentSize.height;

	float scrollLength = getScrollLength();
	if (isnan(scrollLength)) {
		_indicator->setVisible(false);
	} else {
		_indicator->setVisible(_indicatorVisible);
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
			float h = (scrollHeight - 4 - paddingLocal.top - paddingLocal.bottom) * scrollHeight / scrollLength;
			if (h < 20) {
				h = 20;
			}
			float r = scrollHeight - h - 4 - paddingLocal.top - paddingLocal.bottom;
			float value = (_scrollPosition - getScrollMinPosition()) / (getScrollMaxPosition() - getScrollMinPosition());

			_indicator->setContentSize(Size(3, h));
			_indicator->setPosition(Vec2(scrollWidth - 2, paddingLocal.bottom + 2 + r * (1.0f - value)));
			_indicator->setAnchorPoint(Vec2(1, 0));
		} else {
			float h = (scrollWidth - 4 - paddingLocal.left - paddingLocal.right) * scrollWidth / scrollLength;
			if (h < 20) {
				h = 20;
			}
			float r = scrollWidth - h - 4 - paddingLocal.left - paddingLocal.right;
			float value = (_scrollPosition - getScrollMinPosition()) / (getScrollMaxPosition() - getScrollMinPosition());

			_indicator->setContentSize(Size(h, 3));
			_indicator->setPosition(Vec2(paddingLocal.left + 2 + r * (value), 2));
			_indicator->setAnchorPoint(Vec2(0, 0));
		}
		if (_indicator->getOpacity() != 255) {
			cocos2d::Action *a = _indicator->getActionByTag(19);
			if (!a) {
				a = cocos2d::FadeTo::create(progress(0.1f, 0.0f, _indicator->getOpacity() / 255.0f), 255);
				a->setTag(19);
				_indicator->runAction(a);
			}
		}

		_indicator->stopActionByTag(18);
		auto fade = cocos2d::Sequence::create(cocos2d::DelayTime::create(2.0f), cocos2d::FadeOut::create(0.25f), nullptr);
		fade->setTag(18);
		_indicator->runAction(fade);
	} else {
		_indicator->setVisible(false);
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

NS_SP_END
