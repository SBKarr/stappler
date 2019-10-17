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

#if (ANDROID)

namespace stappler::platform::clipboard {
	bool _isAvailable() {
		auto env = spjni::getJniEnv();
		if (auto clipboard = spjni::getService(spjni::Service::Clipboard, env)) {
			auto clipboardClass = clipboard.get_class();
			auto method = spjni::getMethodID(env, clipboardClass, "isClipboardAvailable", "()Z");
			if (method) {
				return env->CallBooleanMethod(clipboard, method);
			}
		}
		return false;
	}

	std::string _getString() {
		auto env = spjni::getJniEnv();
		if (auto clipboard = spjni::getService(spjni::Service::Clipboard, env)) {
			auto clipboardClass = clipboard.get_class();
			auto method = spjni::getMethodID(env, clipboardClass, "getStringFromClipboard", "()Ljava/lang/String;");
			if (method) {
				auto strObj = (jstring)env->CallObjectMethod(clipboard, method);
				if (strObj) {
					return spjni::jStringToStdString(env, strObj);
				}
			}
		}
		return "";
	}
	void _copyString(const std::string &value) {
		auto env = spjni::getJniEnv();
		if (auto clipboard = spjni::getService(spjni::Service::Clipboard, env)) {
			auto clipboardClass = clipboard.get_class();
			auto method = spjni::getMethodID(env, clipboardClass, "copyStringToClipboard", "(Ljava/lang/String;)V");
			if (method) {
				auto str = env->NewStringUTF(value.c_str());
				env->CallVoidMethod(clipboard, method, str);
				env->DeleteLocalRef(str);
			}
		}
	}
}

#endif
