/*
 * MaterialResize.cpp
 *
 *  Created on: 02 апр. 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialResize.h"

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
