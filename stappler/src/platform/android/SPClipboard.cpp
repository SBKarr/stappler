//
//  SPClipboard.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPPlatform.h"
#include "SPJNI.h"

#if (ANDROID)
NS_SP_PLATFORM_BEGIN

namespace clipboard {
	bool _isAvailable() {
		auto env = spjni::getJniEnv();
		auto clipboard = spjni::getService(spjni::Service::Clipboard, env);
		auto clipboardClass = env->GetObjectClass(clipboard);
		auto method = spjni::getMethodID(env, clipboardClass, "isClipboardAvailable", "()Z");
		if (method) {
			return env->CallBooleanMethod(clipboard, method);
		}
		return false;
	}

	std::string _getString() {
		auto env = spjni::getJniEnv();
		auto clipboard = spjni::getService(spjni::Service::Clipboard, env);
		auto clipboardClass = env->GetObjectClass(clipboard);
		auto method = spjni::getMethodID(env, clipboardClass, "getStringFromClipboard", "()Ljava/lang/String;");
		if (method) {
			auto strObj = (jstring)env->CallObjectMethod(clipboard, method);
			if (strObj) {
				return spjni::jStringToStdString(env, strObj);
			}
		}
		return "";
	}
	void _copyString(const std::string &value) {
		auto env = spjni::getJniEnv();
		auto clipboard = spjni::getService(spjni::Service::Clipboard, env);
		auto clipboardClass = env->GetObjectClass(clipboard);
		auto method = spjni::getMethodID(env, clipboardClass, "copyStringToClipboard", "(Ljava/lang/String;)V");
		if (method) {
			auto str = env->NewStringUTF(value.c_str());
			env->CallVoidMethod(clipboard, method, str);
			env->DeleteLocalRef(str);
		}
	}
}

NS_SP_PLATFORM_END
#endif
