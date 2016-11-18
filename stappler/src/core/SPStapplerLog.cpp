/*
 * SPLogDefault.cpp
 *
 *  Created on: 10 дек. 2015 г.
 *      Author: sbkarr
 */

#include "SPCommon.h"
#include "SPDevice.h"
#include "SPThreadManager.h"

#if ANDROID
#include <android/log.h>
#endif

NS_SP_EXT_BEGIN(log)

using sp_log_fn = void (*) (const char *tag, const char *text, unsigned long len);

static sp_log_fn sCustomFn;

void setCustomLogFn(sp_log_fn fn) {
	sCustomFn = fn;
}

static void __log2(const char *tag, const char *text, unsigned long len) {
	if (sCustomFn) {
		sCustomFn(tag, text, len);
	}

	StringStream stream;
#if !ANDROID
	stream << tag << ": ";
#endif

#ifndef NOCC
	stream << ThreadManager::getInstance()->getThreadInfo() << ": ";
#else
	stream << "[Log]: ";
#endif

	stream << text;
#ifndef __apple__
	stream << "\n";
#endif
	String str = stream.str();

#if ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, tag, "%s", str.c_str());
#else
	fwrite(str.c_str(), str.length(), 1, stdout);
	fflush(stdout);
#endif // platform switch
}

void __stappler_log(const char *tag, CustomLog::Type t, CustomLog::VA &va) {
#if DEBUG
	if (t == CustomLog::Text) {
		__log2(tag, va.text.text, va.text.len);
	} else {
#if __APPLE__
		va_list tmpList;
		va_copy(tmpList, va.format.args);
		int size = vsnprintf(nullptr, 0, va.format.format, tmpList);
		va_end(tmpList);

		char *buf = new char[size + 1];
		memset(buf, 0, size + 1);
		size = vsnprintf(buf, size_t(size), va.format.format, va.format.args);
		__log2(tag, buf, size);
		delete [] buf;
#else
		char stackBuf[1_KiB] = { 0 };
		int size = vsnprintf(stackBuf, size_t(1_KiB - 1), va.format.format, va.format.args);
		if (size > int(1_KiB - 1)) {
			char *buf = new char[size + 1];
			memset(buf, 0, size + 1);
			size = vsnprintf(buf, size_t(size), va.format.format, va.format.args);
			__log2(tag, buf, size);
			delete [] buf;
		} else {
			__log2(tag, stackBuf, size);
		}
#endif
	}
#endif // DEBUG
}

NS_SP_EXT_END(log)
