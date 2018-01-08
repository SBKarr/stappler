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
#include "MaterialForegroundLayer.h"
#include "MaterialScene.h"
#include "MaterialLabel.h"
#include "SPGestureListener.h"
#include "SPLayer.h"
#include "SPActions.h"

NS_MD_BEGIN

bool ForegroundLayer::init() {
	if (!Node::init()) {
		return false;
	}

	auto l = construct<gesture::Listener>();
	l->setTouchFilter([] (const Vec2 &, const stappler::gesture::Listener::DefaultTouchFilter &) -> bool {
		return true;
	});
	l->setTouchCallback([] (stappler::gesture::Event, const stappler::gesture::Touch &) -> bool {
		return true;
	});
	l->setPressCallback([this] (stappler::gesture::Event ev, const stappler::gesture::Press &p) -> bool {
		if (ev == stappler::gesture::Event::Began) {
			return onPressBegin(p.location());
		} else if (ev == stappler::gesture::Event::Ended) {
			return onPressEnd(p.location());
		}
		return true;
	});
	l->setSwallowTouches(true);
	l->setEnabled(false);
	addComponent(l);

	auto snackbar = Rc<Layer>::create();
	snackbar->setAnchorPoint(Vec2(0.5f, 0.0f));
	snackbar->setColor(material::Color::Grey_900);

	auto snackbarLabel = Rc<Label>::create(FontType::Body_2);
	snackbarLabel->setLocaleEnabled(true);
	snackbarLabel->setAnchorPoint(Vec2(0.0f, 0.0f));
	snackbarLabel->setColor(material::Color::Grey_200);
	snackbar->addChild(snackbarLabel, 1);

	addChild(snackbar, INT_MAX - 2);

	auto background = Rc<Layer>::create();
	background->setColor(material::Color::Grey_500);
	background->setVisible(false);
	addChild(background, INT_MIN + 2);

	_snackbar = snackbar;
	_snackbarLabel = snackbarLabel;
	_background = background;

	_listener = l;

	return true;
}

void ForegroundLayer::onContentSizeDirty() {
	Node::onContentSizeDirty();
	if (!_nodes.empty()) {
		for (auto &it : _nodes) {
			it->retain();
			auto cbIt = _callbacks.find(it);
			if (cbIt != _callbacks.end()) {
				cbIt->second();
				if (!_callbacks.empty()) {
					_callbacks.erase(cbIt);
				}
			}
			if (it->isRunning()) {
				it->removeFromParent();
			}
			it->release();
		}
		_nodes.clear();
		if (auto scene = Scene::getRunningScene()) {
			scene->releaseContentForNode(this);
			_listener->setEnabled(false);
	 	}
	}

	_snackbar->stopAllActions();
	_snackbar->setVisible(false);
	_snackbar->setContentSize(Size(std::min(_contentSize.width, 536.0f), 48.0f));
	_snackbar->setPosition(Vec2(_contentSize.width / 2, -48.0f));
	_snackbarLabel->setString("");

	for (auto &it : _pendingPush) {
		it->release();
	}
	_pendingPush.clear();
	_background->setContentSize(_contentSize);
}

void ForegroundLayer::onEnter() {
	Node::onEnter();
	if (!_nodes.empty()) {
		_listener->setEnabled(true);
	}
}
void ForegroundLayer::onExit() {
	Node::onExit();
	_listener->setEnabled(false);
}

void ForegroundLayer::pushNode(cocos2d::Node *node, const Function<void()> &func) {
	if (node && !node->isRunning() && _pendingPush.find(node) == _pendingPush.end()) {
		node->retain();
		_listener->setEnabled(true);
		_pendingPush.insert(node);
		runAction(action::sequence(0.3f, [this, node, func] {
			if (!_nodes.empty()) {
				int zIndex = -(int)_nodes.size();
				for (auto node : _nodes) {
					node->setLocalZOrder(zIndex);
					zIndex ++;
				}
			} else {
				enableBackground();
			}
			_nodes.pushBack(node);
			if (func) {
				_callbacks.insert(std::make_pair(node, func));
			}
			addChild(node, 1);
			_listener->setEnabled(true);

			if (_nodes.size() == 1) {
				if (auto scene = Scene::getRunningScene()) {
					scene->captureContentForNode(this);
				}
			}
			_pendingPush.erase(node);
			node->release();
		}));
	}
}
void ForegroundLayer::popNode(cocos2d::Node *node) {
	node->retain();
	if (_nodes.find(node) != _nodes.end()) {
		if (node == _pressNode) {
			_pressNode = nullptr;
		}
		_nodes.eraseObject(node);
		node->removeFromParent();
		_callbacks.erase(node);
		if (!_nodes.empty()) {
			int zIndex = -(int)(_nodes.size() - 1);
			for (auto node : _nodes) {
				if (zIndex == 0) {
					zIndex = 1;
				}
				node->setLocalZOrder(zIndex);
				zIndex ++;
			}
		} else {
			disableBackground();
			_listener->setEnabled(false);
			if (auto scene = Scene::getRunningScene()) {
				scene->releaseContentForNode(this);
			 }
		}
	} else if (node->getParent() == this) {
		node->removeFromParent();
	}
	node->release();
}

