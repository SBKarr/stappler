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
#include "SPActions.h"

NS_MD_BEGIN

bool ContentLayer::init() {
	if (!Node::init()) {
		return false;
	}

	return true;
}

void ContentLayer::onContentSizeDirty() {
	Node::onContentSizeDirty();
	for (auto &node : _nodes) {
		node->setAnchorPoint(Vec2(0.5f, 0.5f));
		node->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
		node->setContentSize(_contentSize);
	}

	for (auto &overlay : _overlays) {
		overlay->setAnchorPoint(Vec2(0.5f, 0.5f));
		overlay->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
		overlay->setContentSize(_contentSize);
	}
}

void ContentLayer::replaceNode(Layout *node, Transition *enterTransition) {
	if (!node || node->isRunning()) {
		return;
	}

	node->setAnchorPoint(Vec2(0.5f, 0.5f));
	node->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
	node->setContentSize(_contentSize);

	auto enter = enterTransition ? Rc<Transition>(enterTransition) : node->getDefaultEnterTransition();

	if (_nodes.empty()) {
		pushNode(node, enterTransition);
		return;
	}

	int zIndex = -(int)_nodes.size();
	for (auto node : _nodes) {
		node->setLocalZOrder(zIndex);
		zIndex ++;
	}

	_nodes.push_back(node);

	node->ignoreAnchorPointForPosition(false);
	node->setAnchorPoint(Vec2(0.5f, 0.5f));
	node->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
	node->setContentSize(_contentSize);
	addChild(node, -1);

	for (auto &it : _nodes) {
		if (it != node) {
			it->onPopTransitionBegan(this, true);
		} else {
			it->onPush(this, true);
		}
	}

	auto fn = [this, node] {
		for (auto &it : _nodes) {
			if (it != node) {
				it->onPop(this, true);
			} else {
				it->onPushTransitionEnded(this, true);
			}
		}
		replaceNodes();
	};

	if (enter) {
		node->runAction(action::sequence(enter, fn));
	} else {
		fn();
	}
}

void ContentLayer::pushNode(Layout *node, Transition *enterTransition, Transition *exitTransition) {
	if (!node || node->isRunning()) {
		return;
	}

	node->setAnchorPoint(Vec2(0.5f, 0.5f));
	node->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
	node->setContentSize(_contentSize);

	auto enter = enterTransition ? Rc<Transition>(enterTransition) : node->getDefaultEnterTransition();
	auto exit = exitTransition ? Rc<Transition>(exitTransition) : node->getDefaultExitTransition();

	pushNodeInternal(node, enterTransition, exitTransition, nullptr);
}

void ContentLayer::replaceTopNode(Layout *node, Transition *enterTransition, Transition *exitTransition) {
	if (!node || node->isRunning()) {
		return;
	}

	node->setAnchorPoint(Vec2(0.5f, 0.5f));
	node->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
	node->setContentSize(_contentSize);

	auto enter = enterTransition ? Rc<Transition>(enterTransition) : node->getDefaultEnterTransition();
	auto exit = exitTransition ? Rc<Transition>(exitTransition) : node->getDefaultExitTransition();

	if (!_nodes.empty()) {
		auto back = _nodes.back();
		_nodes.pop_back();
		back->onPopTransitionBegan(this, false);

		// just push node, then silently remove previous

		pushNodeInternal(node, enter, exit, [this, back] {
			eraseNode(back);
			back->onPop(this, false);
		});
	}
}

void ContentLayer::popNode(Layout *node) {
	auto it = std::find(_nodes.begin(), _nodes.end(), node);
	if (it == _nodes.end()) {
		return;
	}

	node->retain();
	_nodes.erase(it);

	node->onPopTransitionBegan(this, false);
	if (!_nodes.empty()) {
		_nodes.back()->onForegroundTransitionBegan(this, node);
	}

	auto fn = [this, node] {
		eraseNode(node);
		node->onPop(this, false);
		if (!_nodes.empty()) {
			_nodes.back()->onForeground(this, node);
		}
		node->release();
	};

	auto tit = _nodeExit.find(node);
	if (tit != _nodeExit.end()) {
		node->runAction(action::sequence(tit->second, fn));
	} else {
		fn();
	}
}

