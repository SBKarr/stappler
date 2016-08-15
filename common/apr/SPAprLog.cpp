/*
 * SPLogApr.cpp
 *
 *  Created on: 10 дек. 2015 г.
 *      Author: sbkarr
 */

#include "SPCommon.h"
#include "SPLog.h"
#include "SPAprAllocStack.h"

#ifdef SPAPR

APLOG_USE_MODULE(serenity);

NS_SP_EXT_BEGIN(log)

static void __log2(const char *tag, const char *str) {
	auto log = apr::AllocStack::get().log();
	switch (log.target) {
	case apr::LogContext::Target::Pool:
		ap_log_perror(APLOG_MARK, 0, 0, log.pool, "%s: %s", tag, str);
		break;
	case apr::LogContext::Target::Server:
		ap_log_error(APLOG_MARK, 0, 0, log.server, "%s: %s", tag, str);
		break;
	case apr::LogContext::Target::Connection:
		ap_log_cerror(APLOG_MARK, 0, 0, log.conn, "%s: %s", tag, str);
		break;
	case apr::LogContext::Target::Request:
		ap_log_rerror(APLOG_MARK, 0, 0, log.request, "%s: %s", tag, str);
		break;
	}
}

static void __log(const char *tag, CustomLog::Type t, CustomLog::VA &va) {
	if (t == CustomLog::Text) {
		if (va.text.text && va.text.text[0] != 0) {
			if (va.text.len != strlen(va.text.text)) {
				auto pool = apr::AllocStack::get().top();
				__log2(tag, apr_pstrndup(pool, va.text.text, va.text.len));
			} else {
				__log2(tag, va.text.text);
			}
		}
	} else {
		auto pool = apr::AllocStack::get().top();
		auto str = apr_pvsprintf(pool, va.format.format, va.format.args);
		__log2(tag, str);
	}
}

static CustomLog AprLog(&__log);

NS_SP_EXT_END(log)
#endif
