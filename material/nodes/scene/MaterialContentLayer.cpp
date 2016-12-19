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
#include "MaterialContentLayer.h"
#include "MaterialLayout.h"

#include "SPGestureListener.h"
#include "SPActions.h"

NS_MD_BEGIN

bool ContentLayer::init() {
	if (!Node::init()) {
		return false;
	}

	auto l = construct<gesture::Listener>();
	l->setTouchFilter([] (const cocos2d::Vec2 &, const stappler::gesture::Listener::DefaultTouchFilter &) -> bool {
		return true;
	});
	l->setTouchCallback([] (stappler::gesture::Event, const stappler::gesture::Touch &) -> bool {
		return true;
	});
	l->setSwallowTouches(true);
	l->setEnabled(false);
	addComponent(l);

	_listener = l;

	return true;
}

void ContentLayer::onContentSizeDirty() {
	Node::onContentSizeDirty();
	for (auto &node : _nodes) {
		node->setAnchorPoint(Vec2(0.5f, 0.5f));
		node->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
		node->setContentSize(_contentSize);
	}
}

void ContentLayer::replaceNode(Layout *node, Transition *enterTransition) {
	if (node && !node->isRunning()) {
		if (_nodes.empty()) {
			pushNode(node, enterTransition);
			return;
		}

		int zIndex = -(int)_nodes.size();
		for (auto node : _nodes) {
			node->setLocalZOrder(zIndex);
			zIndex ++;
		}

		_nodes.pushBack(node);

		node->ignoreAnchorPointForPosition(false);
		node->setAnchorPoint(Vec2(0.5f, 0.5f));
		node->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
		node->setContentSize(_contentSize);
		addChild(node, 1);

		for (auto &it : _nodes) {
			if (it != node) {
				it->onPopTransitionBegan(this, true);
			} else {
				it->onPush(this, true);
			}
		}

		if (enterTransition) {
			node->runAction(action::callback(enterTransition, [this, node] {
				for (auto &it : _nodes) {
					if (it != node) {
						it->onPop(this, true);
					} else {
						it->onPushTransitionEnded(this, true);
					}
				}
				replaceNodes();
			}));
		} else {
			for (auto &it : _nodes) {
				if (it != node) {
					it->onPop(this, true);
				} else {
					it->onPushTransitionEnded(this, true);
				}
			}
			replaceNodes();
		}

		_listener->setEnabled(true);
	}
}

void ContentLayer::pushNode(Layout *node, Transition *enterTransition, Transition *exitTransition) {
	if (node && !node->isRunning()) {
		if (!_nodes.empty()) {
			int zIndex = -(int)_nodes.size();
			for (auto node : _nodes) {
				node->setLocalZOrder(zIndex);
				zIndex ++;
			}
		}
		_nodes.pushBack(node);
		if (exitTransition) {
			_exitTransitions.insert(node, exitTransition);
		}

		node->ignoreAnchorPointForPosition(false);
		node->setAnchorPoint(Vec2(0.5f, 0.5f));
		node->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
		node->setContentSize(_contentSize);
		addChild(node, 1);

		if (_nodes.size() > 1) {
			_nodes.at(_nodes.size() - 2)->onBackground(this, node);
		}
		node->onPush(this, false);

		if (enterTransition) {
			node->runAction(action::callback(enterTransition, [this, node] {
				updateNodesVisibility();
				if (_nodes.size() > 1) {
					_nodes.at(_nodes.size() - 2)->onBackgroundTransitionEnded(this, node);
				}
				node->onPushTransitionEnded(this, false);
			}));
		} else {
			updateNodesVisibility();
			if (_nodes.size() > 1) {
				_nodes.at(_nodes.size() - 2)->onBackgroundTransitionEnded(this, node);
			}
			node->onPushTransitionEnded(this, false);
		}

		_listener->setEnabled(true);
	}
}

