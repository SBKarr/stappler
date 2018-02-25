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
#include "MaterialIconSprite.h"
#include "MaterialFloatingActionButton.h"
#include "MaterialLabel.h"
#include "SPProgressAction.h"

#include "SPShadowSprite.h"
#include "SPRoundedSprite.h"
#include "SPClippingNode.h"
#include "SPDrawPathNode.h"
#include "SPGestureListener.h"

#include "2d/CCActionInterval.h"
#include "2d/CCActionInstant.h"

NS_MD_BEGIN

bool FloatingActionButton::init(const TapCallback &tapCallback, const TapCallback &longTapCallback) {
	if (!MaterialNode::init()) {
		return false;
	}

	auto l = Rc<gesture::Listener>::create();
	l->setPressCallback([this] (stappler::gesture::Event ev, const stappler::gesture::Press &g) -> bool {
		if (ev == stappler::gesture::Event::Began) {
			return onPressBegin(g.location());
		} else if (ev == stappler::gesture::Event::Activated) {
			onLongPress();
			return true;
		} else if (ev == stappler::gesture::Event::Ended) {
			return onPressEnd();
		} else {
			onPressCancel();
			return true;
		}
	});
	l->setSwallowTouches(false);
	addComponent(l);

	_listener = l;
	_tapCallback = tapCallback;
	_longTapCallback = longTapCallback;

	auto drawNode = Rc<draw::PathNode>::create(48, 48, stappler::draw::Format::RGBA8888);
	drawNode->setAnchorPoint(Vec2(0.5f, 0.5f));
	drawNode->setPosition(Vec2(0.0f, 0.0f));
	drawNode->setColor(Color::White);
	drawNode->setAntialiased(true);
	_drawNode = addChildNode(drawNode, 31);

	auto icon = Rc<IconSprite>::create();
	icon->setAnchorPoint(Vec2(0.5f, 0.5f));
	icon->setNormalized(false);
	_icon = addChildNode(icon, 1);

	auto label = Rc<Label>::create(FontType::Caption);
	label->setAnchorPoint(Vec2(0.5f, 0.5f));
	label->setOpacity(222);
	_label = addChildNode(label, 2);

	setContentSize(Size(48.0f, 48.0));
	setShadowZIndex(2.0f);

	updateEnabled();

	return true;
}

void FloatingActionButton::onContentSizeDirty() {
	float width = MIN(_contentSize.width, _contentSize.height);
	setBorderRadius((uint32_t)width);

	MaterialNode::onContentSizeDirty();

	_drawNode->setContentSize(_contentSize * 2);
	_drawNode->setPosition(_contentSize.width / 2, _contentSize.height / 2);
	_label->setPosition(_contentSize.width / 2, _contentSize.height / 2);
	_icon->setPosition(_contentSize.width / 2, _contentSize.height / 2);
}

void FloatingActionButton::setBackgroundColor(const Color &c) {
	MaterialNode::setBackgroundColor(c);
	_label->setColor(c.text());
	_icon->setColor(c.text());
}

IconSprite *FloatingActionButton::getIcon() const {
	return _icon;
}
Label *FloatingActionButton::getLabel() const {
	return _label;
}
stappler::draw::PathNode *FloatingActionButton::getPathNode() const {
	return _drawNode;
}
float FloatingActionButton::getProgress() const {
	return _progress;
}
void FloatingActionButton::setProgress(float value) {
	if (!isnan(value) && _progress != value) {
		_progress = value;
		if (_progress == 0.0f && _progressPath) {
			_drawNode->removePath(_progressPath);
			_progressPath = nullptr;
		} else if (_progress != 0.0f) {
			if (!_progressPath) {
				_progressPath = _drawNode->addPath().setStrokeColor(Color4B(_progressColor, 222)).setStrokeWidth(_progressWidth / 2.0f);
			}

			_progressPath.clear()
					.setStyle(draw::Path::Style::Stroke)
					.addArc(Rect(13, 13, 22, 22), -90_to_rad, 360_to_rad * _progress);
		}
	}
}

void FloatingActionButton::setEnabled(bool value) {
	_enabled = value;
	updateEnabled();
}

bool FloatingActionButton::isEnabled() const {
	return _enabled;
}

void FloatingActionButton::setProgressColor(const Color &c) {
	_progressColor = c;
	if (_progressPath) {
		_progressPath.setStrokeColor(c);
	}
}

