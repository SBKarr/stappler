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

#ifndef SP_RESTRICT

#include "SPScreen.h"
#include "SPDevice.h"
#include "SPPlatform.h"
#include "base/CCDirector.h"
#include "base/CCScheduler.h"
#include "platform/CCGLView.h"
#include "SPEvent.h"

#include "2d/CCActionInterval.h"

NS_SP_BEGIN

SP_DECLARE_EVENT_CLASS(Screen, onOrientation);
SP_DECLARE_EVENT_CLASS(Screen, onTransition);

static Screen *s_screen = nullptr;
Screen *Screen::getInstance() {
	if (!s_screen) {
		s_screen = new Screen();
	}
	return s_screen;
}

Screen::Screen() {
	CCAssert(s_screen == nullptr, "Screen should follow singleton pattern");
	bool isTablet = platform::device::_isTablet();
	_screenSize = platform::device::_screenSize();
	_scale = platform::device::_designScale(_screenSize, isTablet);
	_viewportSize = platform::device::_viewportSize(_screenSize, isTablet);
	_statusBarHeight = platform::statusbar::_getHeight(_screenSize, isTablet);
    _orientation = retriveCurrentOrientation();

	float inches = getSizeInInches();
	float scale = (float)getDPI() / (254.0f * 1.75f);
	float diff = (5.0f + getSizeInInches()) * 0.1f;
	if (diff > 1.0f) {
		scale *= diff;
	}

	_realScale = scale;
	_realSize = Size(_screenSize.width / scale, _screenSize.height / scale);

	_density = platform::device::_density();

	_screenSizeInDP = Size(_screenSize.width / _density, _screenSize.height / _density);

	if (_screenSizeInDP.height < 426.0f) {
		_device = DeviceType::SmallPhone;
	} else if (_screenSizeInDP.width >= 660) {
		_device = DeviceType::LargeTablet;
	} else if (_screenSizeInDP.width >= 540 || inches > 6.0) {
		_device = DeviceType::SmallTablet;
	} else if (_screenSizeInDP.width >= 400) {
		_device = DeviceType::LargePhone;
	} else {
		_device = DeviceType::NormalPhone;
	}
}

Size Screen::getCurrentRealSize() const {
	return Size(_designResolution.width / _realScale, _designResolution.height / _realScale);
}

cocos2d::Scene * Screen_getCurrentScene() {
    cocos2d::Scene *scene = cocos2d::Director::getInstance()->getNextScene();
    if (!scene) {
        scene = cocos2d::Director::getInstance()->getRunningScene();
    }

    return scene;
}

void Screen::initScreenResolution() {
    _orientation = retriveCurrentOrientation();
    updateDesignResolution(_orientation);
	onOrientation(this, _orientation.asInt64());
}

void Screen::updateDesignResolution(const ScreenOrientation &o) {
	cocos2d::GLView *view = cocos2d::Director::getInstance()->getOpenGLView();
	Size currentFrameSize = view->getFrameSize();

    if (o.isLandscape()) {
		if (currentFrameSize.width < currentFrameSize.height) {
			currentFrameSize = Size(currentFrameSize.height, currentFrameSize.width);
		}
    } else {
		if (currentFrameSize.width > currentFrameSize.height) {
			currentFrameSize = Size(currentFrameSize.height, currentFrameSize.width);
		}
    }

    Size designResolution(currentFrameSize.width, currentFrameSize.height);

    _frameSize = (currentFrameSize.width > currentFrameSize.height)?Size(currentFrameSize.height, currentFrameSize.width):currentFrameSize;

	_designResolution = designResolution;
	view->setFrameSize(currentFrameSize.width, currentFrameSize.height);
    view->setDesignResolutionSize(designResolution.width, designResolution.height, ResolutionPolicy::SHOW_ALL);
}

