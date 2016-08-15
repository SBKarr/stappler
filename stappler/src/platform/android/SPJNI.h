
#include "SPDefine.h"
#include "SPDefine.h"

#if (ANDROID)

#include "android/asset_manager.h"
#include "android/asset_manager_jni.h"
#include "platform/android/jni/JniHelper.h"

NS_SP_BEGIN

namespace spjni {

enum class Service {
	DownloadManager,
	Clipboard,
	Device,
	StoreKit,
	PlayServices,
};

void init();

jclass getClassID(JNIEnv *pEnv, const std::string &className);
jclass getClassID(JNIEnv *pEnv, jobject obj);
jmethodID getMethodID(JNIEnv *pEnv, jclass classID, const std::string &methodName, const std::string &paramCode);
jmethodID getStaticMethodID(JNIEnv *pEnv, jclass classID, const std::string &methodName, const std::string &paramCode);
JNIEnv *getJniEnv();

struct JObjectLocalRef {
	JObjectLocalRef();
	JObjectLocalRef(JNIEnv *env, jobject obj);
	JObjectLocalRef(jobject obj);
	~JObjectLocalRef();
	JObjectLocalRef(const JObjectLocalRef & other);
	JObjectLocalRef(JObjectLocalRef && other);
	JObjectLocalRef & operator = (const JObjectLocalRef & other);
	JObjectLocalRef & operator = (JObjectLocalRef && other);
	JObjectLocalRef & operator = (const nullptr_t &);
	void clear();

	operator bool () const;
	operator jobject ();

	JNIEnv *_env = nullptr;
	jobject _obj = nullptr;
};

JObjectLocalRef getActivity(JNIEnv *pEnv = nullptr);
JObjectLocalRef getService(Service serv, JNIEnv *pEnv = nullptr);

AAssetManager * getAssetManager(JNIEnv *pEnv = nullptr);

std::string jStringToStdString(JNIEnv *env, jstring jStr);
std::u16string jStringToStdU16String(JNIEnv *env, jstring jStr);

void attachJniEnv(JNIEnv *pEnv);
void printClassName(JNIEnv *pEnv, jclass classID, jobject object);

void finalize();

};

NS_SP_END

#endif
