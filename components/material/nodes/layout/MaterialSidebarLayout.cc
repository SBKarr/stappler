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
#include "MaterialSidebarLayout.h"
#include "MaterialNode.h"

#include "SPLayer.h"
#include "SPGestureListener.h"
#include "SPProgressAction.h"
#include "SPIME.h"

#include "2d/CCActionEase.h"

NS_MD_BEGIN

#define SHOW_ACTION_TAG 154
#define HIDE_ACTION_TAG 155

bool SidebarLayout::init(Position pos) {
	if (!Layout::init()) {
		return false;
	}

	_position = pos;

	auto listener = Rc<gesture::Listener>::create();
	listener->setTouchFilter([this] (const Vec2 &loc, const stappler::gesture::Listener::DefaultTouchFilter &) -> bool {
		if (!_node) {
			return false;
		}
		if (ime::isInputEnabled()) {
			return false;
		}
		if (isNodeEnabled() || (isNodeVisible() && _swallowTouches)) {
			return true;
		} else {
			auto pos = convertToNodeSpace(loc);
			if (node::isTouched(_node, loc)) {
				return true;
			}
			if (_edgeSwipeEnabled) {
				if ((_position == Left && pos.x < 16.0f) || (_position == Right && pos.x > _contentSize.width - 16.0f)) {
					return true;
				}
			}
			return false;
		}
	});
	listener->setPressCallback([this] (stappler::gesture::Event ev, const stappler::gesture::Press &p) -> bool {
		if (isNodeEnabled() && !node::isTouched(_node, p.location())) {
			if (ev == stappler::gesture::Event::Ended) {
				hide();
			}
			return true;
		}
		return false;
	});
	listener->setSwipeCallback([this] (stappler::gesture::Event ev, const stappler::gesture::Swipe &s) -> bool {
		if (!isNodeVisible() && !_edgeSwipeEnabled) {
			return false;
		}

    	if (ev == gesture::Event::Began) {
			auto loc = convertToNodeSpace(s.location());
    		if (fabsf(s.delta.y) < fabsf(s.delta.x) && !node::isTouched(_node, s.location())) {
    			stopNodeActions();
				onSwipeDelta(s.delta.x / stappler::screen::density());
				return true;
    		}
    		return false;
    	} else if (ev == gesture::Event::Activated) {
			onSwipeDelta(s.delta.x / stappler::screen::density());
    		return true;
    	} else {
    		onSwipeFinished(s.velocity.x / stappler::screen::density());
    		return true;
    	}

		return true;
	});
	listener->setSwallowTouches(true);
	_listener = addComponentItem(listener);

	auto background = Rc<Layer>::create(material::Color::Grey_500);
	background->setAnchorPoint(cocos2d::Vec2(0.0f, 0.0f));
	background->setVisible(false);
	background->setOpacity(_backgroundPassiveOpacity);
	_background = addChildNode(background);

	return true;
}

void SidebarLayout::onContentSizeDirty() {
	Layout::onContentSizeDirty();

	_background->setContentSize(_contentSize);

	stopNodeActions();

	if (_widthCallback) {
		_nodeWidth = _widthCallback(_contentSize);
	}

	if (_node) {
		_node->setContentSize(cocos2d::Size(_nodeWidth, _contentSize.height));
		if (_position == Left) {
			_node->setPosition(0.0f, 0.0f);
		} else {
			_node->setPosition(_contentSize.width, 0.0f);
		}
	}

	setProgress(0.0f);
}

void SidebarLayout::setNode(MaterialNode *n, int zOrder) {
	if (_node) {
		_node->removeFromParent();
		_node = nullptr;
	}
	_node = n;
	if (getProgress() == 0) {
		_node->setVisible(false);
	}
	addChild(_node, zOrder);
	_contentSizeDirty = true;
}

MaterialNode *SidebarLayout::getNode() const {
	return _node;
}

void SidebarLayout::setNodeWidth(float value) {
	_nodeWidth = value;
}
float SidebarLayout::getNodeWidth() const {
	return _nodeWidth;
}

void SidebarLayout::setNodeWidthCallback(const WidthCallback &cb) {
	_widthCallback = cb;
}
const SidebarLayout::WidthCallback &SidebarLayout::getNodeWidthCallback() const {
	return _widthCallback;
}

void SidebarLayout::setSwallowTouches(bool value) {
	_swallowTouches = value;
}
bool SidebarLayout::isSwallowTouches() const {
	return _swallowTouches;
}

void SidebarLayout::setEdgeSwipeEnabled(bool value) {
	_edgeSwipeEnabled = value;
	if (isNodeVisible()) {
		_listener->setSwallowTouches(value);
	}
}
bool SidebarLayout::isEdgeSwipeEnabled() const {
	return _edgeSwipeEnabled;
}

void SidebarLayout::setBackgroundColor(const Color &c) {
	_background->setColor(c);
}