void ContentLayer::replaceTopNode(Layout *node, Transition *enterTransition, Transition *exitTransition) {
	if (!_nodes.empty()) {
		auto back = _nodes.back();
		back->retain();
		_nodes.popBack();
		back->onPopTransitionBegan(this, false);

		auto tit = _exitTransitions.find(back);
		if (tit != _exitTransitions.end()) {
			safe_retain(node);
			safe_retain(enterTransition);
			safe_retain(exitTransition);
			back->runAction(action::callback(tit->second, [this, back, node, enterTransition, exitTransition] {
				eraseNode(back);
				back->onPop(this, false);
				back->release();
				pushNode(node, enterTransition, exitTransition);
				safe_release(node);
				safe_release(enterTransition);
				safe_release(exitTransition);
			}));
		} else {
			eraseNode(back);
			back->onPop(this, false);
			back->release();
			pushNode(node, enterTransition, exitTransition);
		}
	}
}

void ContentLayer::popNode(Layout *node) {
	auto it = _nodes.find(node);
	if (it == _nodes.end()) {
		return;
	}

	node->retain();
	_nodes.erase(it);

	node->onPopTransitionBegan(this, false);
	if (!_nodes.empty()) {
		_nodes.back()->onForegroundTransitionBegan(this, node);
	}
	auto tit = _exitTransitions.find(node);
	if (tit != _exitTransitions.end()) {
		node->runAction(action::callback(tit->second, [this, node] {
			eraseNode(node);
			node->onPop(this, false);
			if (!_nodes.empty()) {
				_nodes.back()->onForeground(this, node);
			}
			node->release();
		}));
	} else {
		eraseNode(node);
		node->onPop(this, false);
		if (!_nodes.empty()) {
			_nodes.back()->onForeground(this, node);
		}
		node->release();
	}
}

void ContentLayer::eraseNode(Layout *node) {
	node->removeFromParent();
	if (!_nodes.empty()) {
		int zIndex = -((int)_nodes.size() - 1);
		for (auto n : _nodes) {
			if (zIndex == 0) {
				zIndex = 1;
			}
			n->setLocalZOrder(zIndex);
			n->setVisible(false);
			zIndex ++;
		}

		_exitTransitions.erase(node);
		updateNodesVisibility();
	} else {
		_listener->setEnabled(false);
	}
}


void ContentLayer::replaceNodes() {
	if (!_nodes.empty()) {
		auto vec = _nodes;
		for (auto &node : vec) {
			if (node != _nodes.back()) {
				node->removeFromParent();
				_exitTransitions.erase(node);
			}
		}

		_nodes.erase(_nodes.begin(), _nodes.begin() + _nodes.size() - 1);
	}
}

void ContentLayer::updateNodesVisibility() {
	for (ssize_t i = 0; i < _nodes.size(); i++) {
		if (i == _nodes.size() - 1) {
			_nodes.at(i)->setVisible(true);
		} else {
			_nodes.at(i)->setVisible(false);
		}
	}
}

Layout *ContentLayer::getRunningNode() {
	if (!_nodes.empty()) {
		return _nodes.back();
	} else {
		return nullptr;
	}
}

Layout *ContentLayer::getPrevNode() {
	if (_nodes.size() > 1) {
		return *(_nodes.end() - 2);
	}
	return nullptr;
}

bool ContentLayer::popLastNode() {
	if (_nodes.size() > 1) {
		popNode(_nodes.back());
		return true;
	}
	return false;
}

bool ContentLayer::isActive() const {
	return !_nodes.empty();
}
bool ContentLayer::onBackButton() {
	if (_nodes.empty()) {
		return false;
	} else {
		if (!_nodes.back()->onBackButton()) {
			if (!popLastNode()) {
				return false;
			}
		}
		return true;
	}
}

size_t ContentLayer::getNodesCount() const {
	return _nodes.size();
}

NS_MD_END
