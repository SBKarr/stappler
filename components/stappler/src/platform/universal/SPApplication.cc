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
#include "SPApplication.h"
#include "base/CCDirector.h"
#include "platform/CCFileUtils.h"
#include "platform/CCGLView.h"
#include "SPDevice.h"
#include "SPScreen.h"
#include "SPDynamicBatchScene.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    #include "platform/ios/CCGLViewImpl-ios.h"
#endif // CC_TARGET_PLATFORM == CC_PLATFORM_IOS

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    #include "platform/android/CCGLViewImpl-android.h"
#endif // CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

#if (CC_TARGET_PLATFORM == CC_PLATFORM_BLACKBERRY)
    #include "platform/blackberry/CCGLViewImpl.h"
#endif // CC_TARGET_PLATFORM == CC_PLATFORM_BLACKBERRY

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
    #include "platform/desktop/CCGLViewImpl-desktop.h"
#endif // CC_TARGET_PLATFORM == CC_PLATFORM_WIN32

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
    #include "platform/desktop/CCGLViewImpl-desktop.h"
#endif // CC_TARGET_PLATFORM == CC_PLATFORM_MAC

#if (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
    #include "platform/desktop/CCGLViewImpl-desktop.h"
#endif // CC_TARGET_PLATFORM == CC_PLATFORM_LINUX

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
	#include "platform/winrt/CCGLViewImpl-winrt.h"
#endif // CC_TARGET_PLATFORM == CC_PLATFORM_WINRT

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WP8)
	#include "platform/wp8/CCGLViewImpl-wp8.h"
#endif // CC_TARGET_PLATFORM == CC_PLATFORM_WP8

NS_SP_BEGIN

#ifndef SP_RESTRICT

static std::atomic<bool> s_atomicApplicationExists;

bool Application::isApplicationExists() {
	return s_atomicApplicationExists.load();
}

Application::Application() {
	memory::pool::initialize();
	_applicationPool = memory::pool::createTagged(memory::pool::acquire(), "Application");
	_framePool = memory::pool::createTagged(_applicationPool, "ApplicationFrame");
	memory::pool::push(_applicationPool);
	s_atomicApplicationExists.store(true);
}

Application::~Application() {
	s_atomicApplicationExists.store(false);
	memory::pool::pop();
	memory::pool::destroy(_applicationPool);
	memory::pool::terminate();
}

bool Application::applicationDidFinishLaunching() {
	Device::getInstance();

    auto director = cocos2d::Director::getInstance();
    auto glview = director->getOpenGLView();
    if (!glview) {
        glview = cocos2d::GLViewImpl::create(stappler::Device::getInstance()->getBundleName());
        director->setOpenGLView(glview);
        Screen::getInstance()->initScreenResolution();
    }

	director->setProjection(cocos2d::Director::Projection::_EUCLID);

    return true;
}

void Application::applicationDidEnterBackground() {
	Device::getInstance()->didEnterBackground();
    cocos2d::Director::getInstance()->stopAnimation();
}

void Application::applicationWillEnterForeground() {
	cocos2d::Director::getInstance()->startAnimation();
	Device::getInstance()->willEnterForeground();
}

void Application::applicationFocusGained() {
	Device::getInstance()->onFocusGained();
}
void Application::applicationFocusLost() {
	Device::getInstance()->onFocusLost();
}

void Application::applicationDidReceiveMemoryWarning() {
	if (auto scene = dynamic_cast<DynamicBatchScene *>(cocos2d::Director::getInstance()->getRunningScene())) {
		scene->clearCachedMaterials(true);
	}
	cocos2d::Director::getInstance()->purgeCachedData();
}

void Application::applicationFrameBegin() {
	memory::pool::push(_framePool);
}
void Application::applicationFrameEnd() {
	memory::pool::pop();
	memory::pool::clear(_framePool);
}

#endif

NS_SP_END
