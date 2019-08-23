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

	auto line = Rc<RoundedSprite>::create(0);
	line->setPosition(Vec2(0, 0));
	line->setAnchorPoint(Vec2(0, 0));
	line->setColor(Color::Grey_500);
	line->setOpacity(255);
	_line = addChildNode(line, 1);

	auto bar = Rc<RoundedSprite>::create(0);
	bar->setPosition(Vec2(0, 0));
	bar->setAnchorPoint(Vec2(0, 0));
	bar->setColor(Color::Black);
	bar->setOpacity(255);
	_bar = addChildNode(bar, 2);

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

		_bar->setContentSize(Size(width, _contentSize.height));
		_bar->setPosition(Vec2(diff * (_inverted?(1.0f - _progress):_progress), 0));
	} else {
		float height = _contentSize.height * _barScale;
		if (height < _contentSize.width) {
			height = _contentSize.width;
		}
		if (height > _contentSize.height) {
			height = _contentSize.height;
		}

		float diff = _contentSize.height - height;

		_bar->setContentSize(Size(_contentSize.width, height));
		_bar->setPosition(Vec2(0.0f, diff * (_inverted?(1.0f - _progress):_progress)));
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
			auto a = Rc<ProgressAction>::create(anim, value, [this] (ProgressAction *, float time) {
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
