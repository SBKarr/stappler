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
#include "SPLog.h"

NS_SP_EXT_BEGIN(log)

static const constexpr int MAX_LOG_FUNC = 16;

static CustomLog::log_fn s_logFuncArr[MAX_LOG_FUNC] = { 0 };
static std::atomic<int> s_logFuncCount;
static std::mutex s_logFuncMutex;

static void DefaultLog2(const StringView &tag, const StringView &text) {
#if MSYS
	std::cout << '[' << tag << "] " << text << '\n';
	std::cout.flush();
#else
	std::cerr << '[' << tag << "] " << text << '\n';
	std::cerr.flush();
#endif
}

static void DefaultLog(const StringView &tag, CustomLog::Type t, CustomLog::VA &va) {
	if (t == CustomLog::Text) {
		DefaultLog2(tag, va.text);
	} else {
		char stackBuf[1_KiB];
		va_list tmpList;
		va_copy(tmpList, va.format.args);
		int size = vsnprintf(stackBuf, size_t(1_KiB - 1), va.format.format, tmpList);
		va_end(tmpList);
		if (size > int(1_KiB - 1)) {
			char *buf = new char[size + 1];
			size = vsnprintf(buf, size_t(size), va.format.format, va.format.args);
			DefaultLog2(tag, StringView(buf, size));
			delete [] buf;
		} else if (size >= 0) {
			DefaultLog2(tag, StringView(stackBuf, size));
		} else {
			DefaultLog2(tag, "Log error");
		}
	}
}

static void __log3(const StringView tag, CustomLog::Type t, CustomLog::VA &va) {
	int count = s_logFuncCount.load();
	if (count == 0) {
		DefaultLog(tag, t, va);
	} else {
		s_logFuncMutex.lock();
		count = s_logFuncCount.load();
		for (int i = 0; i < count; i++) {
			s_logFuncArr[i](tag, t, va);
		}
		s_logFuncMutex.unlock();
	}
}

static void CustomLog_insert(CustomLog::log_fn fn) {
	s_logFuncMutex.lock();
	if (s_logFuncCount.load() < MAX_LOG_FUNC) {
		s_logFuncArr[s_logFuncCount] = fn;
		++ s_logFuncCount;
	}
	s_logFuncMutex.unlock();
}

static void CustomLog_remove(CustomLog::log_fn fn) {
	s_logFuncMutex.lock();
	int count = s_logFuncCount.load();
	for (int i = 0; i < count; i++) {
		if (s_logFuncArr[i] == fn) {
			if (i != count - 1) {
				memmove(&s_logFuncArr[i], &s_logFuncArr[i + 1], (count - i - 1) * sizeof(CustomLog::log_fn));
			}
			-- s_logFuncCount;
			break;
		}
	}
	s_logFuncMutex.unlock();
}

CustomLog::CustomLog(log_fn fn) : fn(fn) {
	if (fn) {
		CustomLog_insert(fn);
	}
}

CustomLog::~CustomLog() {
	if (fn) {
		CustomLog_remove(fn);
	}
}

CustomLog::CustomLog(CustomLog && other) : fn(other.fn) {
	other.fn = nullptr;
}
CustomLog& CustomLog::operator=(CustomLog && other) {
	fn = other.fn;
	other.fn = nullptr;
	return *this;
}

void format(const StringView &tag, const char *fmt, ...) {
	CustomLog::VA va;
    va_start(va.format.args, fmt);
    va.format.format = fmt;

	__log3(tag, CustomLog::Format, va);

    va_end(va.format.args);
}

void text(const StringView &tag, const StringView &text) {
	CustomLog::VA va;
	va.text = text;
	__log3(tag, CustomLog::Text, va);
}

NS_SP_EXT_END(log)