void Screen::setOrientation(const ScreenOrientation &o) {
	if (o == _orientation) {
        updateDesignResolution(o);
        return;
    }

    if (!_orientation.isTransition(o)) {
        _orientation = o;
        updateDesignResolution(o);
        return;
    }

	cocos2d::Scene *scene = Screen_getCurrentScene();

    updateDesignResolution(o);
    updateOrientation(o, scene);
}

void Screen::updateOrientation(const ScreenOrientation &o, cocos2d::Scene *pScene) {
	cocos2d::GLView *view = cocos2d::Director::getInstance()->getOpenGLView();
    Size currentFrameSize = view->getFrameSize();

    _frameSize = (currentFrameSize.width > currentFrameSize.height)?Size(currentFrameSize.height, currentFrameSize.width):currentFrameSize;

    _orientation = o;

	onOrientation(this, _orientation.asInt64());
}

float Screen::getStatusBarHeight() const {
	bool isTablet = platform::device::_isTablet();
	return platform::statusbar::_getHeight(_screenSize, isTablet);
}
float Screen::getHeightScale() const {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	return 1;
#else
	return _scale;
#endif
}

const ScreenOrientation &Screen::getOrientation() {
    return _orientation;
}
const ScreenOrientation &Screen::retriveCurrentOrientation() {
	return platform::device::_currentDeviceOrientation();
}

float Screen::getSizeInInches() {
	static float inches = 0;

	if (inches == 0) {
		float size = sqrtf(_screenSize.width * _screenSize.width + _screenSize.height * _screenSize.height);
		inches = size / getDPI();
	}

	return inches;
}

int Screen::getDPI() {
	return platform::device::_dpi();
}

void Screen::setStatusBarEnabled(bool enabled, bool animated) {
	platform::statusbar::_setEnabled(enabled);
}

bool Screen::isStatusBarEnabled() const {
	return platform::statusbar::_isEnabled();
}

void Screen::setStatusBarColor(StatusBarColor color) {
	platform::statusbar::_setColor((platform::statusbar::_StatusBarColor)color);
}

void Screen::setTransition(float duration) {
	if (!_isInTransition) {
		_isInTransition = true;
		_transitionTime = duration;
		onTransition(this, true);

		cocos2d::Director::getInstance()->getScheduler()->scheduleUpdate(this, 0, false);
	} else {
		if (_transitionTime < duration) {
			_transitionTime = duration;
		}
	}
}

void Screen::update(float dt) {
	if (_isInTransition) {
		_transitionTime -= dt;
		if (_transitionTime < 0) {
			_isInTransition = false;
			_transitionTime = 0;
			onTransition(this, false);
		}
	}

	if (!_nextFrameSize.equals(Size::ZERO)) {
	    //view->_screenSize = Size(_nextFrameSize.width, _nextFrameSize.height);
		auto o = stappler::Screen::getInstance()->retriveCurrentOrientation();

		if (o == _orientation) {
	        updateDesignResolution(o);
			onOrientation(this, _orientation.asInt64());
	    } else {
			stappler::Screen::getInstance()->setOrientation(o);
	    }
		_nextFrameSize = Size::ZERO;
	}

	if (!_isInTransition && _transitionTime == 0.0f && _nextFrameSize.equals(Size::ZERO)) {
		cocos2d::Director::getInstance()->getScheduler()->unscheduleUpdate(this);
	}
}

void Screen::setNextFrameSize(const Size &size) {
	cocos2d::Director::getInstance()->getScheduler()->scheduleUpdate(this, 0, false);
	_nextFrameSize = size;
}

void Screen::requestRender() {
	platform::render::_requestRender();
}

void Screen::framePerformed() {
	platform::render::_framePerformed();
}


float stappler::screen::density() {
	static float density = 0.0f;
	if (density == 0.0f) {
		density = Screen::getInstance()->getDensity();
	}
	return density;
}
int stappler::screen::dpi() {
	return Screen::getInstance()->getDPI();
}
float stappler::screen::statusBarHeight() {
	return Screen::getInstance()->getStatusBarHeight() / density();
}

NS_SP_END

#endif
