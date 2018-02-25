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
#include "SPEventListener.h"
#include "SPIME.h"

NS_MD_BEGIN

bool FlexibleLayout::init() {
	if (!cocos2d::Node::init()) {
		return false;
	}

	auto el = Rc<EventListener>::create();
	el->onEvent(Device::onStatusBar, [this] (const Event &val) {
		auto h = val.getFloatValue();
		onStatusBarHeight(h);
	});
	addComponent(el);

	setCascadeOpacityEnabled(true);

	auto statusBar = Rc<Layer>::create(Color::Grey_500);
	statusBar->setAnchorPoint(Vec2(0, 1));
	statusBar->setContentSize(Size(0, getStatusBarHeight()));
	statusBar->setVisible(false);
	_statusBar = addChildNode(statusBar, 5);

	return true;
}

void FlexibleLayout::onContentSizeDirty() {
	Layout::onContentSizeDirty();
	if (_flexibleHeightFunction) {
		auto ret = _flexibleHeightFunction();
		_flexibleMinHeight = ret.first;
		_flexibleMaxHeight = ret.second;
	}

	_flexibleExtraSpace = 0.0f;
	updateFlexParams();
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

void FlexibleLayout::updateFlexParams() {
	node::Params statusBarParams, flexibleNodeParams, baseNodeParams;

	float statusBar = getStatusBarHeight();
	float flexSize = _flexibleMinHeight + (_flexibleMaxHeight + (_statusBarTracked?statusBar:0) - _flexibleMinHeight) * _flexibleLevel;

	auto size = _contentSize;
	if (flexSize >= _flexibleMaxHeight && _statusBarTracked) {
		statusBar = (flexSize - _flexibleMaxHeight);
		statusBarParams.setContentSize(Size(_contentSize.width, statusBar + 1.0f));
		size.height -= statusBar;
		flexSize = _flexibleMaxHeight;
		statusBarParams.setPosition(Vec2(0, _contentSize.height + 1.0f));
		statusBarParams.setVisible(true);
	} else {
		statusBarParams.setVisible(false);
	}

	flexibleNodeParams.setPosition(0, size.height);
	flexibleNodeParams.setAnchorPoint(Vec2(0, 1));
	flexibleNodeParams.setContentSize(Size(size.width, flexSize + _flexibleExtraSpace));

	if (flexSize <= 0.0f) {
		flexibleNodeParams.setVisible(false);
	} else {
		flexibleNodeParams.setVisible(true);
	}

	if (_statusBarTracked) {
		if (_flexibleLevel == 1.0f) {
			stappler::Screen::getInstance()->setStatusBarEnabled(true, true);
		} else {
			stappler::Screen::getInstance()->setStatusBarEnabled(false, true);
		}
	}

	Padding padding;
	if (_baseNode) {
		padding = _baseNode->getPadding();
	}

	float baseNodeOffset = getCurrentFlexibleHeight();
	Padding baseNodePadding(padding.setTop(getCurrentFlexibleMax() + _baseNodePadding));

	baseNodeParams.setAnchorPoint(Vec2(0, 0));
	baseNodeParams.setPosition(0, 0 + _keyboardSize.height);

	if (_flexibleBaseNode) {
		baseNodeParams.setContentSize(Size(_contentSize.width, _contentSize.height - _keyboardSize.height));
	} else {
		baseNodeParams.setContentSize(Size(_contentSize.width, _contentSize.height - _keyboardSize.height - getCurrentFlexibleMax() - 0.0f));
		baseNodePadding = padding.setTop(4.0f);
		baseNodeOffset = 0.0f;
	}

	onStatusBarNode(statusBarParams);
	onFlexibleNode(flexibleNodeParams);
	onBaseNode(baseNodeParams, baseNodePadding, baseNodeOffset);
}

void FlexibleLayout::onScroll(float delta, bool finished) {
	auto size = _baseNode->getScrollableAreaSize();
	if (!isnan(size) && size < _contentSize.height) {
		clearFlexibleExpand(0.25f);
		setFlexibleLevel(1.0f);
		return;
	}

	clearFlexibleExpand(0.25f);
	if (!finished && delta != 0.0f) {
		const auto distanceFromStart = _baseNode->getDistanceFromStart();
		const auto trigger = _safeTrigger ? ( _flexibleMaxHeight - _flexibleMinHeight) : 8.0f;
		if (isnan(distanceFromStart) || distanceFromStart > trigger || delta < 0) {
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
				auto a = Rc<ProgressAction>::create(progress(0.0f, 0.3f, open?_flexibleLevel:(1.0f - _flexibleLevel)), open?1.0f:0.0f,
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
const Color3B &FlexibleLayout::getStatusBarBackgroundColor() const {
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

	_flexibleLevel = value;
	updateFlexParams();
}

void FlexibleLayout::setFlexibleLevelAnimated(float value, float duration) {
	stopActionByTag("FlexibleLevel"_tag);
	if (duration <= 0.0f) {
		setFlexibleLevel(value);
	} else {
		auto a = cocos2d::EaseCubicActionOut::create(Rc<ProgressAction>::create(
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

void FlexibleLayout::onStatusBarNode(const node::Params &p) {
	if (_statusBar) {
		node::apply(_statusBar, p);
	}
}
void FlexibleLayout::onFlexibleNode(const node::Params &p) {
	if (_flexibleNode) {
		node::apply(_flexibleNode, p);
	}
}
void FlexibleLayout::onBaseNode(const node::Params &p, const Padding &padding, float offset) {
	if (_baseNode) {
		node::apply(_baseNode, p);
		if (_baseNode->isVertical()) {
			_baseNode->setOverscrollFrontOffset(offset);
			_baseNode->setPadding(padding);
		}
	}
}

void FlexibleLayout::setKeyboardTracked(bool value) {
	if (_trackKeyboard != value) {
		_trackKeyboard = value;
		_keyboardEnabled = false;
		if (!_keyboardSize.equals(Size::ZERO)) {
			_keyboardSize = Size::ZERO;
			_contentSizeDirty = true;
		}

		if (_trackKeyboard) {
			auto keyboardEventListener = Rc<EventListener>::create();
			keyboardEventListener->onEvent(ime::onKeyboard, [this] (const Event &ev) {
				onKeyboard(ev.getBoolValue(), ime::getKeyboardRect(), ime::getKeyboardDuration());
			});
			_keyboardEventListener = addComponentItem(keyboardEventListener);
		} else if (_keyboardEventListener) {
			removeComponent(_keyboardEventListener);
			_keyboardEventListener = nullptr;
		}
	}
}
bool FlexibleLayout::isKeyboardTracked() const {
	return _trackKeyboard;
}

void FlexibleLayout::setSafeTrigger(bool value) {
	_safeTrigger = value;
}

bool FlexibleLayout::isSafeTrigger() const {
	return _safeTrigger;
}

void FlexibleLayout::expandFlexibleNode(float extraSpace, float duration) {
	stopActionByTag("FlexibleExtraSpace"_tag);
	stopActionByTag("FlexibleExtraClear"_tag);
	if (duration > 0.0f) {
		auto prevSpace = _flexibleExtraSpace;
		if (extraSpace > prevSpace) {
			auto a = cocos2d::EaseCubicActionOut::create(Rc<ProgressAction>::create(
					duration, [this, extraSpace, prevSpace] (ProgressAction *a, float p) {
				_flexibleExtraSpace = progress(prevSpace, extraSpace, p);
				updateFlexParams();
			}));
			runAction(a, "FlexibleExtraSpace"_tag);
		} else {
			auto a = cocos2d::EaseCubicActionIn::create(Rc<ProgressAction>::create(
					duration, [this, extraSpace, prevSpace] (ProgressAction *a, float p) {
				_flexibleExtraSpace = progress(prevSpace, extraSpace, p);
				updateFlexParams();
			}));
			runAction(a, "FlexibleExtraSpace"_tag);
		}
	} else {
		_flexibleExtraSpace = extraSpace;
		updateFlexParams();
	}
}

void FlexibleLayout::clearFlexibleExpand(float duration) {
	if (_flexibleExtraSpace == 0.0f) {
		return;
	}

	if (duration > 0.0f) {
		auto a = getActionByTag("FlexibleExtraClear"_tag);
		if (!a) {
			stopActionByTag("FlexibleExtraSpace"_tag);
			auto prevSpace = _flexibleExtraSpace;
			auto a = cocos2d::EaseCubicActionInOut::create(Rc<ProgressAction>::create(
					duration, [this, prevSpace] (ProgressAction *a, float p) {
				_flexibleExtraSpace = progress(prevSpace, 0.0f, p);
				updateFlexParams();
			}));
			runAction(a, "FlexibleExtraClear"_tag);
		}
	} else {
		_flexibleExtraSpace = 0.0f;
		updateFlexParams();
	}
}

void FlexibleLayout::onKeyboard(bool enabled, const Rect &rect, float duration) {
	if (_trackKeyboard && enabled) {
		_keyboardSize = rect.size / screen::density();
		_contentSizeDirty = true;
		_keyboardEnabled = true;
	} else {
		_keyboardSize = Size::ZERO;
		_contentSizeDirty = true;
		_keyboardEnabled = false;
	}
	log::format("Keyboard", "%d %f %f - %f", enabled, rect.size.width, rect.size.height, duration);
}

NS_MD_END
