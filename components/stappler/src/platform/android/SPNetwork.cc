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
#include "SPThread.h"
#include "SPJNI.h"

#if (ANDROID)

namespace stappler::platform::network {
	using namespace stappler::spjni;

	bool _init = false;
	std::function<void(bool isOnline)> _callback;

	void _setNetworkCallback(const Function<void(bool isOnline)> &callback) {
		_callback = callback;
	}
	bool _isNetworkOnline() {
		auto env = getJniEnv();
		if (auto device = getService(Service::Device, env)) {
			auto deviceClass = getClassID(env, device);
			if (auto isNetworkOnline = getMethodID(env, deviceClass, "isNetworkOnline", "()Z")) {
				return env->CallBooleanMethod(device, isNetworkOnline);
			}
		}
		return true;
	}
}

NS_SP_EXTERN_BEGIN

void Java_org_stappler_NetworkReceiver_onConnectivityChanged(JNIEnv *env, jobject unused) {
	if (auto device = getService(Service::Device, env)) {
		auto deviceClass = getClassID(env, device);
		if (auto isNetworkOnline = getMethodID(env, deviceClass, "isNetworkOnline", "()Z")) {
			bool ret = env->CallBooleanMethod(device, isNetworkOnline);
			stappler::Thread::onMainThread([ret] () {
				if (stappler::platform::network::_callback) {
					stappler::platform::network::_callback(ret);
				}
			});
		}
	}
}

NS_SP_EXTERN_END

#endif
