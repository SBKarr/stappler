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

#include "Material.h"
#include "MaterialFlexibleLayout.h"

#include "SPScrollView.h"
#include "SPEventListener.h"
#include "SPDevice.h"
#include "SPLayer.h"
#include "SPScreen.h"
#include "SPActions.h"

NS_MD_BEGIN

bool FlexibleLayout::init() {
	if (!cocos2d::Node::init()) {
		return false;
	}

	auto el = Rc<EventListener>::create();
	el->onEvent(Device::onStatusBar, [this] (const Event *val) {
		auto h = val->getFloatValue();
		onStatusBarHeight(h);
	});
	addComponent(el);

	setCascadeOpacityEnabled(true);

	_statusBar = construct<stappler::Layer>(Color::Grey_500);
	_statusBar->setAnchorPoint(cocos2d::Vec2(0, 1));
	_statusBar->setContentSize(cocos2d::Size(0, getStatusBarHeight()));
	_statusBar->setVisible(false);
	addChild(_statusBar, 5);

	return true;
}
void FlexibleLayout::onContentSizeDirty() {
	Layout::onContentSizeDirty();
	if (_flexibleHeightFunction) {
		auto ret = _flexibleHeightFunction();
		_flexibleMinHeight = ret.first;
		_flexibleMaxHeight = ret.second;
	}

	stappler::Padding padding;
	if (_baseNode) {
		padding = _baseNode->getPadding();
	}

	auto size = _contentSize;

	float statusBar = getStatusBarHeight();
	if (_statusBarTracked && statusBar > 0.0f) {
		_statusBar->setVisible(true);
		_statusBar->setPosition(cocos2d::Vec2(0, _contentSize.height));
		_statusBar->setContentSize(cocos2d::Size(_contentSize.width, statusBar));
	} else {
		_statusBar->setVisible(false);
	}

	float flexSize = _flexibleMinHeight + (_flexibleMaxHeight + (_statusBarTracked?statusBar:0) - _flexibleMinHeight) * _flexibleLevel;

	if (flexSize >= _flexibleMaxHeight && _statusBarTracked) {
		statusBar = (flexSize - _flexibleMaxHeight);
		_statusBar->setContentSize(cocos2d::Size(_contentSize.width, statusBar));
		size.height -= statusBar;
		flexSize = _flexibleMaxHeight;
	} else {
		_statusBar->setVisible(false);
	}

	_flexibleNode->ignoreAnchorPointForPosition(false);
	_flexibleNode->setPosition(0, size.height);
	_flexibleNode->setAnchorPoint(cocos2d::Vec2(0, 1));
	_flexibleNode->setContentSize(cocos2d::Size(size.width, flexSize));

	if (flexSize <= 0.0f) {
		_flexibleNode->setVisible(false);
	} else {
		_flexibleNode->setVisible(true);
	}

	if (_baseNode) {
		_baseNode->setAnchorPoint(cocos2d::Vec2(0, 0));
		_baseNode->setPosition(0, 0);
		_baseNode->setContentSize(_contentSize);
		if (_baseNode->isVertical()) {
			_baseNode->setPadding(padding.setTop(getCurrentFlexibleMax() + _baseNodePadding));
			_baseNode->setOverscrollFrontOffset(getCurrentFlexibleHeight());
		}
	}
}

void FlexibleLayout::setBaseNode(stappler::ScrollView *node, int zOrder) {
	if (node != _baseNode) {
		if (_baseNode) {
			_baseNode->removeFromParent();
			_baseNode = nullptr;
		}
		_baseNode = node;
		if (_baseNode) {
			_baseNode->setScrollCallback(std::bind(&FlexibleLayout::onScroll, this,
					std::placeholders::_1, std::placeholders::_2));
			if (_baseNode->isVertical()) {
				_baseNode->setOverscrollFrontOffset(getCurrentFlexibleHeight());
			}
			if (!_baseNode->getParent()) {
				addChild(_baseNode, zOrder);
			}
		}
		_contentSizeDirty = true;
	}
}
void FlexibleLayout::setFlexibleNode(cocos2d::Node *node, int zOrder) {
	if (node != _flexibleNode) {
		if (_flexibleNode) {
			_flexibleNode->removeFromParent();
			_flexibleNode = nullptr;
		}
		_flexibleNode = node;
		if (_flexibleNode) {
			addChild(_flexibleNode, zOrder);
		}
		_contentSizeDirty = true;
	}
}

void FlexibleLayout::setFlexibleAutoComplete(bool value) {
	if (_flexibleAutoComplete != value) {
		_flexibleAutoComplete = value;
	}
}

void FlexibleLayout::setFlexibleMinHeight(float height) {
	if (_flexibleMinHeight != height) {
		_flexibleMinHeight = height;
		_contentSizeDirty = true;
	}
}

