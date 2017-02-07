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

	_line = construct<stappler::Layer>();
	_line->setPosition(0, 0);
	_line->setAnchorPoint(cocos2d::Vec2(0, 0));
	addChild(_line, 1);

	_bar = construct<stappler::Layer>();
	_bar->setPosition(0, 0);
	_bar->setAnchorPoint(cocos2d::Vec2(0, 0));
	addChild(_bar, 2);

	_secondaryBar = construct<stappler::Layer>();
	_secondaryBar->setPosition(0, 0);
	_secondaryBar->setAnchorPoint(cocos2d::Vec2(0, 0));
	addChild(_secondaryBar);
	setCascadeOpacityEnabled(true);

	return true;
}
void LinearProgress::onContentSizeDirty() {
	Node::onContentSizeDirty();
	layoutSubviews();
}

void LinearProgress::setDeterminate(bool value) {
	if (value != _determinate) {
		_determinate = value;
		_contentSizeDirty = true;
	}
}
bool LinearProgress::isDeterminate() const {
	return _determinate;
}

void LinearProgress::setAnimated(bool value) {
	if (_animated != value) {
		_animated = value;
		if (_animated) {
			auto p = construct<ProgressAction>(2.5f, 1.0f, [this] (ProgressAction *a, float time) {
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
	float secondBarGrowTime = 0.15f;
	float firstBarGrowTime = 0.2f;
	float secondBarTime = 0.55f;
	float secondBarSpeed = 1.25f;

	if (_determinate) {
		auto barSize = cocos2d::Size(_progress * _contentSize.width, _contentSize.height);
		_bar->setPosition(0, 0);
		_bar->setContentSize(barSize);

		_secondaryBar->setPosition(0.0f, 0.0f);
		_secondaryBar->setContentSize(Size(0.0f, 0.0f));
	} else {
		_bar->setPosition(0, 0);
		_bar->setContentSize(Size(0, 0));
		_secondaryBar->setPosition(0, 0);
		_secondaryBar->setContentSize(Size(0, 0));
		if ((_progress > secondBarTime)&&(_progress < (secondBarTime+secondBarGrowTime))) {
			_secondaryBar->setPosition(0, 0);
			float aCoeff = (1.0f)/(secondBarGrowTime);
			float bCoeff = (-1.0f)*aCoeff*secondBarTime;

			//in secondBarTime must be null, and in secondBarTime+secondBarGrowTime content size must be _contentSize/2
			_secondaryBar->setContentSize(cocos2d::Size((_contentSize.width/2)*((_progress*aCoeff)+bCoeff), _contentSize.height));
		} else if (_progress >= (secondBarTime+secondBarGrowTime)) {
			float aCoeff = (secondBarSpeed)/(1-(secondBarTime+secondBarGrowTime));
			float bCoeff = secondBarSpeed-aCoeff;
			_secondaryBar->setPosition((_contentSize.width*secondBarSpeed)*(aCoeff*_progress+bCoeff), 0);

			float cCoeff = (-1.0f)/(1-(secondBarTime+secondBarGrowTime));
			float dCoeff = (-1.0f)*cCoeff;
			_secondaryBar->setContentSize(cocos2d::Size((_contentSize.width/2)*((_progress*cCoeff)+dCoeff), _contentSize.height));
		}

		if(_progress < firstBarGrowTime) {
			_bar->setPosition(0, 0);
			_bar->setContentSize(cocos2d::Size((_contentSize.width/2)*(_progress*(1.0f/firstBarGrowTime)), _contentSize.height));
		} else {
			_bar->setPosition(_contentSize.width*(_progress-0.2f)*(1+3.0f*_progress), 0);
			_bar->setContentSize(cocos2d::Size((_contentSize.width/2)*(0.8f+_progress), _contentSize.height));
		}
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
	_secondaryBar->setColor(c);
}

void LinearProgress::setBarOpacity(uint8_t op) {
	_bar->setOpacity(op);
	_secondaryBar->setOpacity(op);
}

NS_MD_END
