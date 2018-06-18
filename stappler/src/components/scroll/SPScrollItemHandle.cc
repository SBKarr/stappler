/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPScrollItemHandle.h"

NS_SP_BEGIN

ScrollItemHandle::~ScrollItemHandle() { }

void ScrollItemHandle::onNodeInserted(ScrollController *c, Item &item, size_t index) {
	if (_owner) {
		_controller = c;
		_itemIndex = index;

		if (_insertCallback) {
			_insertCallback(item);
		}
	}
}

void ScrollItemHandle::onNodeUpdated(ScrollController *c, Item &item, size_t index) {
	if (_owner) {
		_controller = c;
		_itemIndex = index;

		if (_updateCallback) {
			_updateCallback(item);
		}
	}
}

void ScrollItemHandle::onNodeRemoved(ScrollController *c, Item &item, size_t index) {
	if (_owner) {
		_controller = c;
		_itemIndex = index;

		if (_removeCallback) {
			_removeCallback(item);
		}
	}
}

void ScrollItemHandle::setInsertCallback(const Callback &cb) {
	_insertCallback = cb;
}

void ScrollItemHandle::setUpdateCallback(const Callback &cb) {
	_updateCallback = cb;
}

void ScrollItemHandle::setRemoveCallback(const Callback &cb) {
	_removeCallback = cb;
}

void ScrollItemHandle::resize(float newSize, bool forward) {
	if (auto item = _controller->getItem(_itemIndex)) {
		_controller->resizeItem(item, newSize, forward);
	}
}

void ScrollItemHandle::setLocked(bool value) {
	_isLocked = value;
}

bool ScrollItemHandle::isLocked() const {
	return _isLocked;
}

NS_SP_END
