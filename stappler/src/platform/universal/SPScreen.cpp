//
//  SPScreen.cpp
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#ifndef SP_RESTRICT

#include "SPScreen.h"
#include "SPDevice.h"
#include "SPPlatform.h"
#include "base/CCDirector.h"
#include "base/CCScheduler.h"
#include "platform/CCGLView.h"
#include "SPEvent.h"

#include "2d/CCActionInterval.h"
#include "2d/CCTransition.h"

using cocos2d::Size;
using cocos2d::Vec2;
using cocos2d::Vec3;
using cocos2d::Vec4;

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
	_realSize = cocos2d::Size(_screenSize.width / scale, _screenSize.height / scale);

	_density = platform::device::_density();

	_screenSizeInDP = cocos2d::Size(_screenSize.width / _density, _screenSize.height / _density);

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

cocos2d::Size Screen::getCurrentRealSize() const {
	return cocos2d::Size(_designResolution.width / _realScale, _designResolution.height / _realScale);
}

cocos2d::Scene * Screen_getCurrentScene() {
    cocos2d::Scene *scene = cocos2d::Director::getInstance()->getNextScene();
    if (!scene) {
        scene = cocos2d::Director::getInstance()->getRunningScene();
    }

    cocos2d::TransitionScene *transitionScene = dynamic_cast<cocos2d::TransitionScene *>(scene);
    if (transitionScene) {
    	scene = transitionScene->getInScene();
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
	cocos2d::Size currentFrameSize = view->getFrameSize();

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

	if (!_nextFrameSize.equals(cocos2d::Size::ZERO)) {
	    //view->_screenSize = Size(_nextFrameSize.width, _nextFrameSize.height);
		auto o = stappler::Screen::getInstance()->retriveCurrentOrientation();

		if (o == _orientation) {
	        updateDesignResolution(o);
			onOrientation(this, _orientation.asInt64());
	    } else {
			stappler::Screen::getInstance()->setOrientation(o);
	    }
		_nextFrameSize = cocos2d::Size::ZERO;
	}

	if (!_isInTransition && _transitionTime == 0.0f && _nextFrameSize.equals(cocos2d::Size::ZERO)) {
		cocos2d::Director::getInstance()->getScheduler()->unscheduleUpdate(this);
	}
}

void Screen::setNextFrameSize(const cocos2d::Size &size) {
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
