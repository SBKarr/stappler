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
#include "SPJNI.h"
#include "SPDevice.h"
#include "SPScreen.h"
#include "SPPlatform.h"
#include "platform/CCCommon.h"
#include "base/CCDirector.h"
#include "SPThreadManager.h"

#include "base/CCEventDispatcher.h"
#include "base/CCEventCustom.h"
#include "base/CCEventType.h"
#include "renderer/CCGLProgramCache.h"
#include "renderer/CCTextureCache.h"
#include "renderer/ccGLStateCache.h"

#if (ANDROID)

#include "CCGLViewImpl-android.h"
#include "CCApplication-android.h"
#include "platform/android/jni/JniHelper.h"

NS_SP_PLATFORM_BEGIN

namespace render {
	void _stop();
	void _start();
}

NS_SP_PLATFORM_END

NS_SP_BEGIN

namespace spjni {
	JObjectLocalRef::JObjectLocalRef() { }
	JObjectLocalRef::JObjectLocalRef(JNIEnv *env, jobject obj) {
		if (env) {
			_obj = env->NewLocalRef(obj);
			_env = env;
		}
	}

	JObjectLocalRef::JObjectLocalRef(jobject obj) : JObjectLocalRef(getJniEnv(), obj) {}
	JObjectLocalRef::~JObjectLocalRef() {
		clear();
	}

	JObjectLocalRef::JObjectLocalRef(const JObjectLocalRef & other) {
		_env = other._env;
		if (other && _env) {
			_obj = _env->NewLocalRef(other._obj);
		}
	}
	JObjectLocalRef::JObjectLocalRef(JObjectLocalRef && other) {
		_env = other._env;
		_obj = other._obj;

		other._env = nullptr;
		other._obj = nullptr;
	}

	JObjectLocalRef & JObjectLocalRef::operator = (const JObjectLocalRef & other) {
		clear();
		_env = other._env;
		if (other && _env) {
			_obj = _env->NewLocalRef(other._obj);
		}
		return *this;
	}
	JObjectLocalRef & JObjectLocalRef::operator = (JObjectLocalRef && other) {
		clear();
		_env = other._env;
		_obj = other._obj;

		other._env = nullptr;
		other._obj = nullptr;
		return *this;
	}

	JObjectLocalRef & JObjectLocalRef::operator = (const nullptr_t &) {
		clear();
		return *this;
	}

	void JObjectLocalRef::clear() {
		if (_env && _obj) {
			_env->DeleteLocalRef(_obj);
			_env = nullptr;
			_obj = nullptr;
		}
	}

	JObjectLocalRef::operator bool () const { return _obj != nullptr; }
	JObjectLocalRef::operator jobject () { return _obj; }


	struct SharedActivity {

		SharedActivity() {}
		~SharedActivity() {
			auto env = getJniEnv();
			if (env) {
				if (_activity) {
					env->DeleteGlobalRef(_activity);
					_activity = nullptr;
				}
				if (_activityClass) {
					env->DeleteGlobalRef(_activityClass);
					_activityClass = nullptr;
				}
				if (_classLoader) {
					env->DeleteGlobalRef(_classLoader);
					_classLoader = nullptr;
				}
			}
		}