void FlexibleLayout::setFlexibleMaxHeight(float height) {
	if (_flexibleMaxHeight != height) {
		_flexibleMaxHeight = height;
		_contentSizeDirty = true;
	}
}

float FlexibleLayout::getFlexibleMinHeight() const {
	return _flexibleMinHeight;
}

float FlexibleLayout::getFlexibleMaxHeight() const {
	return _flexibleMaxHeight;
}

void FlexibleLayout::setFlexibleHeightFunction(const HeightFunction &cb) {
	_flexibleHeightFunction = cb;
	if (cb) {
		auto ret = cb();
		_flexibleMinHeight = ret.first;
		_flexibleMaxHeight = ret.second;
		_contentSizeDirty = true;
		_flexibleLevel = 1.0f;
	}
}

const FlexibleLayout::HeightFunction &FlexibleLayout::getFlexibleHeightFunction() const {
	return _flexibleHeightFunction;
}

void FlexibleLayout::onScroll(float delta, bool finished) {
	auto size = _baseNode->getScrollableAreaSize();
	if (!isnan(size) && size < _contentSize.height) {
		setFlexibleLevel(1.0f);
		return;
	}

	if (!finished && delta != 0.0f) {
		auto distanceFromStart = _baseNode->getDistanceFromStart();
		if (isnan(distanceFromStart) || distanceFromStart > (_flexibleMaxHeight - _flexibleMinHeight) || delta < 0) {
			stopActionByTag(FlexibleLayout::AutoCompleteTag());
			float height = getCurrentFlexibleHeight();
			//float diff = delta.y / _baseNode->getGlobalScale().y;
			float newHeight = height - delta;
			if (delta < 0) {
				float max = getCurrentFlexibleMax();
				if (newHeight > max) {
					newHeight = max;
				}
			} else {
				if (newHeight < _flexibleMinHeight) {
					newHeight = _flexibleMinHeight;
				}
			}
			setFlexibleHeight(newHeight);
		}
	} else if (finished) {
		if (_flexibleAutoComplete) {
			if (_flexibleLevel < 1.0f && _flexibleLevel > 0.0f) {
				auto distanceFromStart = _baseNode->getDistanceFromStart();
				bool open =  (_flexibleLevel > 0.5) || (!isnan(distanceFromStart) && distanceFromStart < (_flexibleMaxHeight - _flexibleMinHeight));
				auto a = construct<ProgressAction>(progress(0.0f, 0.3f, open?_flexibleLevel:(1.0f - _flexibleLevel)), open?1.0f:0.0f,
						[this] (ProgressAction *a, float p) {
					setFlexibleLevel(p);
				});
				a->setSourceProgress(_flexibleLevel);
				a->setTag(FlexibleLayout::AutoCompleteTag());
				if (open) {
					runAction(cocos2d::EaseQuadraticActionOut::create(a));
				} else {
					runAction(cocos2d::EaseQuadraticActionIn::create(a));
				}
			}
		}
	}
}

void FlexibleLayout::setStatusBarTracked(bool value) {
	if (value != _statusBarTracked) {
		_statusBarTracked = value;
		_contentSizeDirty = true;
	}
}
bool FlexibleLayout::isStatusBarTracked() const {
	return _statusBarTracked;
}

void FlexibleLayout::setStatusBarBackgroundColor(const Color &c) {
	_statusBar->setColor(c);
	if (c.text() == Color::Black) {
		stappler::Screen::getInstance()->setStatusBarColor(stappler::Screen::StatusBarColor::Black);
	} else {
		stappler::Screen::getInstance()->setStatusBarColor(stappler::Screen::StatusBarColor::Light);
	}
}
const cocos2d::Color3B &FlexibleLayout::getStatusBarBackgroundColor() const {
	return _statusBar->getColor();
}

float FlexibleLayout::getFlexibleLevel() const {
	return _flexibleLevel;
}

void FlexibleLayout::setFlexibleLevel(float value) {
	if (value > 1.0f) {
		value = 1.0f;
	} else if (value < 0.0f) {
		value = 0.0f;
	}

	if (value == _flexibleLevel) {
		return;
	}

	float statusBar = getStatusBarHeight();
	float flexSize = _flexibleMinHeight + (_flexibleMaxHeight + (_statusBarTracked?statusBar:0) - _flexibleMinHeight) * value;

	auto size = _contentSize;
	if (flexSize >= _flexibleMaxHeight && _statusBarTracked) {
		statusBar = (flexSize - _flexibleMaxHeight);
		_statusBar->setContentSize(cocos2d::Size(_contentSize.width, statusBar));
		size.height -= statusBar;
		flexSize = _flexibleMaxHeight;
		_statusBar->setVisible(true);
	} else {
		_statusBar->setVisible(false);
	}

	_flexibleNode->setPosition(0, size.height);
	_flexibleNode->setContentSize(cocos2d::Size(size.width, flexSize));

	_flexibleLevel = value;
	if (_baseNode && _baseNode->isVertical()) {
		_baseNode->setOverscrollFrontOffset(getCurrentFlexibleHeight());
	}

	if (flexSize == 0.0f) {
		_flexibleNode->setVisible(false);
	} else {
		_flexibleNode->setVisible(true);
	}

	if (_statusBarTracked) {
		if (_flexibleLevel == 1.0f) {
			stappler::Screen::getInstance()->setStatusBarEnabled(true, true);
		} else {
			stappler::Screen::getInstance()->setStatusBarEnabled(false, true);
		}
	}
}

