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

#include "2d/CCComponent.h"


NS_CC_BEGIN

Component::Component(void)
: _owner(nullptr), _enabled(true) { }

Component::~Component(void) { }

bool Component::init() {
	return true;
}

void Component::onAdded() { }
void Component::onRemoved() { }

void Component::onEnter() {
	_running = true;
}
void Component::onEnterTransitionDidFinish() { }
void Component::onExitTransitionDidStart() { }
void Component::onExit() {
	_running = false;
}

void Component::onVisit(Renderer *renderer, const Mat4& parentTransform, uint32_t parentFlags, const std::vector<int> &) { }

void Component::onContentSizeDirty() { }
void Component::onTransformDirty() { }
void Component::onReorderChildDirty() { }

bool Component::isRunning() const {
	return _running;
}

Node* Component::getOwner() const {
	return _owner;
}

void Component::setOwner(Node *owner) {
	_owner = owner;
}

bool Component::isEnabled() const {
	return _enabled;
}

void Component::setEnabled(bool b) {
	_enabled = b;
}

void Component::setTag(uint32_t tag) {
	_tag = tag;
}

uint32_t Component::getTag() const {
	return _tag;
}

NS_CC_END
