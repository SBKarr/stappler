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

namespace stappler::xenolith {

XL_DECLARE_EVENT_CLASS(VkView, onClipboard);
XL_DECLARE_EVENT_CLASS(VkView, onBackground);
XL_DECLARE_EVENT_CLASS(VkView, onFocus);

VkView::VkView() { }

VkView::~VkView() { }

void VkView::pollEvents() { }

int VkView::getDpi() const {
	return _dpi;
}
float VkView::getDensity() const {
	return _density;
}

const Size & VkView::getScreenSize() const {
    return _screenSize;
}

void VkView::setScreenSize(float width, float height) {
    _screenSize = Size(width, height);
}

void VkView::setViewName(const StringView& viewname) {
    _viewName = viewname.str();
}

StringView VkView::getViewName() const {
    return _viewName;
}

void VkView::handleTouchesBegin(int num, intptr_t ids[], float xs[], float ys[]) { }

void VkView::handleTouchesMove(int num, intptr_t ids[], float xs[], float ys[]) { }

void VkView::handleTouchesEnd(int num, intptr_t ids[], float xs[], float ys[]) { }

void VkView::handleTouchesCancel(int num, intptr_t ids[], float xs[], float ys[]) { }

void VkView::enableOffscreenContext() { }

void VkView::disableOffscreenContext() { }

void VkView::setClipboardString(const StringView &) { }

StringView VkView::getClipboardString() const { return StringView(); }

ScreenOrientation VkView::getScreenOrientation() const {
	return _orientation;
}

bool VkView::isTouchDevice() const {
	return _isTouchDevice;
}

bool VkView::hasFocus() const {
	return _hasFocus;
}

bool VkView::isInBackground() const {
	return _inBackground;
}

}
