//
//  SPRender.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPPlatform.h"
#include "SPThreadManager.h"

#include "CCGLViewImpl-desktop.h"
#include "base/CCDirector.h"

//#if (LINUX)

NS_SP_PLATFORM_BEGIN

namespace render {
	void _init() { }
	void _requestRender() { }
	void _framePerformed() { }
	void _blockRendering() { }
	void _unblockRendering() { }

#ifdef SP_RESTRICT
	bool _enableOffscreenContext() { return false; }
	void _disableOffscreenContext() { }
#else
	bool _enableOffscreenContext() {
		if (!ThreadManager::getInstance()->isMainThread()) {
			auto view = cocos2d::Director::getInstance()->getOpenGLView();
			static_cast<cocos2d::GLViewImpl *>(view)->enableOffscreenContext();
			return true;
		}
		return false;
	}
	void _disableOffscreenContext() {
		auto view = cocos2d::Director::getInstance()->getOpenGLView();
		static_cast<cocos2d::GLViewImpl *>(view)->disableOffscreenContext();
	}
#endif


}

NS_SP_PLATFORM_END

//#endif