		void init() {
			auto pEnv = getJniEnv();

			auto activityClass =  pEnv->FindClass("org/stappler/Activity");
		    auto classClass = pEnv->GetObjectClass(activityClass);
		    auto getClassLoaderMethod = pEnv->GetMethodID(classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");

		    auto classLoader = pEnv->CallObjectMethod(classClass, getClassLoaderMethod);
		    pEnv->ExceptionClear();

		    if (classLoader) {
			    _classLoader = pEnv->NewGlobalRef(classLoader);
			    auto classLoaderClass = pEnv->GetObjectClass(_classLoader);

			    _findClassMethod = pEnv->GetMethodID(classLoaderClass, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");
		    }
		}

		void set(JNIEnv *env, jobject activity) {
			_mutex.lock();

			if (_activity) {
				env->DeleteGlobalRef(_activity);
				_activity = nullptr;
			}

			if (_activityClass) {
				env->DeleteGlobalRef(_activityClass);
				_activityClass = nullptr;
			}

			if (_classLoader) {
				env->DeleteGlobalRef(_classLoader);
				_classLoader = nullptr;
				_findClassMethod = nullptr;
			}

			if (activity) {
				_activity = env->NewGlobalRef(activity);
				_activityClass = (jclass)env->NewGlobalRef(env->GetObjectClass(_activity));

			    auto classClass = env->GetObjectClass(_activityClass);
			    auto getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
			    auto classLoader = env->CallObjectMethod(classClass, getClassLoaderMethod);
			    auto classLoaderClass = env->GetObjectClass(classLoader);

	            env->ExceptionClear();

	            if (classLoader) {
				    _classLoader = env->NewGlobalRef(classLoader);
				    _findClassMethod = env->GetMethodID(classLoaderClass, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	            }
			}

			_mutex.unlock();
		}

		JObjectLocalRef get(JNIEnv *pEnv = nullptr) {
			if (!pEnv) {
				pEnv = getJniEnv();
			}

			JObjectLocalRef ret;
			_mutex.lock();
			if (_activity) {
				ret = JObjectLocalRef(pEnv, _activity);
			}
			_mutex.unlock();
			return ret;
		}

		jclass getClassId(JNIEnv *pEnv, const std::string &className) {
			jclass ret = nullptr;

			if (!pEnv) {
				pEnv = getJniEnv();
			}

			_mutex.lock();
			if (_classLoader && _findClassMethod) {
				auto nameStr = pEnv->NewStringUTF(className.c_str());
				ret = static_cast<jclass>(pEnv->CallObjectMethod(_classLoader, _findClassMethod, nameStr));
				pEnv->DeleteLocalRef(nameStr);

				if (pEnv->ExceptionCheck()) {
					ret = nullptr;
				}
			}
			_mutex.unlock();

			return ret;
		}

		jobject _classLoader = nullptr;
		jmethodID _findClassMethod = nullptr;

		jclass _activityClass = nullptr;
		jobject _activity = nullptr;
		std::mutex _mutex;
	};

	SharedActivity _activity;

	int s_screenWidth = 0;
	int s_screenHeight = 0;

	pthread_key_t jniEnvKey;
};

NS_SP_END

NS_SP_EXTERN_BEGIN

void Java_org_stappler_Activity_nativeOnCreate(JNIEnv *env, jobject activity) {
	stappler::log::format("JNI", "%s", __FUNCTION__);
	stappler::spjni::attachJniEnv(env);
	if (activity) {
		stappler::spjni::_activity.set(env, activity);
	}
	stappler::platform::render::_start();
}

void Java_org_stappler_Activity_nativeOnDestroy(JNIEnv *env, jobject activity) {
	stappler::log::format("JNI", "%s", __FUNCTION__);
	stappler::platform::render::_stop();
	stappler::spjni::_activity.set(env, nullptr);
}
void Java_org_stappler_Activity_nativeOnStart(JNIEnv *env, jobject activity) {
	stappler::spjni::attachJniEnv(env);
	stappler::log::format("JNI", "%s", __FUNCTION__);
}
void Java_org_stappler_Activity_nativeOnRestart(JNIEnv *env, jobject activity) {
	stappler::spjni::attachJniEnv(env);
	stappler::log::format("JNI", "%s", __FUNCTION__);
}
void Java_org_stappler_Activity_nativeOnStop(JNIEnv *env, jobject activity) {
	stappler::log::format("JNI", "%s", __FUNCTION__);
}
void Java_org_stappler_Activity_nativeOnPause(JNIEnv *env, jobject activity) {
	stappler::log::format("JNI", "%s", __FUNCTION__);
}
void Java_org_stappler_Activity_nativeOnResume(JNIEnv *env, jobject activity) {
	stappler::spjni::attachJniEnv(env);
	stappler::platform::render::_requestRender();
	stappler::log::format("JNI", "%s", __FUNCTION__);
}
void Java_org_stappler_gcm_IntentService_onRemoteNotification(JNIEnv *env, jobject obj) {
	stappler::spjni::attachJniEnv(env);
	stappler::Device::onRemoteNotification(stappler::Device::getInstance());
}


void sp_android_terminate () {
	stappler::log::text("JNI", "Crash on exceprion");
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
#ifdef DEBUG
	std::set_terminate(&sp_android_terminate);
#endif
    cocos2d::JniHelper::setJavaVM(vm);
    stappler::spjni::init();
    return JNI_VERSION_1_4;
}

void Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInit(JNIEnv*  env, jobject thiz, jint w, jint h) {
	stappler::spjni::attachJniEnv(env);
    auto director = cocos2d::Director::getInstance();
    auto glview = director->getOpenGLView();
    if (!glview) {
        stappler::ThreadManager::getInstance()->update(0);
        glview = cocos2d::GLViewImpl::create("Android app");
        glview->setFrameSize(w, h);
        director->setOpenGLView(glview);

        auto app = cocos2d::Application::getInstance();
        if (app) {
        	app->run();
        } else {
        	stappler::log::text("JNI", "No AppDelegate initialized");
        }
    } else {
        stappler::ThreadManager::getInstance()->update(0);
        glview->setFrameSize(w, h);
        cocos2d::GL::invalidateStateCache();
        cocos2d::GLProgramCache::getInstance()->reloadDefaultGLPrograms();
        cocos2d::VolatileTextureMgr::reloadAllTextures();

        cocos2d::EventCustom recreatedEvent(EVENT_RENDERER_RECREATED);
        director->getEventDispatcher()->dispatchEvent(&recreatedEvent);
        director->setGLDefaultValues();
    }
}

void Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeOnSurfaceChanged(JNIEnv*  env, jobject thiz, jint w, jint h) {
    auto view = cocos2d::Director::sharedDirector()->getOpenGLView();
    auto screen = stappler::Screen::getInstance();
    view->setFrameSize(w, h);
    auto o = ((w < h)? stappler::ScreenOrientation::Portrait:stappler::ScreenOrientation::Landscape);
    screen->setOrientation(o);
    screen->requestRender();

    cocos2d::Application::getInstance()->applicationScreenSizeChanged(w, h);
}

NS_SP_EXTERN_END

NS_SP_BEGIN;

namespace spjni {

void init() {
	pthread_key_create(&jniEnvKey, nullptr);

	_activity.init();
}

jclass getClassID(JNIEnv *pEnv, const std::string &className) {
	return _activity.getClassId(pEnv, className);
}

jclass getClassID(JNIEnv *pEnv, jobject obj) {
	if (obj) {
	    return pEnv->GetObjectClass(obj);
	}
	return nullptr;
}

jmethodID getMethodID(JNIEnv *pEnv, jclass classID, const std::string &methodName,
		const std::string &paramCode) {
	jmethodID methodID = pEnv->GetMethodID(classID, methodName.c_str(), paramCode.c_str());
	if (!methodID) {
		stappler::log::format("JNI", "Failed to find method id of %s", methodName.c_str());
	}
	return methodID;
}

jmethodID getStaticMethodID(JNIEnv *pEnv, jclass classID, const std::string &methodName,
		const std::string &paramCode) {
	jmethodID methodID = pEnv->GetStaticMethodID(classID, methodName.c_str(), paramCode.c_str());
	if (!methodID) {
		stappler::log::format("JNI", "Failed to find static method id of %s", methodName.c_str());
	}
	return methodID;
}

void attachJniEnv(JNIEnv *pEnv) {
	pthread_setspecific(jniEnvKey, pEnv);
}

void printClassName(JNIEnv *pEnv, jclass classID, jobject object) {
	jmethodID getClassMethod = pEnv->GetMethodID(classID, "getClass",
			"()Ljava/lang/Class;");
	jobject classObject = pEnv->CallObjectMethod(object, getClassMethod);
	jclass classClass = pEnv->GetObjectClass(classObject);

	// Find the getName() method on the class object
	jmethodID getNameMethod = pEnv->GetMethodID(classClass, "getName",
			"()Ljava/lang/String;");
	jstring strObj = (jstring) pEnv->CallObjectMethod(classObject,
			getNameMethod);

	// Now get the c string from the java jstring object
	const char* str = pEnv->GetStringUTFChars(strObj, NULL);
	stappler::log::format("JNI", "ClassName: %s", str);
	pEnv->ReleaseStringUTFChars(strObj, str);
}

JNIEnv *getJniEnv() {
	JNIEnv *env = (JNIEnv *)pthread_getspecific(jniEnvKey);
	if (env) {
		return env;
	}

	JavaVM *jvm = cocos2d::JniHelper::getJavaVM();
    jint ret = jvm->GetEnv((void**)&env, JNI_VERSION_1_4);

    switch (ret) {
    case JNI_OK :
        // Success!
        pthread_setspecific(jniEnvKey, env);
        return env;
    default :
    	stappler::log::format("JNI", "Failed to get the environment using GetEnv()");
        return nullptr;
    }

	return env;
}

JObjectLocalRef getActivity(JNIEnv *pEnv) {
	return _activity.get(pEnv);
}

JObjectLocalRef getService(Service serv, JNIEnv *pEnv) {
	if (!pEnv) {
		pEnv = getJniEnv();
	}

	auto activity = getActivity(pEnv);
	if (!activity) {
		return nullptr;
	}

	std::string method;
	std::string param;
	switch (serv) {
	case Service::Clipboard:
		method = "getClipboard";
		param = "()Lorg/stappler/Clipboard;";
		break;
	case Service::Device:
		method = "getDevice";
		param = "()Lorg/stappler/Device;";
		break;
	case Service::DownloadManager:
		method = "getDownloadManager";
		param = "()Lorg/stappler/downloads/Manager;";
		break;
	case Service::PlayServices:
		method = "getPlayServices";
		param = "()Lorg/stappler/gcm/PlayServices;";
		break;
	case Service::StoreKit:
		method = "getStoreKit";
		param = "()Lorg/stappler/StoreKit;";
		break;
	default:
		break;
	}

	if (method.empty() || param.empty()) {
		log::text("SPJNI", "No method or parameter");
		return nullptr;
	}

	jobject ret = nullptr;
	auto methodId = getMethodID(pEnv, getClassID(pEnv, activity), method, param);
	if (methodId) {
		ret = pEnv->CallObjectMethod(activity, methodId);
		if (!ret) {
			log::format("SPJNI", "Empty result for method: %s (%s)", method.c_str(), param.c_str());
		}
	} else {
		log::format("SPJNI", "No methodID: %s (%s)", method.c_str(), param.c_str());
	}

	return JObjectLocalRef(pEnv, ret);
}

AAssetManager * getAssetManager(JNIEnv *pEnv) {
	if (!pEnv) {
		pEnv = getJniEnv();
	}

	auto activity = getActivity();
	if (!activity) {
		return nullptr;
	}

	std::string method = "getAssets";
	std::string param = "()Landroid/content/res/AssetManager;";

	jobject ret = nullptr;
	auto methodId = getMethodID(pEnv, getClassID(pEnv, activity), method, param);
	if (methodId) {
		ret = pEnv->CallObjectMethod(activity, methodId);
	}

	if (ret) {
		return AAssetManager_fromJava(pEnv, ret);
	}

	return nullptr;
}

std::string jStringToStdString(JNIEnv *env, jstring jStr) {
	if (jStr == nullptr) {
		return "";
	}

	const char *chars = env->GetStringUTFChars(jStr, NULL);
	std::string ret(chars, (size_t)env->GetStringUTFLength(jStr));
	env->ReleaseStringUTFChars(jStr, chars);

	return ret;
}

std::u16string jStringToStdU16String(JNIEnv *env, jstring jStr) {
	if (jStr == nullptr) {
		return u"";
	}

	const char16_t *chars = (const char16_t *)env->GetStringChars(jStr, NULL);
	std::u16string ret(chars, env->GetStringLength(jStr));
	env->ReleaseStringChars(jStr, (const jchar*)chars);

	return ret;
}

}

NS_SP_END

#endif
