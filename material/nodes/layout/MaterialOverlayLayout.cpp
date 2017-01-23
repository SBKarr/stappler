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
#include "MaterialOverlayLayout.h"
#include "MaterialNode.h"
#include "MaterialScene.h"

#include "SPLayer.h"
#include "SPGestureListener.h"
#include "SPEventListener.h"
#include "SPIME.h"

NS_MD_BEGIN

bool OverlayLayout::init(const Size &size) {
	if (!Layout::init()) {
		return false;
	}

	_boundSize = size;

	auto l = construct<gesture::Listener>();
	l->setTouchFilter([this] (const Vec2 &loc, const gesture::Listener::DefaultTouchFilter &) -> bool {
		return true;
	});
	l->setPressCallback([this] (gesture::Event ev, const gesture::Press &p) -> bool {
		if (ev == gesture::Event::Ended && (!_node || !node::isTouched(_node, p.location()))) {
			cancel();
		}
		return true;
	});
	l->setSwallowTouches(true);
	l->setEnabled(false);
	addComponent(l);

	_listener = l;

	_keyboardEventListener = construct<EventListener>();
	_keyboardEventListener->onEvent(ime::onKeyboard, [this] (const Event *ev) {
		onKeyboard(ev->getBoolValue(), ime::getKeyboardRect(), ime::getKeyboardDuration());
	});
	addComponent(_keyboardEventListener);

	_background = construct<Layer>(Color::Grey_500);
	_background->setPosition(0.0f, 0.0f);
	_background->setAnchorPoint(Vec2(0.0f, 0.0f));
	_background->setOpacity(127);
	_background->setVisible(false);
	addChild(_background, -1);

	return true;
}

void OverlayLayout::onContentSizeDirty() {
	Layout::onContentSizeDirty();

	_background->setContentSize(_contentSize);

	if (_node) {
		_node->setAnchorPoint(Vec2(0.5f, 0.5f));
		_node->setPosition(Vec2(_contentSize.width / 2, (_contentSize.height + _keyboardSize.height) / 2));

		auto newSize = Size(std::min(_contentSize.width - metrics::horizontalIncrement() / 2.0f, _boundSize.width),
				std::min(_contentSize.height - screen::statusBarHeight() - _keyboardSize.height - metrics::horizontalIncrement() / 2.0f, _boundSize.height));

		_node->setContentSize(newSize);
	}
}

void OverlayLayout::pushNode(MaterialNode *node) {
	if (_node) {
		_node->removeFromParent();
	}
	_node = node;
	if (_node) {
		_listener->setEnabled(true);
		_background->setVisible(true);
		addChild(_node, 1);
		_contentSizeDirty = true;
	} else {
		_background->setVisible(false);
		_listener->setEnabled(false);
	}
}
MaterialNode *OverlayLayout::getNode() const {
	return _node;
}

void OverlayLayout::setBoundSize(const Size &size) {
	if (!_boundSize.equals(size)) {
		_boundSize = size;
		_contentSizeDirty = true;
	}
}
const Size &OverlayLayout::getBoundSize() const {
	return _boundSize;
}

void OverlayLayout::cancel() {
	if (auto scene = Scene::getRunningScene()) {
		scene->popNode(this);
	} else {
		removeFromParent();
	}
}

void OverlayLayout::setKeyboardTracked(bool value) {
	_trackKeyboard = value;
}
bool OverlayLayout::isKeyboardTracked() const {
	return _trackKeyboard;
}

void OverlayLayout::onKeyboard(bool enabled, const Rect &rect, float duration) {
	if (_trackKeyboard && enabled) {
		_keyboardSize = rect.size / screen::density();
		_contentSizeDirty = true;
		_keyboardEnabled = true;
	} else {
		_keyboardSize = Size::ZERO;
		_contentSizeDirty = true;
		_keyboardEnabled = false;
	}
}

NS_MD_END
