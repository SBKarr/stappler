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
#include "SPDevice.h"

#if (ANDROID)

NS_SP_PLATFORM_BEGIN

#define SP_TERMINATED_DATA(view) (view.terminated()?view.data():view.str().data())

namespace interaction {
	void _goToUrl(const StringView &url, bool external) {
		auto env = spjni::getJniEnv();
		auto & activity = spjni::getActivity(env);
		auto activityClass = activity.get_class();
		jmethodID openWebURL = spjni::getMethodID(env, activityClass, "openWebURL", "(Ljava/lang/String;)V");
		if (openWebURL) {
			jstring jurl = env->NewStringUTF(SP_TERMINATED_DATA(url));
			env->CallVoidMethod(activity, openWebURL, jurl);
			env->DeleteLocalRef(jurl);
		}
	}
	void _makePhoneCall(const StringView &str) {
		auto env = spjni::getJniEnv();
		auto & activity = spjni::getActivity(env);
		auto activityClass = activity.get_class();
		jmethodID openWebURL = spjni::getMethodID(env, activityClass, "openWebURL", "(Ljava/lang/String;)V");
		if (openWebURL) {
			jstring jurl = env->NewStringUTF(SP_TERMINATED_DATA(str));
			env->CallVoidMethod(activity, openWebURL, jurl);
			env->DeleteLocalRef(jurl);
		}
	}
	void _mailTo(const StringView &address) {
		auto env = spjni::getJniEnv();
		auto & activity = spjni::getActivity(env);
		auto activityClass = activity.get_class();
		jmethodID openWebURL = spjni::getMethodID(env, activityClass, "openWebURL", "(Ljava/lang/String;)V");
		if (openWebURL) {
			jstring jurl = env->NewStringUTF(SP_TERMINATED_DATA(address));
			env->CallVoidMethod(activity, openWebURL, jurl);
			env->DeleteLocalRef(jurl);
		}
	}
	void _backKey() {
		auto env = spjni::getJniEnv();
		auto & activity = spjni::getActivity(env);
		auto activityClass = activity.get_class();
		jmethodID perfromBackPressed = spjni::getMethodID(env, activityClass, "perfromBackPressed",
				"()V");
		if (perfromBackPressed) {
			env->CallVoidMethod(activity, perfromBackPressed);
		}
	}
	void _notification(const StringView &title, const StringView &text) {
		auto env = spjni::getJniEnv();
		auto & activity = spjni::getActivity(env);
		auto activityClass = activity.get_class();
		jmethodID openWebURL = spjni::getMethodID(env, activityClass, "showNotification",
				"(Ljava/lang/String;Ljava/lang/String;)V");
		if (openWebURL) {
			jstring jtitle = env->NewStringUTF(SP_TERMINATED_DATA(title));
			jstring jtext = env->NewStringUTF(SP_TERMINATED_DATA(text));
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

#undef SP_TERMINATED_DATA

NS_SP_PLATFORM_END

#endif
