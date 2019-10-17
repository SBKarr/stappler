// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

namespace stappler::platform::render {
	void _stop();
	void _start();
}

NS_SP_BEGIN

namespace spjni {
	std::atomic<uint64_t> _resetTime;

	JClassRef::JClassRef() { }
	JClassRef::JClassRef(JNIEnv *env, jclass c) : _env(env), _class(c) {
		if (!_env) {
			_env = spjni::getJniEnv();
		}
	}
	JClassRef::JClassRef(jclass c) : _env(spjni::getJniEnv()), _class(c) { }
	JClassRef::~JClassRef() {
		if (_env && _class) {
			_env->DeleteLocalRef(_class);
		}
	}

	JClassRef::JClassRef(JClassRef && other) : _env(other._env), _class(other._class) {
		other._env = nullptr;
		other._class = nullptr;
	}
	JClassRef & JClassRef::operator = (JClassRef && other) {
		_env = other._env;
		_class = other._class;

		other._env = nullptr;
		other._class = nullptr;
		return *this;
	}

	JClassRef::operator jclass () const {
		return _class;
	}

	JObjectRef::JObjectRef() : _time(Time::now()) { }
	JObjectRef::JObjectRef(JNIEnv *env, jobject obj, Type type) : _time(Time::now()) {
		if (env) {
			_type = type;
			_env = env;
			_obj = newRef(env, obj, type);
		}
	}
	JObjectRef::JObjectRef(jobject obj, Type type) : JObjectRef(getJniEnv(), obj, type) {}
	JObjectRef::~JObjectRef() {
		clear();
	}
	JObjectRef::JObjectRef(const JObjectRef & other, Type type) : _env(other._env), _type(type), _time(Time::now()) {
		if (!other.empty() && _env) {
			_obj = newRef(_env, other._obj, type);
		}
	}
	JObjectRef::JObjectRef(const JObjectRef & other) : JObjectRef(other, other._type) { }
	JObjectRef::JObjectRef(JObjectRef && other) : _env(other._env), _obj(other._obj), _type(other._type), _time(other._time) {
		other._env = nullptr;
		other._obj = nullptr;
	}

	JObjectRef & JObjectRef::operator = (const JObjectRef & other) {
		clear();
		_env = other._env;
		_type = other._type;
		_time = Time::now();
		if (!other.empty() && _env) {
			_obj = newRef(_env, other._obj, _type);
		}
		return *this;
	}
	JObjectRef & JObjectRef::operator = (JObjectRef && other) {
		clear();
		_env = other._env;
		_obj = other._obj;
		_type = other._type;
		_time = other._time;

		other._env = nullptr;
		other._obj = nullptr;
		return *this;
	}
	JObjectRef & JObjectRef::operator = (const nullptr_t &) {
		clear();
		return *this;
	}

	jobject JObjectRef::newRef(JNIEnv *env, jobject obj, Type t) {
		switch (t) {
		case Local: return env->NewLocalRef(obj); break;
		case Global: return env->NewGlobalRef(obj); break;
		case WeakGlobal: return env->NewWeakGlobalRef(obj); break;
		}
		return nullptr;
	}
	void JObjectRef::clear() {
		if (_obj != nullptr && _env != nullptr) {
			switch (_type) {
				case Local: return _env->DeleteLocalRef(_obj); break;
				case Global: return _env->DeleteGlobalRef(_obj); break;
				case WeakGlobal: return _env->DeleteWeakGlobalRef(_obj); break;
			}
		}
		_env = nullptr;
		_obj = nullptr;
	}
	void JObjectRef::raw_clear() {
		_env = nullptr;
		_obj = nullptr;
	}

	bool JObjectRef::empty() const {
		return _obj == nullptr || _env == nullptr || (_type == WeakGlobal && _env->IsSameObject(_obj, NULL));
	}

	const Time & JObjectRef::time() const {
		return _time;
	}

	JClassRef JObjectRef::get_class() const {
		if (!_obj || !_env) {
			log::format("JObjectRef", "fail to perform get_class with env (%p) and obj (%p)", _env, _obj);
		}
		return JClassRef(_env, _env->GetObjectClass(_obj));
	}

	JObjectRef::operator bool () const { return !empty(); }
	JObjectRef::operator jobject () const { return _obj; }

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
#if DEBUG
			log::text("JNI", "SharedActivity::init");
#endif
			auto pEnv = getJniEnv();

