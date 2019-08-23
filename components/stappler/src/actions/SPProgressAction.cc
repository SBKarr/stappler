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
	_stopped = false;
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
	_stopped = true;
	ActionInterval::stop();
}

void ProgressAction::onStopped() {
	if (_forceStopCallback && !_stopped && _target && _onStop) {
		_onStop(this);
	}
}

void ProgressAction::setSourceProgress(float progress) {
	_sourceProgress = progress;
}
float ProgressAction::getSourceProgress() const {
	return _sourceProgress;
}

void ProgressAction::setTargetProgress(float progress) {
	_targetProgress = progress;
}
float ProgressAction::getTargetProgress() const {
	return _targetProgress;
}

void ProgressAction::setForceStopCallback(bool val) {
	_forceStopCallback = val;
}
bool ProgressAction::isForceStopCallback() const {
	return _forceStopCallback;
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