void ContentLayer::pushNodeInternal(Layout *node, Transition *enterTransition, Transition *exitTransition, const Function<void()> &cb) {
	if (!_nodes.empty()) {
		int zIndex = -(int)_nodes.size();
		for (auto node : _nodes) {
			node->setLocalZOrder(zIndex);
			zIndex ++;
		}
	}
	_nodes.push_back(node);
	if (exitTransition) {
		_nodeExit.emplace(node, exitTransition);
	}

	node->ignoreAnchorPointForPosition(false);
	node->setAnchorPoint(Vec2(0.5f, 0.5f));
	node->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
	node->setContentSize(_contentSize);
	addChild(node, -1);

	if (_nodes.size() > 1) {
		_nodes.at(_nodes.size() - 2)->onBackground(this, node);
	}
	node->onPush(this, false);

	auto fn = [this, node, cb] {
		updateNodesVisibility();
		if (_nodes.size() > 1) {
			_nodes.at(_nodes.size() - 2)->onBackgroundTransitionEnded(this, node);
		}
		node->onPushTransitionEnded(this, false);
		if (cb) {
			cb();
		}
	};

	if (enterTransition) {
		node->runAction(action::sequence(enterTransition, fn));
	} else {
		fn();
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

		_nodeExit.erase(node);
		updateNodesVisibility();
	}
}

void ContentLayer::eraseOverlay(OverlayLayout *l) {
	l->removeFromParent();
	if (!_overlays.empty()) {
		int zIndex = 1;
		for (auto n : _overlays) {
			n->setLocalZOrder(zIndex);
			zIndex ++;
		}

		_overlayExit.erase(l);
		updateNodesVisibility();
	}
}

void ContentLayer::replaceNodes() {
	if (!_nodes.empty()) {
		auto vec = _nodes;
		for (auto &node : vec) {
			if (node != _nodes.back()) {
				node->removeFromParent();
				_nodeExit.erase(node);
			}
		}

		_nodes.erase(_nodes.begin(), _nodes.begin() + _nodes.size() - 1);
	}
}

void ContentLayer::updateNodesVisibility() {
	for (size_t i = 0; i < _nodes.size(); i++) {
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

bool ContentLayer::pushOverlayNode(OverlayLayout *l, Transition *enterTransition, Transition *exitTransition) {
	if (!l || l->isRunning()) {
		return false;
	}

	l->setAnchorPoint(Vec2(0.5f, 0.5f));
	l->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
	l->setContentSize(_contentSize);

	auto enter = enterTransition ? Rc<Transition>(enterTransition) : l->getDefaultEnterTransition();
	auto exit = exitTransition ? Rc<Transition>(exitTransition) : l->getDefaultExitTransition();

	int zIndex = _overlays.size() + 1;

	_overlays.push_back(l);
	if (exit) {
		_overlayExit.emplace(l, exit);
	}

	l->setAnchorPoint(Vec2(0.5f, 0.5f));
	l->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
	l->setContentSize(_contentSize);
	addChild(l, zIndex);

	l->onPush(this, false);

	auto fn = [this, l] {
		l->onPushTransitionEnded(this, false);
	};

	if (enter) {
		l->runAction(action::sequence(enter, fn), "ContentLayer.Transition"_tag);
	} else {
		fn();
	}

	return true;
}

bool ContentLayer::popOverlayNode(OverlayLayout *l) {
	auto it = std::find(_overlays.begin(), _overlays.end(), l);
	if (it == _overlays.end()) {
		return false;
	}

	l->retain();
	_overlays.erase(it);
	l->onPopTransitionBegan(this, false);

	auto fn = [this, l] {
		eraseOverlay(l);
		l->onPop(this, false);
		l->release();
	};

	auto tit = _overlayExit.find(l);
	if (tit != _overlayExit.end()) {
		l->runAction(action::sequence(tit->second, fn));
	} else {
		fn();
	}

	return true;
}

NS_MD_END