			auto activityClass =  pEnv->FindClass("org/stappler/Activity");
		    auto classClass = pEnv->GetObjectClass(activityClass);
		    auto getClassLoaderMethod = pEnv->GetMethodID(classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");

		    auto classLoader = pEnv->CallObjectMethod(classClass, getClassLoaderMethod);
		    pEnv->DeleteLocalRef(activityClass);
		    pEnv->DeleteLocalRef(classClass);
		    pEnv->ExceptionClear();

		    if (classLoader) {
			    _classLoader = pEnv->NewGlobalRef(classLoader);
			    auto classLoaderClass = pEnv->GetObjectClass(_classLoader);

			    _findClassMethod = pEnv->GetMethodID(classLoaderClass, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");
			    pEnv->DeleteLocalRef(classLoaderClass);
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

			_resetTime.store(Time::now().toMicroseconds());
		}

		JObjectRef get(JNIEnv *pEnv = nullptr, JObjectRef::Type type = JObjectRef::Local) {
			if (!pEnv) {
				pEnv = getJniEnv();
			}

			JObjectRef ret;
			_mutex.lock();
			if (_activity) {
				ret = JObjectRef(pEnv, _activity, type);
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
	pthread_key_t jniActivityKey;
	pthread_key_t jniAssetManagerKey;
};

NS_SP_END

NS_SP_EXTERN_BEGIN

void Java_org_stappler_Activity_nativeOnPreCreate(JNIEnv *env, jobject activity) {
    stappler::log::format("JNI", "%s", __FUNCTION__);
    stappler::spjni::attachJniEnv(env);
    if (activity) {
        stappler::spjni::_activity.set(env, activity);
    }
}

void Java_org_stappler_Activity_nativeOnCreate(JNIEnv *env, jobject activity) {
	stappler::log::format("JNI", "%s", __FUNCTION__);
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
void Java_org_stappler_gcm_MessagingService_onRemoteNotification(JNIEnv *env, jobject obj) {
	stappler::spjni::attachJniEnv(env);
	stappler::Device::onRemoteNotification(stappler::Device::getInstance());
}


void sp_android_terminate () {
	stappler::log::text("JNI", "Crash on exceprion");
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
#ifdef DEBUG
	std::set_terminate(&sp_android_terminate);
	stappler::log::format("JNI", "%s", __FUNCTION__);
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
    auto view = cocos2d::Director::getInstance()->getOpenGLView();
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

void spjni_activity_key_destructor(void *ptr) {
	if (ptr) {
		JObjectRef *ref = (JObjectRef *)ptr;
		ref->raw_clear(); // causes memory leak, but JNI memory cleared when JNIEnv is reloaded
		delete ref;
	}
}

void init() {
	pthread_key_create(&jniEnvKey, nullptr);
	pthread_key_create(&jniActivityKey, &spjni_activity_key_destructor);
	pthread_key_create(&jniAssetManagerKey, &spjni_activity_key_destructor);

	_activity.init();
}

JClassRef getClassID(JNIEnv *pEnv, const std::string &className) {
	return JClassRef(pEnv, _activity.getClassId(pEnv, className));
}

JClassRef getClassID(JNIEnv *pEnv, jobject obj) {
	if (obj) {
	    return JClassRef(pEnv, pEnv->GetObjectClass(obj));
	}
	return JClassRef();
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
    case JNI_OK:
        // Success!
        pthread_setspecific(jniEnvKey, env);
        if (!env) {
        	stappler::log::text("JNI", "Failed to get the environment from Java VM");
        }
        return env;
    default:
    	stappler::log::text("JNI", "Failed to get the environment using GetEnv()");
        return nullptr;
    }

	return env;
}

const JObjectRef &getActivity(JNIEnv *pEnv) {
	JObjectRef *activity = (JObjectRef *)pthread_getspecific(jniActivityKey);
	if (activity == nullptr) {
		activity = new JObjectRef(_activity.get(pEnv, JObjectRef::Type::WeakGlobal));
        pthread_setspecific(jniActivityKey, activity);
	} else {
		uint64_t t = _resetTime.load();
		if (t > activity->time()) {
			*activity = JObjectRef(_activity.get(pEnv, JObjectRef::Type::WeakGlobal));
		}
	}

	if (activity->empty()) {
		*activity = _activity.get(pEnv, JObjectRef::Type::WeakGlobal);
	}

	if (activity->empty()) {
    	stappler::log::text("JNI", "Failed to get main Activity");
	}

	return *activity;
}

JObjectRef getService(Service serv, JNIEnv *pEnv, JObjectRef::Type type) {
	if (!pEnv) {
		pEnv = getJniEnv();
	}

	auto & activity = getActivity(pEnv);
	if (activity.empty()) {
		log::text("SPJNI", "No activity for getService");
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
	case Service::StoreKit:
		method = "getStoreKit";
		param = "()Lorg/stappler/StoreKit;";
		break;
	case Service::AssetManager:
		method = "getAssets";
		param = "()Landroid/content/res/AssetManager;";
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

	JObjectRef r(pEnv, ret, type);
	pEnv->DeleteLocalRef(ret);
	return r;
}

AAssetManager * getAssetManager(JNIEnv *pEnv) {
	if (!pEnv) {
		pEnv = getJniEnv();
	}

	JObjectRef *assetManager = (JObjectRef *)pthread_getspecific(jniAssetManagerKey);
	if (assetManager == nullptr) {
		assetManager = new JObjectRef(getService(Service::AssetManager, pEnv, JObjectRef::Type::WeakGlobal));
        pthread_setspecific(jniAssetManagerKey, assetManager);
	} else {
		uint64_t t = _resetTime.load();
		if (t > assetManager->time()) {
			*assetManager = JObjectRef(getService(Service::AssetManager, pEnv, JObjectRef::Type::WeakGlobal));
		}
	}

	if (assetManager->empty()) {
		*assetManager = getService(Service::AssetManager, pEnv, JObjectRef::Type::WeakGlobal);
	}

	if (!assetManager->empty()) {
		return AAssetManager_fromJava(pEnv, *assetManager);
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
