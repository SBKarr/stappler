//
//  SPInteraction.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPPlatform.h"
#include "SPJNI.h"
#include "SPDevice.h"

#if (ANDROID)

NS_SP_PLATFORM_BEGIN

namespace interaction {
	void _goToUrl(const std::string &url, bool external) {
		auto env = spjni::getJniEnv();
		auto activity = spjni::getActivity(env);
		auto activityClass = env->GetObjectClass(activity);
		jmethodID openWebURL = spjni::getMethodID(env, activityClass, "openWebURL", "(Ljava/lang/String;)V");
		if (openWebURL) {
			jstring jurl = env->NewStringUTF(url.c_str());
			env->CallVoidMethod(activity, openWebURL, jurl);
			env->DeleteLocalRef(jurl);
		}
	}
	void _makePhoneCall(const std::string &str) {
		auto env = spjni::getJniEnv();
		auto activity = spjni::getActivity(env);
		auto activityClass = env->GetObjectClass(activity);
		jmethodID openWebURL = spjni::getMethodID(env, activityClass, "openWebURL", "(Ljava/lang/String;)V");
		if (openWebURL) {
			jstring jurl = env->NewStringUTF(str.c_str());
			env->CallVoidMethod(activity, openWebURL, jurl);
			env->DeleteLocalRef(jurl);
		}
	}
	void _mailTo(const std::string &address) {
		auto env = spjni::getJniEnv();
		auto activity = spjni::getActivity(env);
		auto activityClass = env->GetObjectClass(activity);
		jmethodID openWebURL = spjni::getMethodID(env, activityClass, "openWebURL", "(Ljava/lang/String;)V");
		if (openWebURL) {
			jstring jurl = env->NewStringUTF(address.c_str());
			env->CallVoidMethod(activity, openWebURL, jurl);
			env->DeleteLocalRef(jurl);
		}
	}
	void _backKey() {
		auto env = spjni::getJniEnv();
		auto activity = spjni::getActivity(env);
		auto activityClass = env->GetObjectClass(activity);
		jmethodID perfromBackPressed = spjni::getMethodID(env, activityClass, "perfromBackPressed",
				"()V");
		if (perfromBackPressed) {
			env->CallVoidMethod(activity, perfromBackPressed);
		}
	}
	void _notification(const std::string &title, const std::string &text) {
		auto env = spjni::getJniEnv();
		auto activity = spjni::getActivity(env);
		auto activityClass = env->GetObjectClass(activity);
		jmethodID openWebURL = spjni::getMethodID(env, activityClass, "showNotification",
				"(Ljava/lang/String;Ljava/lang/String;)V");
		if (openWebURL) {
			jstring jtitle = env->NewStringUTF(title.c_str());
			jstring jtext = env->NewStringUTF(text.c_str());
			env->CallVoidMethod(activity, openWebURL, jtitle, jtext);
			env->DeleteLocalRef(jtitle);
			env->DeleteLocalRef(jtext);
		}
	}

	void _rateApplication() {
		auto device = Device::getInstance();
		std::string url("market://details?id=");
		url.append(device->getBundleName());
		Device::getInstance()->goToUrl(url);
	}
}

NS_SP_PLATFORM_END

#endif
