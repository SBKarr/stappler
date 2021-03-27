/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#include "XLVkView.h"
#include "XLPlatform.h"
#include "XLVkProgram.h"

namespace stappler::xenolith::vk {

XL_DECLARE_EVENT_CLASS(View, onClipboard);
XL_DECLARE_EVENT_CLASS(View, onBackground);
XL_DECLARE_EVENT_CLASS(View, onFocus);

View::View() { }

View::~View() { }

bool View::init(Instance *impl, PresentationDevice *dev) {
	_instance = impl;
	_device = dev;

	return true;
}

int View::getDpi() const {
	return _dpi;
}
float View::getDensity() const {
	return _density;
}

const Size & View::getScreenSize() const {
    return _screenSize;
}

void View::setScreenSize(float width, float height) {
    _screenSize = Size(width, height);
}

void View::handleTouchesBegin(int num, intptr_t ids[], float xs[], float ys[]) { }

void View::handleTouchesMove(int num, intptr_t ids[], float xs[], float ys[]) { }

void View::handleTouchesEnd(int num, intptr_t ids[], float xs[], float ys[]) { }

void View::handleTouchesCancel(int num, intptr_t ids[], float xs[], float ys[]) { }

void View::enableOffscreenContext() { }

void View::disableOffscreenContext() { }

void View::setClipboardString(StringView) { }

StringView View::getClipboardString() const { return StringView(); }

ScreenOrientation View::getScreenOrientation() const {
	return _orientation;
}

bool View::isTouchDevice() const {
	return _isTouchDevice;
}

bool View::hasFocus() const {
	return _hasFocus;
}

bool View::isInBackground() const {
	return _inBackground;
}

void View::pushEvent(ViewEvent::Value events) {
	_events |= events;
}

ViewEvent::Value View::popEvents() {
	return _events.exchange(ViewEvent::None);
}

}
