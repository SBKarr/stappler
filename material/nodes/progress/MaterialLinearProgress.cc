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
#include "MaterialLinearProgress.h"

#include "SPProgressAction.h"

NS_MD_BEGIN

bool LinearProgress::init() {
	if (!cocos2d::Node::init()) {
		return false;
	}

	auto line = Rc<Layer>::create();
	line->setPosition(0, 0);
	line->setAnchorPoint(cocos2d::Vec2(0, 0));
	_line = addChildNode(line, 1);

	auto bar = Rc<Layer>::create();
	bar->setPosition(0, 0);
	bar->setAnchorPoint(cocos2d::Vec2(0, 0));
	_bar = addChildNode(bar, 2);

	setCascadeOpacityEnabled(true);

	return true;
}
void LinearProgress::onContentSizeDirty() {
	Node::onContentSizeDirty();
	layoutSubviews();
}

void LinearProgress::setAnimated(bool value) {
	if (_animated != value) {
		_animated = value;
		if (_animated) {
			auto p = Rc<ProgressAction>::create(2.0f, 1.0f, [this] (ProgressAction *a, float time) {
				setProgress(time);
			});
			auto a = cocos2d::RepeatForever::create(p);
			a->setTag(2);
			runAction(a);
		} else {
			stopActionByTag(2);
		}
	}
}
bool LinearProgress::isAnimated() const {
	return _animated;
}

void LinearProgress::setProgress(float value) {
	if (_progress != value) {
		_progress = value;
		_contentSizeDirty = true;
	}
}
float LinearProgress::getProgress() const {
	return _progress;
}

void LinearProgress::layoutSubviews() {
	_line->setContentSize(_contentSize);
	if (!_animated) {
		auto barSize = cocos2d::Size(_progress * _contentSize.width, _contentSize.height);
		_bar->setPosition(0, 0);
		_bar->setContentSize(barSize);
	} else {
		const float sep = 0.60f;
		float p = 0.0f;
		bool invert = false;
		if (_progress < sep) {
			p = _progress / sep;
		} else {
			p = (_progress - sep) / (1.0f - sep);
			invert = true;
		}

		float start = 0.0f;
		float end = _contentSize.width;

		const float ePos = invert ? 0.15f : 0.45f;
		const float sPos = invert ? 0.35f : 0.20f;

		if (p < (1.0f - ePos)) {
			end = _contentSize.width * p / (1.0f - ePos);
		}

		if (p > sPos) {
			start = _contentSize.width * (p - sPos) / (1.0f - sPos);
		}

		_bar->setPosition(Vec2(start, 0.0f));
		_bar->setContentSize(Size(end - start, _contentSize.height));
	}
}

void LinearProgress::setLineColor(const Color &c) {
	_line->setColor(c);
}

void LinearProgress::setLineOpacity(uint8_t op) {
	_line->setOpacity(op);
}

void LinearProgress::setBarColor(const Color &c) {
	_bar->setColor(c);
}

void LinearProgress::setBarOpacity(uint8_t op) {
	_bar->setOpacity(op);
}

NS_MD_END
