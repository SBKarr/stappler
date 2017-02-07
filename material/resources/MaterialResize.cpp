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
#include "MaterialResize.h"
#include "2d/CCNode.h"

NS_MD_BEGIN

bool ResizeTo::init(float duration, const cocos2d::Size &size) {
	if (!ActionInterval::initWithDuration(duration)) {
		return false;
	}

	_targetSize = size;
	return true;
}

void ResizeTo::startWithTarget(cocos2d::Node *t) {
	ActionInterval::startWithTarget(t);
	if (t) {
		_sourceSize = t->getContentSize();
	}
}

void ResizeTo::update(float time) {
	if (_target) {
		float w = 0.0f, h = 0.0f;
		if (_sourceSize.width > _targetSize.width) {
			w = _sourceSize.width + (_targetSize.width - _sourceSize.width) * time * time;
		} else {
			w = _sourceSize.width + (_targetSize.width - _sourceSize.width) * (-time*(time-2));
		}

		if (_sourceSize.height > _targetSize.height) {
			h = _sourceSize.height + (_targetSize.height - _sourceSize.height) * (-time*(time-2));
		} else {
			h = _sourceSize.height + (_targetSize.height - _sourceSize.height) * time * time;
		}

		_target->setContentSize(cocos2d::Size(w, h));
	}
}

ResizeBy* ResizeBy::create(float duration, const cocos2d::Size &size) {
	auto ret = new ResizeBy();
	if (ret->init(duration, size)) {
		ret->autorelease();
		return ret;
	} else {
		delete ret;
		return nullptr;
	}
}

bool ResizeBy::init(float duration, const cocos2d::Size &size) {
	if (!ActionInterval::initWithDuration(duration)) {
		return false;
	}

	_additionalSize = size;
	return true;
}

void ResizeBy::startWithTarget(cocos2d::Node *t) {
	ActionInterval::startWithTarget(t);
	if (t) {
		_sourceSize = t->getContentSize();
	}
}

void ResizeBy::update(float time) {
	if (_target) {
		_target->setContentSize(_sourceSize + (_additionalSize) * time);
	}
}

NS_MD_END
