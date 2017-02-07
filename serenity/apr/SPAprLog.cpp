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
