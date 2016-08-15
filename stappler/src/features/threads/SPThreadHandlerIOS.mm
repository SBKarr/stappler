//
//  SPTaskManagerIOS.c
//  chieftime-federal
//
//  Created by SBKarr on 6/27/13.
//
//

#include "SPThreadManager.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
NS_SP_BEGIN
void ThreadHandlerInterface::workerThread(ThreadHandlerInterface *tm) {
    @autoreleasepool {
		tm->initializeThread();
		tm->threadInit();
        while (tm->worker()) {
        }
		tm->finalizeThread();
    }
}
NS_SP_END
#endif