void FlexibleLayout::setFlexibleLevelAnimated(float value, float duration) {
	stopActionByTag("FlexibleLevel"_tag);
	if (duration <= 0.0f) {
		setFlexibleLevel(value);
	} else {
		auto a = cocos2d::EaseCubicActionOut::create(construct<ProgressAction>(
				duration, _flexibleLevel, value,
				[this] (ProgressAction *a, float progress) {
			setFlexibleLevel(progress);
		}));
		a->setTag("FlexibleLevel"_tag);
		runAction(a);
	}
}

void FlexibleLayout::setFlexibleHeight(float height) {
	float size = getCurrentFlexibleMax() - _flexibleMinHeight;
	if (size > 0.0f) {
		float value = (height - _flexibleMinHeight) / (getCurrentFlexibleMax() - _flexibleMinHeight);
		setFlexibleLevel(value);
	} else {
		setFlexibleLevel(1.0f);
	}
}

void FlexibleLayout::setBaseNodePadding(float val) {
	if (_baseNodePadding != val) {
		_baseNodePadding = val;
		_contentSizeDirty = true;
	}
}
float FlexibleLayout::getBaseNodePadding() const {
	return _baseNodePadding;
}

float FlexibleLayout::getCurrentFlexibleHeight() const {
	return (getCurrentFlexibleMax() - _flexibleMinHeight) * _flexibleLevel + _flexibleMinHeight;
}
float FlexibleLayout::getCurrentFlexibleMax() const {
	return _flexibleMaxHeight + (_statusBarTracked?getStatusBarHeight():0);
}
void FlexibleLayout::onPush(ContentLayer *l, bool replace) {
	Layout::onPush(l, replace);

	if (_statusBarTracked) {
		auto screen = Screen::getInstance();
		if (screen->isStatusBarEnabled() && _flexibleLevel < 1.0f) {
			stappler::Screen::getInstance()->setStatusBarEnabled(false, true);
		} else if (!screen->isStatusBarEnabled() && _flexibleLevel >= 1.0f) {
			stappler::Screen::getInstance()->setStatusBarEnabled(true, true);
		}
	}
}
void FlexibleLayout::onForegroundTransitionBegan(ContentLayer *l, Layout *overlay) {
	Layout::onForegroundTransitionBegan(l, overlay);

	Color c(_statusBar->getColor());
	if (c.text() == Color::Black) {
		stappler::Screen::getInstance()->setStatusBarColor(stappler::Screen::StatusBarColor::Black);
	} else {
		stappler::Screen::getInstance()->setStatusBarColor(stappler::Screen::StatusBarColor::Light);
	}

	if (_statusBarTracked) {
		auto screen = Screen::getInstance();
		if (screen->isStatusBarEnabled() && _flexibleLevel < 1.0f) {
			stappler::Screen::getInstance()->setStatusBarEnabled(false, true);
		} else if (!screen->isStatusBarEnabled() && _flexibleLevel >= 1.0f) {
			stappler::Screen::getInstance()->setStatusBarEnabled(true, true);
		}
	}
}

float FlexibleLayout::getStatusBarHeight() const {
	if (isnan(_statusBarHeight)) {
		return screen::statusBarHeight();
	}
	return _statusBarHeight;
}

void FlexibleLayout::onStatusBarHeight(float val) {
	stopActionByTag("onStatusBarHeight"_tag);
	if (_statusBarTracked) {
		float origHeight = screen::statusBarHeight();
		float newHeight = val / screen::density();

		runAction(Rc<ProgressAction>::create(0.15, [this, origHeight, newHeight] (ProgressAction *a, float time) {
			_statusBarHeight = progress(origHeight, newHeight, time);
			_contentSizeDirty = true;
		}, [this] (ProgressAction *a) {
			_statusBarHeight = nan();
			_contentSizeDirty = true;
		}), "onStatusBarHeight"_tag);
	} else {
		_statusBarHeight = nan();
	}
}

NS_MD_END
