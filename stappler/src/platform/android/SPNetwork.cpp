//
//  SPNetwork.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPPlatform.h"
#include "SPThread.h"
#include "SPJNI.h"

#if (ANDROID)

using namespace stappler::spjni;

NS_SP_PLATFORM_BEGIN

namespace network {
	bool _init = false;
	std::function<void(bool isOnline)> _callback;

	void _setNetworkCallback(std::function<void(bool isOnline)> callback) {
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

NS_SP_PLATFORM_END

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
