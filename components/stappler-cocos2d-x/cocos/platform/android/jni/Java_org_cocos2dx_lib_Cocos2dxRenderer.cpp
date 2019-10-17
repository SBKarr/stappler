
#if (ANDROID)

#include "base/CCEventDispatcher.h"
#include "base/CCDirector.h"
#include "base/CCEventType.h"
#include "base/CCEventCustom.h"
#include "platform/CCApplication.h"
#include "platform/CCFileUtils.h"
#include "JniHelper.h"

#include <jni.h>

using namespace cocos2d;

extern "C" {

    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeRender(JNIEnv* env) {
        cocos2d::Director::getInstance()->mainLoop();
        stappler::platform::render::_framePerformed();
    }

    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeOnPause() {
    	if (auto app = Application::getInstance()) {
    		app->applicationDidEnterBackground();
            cocos2d::EventCustom backgroundEvent(EVENT_COME_TO_BACKGROUND);
            cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&backgroundEvent);
    	}
    }

    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeOnResume() {
    	if (auto app = Application::getInstance()) {
    		if (Director::getInstance()->getOpenGLView()) {
    			app->applicationWillEnterForeground();
        	}
    	}
    }

    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInsertText(JNIEnv* env, jobject thiz, jstring text) {

    }

    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeDeleteBackward(JNIEnv* env, jobject thiz) {

    }

    JNIEXPORT jstring JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeGetContentText() {
        JNIEnv * env = 0;
        if (JniHelper::getJavaVM()->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK || ! env) {
            return 0;
        }
        return env->NewStringUTF("");
    }
}

#endif
