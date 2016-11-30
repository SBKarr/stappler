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

#ifndef __stappler__SPScreen__
#define __stappler__SPScreen__

#include "SPDefine.h"

#include "SPEventHeader.h"
#include "SPEventHandler.h"
#include "SPScreenOrientation.h"
#include "math/CCGeometry.h"

NS_SP_BEGIN

/*
 Screen class provides possibilities to control:
 - screen orientation
 - design resolution
 - on-demand rendering
 - status bar (iOS)
 - immersive fullscreen (Android)
 */

class Screen : public cocos2d::Ref {
public:
	enum class DeviceType {
		SmallPhone = 1, // dp height < 426
		NormalPhone,
		LargePhone, // dp width >= 400
		SmallTablet, // dp width >= 540
		LargeTablet, // dp width >= 660
	};

	static EventHeader onOrientation;
	static EventHeader onTransition;

	static Screen *getInstance();

	/* Device screen size in pixels */
    const cocos2d::Size &getScreenSize() const { return _screenSize; }
	/* Current appliation viewport (design resolution) */
	const cocos2d::Size &getViewportSize() const { return _viewportSize; }
	const cocos2d::Size &getDesignResolutionSize() const { return _designResolution; }
	const cocos2d::Size &getFrameSize() const { return _frameSize; }

	float getRealScale() const { return _realScale; }
	const cocos2d::Size &getRealSize() const { return _realSize; }
	cocos2d::Size getCurrentRealSize() const;

	int getDPI();
	float getSizeInInches();

	float getDensity() const { return _density; }
    const cocos2d::Size &getScreenSizeInDP() const { return _screenSizeInDP; }
    DeviceType getDeviceType() const { return _device; }
    inline bool deviceIsTablet() const { return (int)_device >= (int)DeviceType::SmallTablet; }

public:	/* screen resolution and orientation control */
	void initScreenResolution();
	void setOrientation(const ScreenOrientation &o);

	const ScreenOrientation &getOrientation();
	const ScreenOrientation &retriveCurrentOrientation();

public:	/* status bar */
    enum class StatusBarColor : int {
        Light = 1,
        Black = 2,
    };

	float getStatusBarHeight() const;
	float getHeightScale() const;

    void setStatusBarEnabled(bool enabled, bool animated = false);
	bool isStatusBarEnabled() const;

    void setStatusBarColor(StatusBarColor color);

public: /* render-on-demand engine */
    void requestRender();
    void framePerformed();

public:
	bool isInTransition() const { return _isInTransition; }
	void setTransition(float duration);
	float getTransitionDuration() const { return _transitionTime; }

	void update(float dt);

	void setNextFrameSize(const cocos2d::Size &size);
protected:
	Screen();

    void updateDesignResolution(const ScreenOrientation &o);
	void updateOrientation(const ScreenOrientation &o, cocos2d::Scene *pScene = nullptr);

	cocos2d::Size _viewportSize;
	cocos2d::Size _screenSize;
	cocos2d::Size _frameSize;
	cocos2d::Size _designResolution;
	cocos2d::Size _realSize;
	cocos2d::Size _nextFrameSize;

    ScreenOrientation _orientation;

	bool _statusBarEnabled = true;
	bool _isInTransition = false;
	float _statusBarHeight = 0;
	float _scale = 1.0f;
	float _realScale = 1.0f;

	float _transitionTime = 0.0f;

	float _density = 1.0f;
	cocos2d::Size _screenSizeInDP;
	DeviceType _device;
};

NS_SP_END

#endif /* defined(__stappler__SPScreen__) */
