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
#include "SPPlatform.h"
#include "SPIME.h"
#include "SPJNI.h"
#include "SPThread.h"

#if (ANDROID)

NS_SP_PLATFORM_BEGIN

namespace ime {
	void _updateCursor(uint32_t pos, uint32_t len) {
		auto env = spjni::getJniEnv();
		auto activity = spjni::getActivity();
		auto activityClass = spjni::getClassID(env, activity);
		if (auto updateCursor = spjni::getMethodID(env, activityClass, "updateCursor", "(II)V")) {
			env->CallVoidMethod(activity, updateCursor, (int)pos, (int)len);
		}
	}

	void _updateText(const std::u16string &str, uint32_t pos, uint32_t len) {
		auto env = spjni::getJniEnv();
		auto activity = spjni::getActivity();
		auto activityClass = spjni::getClassID(env, activity);
		if (auto updateText = spjni::getMethodID(env, activityClass, "updateText", "(Ljava/lang/String;II)V")) {
			jstring jstr = env->NewString((const jchar*)str.c_str(), str.length());
			env->CallVoidMethod(activity, updateText, jstr, (int)pos, (int)len);
			env->DeleteLocalRef(jstr);
		}
	}

	void _runWithText(const std::u16string &str, uint32_t pos, uint32_t len) {
		auto env = spjni::getJniEnv();
		auto activity = spjni::getActivity();
		auto activityClass = spjni::getClassID(env, activity);
		if (auto runInput = spjni::getMethodID(env, activityClass, "runInput", "(Ljava/lang/String;II)V")) {
			jstring jstr = env->NewString((const jchar*)str.c_str(), str.length());
			env->CallVoidMethod(activity, runInput, jstr, (int)pos, (int)len);
			env->DeleteLocalRef(jstr);
		}
	}

	void _cancel() {
		auto env = spjni::getJniEnv();
		auto activity = spjni::getActivity();
		auto activityClass = spjni::getClassID(env, activity);
		if (auto cancelInput = spjni::getMethodID(env, activityClass, "cancelInput", "()V")) {
			env->CallVoidMethod(activity, cancelInput);
		}
	}
}

NS_SP_PLATFORM_END

NS_SP_EXTERN_BEGIN

void Java_org_cocos2dx_lib_Cocos2dxGLSurfaceView_nativeTextChanged(JNIEnv *pEnv, jobject self, jstring jtext, jint start, jint len) {
	std::u16string text = stappler::spjni::jStringToStdU16String(pEnv, jtext);
	stappler::Thread::onMainThread([text, start, len] () {
		stappler::platform::ime::native::textChanged(text, start, len);
	});
	stappler::platform::render::_requestRender();
}

void Java_org_cocos2dx_lib_Cocos2dxGLSurfaceView_onInputEnabled(JNIEnv *pEnv, jobject self, jboolean value) {
	stappler::log::format("IME", "onInputEnabled %d", value);
	stappler::Thread::onMainThread([value] () {
		stappler::log::format("IME", "Thread onInputEnabled %d", value);
		stappler::platform::ime::native::setInputEnabled(value);
	});
	stappler::platform::render::_requestRender();
}

void Java_org_cocos2dx_lib_Cocos2dxGLSurfaceView_onKeyboardNotification(JNIEnv *pEnv, jobject self, jboolean value, jfloat width, jfloat height) {
	stappler::Thread::onMainThread([value, width, height] () {
		if (value) {
			stappler::platform::ime::native::onKeyboardShow(cocos2d::Rect(0, 0, width, height), 0);
		} else {
			stappler::platform::ime::native::onKeyboardHide(0);
		}
	});
	stappler::platform::render::_requestRender();
}

void Java_org_cocos2dx_lib_Cocos2dxGLSurfaceView_onCancelInput(JNIEnv *pEnv, jobject self) {
	stappler::log::text("IME", "onCancelInput");
	stappler::Thread::onMainThread([] () {
		stappler::log::text("IME", "Thread onCancelInput");
		stappler::platform::ime::native::cancel();
	});
	stappler::platform::render::_requestRender();
}

NS_SP_EXTERN_END

#endif
