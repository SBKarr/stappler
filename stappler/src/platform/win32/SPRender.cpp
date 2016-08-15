//
//  SPRender.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPPlatform.h"

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
}

NS_SP_PLATFORM_END

//#endif
