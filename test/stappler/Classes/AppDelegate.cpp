// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "Material.h"
#include "AppDelegate.h"
#include "base/CCDirector.h"
#include "platform/CCFileUtils.h"
#include "platform/CCGLView.h"
#include "SPDevice.h"
#include "SPStoreKit.h"
#include "cocos2d.h"
#include "SPResource.h"
#include "Application.h"

NS_SP_BEGIN

static AppDelegate s_delegate;

AppDelegate::AppDelegate() { }

AppDelegate::~AppDelegate() { }

bool AppDelegate::applicationDidFinishLaunching() {
	Device::getInstance();

	resource::setFallbackFont("common/fonts/DejaVuSansStappler.woff");

    auto director = cocos2d::Director::getInstance();
    auto glview = director->getOpenGLView();
    if (!glview) {
        glview = cocos2d::GLViewImpl::create(stappler::Device::getInstance()->getBundleName());
        director->setOpenGLView(glview);
    }

	director->setProjection(cocos2d::Director::Projection::_EUCLID);

	(new stappler::app::Application());

    return true;
}

void AppDelegate::applicationDidEnterBackground() {
	Device::getInstance()->didEnterBackground();
    cocos2d::Director::getInstance()->stopAnimation();
}

void AppDelegate::applicationWillEnterForeground() {
	cocos2d::Director::getInstance()->startAnimation();
	Device::getInstance()->willEnterForeground();
}

NS_SP_END
