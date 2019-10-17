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
#include "SPScreenOrientation.h"
#include "SPJNI.h"
#include "SPDevice.h"
#include "SPThread.h"

#include "math/CCGeometry.h"
#include "platform/CCDevice.h"

#if (ANDROID)

namespace stappler::platform::device {
	bool _valIsTablet = false;
	std::string _valUserAgent;
	std::string _valDeviceIdentifier;
	std::string _valBundleName;
	std::string _valUserLanguage;
	std::string _valApplicationName;
	std::string _valApplicationVersion;
	std::string _valUrl;
	std::string _valAction;
	cocos2d::Size _valScreenSize;
	cocos2d::Size _valCurrentSize;
	std::string _valDeviceToken;
	float _valDensity = 0.0f;

	bool _isTablet() {
		return _valIsTablet;
	}
	std::string _userAgent() {
		return _valUserAgent;
	}
	std::string _deviceIdentifier() {
		return _valDeviceIdentifier;
	}
	std::string _bundleName() {
		return _valBundleName;
	}
	std::string _userLanguage() {
		return _valUserLanguage;
	}
	std::string _applicationName() {
		return _valApplicationName;
	}
	std::string _applicationVersion() {
		return _valApplicationVersion;
	}
	Size _screenSize() {
		return _valScreenSize;
	}
	Size _viewportSize(const Size &screenSize, bool isTablet) {
		if (_valCurrentSize.width > _valCurrentSize.height) {
			return cocos2d::Size(_valCurrentSize.height, _valCurrentSize.width);
		}
		return _valCurrentSize;
	}
	float _designScale(const Size &screenSize, bool isTablet) {
		return 1.0f;
	}
	const ScreenOrientation &_currentDeviceOrientation() {
		if (_valCurrentSize.width > _valCurrentSize.height) {
			return ScreenOrientation::Landscape;
		} else {
			return ScreenOrientation::Portrait;
		}
	}
	std::pair<uint64_t, uint64_t> _diskSpace() {
		auto env = spjni::getJniEnv();
		auto device = spjni::getService(spjni::Service::Device, env);
		auto deviceClass = device.get_class();

		uint64_t totalSpace = 0;
	    uint64_t totalFreeSpace = 0;

		if (auto getTotalSpace = spjni::getMethodID(env, deviceClass, "getTotalSpace", "()J")) {
			totalSpace = env->CallLongMethod(device, getTotalSpace);
		}

		if (auto getFreeSpace = spjni::getMethodID(env, deviceClass, "getFreeSpace", "()J")) {
			totalFreeSpace = env->CallLongMethod(device, getFreeSpace);
		}

	    return std::make_pair(totalSpace, totalFreeSpace);
	}
	int _dpi() {
		return cocos2d::Device::getDPI();
	}

	void _onDirectorStarted() {
		JNIEnv *env = spjni::getJniEnv();
		auto &activity = spjni::getActivity(env);
		if (!activity.empty()) {
			auto activityClass = activity.get_class();
			jmethodID directorStarted = spjni::getMethodID(env, activityClass, "directorStarted", "()V");
			if (directorStarted) {
				env->CallVoidMethod(activity, directorStarted);
			}
		}
	}

	float _density() {
		if (_valDensity == 0.0f) {
			auto dpi = _dpi();
			if (dpi >= 560) {
				return 4.0f;
			} else if (dpi >= 400) {
				return 3.0f;
			} else if (dpi >= 264) {
				return 2.0f;
			} else if (dpi >= 220) {
				return 1.5f;
			} else if (dpi >= 180) {
				return 1.25f;
			} else if (dpi >= 132) {
				return 1.0f;
			} else {
				return 0.75f;
			}
		} else {
			return _valDensity;
		}
	}
}

using namespace stappler::platform::device;
using namespace stappler::spjni;

NS_SP_EXTERN_BEGIN

void Java_org_stappler_Device_setIsTablet(JNIEnv *env, jobject activity, jboolean val) {
	_valIsTablet = val;
}
void Java_org_stappler_Device_setUserAgent(JNIEnv *env, jobject activity, jstring val) {
	_valUserAgent = jStringToStdString(env, val);
}
void Java_org_stappler_Device_setDeviceIdentifier(JNIEnv *env, jobject activity, jstring val) {
	_valDeviceIdentifier = jStringToStdString(env, val);
}
void Java_org_stappler_Device_setBundleName(JNIEnv *env, jobject activity, jstring val) {
	_valBundleName = jStringToStdString(env, val);
	std::replace( _valBundleName.begin(), _valBundleName.end(), '_', '-');
}
void Java_org_stappler_Device_setUserLanguage(JNIEnv *env, jobject activity, jstring val) {
	_valUserLanguage = jStringToStdString(env, val);
}
void Java_org_stappler_Device_setApplicationName(JNIEnv *env, jobject activity, jstring val) {
	_valApplicationName = jStringToStdString(env, val);
}
void Java_org_stappler_Device_setApplicationVersion(JNIEnv *env, jobject activity, jstring val) {
	_valApplicationVersion = jStringToStdString(env, val);
}
void Java_org_stappler_Device_setScreenSize(JNIEnv *env, jobject activity, jint width, jint height) {
	_valScreenSize = cocos2d::Size(width, height);
}
void Java_org_stappler_Device_setCurrentSize(JNIEnv *env, jobject activity, jint width, jint height) {
	_valCurrentSize = cocos2d::Size(width, height);
}
void Java_org_stappler_Device_setDeviceToken(JNIEnv *env, jobject activity, jstring val) {
	auto str = jStringToStdString(env, val);
	stappler::Thread::onMainThread([str] () {
		_valDeviceToken = str;
		stappler::Device::getInstance()->registerDeviceToken(_valDeviceToken);
	});
}
void Java_org_stappler_gcm_MessagingService_setDeviceToken(JNIEnv *env, jobject activity, jstring val) {
	auto str = jStringToStdString(env, val);
	stappler::Thread::onMainThread([str] () {
		_valDeviceToken = str;
		stappler::Device::getInstance()->registerDeviceToken(_valDeviceToken);
	});
}
void Java_org_stappler_Device_setDensity(JNIEnv *env, jobject activity, jfloat val) {
	_valDensity = val;
}

void Java_org_stappler_Activity_setInitialUrl(JNIEnv *env, jobject activity, jstring jurl, jstring action) {
	auto url = jStringToStdString(env, jurl);
	_valAction = jStringToStdString(env, action);
	stappler::Thread::onMainThread([url] () {
		_valUrl = url;
		stappler::Device::getInstance()->setLaunchUrl(url);
	});
}

void Java_org_stappler_Activity_processUrl(JNIEnv *env, jobject activity, jstring jurl, jstring action) {
	auto url = jStringToStdString(env, jurl);
	_valAction = jStringToStdString(env, action);
	stappler::Thread::onMainThread([url] () {
		_valUrl = url;
		stappler::Device::getInstance()->processLaunchUrl(url);
	});
}

NS_SP_EXTERN_END

#endif
