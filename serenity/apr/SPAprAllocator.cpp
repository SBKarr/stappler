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

#include "SPCore.h"
#include "SPAprAllocator.h"

#ifdef SPAPR
NS_SP_EXT_BEGIN(apr)

namespace pool {

static server_rec *getServerFromContext(pool_t *p, uint32_t tag, void *ptr) {
	switch (tag) {
	case uint32_t(Server): return (server_rec *)ptr; break;
	case uint32_t(Connection): return ((conn_rec *)ptr)->base_server; break;
	case uint32_t(Request): return ((request_rec *)ptr)->server; break;
	case uint32_t(Pool): return nullptr; break;
	}
	return nullptr;
}

static bool getServerFromContext(void *data, pool_t *p, uint32_t tag, void *ptr) {
	server_rec **serv = (server_rec **)data;
	if (auto s = getServerFromContext(p, tag, ptr)) {
		*serv = s;
		return false;
	}
	return true;
}

server_rec *server() {
	auto l = info();
	if (auto s = getServerFromContext(nullptr, l.first, l.second))  {
		return s;
	} else {
		server_rec *rec = nullptr;
		foreach_info(&rec, &getServerFromContext);
		return rec;
	}
}

request_rec *request() {
	auto l = info();
	switch (l.first) {
	case uint32_t(Request): return (request_rec *)l.second; break;
	default: break;
	}
	return nullptr;
}

}

NS_SP_EXT_END(apr)

#include "SPLog.h"

APLOG_USE_MODULE(serenity);

NS_SP_EXT_BEGIN(log)

static void __log2(const char *tag, const char *str) {
	auto log = apr::pool::info();
	switch (log.first) {
	case uint32_t(apr::pool::Info::Pool):
		ap_log_perror(APLOG_MARK, 0, 0, (apr_pool_t *)log.second, "%s: %s", tag, str);
		break;
	case uint32_t(apr::pool::Info::Server):
		ap_log_error(APLOG_MARK, 0, 0, (server_rec *)log.second, "%s: %s", tag, str);
		break;
	case uint32_t(apr::pool::Info::Connection):
		ap_log_cerror(APLOG_MARK, 0, 0, (conn_rec *)log.second, "%s: %s", tag, str);
		break;
	case uint32_t(apr::pool::Info::Request):
		ap_log_rerror(APLOG_MARK, 0, 0, (request_rec *)log.second, "%s: %s", tag, str);
		break;
	}
}

static void __log(const char *tag, CustomLog::Type t, CustomLog::VA &va) {
	if (t == CustomLog::Text) {
		if (va.text.text && va.text.text[0] != 0) {
			if (va.text.len != strlen(va.text.text)) {
				auto pool = apr::pool::acquire();
				__log2(tag, apr_pstrndup(pool, va.text.text, va.text.len));
			} else {
				__log2(tag, va.text.text);
			}
		}
	} else {
		auto pool = apr::pool::acquire();
		auto str = apr_pvsprintf(pool, va.format.format, va.format.args);
		__log2(tag, str);
	}
}

static CustomLog AprLog(&__log);

NS_SP_EXT_END(log)
#endif
