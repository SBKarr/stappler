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

#include "SPCommon.h"
#include "SPDevice.h"
#include "SPThreadManager.h"

#if ANDROID
#include <android/log.h>
#endif

NS_SP_EXT_BEGIN(log)

using sp_log_fn = void (*) (const StringView &tag, const StringView &text);

static sp_log_fn sCustomFn;

void setCustomLogFn(sp_log_fn fn) {
	sCustomFn = fn;
}

static void __log2(const StringView &tag, const StringView &text) {
	if (sCustomFn) {
		sCustomFn(tag, text);
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
	__android_log_print(ANDROID_LOG_DEBUG, tag.data(), "%s", str.c_str());
#else
	fwrite(str.c_str(), str.length(), 1, stdout);
	fflush(stdout);
#endif // platform switch
}

void __stappler_log(const StringView &tag, CustomLog::Type t, CustomLog::VA &va) {
#if DEBUG
	if (t == CustomLog::Text) {
		__log2(tag, va.text);
	} else {
		char stackBuf[1_KiB] = { 0 };

		va_list tmpList;
		va_copy(tmpList, va.format.args);
		int size = vsnprintf(stackBuf, size_t(1_KiB - 1), va.format.format, tmpList);
		va_end(tmpList);

		if (size > int(1_KiB - 1)) {
			char *buf = new char[size + 1];
			memset(buf, 0, size + 1);
			size = vsnprintf(buf, size_t(size), va.format.format, va.format.args);
			__log2(tag, StringView(buf, size));
			delete [] buf;
		} else {
			__log2(tag, StringView(stackBuf, size));
		}
	}
#endif // DEBUG
}

NS_SP_EXT_END(log)