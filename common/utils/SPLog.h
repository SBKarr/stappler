/*
 * SPLog.h
 *
 *  Created on: 10 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef COMMON_CORE_SPLOG_H_
#define COMMON_CORE_SPLOG_H_

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
		if (strlen(msg)) { stappler::logTag("Assert", "%s", msg);} \
		assert(cond); \
	} \
} while (0)

NS_SP_EXT_END(log)

#endif /* COMMON_CORE_SPLOG_H_ */
