/**
Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_XENOLITH_PLATFORM_LINUX_XLVKVIEWIMPL_LINUX_H_
#define COMPONENTS_XENOLITH_PLATFORM_LINUX_XLVKVIEWIMPL_LINUX_H_

#include "XLVkView.h"

#if LINUX

namespace stappler::xenolith::vk {

class LinuxViewInterface : public Ref {
public:
	virtual bool run(Rc<PresentationLoop> loop, const Callback<bool(uint64_t)> &cb) = 0;
	virtual void onEventPushed() = 0;
};

class ViewImpl : public View {
public:
	ViewImpl();
    virtual ~ViewImpl();

    bool init(Rc<Instance> instance, StringView viewName, URect rect);
    bool init(Rc<Instance> instance, StringView viewName);

	virtual void end() override;
	virtual void setIMEKeyboardState(bool open) override;

	virtual bool run(Application *, Rc<Director>, const Callback<bool(uint64_t)> &) override;

	virtual void setScreenSize(float width, float height) override;

	virtual void setClipboardString(StringView) override;
	virtual StringView getClipboardString() const override;

	virtual void selectPresentationOptions(Instance::PresentationOptions &opts) const override;

	virtual void pushEvent(ViewEvent::Value) override;

protected:
	Rc<LinuxViewInterface> _view;
	VkSurfaceKHR _surface = nullptr;

	uint32_t _frameWidth = 0;
	uint32_t _frameHeight = 0;
	uint64_t _frameTimeMicroseconds = 1000'000 / 60;
};

}

#endif

#endif /* COMPONENTS_XENOLITH_PLATFORM_LINUX_XLVKVIEWIMPL_LINUX_H_ */
