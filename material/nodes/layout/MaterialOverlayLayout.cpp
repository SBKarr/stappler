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
#include "MaterialResize.h"

#include "SPActions.h"
#include "SPLayer.h"
#include "SPGestureListener.h"
#include "SPEventListener.h"
#include "SPIME.h"

NS_MD_BEGIN

bool OverlayLayout::init(MaterialNode *node, const Size &size, const Function<void()> &cb) {
	if (!Layout::init()) {
		return false;
	}

	_boundSize = size;
	_cancelCallback = cb;

	auto l = Rc<gesture::Listener>::create();
	l->setTouchFilter([] (const Vec2 &loc, const gesture::Listener::DefaultTouchFilter &) -> bool {
		return true;
	});
	l->setPressCallback([this, cb] (gesture::Event ev, const gesture::Press &p) -> bool {
		if (ev == gesture::Event::Ended && (!_node || !node::isTouched(_node, p.location()))) {
			if (_cancelCallback) {
				_cancelCallback();
			} else {
				cancel();
			}
		}
		return true;
	});
	l->setSwallowTouches(true);
	l->setEnabled(false);
	addComponent(l);

	_listener = l;

	auto keyboardListener = Rc<EventListener>::create();
	keyboardListener->onEvent(ime::onKeyboard, [this] (const Event &ev) {
		onKeyboard(ev.getBoolValue(), ime::getKeyboardRect(), ime::getKeyboardDuration());
	});
	addComponent(keyboardListener);
	_keyboardEventListener = keyboardListener;

	auto bg = Rc<Layer>::create(Color::Grey_500);
	bg->setPosition(0.0f, 0.0f);
	bg->setAnchorPoint(Vec2(0.0f, 0.0f));
	bg->setOpacity(127);
	bg->setVisible(false);
	addChild(bg, -1);
	_background = bg;

	addChild(node);
	_node = node;

	setOpacity(0);

	return true;
}

void OverlayLayout::onContentSizeDirty() {
	Layout::onContentSizeDirty();

	_background->setContentSize(_contentSize);

	stopAllActionsByTag("OverlayLayout.Animation"_tag);
	if (_node && !_inTransition) {
		auto newSize = getActualSize(_boundSize);
		_node->setAnchorPoint(Vec2(0.5f, 0.5f));
		_node->setPosition(Vec2(_contentSize.width / 2, (_contentSize.height + _keyboardSize.height) / 2));
		_node->setContentSize(newSize);
	}
}

MaterialNode *OverlayLayout::getNode() const {
	return _node;
}

const Size &OverlayLayout::getBoundSize() const {
	return _boundSize;
}

void OverlayLayout::resize(const Size &size, bool animated) {
	auto newSize = getActualSize(size);
	if (!_contentSize.equals(newSize)) {
		_boundSize = newSize;
		if (animated) {
			_node->stopAllActionsByTag("OverlayLayout.Resize"_tag);
			_node->runAction(action::sequence(Rc<material::ResizeTo>::create(0.35f, newSize), std::bind(&OverlayLayout::onResized, this)), "OverlayLayout.Resize"_tag);
		} else {
			_node->setContentSize(newSize);
			onResized();
		}
	}
}

void OverlayLayout::setBackgroundColor(const Color &c) {
	_background->setColor(c);
}
void OverlayLayout::setBackgroundOpacity(uint8_t op) {
	_background->setOpacity(op);
}

void OverlayLayout::cancel() {
	if (auto scene = Scene::getRunningScene()) {
		scene->popNode(this);
	} else {
		removeFromParent();
	}
}

void OverlayLayout::setOpenAnimation(const Vec2 &origin, const Function<void()> &cb) {
	_animationPending = true;
	_animationOrigin = origin;
	_animationCallback = cb;
}

void OverlayLayout::onPush(ContentLayer *l, bool replace) {
	Layout::onPush(l, replace);
	_background->setVisible(true);
	_listener->setEnabled(true);
}

void OverlayLayout::onPop(ContentLayer *l, bool replace) {
	_listener->setEnabled(false);
	_background->setVisible(false);
	Layout::onPop(l, replace);
}


Rc<OverlayLayout::Transition> OverlayLayout::getDefaultEnterTransition() const {
	if (_animationPending && _node) {
		auto newSize = getActualSize(_boundSize);
		return action::sequence(
				0.25f,
				[this] {
			_node->setPosition(_animationOrigin);
			_node->setContentSize(Size());
				},
				action::spawn(
						cocos2d::TargetedAction::create(_node, action::spawn(
								cocos2d::MoveTo::create(0.25f, Vec2(_contentSize.width / 2, (_contentSize.height + _keyboardSize.height) / 2)),
								Rc<ResizeTo>::create(0.25f, newSize))),
						Rc<FadeIn>::create(0.25f)
				), _animationCallback);
	}
	return Rc<FadeIn>::create(2.25f);
}
Rc<OverlayLayout::Transition> OverlayLayout::getDefaultExitTransition() const {
	return Rc<FadeOut>::create(0.25f);
}

void OverlayLayout::setKeyboardTracked(bool value) {
	_trackKeyboard = value;
}
bool OverlayLayout::isKeyboardTracked() const {
	return _trackKeyboard;
}

Size OverlayLayout::getActualSize(const Size &size) const {
	return Size(std::min(_contentSize.width - metrics::horizontalIncrement() / 2.0f, size.width),
			std::min(_contentSize.height - screen::statusBarHeight() - _keyboardSize.height - metrics::horizontalIncrement() / 2.0f, size.height));
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

void OverlayLayout::onResized() { }

NS_MD_END