void ForegroundLayer::pushFloatNode(cocos2d::Node *n, int z) {
	addChild(n, z);
}
void ForegroundLayer::popFloatNode(cocos2d::Node *n) {
	if (n && n->getParent() == this) {
		n->removeFromParent();
	}
}

bool ForegroundLayer::onPressBegin(const Vec2 &loc) {
	if (!_nodes.empty()) {
		_pressNode = _nodes.back();
	}
	return true;
}
bool ForegroundLayer::onPressEnd(const Vec2 &loc) {
	if (_pressNode && !stappler::node::isTouched(_pressNode, loc)) {
		auto cbIt = _callbacks.find(_pressNode);
		if (cbIt != _callbacks.end()) {
			cbIt->second();
		}
	}
	return true;
}

void ForegroundLayer::clear() {
	for (auto node : _nodes) {
		auto cbIt = _callbacks.find(node);
		if (cbIt != _callbacks.end()) {
			cbIt->second();
		} else {
			node->removeFromParent();
		}
	}
	_nodes.clear();
	_callbacks.clear();
}

bool ForegroundLayer::isActive() const {
	return !_nodes.empty();
}

void ForegroundLayer::setSnackbarString(const String &str, const Color &color) {
	_snackbar->stopAllActions();
	_snackbarString = str;
	if (!_snackbar->isVisible()) {
		setSnackbarStringInternal(str, color);
	} else {
		hideSnackbar([this, color] {
			setSnackbarStringInternal(_snackbarString, color);
		});
	}
}
const String &ForegroundLayer::getSnackbarString() const {
	return _snackbarString;
}

void ForegroundLayer::setBackgroundOpacity(uint8_t op) {
	_backgroundOpacity = op;
	if (_backgroundEnabled) {
		_background->stopAllActions();
		_background->runAction(cocos2d::FadeTo::create(0.25f, _backgroundOpacity));
	}
}

uint8_t ForegroundLayer::getBackgroundOpacity() const {
	return _backgroundOpacity;
}

void ForegroundLayer::setBackgroundColor(const Color &c) {
	_backgroundColor = c;
	_background->setColor(c);
}
const Color & ForegroundLayer::getBackgroundColor() const {
	return _backgroundColor;
}

void ForegroundLayer::setSnackbarStringInternal(const String &str, const Color &color) {
	_snackbar->setVisible(true);
	_snackbar->setOpacity(255);
	_snackbarLabel->setString(str);
	_snackbarLabel->setWidth(_snackbar->getContentSize().width - 48.0f);
	_snackbarLabel->setColor(color);
	_snackbarLabel->tryUpdateLabel();
	_snackbarLabel->setPosition(Vec2(24.0f, 16.0f));
	_snackbar->setContentSize(Size(_snackbar->getContentSize().width, _snackbarLabel->getContentSize().height + 32.0f));
	_snackbar->setPosition(Size(_contentSize.width / 2, -_snackbar->getContentSize().height));
	_snackbar->runAction(action::sequence(cocos2d::MoveTo::create(0.25f, Vec2(_contentSize.width / 2, 0)), 4.0f, std::bind(&ForegroundLayer::hideSnackbar, this, nullptr)));
}

void ForegroundLayer::hideSnackbar(const Function<void()> &cb) {
	if (!cb) {
		_snackbar->runAction(action::sequence(
				cocos2d::EaseQuarticActionIn::create(cocos2d::MoveTo::create(0.25f, Vec2(_contentSize.width / 2, -_snackbar->getContentSize().height))),
				std::bind(&ForegroundLayer::onSnackbarHidden, this)));
	} else {
		_snackbar->runAction(action::sequence(
				cocos2d::EaseQuarticActionIn::create(cocos2d::MoveTo::create(0.25f, Vec2(_contentSize.width / 2, -_snackbar->getContentSize().height))),
				cb));
	}
}

void ForegroundLayer::onSnackbarHidden() {
	_snackbar->stopAllActions();
	_snackbar->setVisible(false);
	_snackbar->setPosition(Vec2(_contentSize.width / 2, -_snackbar->getContentSize().height));
	_snackbarLabel->setString("");
}

void ForegroundLayer::enableBackground() {
	if (!_backgroundEnabled) {
		_background->stopAllActions();
		_background->setVisible(true);
		_background->setOpacity(0);
		if (_backgroundOpacity > 0) {
			_background->runAction(cocos2d::FadeTo::create(0.25f, _backgroundOpacity));
		}
		_backgroundEnabled = true;
	}
}
void ForegroundLayer::disableBackground() {
	if (_backgroundEnabled) {
		_background->stopAllActions();
		if (_background->getOpacity() > 0) {
			_background->runAction(action::sequence(cocos2d::FadeTo::create(0.25f, 0), cocos2d::Hide::create()));
		} else {
			_background->setVisible(false);
		}
		_backgroundEnabled = false;
	}
}

NS_MD_END
