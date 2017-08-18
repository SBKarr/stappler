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

#ifndef COMMON_UTILS_SPLOG_H_
#define COMMON_UTILS_SPLOG_H_

#include "SPCommon.h"

NS_SP_EXT_BEGIN(log)

struct CustomLog {
	union VA {
		struct {
			const char *text;
			size_t len;
		} text;
		struct {
			const char *format;
			va_list args;
		} format;
	};

	enum Type {
		Text,
		Format
	};

	using log_fn = void (*) (const char *, Type, VA &);

	CustomLog(log_fn fn);
	~CustomLog();

	CustomLog(const CustomLog &) = delete;
	CustomLog& operator=(const CustomLog &) = delete;

	CustomLog(CustomLog &&);
	CustomLog& operator=(CustomLog &&);

	log_fn fn;
};

void format(const char *tag, const char *, ...) SPPRINTF(2, 3);
void text(const char *tag, const char *text, size_t len = maxOf<size_t>());
void text(const String &, const char *text, size_t len = maxOf<size_t>());
void text(const char *tag, const String &);
void text(const String &, const String &);


#define SPASSERT(cond, msg) do { \
	if (!(cond)) { \
		if (strlen(msg)) { stappler::log::format("Assert", "%s", msg);} \
		assert(cond); \
	} \
} while (0)

NS_SP_EXT_END(log)

#endif /* COMMON_UTILS_SPLOG_H_ */
