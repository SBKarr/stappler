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

#ifndef COMPONENTS_XENOLITH_PLATFORM_COMMON_XLVKVIEW_H_
#define COMPONENTS_XENOLITH_PLATFORM_COMMON_XLVKVIEW_H_

#include "XLGestureData.h"
#include "XLEventHeader.h"
#include "XLVkPresentation.h"

namespace stappler::xenolith::vk {

struct ContextAttrs {
	int redBits;
	int greenBits;
	int blueBits;
	int alphaBits;
	int depthBits;
	int stencilBits;
};

class View : public Ref {
public:
	static EventHeader onClipboard;
	static EventHeader onBackground;
	static EventHeader onFocus;

	View();
	virtual ~View();

	virtual bool init(Instance *, PresentationDevice *);

	virtual void end() = 0;
	virtual bool isVkReady() const = 0;
	virtual void swapBuffers() = 0;

	virtual void setIMEKeyboardState(bool open) = 0;
	virtual bool windowShouldClose() const = 0;

	virtual double getTimeCounter() const = 0;

	virtual void setAnimationInterval(double) = 0;

	virtual bool run(Application *, Rc<Director>, const Callback<bool(double)> &) = 0;

	virtual void pollEvents();

	virtual void setCursorVisible(bool isVisible) { }

	virtual int getDpi() const;
	virtual float getDensity() const;

	virtual const Size & getScreenSize() const;
	virtual void setScreenSize(float width, float height);

	virtual void setViewName(const StringView & viewname);
	virtual StringView getViewName() const;

	virtual void handleTouchesBegin(int num, intptr_t ids[], float xs[], float ys[]);
	virtual void handleTouchesMove(int num, intptr_t ids[], float xs[], float ys[]);
	virtual void handleTouchesEnd(int num, intptr_t ids[], float xs[], float ys[]);
	virtual void handleTouchesCancel(int num, intptr_t ids[], float xs[], float ys[]);

	virtual void enableOffscreenContext();
	virtual void disableOffscreenContext();

	virtual void setClipboardString(const StringView &);
	virtual StringView getClipboardString() const;

	virtual ScreenOrientation getScreenOrientation() const;

	virtual bool isTouchDevice() const;
	virtual bool hasFocus() const;
	virtual bool isInBackground() const;

	Instance *getInstance() const { return _instance; }

	virtual void selectPresentationOptions(Instance::PresentationOptions &opts) const = 0;

	void lock() { _glSync.lock(); }
	void unlock() { _glSync.unlock(); }
	bool try_lock() { return _glSync.try_lock(); }

	void resetFrame() { _dropFrameDelay.clear(); _glSyncVar.notify_all(); }

public: /* render-on-demand engine */
	// virtual void requestRender();
	// virtual void framePerformed();

protected:
	Size _screenSize;

	int _dpi = 96;
	float _density = 1.0f;

    ScreenOrientation _orientation = ScreenOrientation::Landscape;

	bool _isTouchDevice = false;
	bool _inBackground = false;
	bool _hasFocus = true;
	std::atomic_flag _dropFrameDelay;

	String _viewName;

	Rc<Instance> _instance; // api instance
	Rc<PresentationDevice> _device; // logical presentation device
	Rc<PresentationLoop> _loop;

	std::mutex _glSync;
	std::condition_variable _glSyncVar;
};

}

#endif
