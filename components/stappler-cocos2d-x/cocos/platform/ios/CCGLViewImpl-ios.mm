/****************************************************************************
 Copyright (c) 2010-2012 cocos2d-x.org
 Copyright (c) 2013-2014 Chukong Technologies Inc.

 http://www.cocos2d-x.org

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
 ****************************************************************************/

#include "platform/CCPlatformConfig.h"
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS

#import <UIKit/UIKit.h>

#include "CCEAGLView-ios.h"
#include "CCDirectorCaller-ios.h"
#include "CCGLViewImpl-ios.h"
#include "base/CCTouch.h"

NS_CC_BEGIN

struct CCEAGLViewStorage {
	CCEAGLView *view;
};

struct CCNSStringStorage {
	NSString *string;
};

CCNSStringStorage * GLViewImpl::_pixelFormat = nullptr;
int GLViewImpl::_depthFormat = GL_DEPTH_COMPONENT16;

GLViewImpl* GLViewImpl::createWithEAGLView(CCEAGLViewStorage *eaglview) {
    auto ret = new (std::nothrow) GLViewImpl;
    if(ret && ret->initWithEAGLView(eaglview)) {
        ret->autorelease();
        return ret;
    }

    return nullptr;
}

GLViewImpl* GLViewImpl::create(const std::string& viewName)
{
    auto ret = new (std::nothrow) GLViewImpl;
    if(ret && ret->initWithFullScreen(viewName)) {
        ret->autorelease();
        return ret;
    }

    return nullptr;
}

GLViewImpl* GLViewImpl::createWithRect(const std::string& viewName, Rect rect, float frameZoomFactor)
{
    auto ret = new (std::nothrow) GLViewImpl;
    if(ret && ret->initWithRect(viewName, rect, frameZoomFactor)) {
        ret->autorelease();
        return ret;
    }

    return nullptr;
}

GLViewImpl* GLViewImpl::createWithFullScreen(const std::string& viewName)
{
    auto ret = new (std::nothrow) GLViewImpl();
    if(ret && ret->initWithFullScreen(viewName)) {
        ret->autorelease();
        return ret;
    }

    return nullptr;
}

void GLViewImpl::convertAttrs()
{
    if(_glContextAttrs.redBits==8 && _glContextAttrs.greenBits==8 && _glContextAttrs.blueBits==8 && _glContextAttrs.alphaBits==8)
    {
    	if (_pixelFormat) {
    		delete _pixelFormat;
    	}
        _pixelFormat = new CCNSStringStorage{kEAGLColorFormatRGBA8};
    }
    if(_glContextAttrs.depthBits==24 && _glContextAttrs.stencilBits==8)
    {
        _depthFormat = GL_DEPTH24_STENCIL8_OES;
    }
    if (!_pixelFormat) {
    	_pixelFormat = new CCNSStringStorage{kEAGLColorFormatRGB565};
    }
}

GLViewImpl::GLViewImpl()
{
}

GLViewImpl::~GLViewImpl()
{
    if (_eaglview) {
    	delete _eaglview;
    	_eaglview = nullptr;
    }
}

bool GLViewImpl::initWithEAGLView(CCEAGLViewStorage *eaglview)
{
    _eaglview = eaglview;
    CCEAGLView *glview = _eaglview->view;

    _screenSize.width = _designResolutionSize.width = [glview getWidth];
    _screenSize.height = _designResolutionSize.height = [glview getHeight];
//    _scaleX = _scaleY = [glview contentScaleFactor];

    return true;
}

bool GLViewImpl::initWithRect(const std::string& viewName, Rect rect, float frameZoomFactor)
{
    CGRect r = CGRectMake(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
    convertAttrs();
    CCEAGLView *eaglview = [[CCEAGLView alloc] initWithFrame: r
                                       pixelFormat: _pixelFormat->string
                                       depthFormat: _depthFormat
                                preserveBackbuffer: NO
                                        sharegroup: nil
                                     multiSampling: NO
                                   numberOfSamples: 0];
    
    [eaglview setMultipleTouchEnabled:YES];

    _screenSize.width = _designResolutionSize.width = [eaglview getWidth];
    _screenSize.height = _designResolutionSize.height = [eaglview getHeight];
//    _scaleX = _scaleY = [eaglview contentScaleFactor];

    _eaglview = new CCEAGLViewStorage{eaglview};

    return true;
}

bool GLViewImpl::initWithFullScreen(const std::string& viewName)
{
    CGRect rect = [[UIScreen mainScreen] bounds];
    Rect r;
    r.origin.x = rect.origin.x;
    r.origin.y = rect.origin.y;
    r.size.width = rect.size.width;
    r.size.height = rect.size.height;

    return initWithRect(viewName, r, 1);
}

bool GLViewImpl::isOpenGLReady()
{
    return _eaglview != nullptr;
}

bool GLViewImpl::setContentScaleFactor(float contentScaleFactor)
{
    CC_ASSERT(_resolutionPolicy == ResolutionPolicy::UNKNOWN); // cannot enable retina mode
    _scaleX = _scaleY = contentScaleFactor;

    CCEAGLView *eaglview = _eaglview->view;
    [eaglview setNeedsLayout];

    return true;
}

float GLViewImpl::getContentScaleFactor() const
{
    CCEAGLView *eaglview = _eaglview->view;

    float scaleFactor = [eaglview contentScaleFactor];

//    CCASSERT(scaleFactor == _scaleX == _scaleY, "Logic error in GLView::getContentScaleFactor");

    return scaleFactor;
}

void GLViewImpl::end()
{
    [CCDirectorCaller destroy];
    
    // destroy EAGLView
    CCEAGLView *eaglview = _eaglview->view;

    [eaglview removeFromSuperview];
    //[eaglview release];
}


void GLViewImpl::swapBuffers()
{
    CCEAGLView *eaglview = _eaglview->view;
    [eaglview swapBuffers];
}

void GLViewImpl::setIMEKeyboardState(bool open)
{
    CCEAGLView *eaglview = _eaglview->view;

    if (open)
    {
        [eaglview becomeFirstResponder];
    }
    else
    {
        [eaglview resignFirstResponder];
    }
}

void GLViewImpl::enableOffscreenContext() {
    [EAGLContext setCurrentContext: (_eaglview->view).sharedContext];
}

void GLViewImpl::disableOffscreenContext() {
    [EAGLContext setCurrentContext: nil];
}

void* GLViewImpl::getEAGLView() const {
	return (__bridge void *)_eaglview->view;
}

NS_CC_END

#endif // CC_PLATFOR_IOS

