/*
 * MaterialProgress.cpp
 *
 *  Created on: 18 дек. 2014 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPProgressAction.h"

NS_SP_BEGIN

bool ProgressAction::init(float duration, const onUpdateCallback &update,
		const onStartCallback &start, const onStopCallback &stop) {
	return init(duration, 0.0f, 1.0f, update, start, stop);
}

bool ProgressAction::init(float duration, float targetProgress, const onUpdateCallback &update,
		const onStartCallback &start, const onStopCallback &stop) {
	return init(duration, 0.0f, targetProgress, update, start, stop);
}

bool ProgressAction::init(float duration, float sourceProgress, float targetProgress,
		const onUpdateCallback &update, const onStartCallback &start, const onStopCallback &stop) {
	if (!ActionInterval::initWithDuration(duration)) {
		return false;
	}

	_sourceProgress = sourceProgress;
	_targetProgress = targetProgress;
	_onUpdate = update;
	_onStart = start;
	_onStop = stop;

	return true;
}

void ProgressAction::startWithTarget(cocos2d::Node *t) {
	ActionInterval::startWithTarget(t);
	if (_onStart) {
		_onStart(this);
	}
}

void ProgressAction::update(float time) {
	if (_onUpdate) {
		_onUpdate(this, _sourceProgress + (_targetProgress - _sourceProgress) * time);
	}
}

void ProgressAction::stop() {
	if (_onStop) {
		_onStop(this);
	}
	ActionInterval::stop();
}

void ProgressAction::setSourceProgress(float progress) {
	_sourceProgress = progress;
}
float ProgressAction::getSourceProgress() {
	return _sourceProgress;
}

void ProgressAction::setTargetProgress(float progress) {
	_targetProgress = progress;
}
float ProgressAction::getTargetProgress() {
	return _targetProgress;
}

void ProgressAction::setStartCallback(const onStartCallback &cb) {
	_onStart = cb;
}
void ProgressAction::setUpdateCallback(const onUpdateCallback &cb) {
	_onUpdate = cb;
}
void ProgressAction::setStopCallback(const onStopCallback &cb) {
	_onStop = cb;
}

NS_SP_END