void SidebarLayout::setBackgroundActiveOpacity(uint8_t value) {
	_backgroundActiveOpacity = value;
	_background->setOpacity(progress(_backgroundPassiveOpacity, _backgroundActiveOpacity, getProgress()));
}
void SidebarLayout::setBackgroundPassiveOpacity(uint8_t value) {
	_backgroundPassiveOpacity = value;
	_background->setOpacity(progress(_backgroundPassiveOpacity, _backgroundActiveOpacity, getProgress()));
}

void SidebarLayout::show() {
	stopActionByTag(HIDE_ACTION_TAG);
	if (getActionByTag(SHOW_ACTION_TAG) == nullptr) {
		auto a = cocos2d::EaseCubicActionOut::create(Rc<ProgressAction>::create(
				progress(0.35f, 0.0f, getProgress()), getProgress(), 1.0f,
				[this] (ProgressAction *a, float progress) {
			setProgress(progress);
		}));
		a->setTag(SHOW_ACTION_TAG);
		runAction(a);
	}
}
void SidebarLayout::hide(float factor) {
	stopActionByTag(SHOW_ACTION_TAG);
	if (getActionByTag(HIDE_ACTION_TAG) == nullptr) {
		if (factor <= 1.0f) {
			auto a = cocos2d::EaseCubicActionIn::create(Rc<ProgressAction>::create(
					progress(0.0f, 0.35f / factor, getProgress()), getProgress(), 0.0f,
					[this] (ProgressAction *a, float progress) {
				setProgress(progress);
			}));
			a->setTag(HIDE_ACTION_TAG);
			runAction(a);
		} else {
			auto a = cocos2d::EaseQuadraticActionIn::create(Rc<ProgressAction>::create(
					progress(0.0f, 0.35f / factor, getProgress()), getProgress(), 0.0f,
					[this] (ProgressAction *a, float progress) {
				setProgress(progress);
			}));
			a->setTag(HIDE_ACTION_TAG);
			runAction(a);
		}
	}
}

float SidebarLayout::getProgress() const {
	if (_node) {
		return (_position == Left)?(1.0f - _node->getAnchorPoint().x):(_node->getAnchorPoint().x);
	}
	return 0.0f;
}
void SidebarLayout::setProgress(float value) {
	auto prev = getProgress();
	if (_node && value != getProgress()) {
		_node->setAnchorPoint(cocos2d::Vec2((_position == Left)?(1.0f - value):(value), 0.0f));
		if (value == 0.0f) {
			if (_node->isVisible()) {
				_node->setVisible(false);
				onNodeVisible(false);
			}
		} else {
			if (!_node->isVisible()) {
				_node->setVisible(true);
				onNodeVisible(true);
			}

			if (value == 1.0f && prev != 1.0f) {
				onNodeEnabled(true);
			} else if (value != 1.0f && prev == 1.0f) {
				onNodeEnabled(false);
			}
		}

		_background->setOpacity(progress(_backgroundPassiveOpacity, _backgroundActiveOpacity, value));
		if (!_background->isVisible() && _background->getOpacity() > 0) {
			_background->setVisible(true);
		}
	}
}

void SidebarLayout::onSwipeDelta(float value) {
	float d = value / _nodeWidth;

	float progress = getProgress() - d * (_position==Left ? -1 : 1);
	if (progress >= 1.0f) {
		progress = 1.0f;
	} else if (progress <= 0.0f) {
		progress = 0.0f;
	}

	setProgress(progress);
}

void SidebarLayout::onSwipeFinished(float value) {
	float v = value / _nodeWidth;
	auto t = fabsf(v) / (5000 / _nodeWidth);
	auto d = v * t - (5000.0f * t * t / _nodeWidth) / 2.0f;

	float progress = getProgress() - d * (_position==Left ? -1 : 1);

	if (progress > 0.5) {
		show();
	} else {
		hide(1.0f + fabsf(d) * 2.0f);
	}
}

bool SidebarLayout::isNodeVisible() const {
	return getProgress() > 0.0f;
}
bool SidebarLayout::isNodeEnabled() const {
	return getProgress() == 1.0f;
}

void SidebarLayout::setEnabled(bool value) {
	_listener->setEnabled(value);
}
bool SidebarLayout::isEnabled() const {
	return _listener->isEnabled();
}

void SidebarLayout::setNodeVisibleCallback(const BoolCallback &cb) {
	_visibleCallback = cb;
}
void SidebarLayout::setNodeEnabledCallback(const BoolCallback &cb) {
	_enabledCallback = cb;
}

void SidebarLayout::onNodeEnabled(bool value) {
	if (_enabledCallback) {
		_enabledCallback(value);
	}
}
void SidebarLayout::onNodeVisible(bool value) {
	if (value) {
		if (_swallowTouches) {
			_listener->setSwallowTouches(true);
		}
		if (_backgroundPassiveOpacity == 0) {
			_background->setVisible(false);
		}
	} else {
		_background->setVisible(true);
		_listener->setSwallowTouches(false);
	}

	if (_visibleCallback) {
		_visibleCallback(value);
	}
}

void SidebarLayout::stopNodeActions() {
	stopActionByTag(SHOW_ACTION_TAG);
	stopActionByTag(HIDE_ACTION_TAG);
}

NS_MD_END
