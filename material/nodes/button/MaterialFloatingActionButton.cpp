/*
 * MaterialFloatingActionButton.cpp
 *
 *  Created on: 25 мая 2015 г.
 *      Author: sbkarr
 */

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

	auto l = construct<gesture::Listener>();
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

	_drawNode = construct<draw::PathNode>(48.0f * 2, 48.0f * 2, stappler::draw::Format::RGBA8888);
	_drawNode->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
	_drawNode->setPosition(cocos2d::Vec2(0.0f, 0.0f));
	_drawNode->setColor(Color::White);
	_drawNode->setScale(1.0f / stappler::screen::density());
	_drawNode->setAntialiased(true);
	addChild(_drawNode, 31);

	_icon = construct<IconSprite>();
	_icon->setAnchorPoint(Vec2(0.5f, 0.5f));
	_icon->setNormalized(false);
	addChild(_icon, 1);

	_label = construct<Label>(Font::systemFont(Font::Type::System_Caption));
	_label->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
	_label->setOpacity(222);
	addChild(_label, 2);

	setContentSize(cocos2d::Size(48.0f, 48.0));
	setShadowZIndex(5.0f);

	updateEnabled();

	return true;
}

void FloatingActionButton::onContentSizeDirty() {
	float width = MIN(_contentSize.width, _contentSize.height);
	setBorderRadius((uint32_t)width);

	MaterialNode::onContentSizeDirty();

	_drawNode->setContentSize(_contentSize * 2 * stappler::screen::density());
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
				_progressPath = construct<draw::Path>();
				_progressPath->setStrokeColor(Color4B(_progressColor, 222));
				_progressPath->setStrokeWidth(_progressWidth);
				_progressPath->setAntialiased(true);
				_drawNode->addPath(_progressPath);
			}

			_progressPath->clear();
			_progressPath->setStyle(stappler::draw::Path::Style::Stroke);
			_progressPath->addArc(cocos2d::Rect(26, 26, 44, 44), -90_to_rad, 360_to_rad * _progress);
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
		_progressPath->setStrokeColor(c);
	}
}

void FloatingActionButton::setProgressWidth(float f) {
	_progressWidth = f;
	if (_progressPath) {
		_progressPath->setStrokeWidth(f);
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

uint8_t FloatingActionButton::getOpacityForAmbientShadow(float value) const {
	return (value < 2.0)?(value * 0.5f * 128.0f):128;
}

uint8_t FloatingActionButton::getOpacityForKeyShadow(float value) const {
	return (value < 2.0)?(value * 0.5f * 192.0f):192;
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
		_tapAnimationPath = construct<draw::Path>();
		_tapAnimationPath->setStyle(stappler::draw::Path::Style::Fill);
		_tapAnimationPath->setFillColor(cocos2d::Color4B(Color::White, 32));
		_drawNode->addPath(_tapAnimationPath);
	}

	_tapAnimationPath->clear();
	stopActionByTag(123);
	auto a = construct<ProgressAction>(0.15, [this] (ProgressAction *a, float time) {
		if (_tapAnimationPath) {
			_tapAnimationPath->clear();
			_tapAnimationPath->setFillOpacity(32);
			_tapAnimationPath->addCircle(48.0f, 48.0f, 24.0f + (24.0f * time));
		}
	});
	a->setTag(123);
	runAction(a);
}

void FloatingActionButton::animateDeselection() {
	stopActionByTag(123);
	auto a = construct<ProgressAction>(0.25, [this] (ProgressAction *a, float time) {
		if (_tapAnimationPath) {
			_tapAnimationPath->setFillOpacity(32 * (1.0f - time));
		}
	}, nullptr, [this] (ProgressAction *a) {
		if (_tapAnimationPath) {
			_drawNode->removePath(_tapAnimationPath);
			_tapAnimationPath = nullptr;
		}
	});
	a->setTag(123);
	runAction(a);
}

NS_MD_END
