/*
 * MaterialRoundedProgress.cpp
 *
 *  Created on: 30 мая 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialRoundedProgress.h"

#include "SPProgressAction.h"
#include "SPRoundedSprite.h"

NS_MD_BEGIN

bool RoundedProgress::init(Layout l) {
	if (!cocos2d::Node::init()) {
		return false;
	}

	_layout = l;
	setCascadeOpacityEnabled(true);

	_line = construct<RoundedSprite>(0);
	_line->setPosition(cocos2d::Vec2(0, 0));
	_line->setAnchorPoint(cocos2d::Vec2(0, 0));
	_line->setColor(Color::Grey_500);
	_line->setOpacity(255);
	addChild(_line, 1);

	_bar = construct<RoundedSprite>(0);
	_bar->setPosition(cocos2d::Vec2(0, 0));
	_bar->setAnchorPoint(cocos2d::Vec2(0, 0));
	_bar->setColor(Color::Black);
	_bar->setOpacity(255);
	addChild(_bar, 2);

	return true;
}

void RoundedProgress::setLayout(Layout l) {
	if (_layout != l) {
		_layout = l;
		_contentSizeDirty = true;
	}
}

RoundedProgress::Layout RoundedProgress::getLayout() const {
	return _layout;
}

void RoundedProgress::setInverted(bool value) {
	if (_inverted != value) {
		_inverted = value;
		_contentSizeDirty = true;
	}
}

bool RoundedProgress::isInverted() const {
	return _inverted;
}

void RoundedProgress::onContentSizeDirty() {
	Node::onContentSizeDirty();
	_line->setContentSize(_contentSize);

	auto l = _layout;
	if (l == Auto) {
		if (_contentSize.width > _contentSize.height) {
			l = Horizontal;
		} else {
			l = Vertical;
		}
	}

	if (l == Horizontal) {
		float width = _contentSize.width * _barScale;
		if (width < _contentSize.height) {
			width = _contentSize.height;
		}
		if (width > _contentSize.width) {
			width = _contentSize.width;
		}

		float diff = _contentSize.width - width;

		_bar->setContentSize(cocos2d::Size(width, _contentSize.height));
		_bar->setPosition(cocos2d::Vec2(diff * (_inverted?(1.0f - _progress):_progress), 0));
	} else {
		float height = _contentSize.height * _barScale;
		if (height < _contentSize.width) {
			height = _contentSize.width;
		}
		if (height > _contentSize.height) {
			height = _contentSize.height;
		}

		float diff = _contentSize.height - height;

		_bar->setContentSize(cocos2d::Size(_contentSize.width, height));
		_bar->setPosition(cocos2d::Vec2(0.0f, diff * (_inverted?(1.0f - _progress):_progress)));
	}
}

void RoundedProgress::setBorderRadius(uint32_t value) {
	_line->setTextureSize(value);
	_bar->setTextureSize(value);
}

uint32_t RoundedProgress::getBorderRadius() const {
	return _line->getTextureSize();
}

void RoundedProgress::setProgress(float value, float anim) {
	if (value < 0.0f) {
		value = 0.0f;
	} else if (value > 1.0f) {
		value = 1.0f;
	}
	if (_progress != value) {
		if (anim == 0.0f) {
			_progress = value;
			_contentSizeDirty = true;
		} else {
			stopActionByTag(129);
			auto a = construct<ProgressAction>(anim, value, [this] (ProgressAction *, float time) {
				_progress = time;
				_contentSizeDirty = true;
			});
			a->setSourceProgress(_progress);
			a->setTag(129);
			runAction(a);
		}
	}
}

float RoundedProgress::getProgress() const {
	return _progress;
}

void RoundedProgress::setBarScale(float value) {
	if (_barScale != value) {
		_barScale = value;
		_contentSizeDirty = true;
	}
}

float RoundedProgress::getBarScale() const {
	return _barScale;
}

void RoundedProgress::setLineColor(const Color &c) {
	_line->setColor(c);
}
void RoundedProgress::setLineOpacity(uint8_t o) {
	_line->setOpacity(o);
}

void RoundedProgress::setBarColor(const Color &c) {
	_bar->setColor(c);
}
void RoundedProgress::setBarOpacity(uint8_t o) {
	_bar->setOpacity(o);
}

NS_MD_END
