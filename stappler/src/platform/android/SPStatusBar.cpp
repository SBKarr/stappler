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
#include "SPJNI.h"
#include "math/CCGeometry.h"
#include "SPDevice.h"

#if (ANDROID)

NS_SP_PLATFORM_BEGIN

namespace statusbar {
	float _height = 0.0f;
	float _commonHeight = 0.0f;

	void collapseStatusBar() {
		if (_height != 0.0f) {
			Device::onStatusBar(Device::getInstance(), 0.0f);
			_height = 0.0f;
		}
	}

	void restoreStatusBar() {
		if (_height != _commonHeight) {
			Device::onStatusBar(Device::getInstance(), _commonHeight);
			_height = _commonHeight;
		}
	}

	void _setEnabled(bool enabled) {
		if (_height == 0.0f) {
			return;
		}

		auto env = spjni::getJniEnv();
		auto activity = spjni::getActivity(env);
		auto activityClass = env->GetObjectClass(activity);
		auto method = spjni::getMethodID(env, activityClass, "setStatusBarEnabled", "(Z)V");
		if (method) {
			env->CallVoidMethod(activity, method, enabled);
		}
	}
	bool _isEnabled() {
		if (_height == 0.0f) {
			return false;
		}

		auto env = spjni::getJniEnv();
		auto activity = spjni::getActivity(env);
		auto activityClass = env->GetObjectClass(activity);
		auto method = spjni::getMethodID(env, activityClass, "isStatusBarEnabled", "()Z");
		if (method) {
			return env->CallBooleanMethod(activity, method);
		}
		return false;
	}
	void _setColor(_StatusBarColor color) {
		bool lightColor = false;
		if (color == _StatusBarColor::Light) {
			lightColor = true;
		}

		auto env = spjni::getJniEnv();
		auto activity = spjni::getActivity(env);
		auto activityClass = env->GetObjectClass(activity);
		auto method = spjni::getMethodID(env, activityClass, "setStatusBarLight", "(Z)V");
		if (method) {
			env->CallVoidMethod(activity, method, lightColor);
		}
	}
	float _getHeight(const cocos2d::Size &screenSize, bool isTablet) {
		return _height;
	}
}

NS_SP_PLATFORM_END

NS_SP_EXTERN_BEGIN

void Java_org_stappler_Activity_setStatusBarHeight(JNIEnv *env, jobject activity, jint height) {
	stappler::platform::statusbar::_height = stappler::platform::statusbar::_commonHeight = height;
}

NS_SP_EXTERN_END

#endif