void FloatingActionButton::setProgressWidth(float f) {
	_progressWidth = f;
	if (_progressPath) {
		_progressPath.setStrokeWidth(f);
	}
}

void FloatingActionButton::setString(const std::string &str) {
	_label->setString(str);
}
void FloatingActionButton::setLabelColor(const Color &c) {
	_label->setColor(c);
}
void FloatingActionButton::setLabelOpacity(uint8_t o) {
	_label->setOpacity(o);
}

void FloatingActionButton::setIcon(IconName name, bool animated) {
	if (_icon->getIconName() != name) {
		if (animated) {
			_icon->stopActionByTag(125);
			auto a = cocos2d::Sequence::create(cocos2d::FadeTo::create(0.15f, 0), cocos2d::CallFunc::create([this, name] {
				_icon->setIconName(name);
			}), cocos2d::FadeTo::create(0.15f, _iconOpacity), nullptr);
			a->setTag(125);
			_icon->runAction(a);
		} else {
			_icon->stopActionByTag(125);
			_icon->setIconName(name);
		}
	}
}
void FloatingActionButton::setIconColor(const Color &c) {
	_icon->setColor(c);
}
void FloatingActionButton::setIconOpacity(uint8_t o) {
	_icon->setOpacity(o);
	_iconOpacity = o;
}

void FloatingActionButton::setTapCallback(const TapCallback &tapCallback) {
	_tapCallback = tapCallback;
	updateEnabled();
}

void FloatingActionButton::setLongTapCallback(const TapCallback &longTapCallback) {
	_longTapCallback = longTapCallback;
	updateEnabled();
}
void FloatingActionButton::setSwallowTouches(bool value) {
	static_cast<stappler::gesture::Listener *>(_listener)->setSwallowTouches(value);
}

void FloatingActionButton::clearAnimations() {
	if (_tapAnimationPath) {
		_drawNode->removePath(_tapAnimationPath);
		_tapAnimationPath = nullptr;
	}
	if (_progressPath) {
		_drawNode->removePath(_progressPath);
		_progressPath = nullptr;
	}
}

void FloatingActionButton::animateIcon(float value, float duration) {
	if (_icon) {
		_icon->animate(value, duration);
	}
}

uint8_t FloatingActionButton::getOpacityForAmbientShadow(float value) const {
	return 72;
}

uint8_t FloatingActionButton::getOpacityForKeyShadow(float value) const {
	return 128;
}


void FloatingActionButton::updateEnabled() {
	if ((_longTapCallback || _tapCallback) && _enabled) {
		_listener->setEnabled(true);
	} else {
		_listener->setEnabled(false);
	}
}

bool FloatingActionButton::onPressBegin(const cocos2d::Vec2 &location) {
	if (!_enabled) {
		return false;
	}
	if (stappler::node::isTouched(this, location, 8)) {
		animateSelection();
		return true;
	}
	return false;
}
void FloatingActionButton::onLongPress() {
	if (_longTapCallback) {
		_longTapCallback();
	}
}
bool FloatingActionButton::onPressEnd() {
	animateDeselection();
	if (_tapCallback) {
		_tapCallback();
	}
	return true;
}
void FloatingActionButton::onPressCancel() {
	animateDeselection();
}

void FloatingActionButton::animateSelection() {
	if (!_tapAnimationPath) {
		_tapAnimationPath = _drawNode->addPath().setStyle(draw::Path::Style::Fill).setFillColor(Color4B(Color::White, 32));
	}

	_tapAnimationPath.clear();
	stopActionByTag(123);
	auto a = Rc<ProgressAction>::create(0.15, [this] (ProgressAction *a, float time) {
		if (_tapAnimationPath.valid()) {
			_tapAnimationPath.clear().setFillOpacity(64).addCircle(24.0f, 24.0f, 6.0f + (18.0f * time));
		}
	});
	a->setTag(123);
	runAction(a);
}

void FloatingActionButton::animateDeselection() {
	stopActionByTag(123);
	auto a = Rc<ProgressAction>::create(0.25, [this] (ProgressAction *a, float time) {
		if (_tapAnimationPath) {
			_tapAnimationPath.setFillOpacity(64 * (1.0f - time));
		}
	}, nullptr, [this] (ProgressAction *a) {
		if (_tapAnimationPath.valid()) {
			_drawNode->removePath(_tapAnimationPath);
			_tapAnimationPath = nullptr;
		}
	});
	a->setTag(123);
	runAction(a);
}

NS_MD_END
