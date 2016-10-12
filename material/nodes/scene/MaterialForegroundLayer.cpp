/*
 * ForegroundLayer.cpp
 *
 *  Created on: 16 марта 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialForegroundLayer.h"
#include "MaterialScene.h"
#include "MaterialLabel.h"
#include "SPGestureListener.h"
#include "SPLayer.h"

#include "2d/CCActionInstant.h"
#include "2d/CCActionInterval.h"
#include "2d/CCActionEase.h"

NS_MD_BEGIN

bool ForegroundLayer::init() {
	if (!cocos2d::Layer::init()) {
		return false;
	}

	auto l = construct<gesture::Listener>();
	l->setTouchFilter([] (const cocos2d::Vec2 &, const stappler::gesture::Listener::DefaultTouchFilter &) -> bool {
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

	_snackbar = construct<stappler::Layer>();
	_snackbar->setAnchorPoint(cocos2d::Vec2(0.5f, 0.0f));
	_snackbar->setColor(material::Color::Grey_900);

	_snackbarLabel = construct<Label>(FontType::Body_2);
	_snackbarLabel->setAnchorPoint(cocos2d::Vec2(0.0f, 0.0f));
	_snackbarLabel->setColor(material::Color::Grey_200);
	_snackbar->addChild(_snackbarLabel, 1);

	addChild(_snackbar, INT_MAX - 2);

	_listener = l;

	return true;
}

void ForegroundLayer::onContentSizeDirty() {
	Layer::onContentSizeDirty();
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
	_snackbar->setContentSize(cocos2d::Size(MIN(_contentSize.width, 536.0f), 48.0f));
	_snackbar->setPosition(cocos2d::Vec2(_contentSize.width / 2, -48.0f));
	_snackbarLabel->setString("");

	for (auto &it : _pendingPush) {
		it->release();
	}
	_pendingPush.clear();
}

void ForegroundLayer::onEnter() {
	cocos2d::Layer::onEnter();
	if (!_nodes.empty()) {
		_listener->setEnabled(true);
	}
}
void ForegroundLayer::onExit() {
	cocos2d::Layer::onExit();
	_listener->setEnabled(false);
}

void ForegroundLayer::pushNode(cocos2d::Node *node, const std::function<void()> &func) {
	if (node && !node->isRunning() && _pendingPush.find(node) == _pendingPush.end()) {
		node->retain();
		_listener->setEnabled(true);
		_pendingPush.insert(node);
		runAction(cocos2d::Sequence::createWithTwoActions(cocos2d::DelayTime::create(0.3f), cocos2d::CallFunc::create([this, node, func] {
			if (!_nodes.empty()) {
				int zIndex = -(int)_nodes.size();
				for (auto node : _nodes) {
					node->setLocalZOrder(zIndex);
					zIndex ++;
				}
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
		})));
	}
}
void ForegroundLayer::popNode(cocos2d::Node *node) {
	node->retain();
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
		_listener->setEnabled(false);
		if (auto scene = Scene::getRunningScene()) {
			scene->releaseContentForNode(this);
		 }
	}
	node->release();
}

bool ForegroundLayer::onPressBegin(const cocos2d::Vec2 &loc) {
	if (!_nodes.empty()) {
		_pressNode = _nodes.back();
	}
	return true;
}
bool ForegroundLayer::onPressEnd(const cocos2d::Vec2 &loc) {
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

void ForegroundLayer::setSnackbarString(const std::string &str, const Color &color) {
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
const std::string &ForegroundLayer::getSnackbarString() const {
	return _snackbarString;
}

void ForegroundLayer::setSnackbarStringInternal(const std::string &str, const Color &color) {
	_snackbar->setVisible(true);
	_snackbar->setOpacity(255);
	_snackbarLabel->setString(str);
	_snackbarLabel->setWidth(_snackbar->getContentSize().width - 48.0f);
	_snackbarLabel->setColor(color);
	_snackbarLabel->updateLabel();
	_snackbarLabel->setPosition(cocos2d::Vec2(24.0f, 16.0f));
	_snackbar->setContentSize(cocos2d::Size(_snackbar->getContentSize().width, _snackbarLabel->getContentSize().height + 32.0f));
	_snackbar->setPosition(cocos2d::Size(_contentSize.width / 2, -_snackbar->getContentSize().height));
	_snackbar->runAction(
			cocos2d::Sequence::create(cocos2d::EaseQuarticActionOut::create(cocos2d::MoveTo::create(0.25f, cocos2d::Vec2(_contentSize.width / 2, 0))),
					cocos2d::DelayTime::create(4.0f), cocos2d::CallFunc::create(std::bind(&ForegroundLayer::hideSnackbar, this, nullptr)), nullptr));
}

void ForegroundLayer::hideSnackbar(const std::function<void()> &cb) {
	if (!cb) {
		_snackbar->runAction(cocos2d::Sequence::createWithTwoActions(
				cocos2d::EaseQuarticActionIn::create(cocos2d::MoveTo::create(0.25f, cocos2d::Vec2(_contentSize.width / 2, -_snackbar->getContentSize().height))),
				cocos2d::CallFunc::create(std::bind(&ForegroundLayer::onSnackbarHidden, this))));
	} else {
		_snackbar->runAction(cocos2d::Sequence::createWithTwoActions(
				cocos2d::EaseQuarticActionIn::create(cocos2d::MoveTo::create(0.25f, cocos2d::Vec2(_contentSize.width / 2, -_snackbar->getContentSize().height))),
				cocos2d::CallFunc::create(cb)));
	}
}

void ForegroundLayer::onSnackbarHidden() {
	_snackbar->stopAllActions();
	_snackbar->setVisible(false);
	_snackbar->setPosition(cocos2d::Vec2(_contentSize.width / 2, -_snackbar->getContentSize().height));
	_snackbarLabel->setString("");
}

NS_MD_END
