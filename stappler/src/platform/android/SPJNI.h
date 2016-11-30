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
