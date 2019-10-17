/****************************************************************************
Copyright (c) 2013-2014 Chukong Technologies Inc.

http://www.cocos2d-x.org

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
****************************************************************************/


#include "2d/CCComponentContainer.h"
#include "2d/CCComponent.h"
#include "2d/CCNode.h"

NS_CC_BEGIN

ComponentContainer::ComponentContainer(Node *node)
: _owner(node) { }

ComponentContainer::~ComponentContainer(void) { }

bool ComponentContainer::add(Component *com) {
	CCASSERT(com != nullptr, "Argument must be non-nil");
	CCASSERT(com->getOwner() == nullptr, "Component already added. It can't be added again");

	com->setOwner(_owner);
	_components.pushBack(com);
	com->onAdded();
	if (_owner->isRunning()) {
		com->onEnter();
		if (_owner->isTransitionFinished()) {
			com->onEnterTransitionDidFinish();
		}
	}

	return true;
}

bool ComponentContainer::remove(Component *com) {
	if (_components.empty()) {
		return false;
	}

	for (auto iter = _components.begin(); iter != _components.end(); ++iter) {
		if ((*iter) == com) {
			if (_owner->isRunning()) {
				com->onExitTransitionDidStart();
				com->onExit();
			}
			com->onRemoved();
			com->setOwner(nullptr);
			_components.erase(iter);
			return true;
		}
	}
    return false;
}

void ComponentContainer::removeAll() {
	for (auto iter : _components) {
		if (_owner->isRunning()) {
			iter->onExitTransitionDidStart();
			iter->onExit();
		}
		iter->onRemoved();
		iter->setOwner(nullptr);
	}

	_components.clear();
}

const Vector<Component *> &ComponentContainer::getComponents() const {
	return _components;
}

void ComponentContainer::onVisit(Renderer *renderer, const Mat4& parentTransform, uint32_t parentFlags, const std::vector<int> &zPath) {
	CC_SAFE_RETAIN(_owner);
	for (auto iter : _components) {
		iter->onVisit(renderer, parentTransform, parentFlags, zPath);
	}
	CC_SAFE_RELEASE(_owner);
}

void ComponentContainer::onEnter() {
	CC_SAFE_RETAIN(_owner);
	for (auto iter : _components) {
		iter->onEnter();
	}
	CC_SAFE_RELEASE(_owner);
}

void ComponentContainer::onEnterTransitionDidFinish() {
	CC_SAFE_RETAIN(_owner);
	for (auto iter : _components) {
		iter->onEnterTransitionDidFinish();
	}
	CC_SAFE_RELEASE(_owner);
}

void ComponentContainer::onExitTransitionDidStart() {
	CC_SAFE_RETAIN(_owner);
	for (auto iter : _components) {
		iter->onExitTransitionDidStart();
	}
	CC_SAFE_RELEASE(_owner);
}

void ComponentContainer::onExit() {
	CC_SAFE_RETAIN(_owner);
	for (auto iter : _components) {
		iter->onExit();
	}
	CC_SAFE_RELEASE(_owner);
}

void ComponentContainer::onContentSizeDirty() {
	CC_SAFE_RETAIN(_owner);
	for (auto iter : _components) {
		iter->onContentSizeDirty();
	}
	CC_SAFE_RELEASE(_owner);
}

void ComponentContainer::onTransformDirty() {
	CC_SAFE_RETAIN(_owner);
	for (auto iter : _components) {
		iter->onTransformDirty();
	}
	CC_SAFE_RELEASE(_owner);
}

void ComponentContainer::onReorderChildDirty() {
	CC_SAFE_RETAIN(_owner);
	for (auto iter : _components) {
		iter->onReorderChildDirty();
	}
	CC_SAFE_RELEASE(_owner);
}

bool ComponentContainer::isEmpty() const {
    return (_components.empty());
}

NS_CC_END
