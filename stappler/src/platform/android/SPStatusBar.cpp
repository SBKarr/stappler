//
//  SPStatusBar.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

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
