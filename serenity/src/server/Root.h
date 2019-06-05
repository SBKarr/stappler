/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SASERVER_H
#define	SASERVER_H

#include "Server.h"

NS_SA_BEGIN

class Root {
public:
	static Root *getInstance();

	Root();
	~Root();

	void onChildInit();

	/* Server Handling */

	void onServerChildInit(apr_pool_t *p, server_rec *s);
	void onOpenLogs(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s);

	void initHeartBeat();
	void onHeartBeat();

	/* Connection Handling */

	int onPreConnection(conn_rec *c, void *csd);
	int onProcessConnection(conn_rec *c);
	bool isSecureConnection(conn_rec *c);

	/* Protocol switch */

	int onProtocolPropose(conn_rec *c, request_rec *r, server_rec *s,
	        const apr_array_header_t *offers, apr_array_header_t *proposals);
	int onProtocolSwitch(conn_rec *c, request_rec *r, server_rec *s, const char *protocol);
	const char * getProtocol(const conn_rec *c);

	/* Request protocol */

	int onPostReadRequest(request_rec *r);
	int onTranslateName(request_rec *r);
	int onCheckAccess(request_rec *r);
	int onQuickHandler(request_rec *r, int v);
	void onInsertFilter(request_rec *r);
	int onHandler(request_rec *r);

	void onFilterInit(InputFilter *f);
	void onFilterUpdate(InputFilter *f);
	void onFilterComplete(InputFilter *f);

	void onBroadcast(const data::Value &);

	void setProcPool(apr_pool_t *);
	apr_pool_t *getProcPool() const;

	ap_dbd_t * dbdOpen(apr_pool_t *, server_rec *);
	void dbdClose(server_rec *, ap_dbd_t *);
	ap_dbd_t * dbdRequestAcquire(request_rec *);
	ap_dbd_t * dbdConnectionAcquire(conn_rec *);
	ap_dbd_t * dbdPoolAcquire(server_rec *, apr_pool_t *);
	void dbdPrepare(server_rec *, const char *, const char *);

	void performStorage(apr_pool_t *, const Server &, const Callback<void(const storage::Adapter &)> &);

	bool isDebugEnabled() const;
	void setDebugEnabled(bool);

	bool performTask(const Server &server, Task *task, bool performFirst);
	bool scheduleTask(const Server &server, Task *task, TimeInterval);

protected:
	static Root *s_sharedServer;

	static void *logWriterInit(apr_pool_t *p, server_rec *s, const char *name);
	static apr_status_t logWriter(request_rec *r, void *handle, const char **portions,
			int *lengths, int nelts, apr_size_t len);

	apr_pool_t *_pool = nullptr;
	apr_pool_t *_heartBeatPool = nullptr;

	Server _rootServerContext;

	APR_OPTIONAL_FN_TYPE(ssl_is_https) * _sslIsHttps = nullptr;

	APR_OPTIONAL_FN_TYPE(ap_dbd_open) * _dbdOpen = nullptr;
	APR_OPTIONAL_FN_TYPE(ap_dbd_close) * _dbdClose = nullptr;
	APR_OPTIONAL_FN_TYPE(ap_dbd_acquire) * _dbdRequestAcquire = nullptr;
	APR_OPTIONAL_FN_TYPE(ap_dbd_cacquire) * _dbdConnectionAcquire = nullptr;
	APR_OPTIONAL_FN_TYPE(ap_dbd_prepare) * _dbdPrepare = nullptr;

	apr_thread_t *_timerThread = nullptr;
	apr_time_t _timerValue = 0;

	ap_log_writer_init *_defaultInit = nullptr;
	ap_log_writer *_defaultWriter = nullptr;

	apr_thread_pool_t *_threadPool = nullptr;

#if DEBUG
	bool _debug = true;
#else
	bool _debug = false;
#endif
};

/* Also export them as optional functions for modules that prefer it */
APR_DECLARE_OPTIONAL_FN(ap_dbd_t*, ap_dbd_open, (apr_pool_t*, server_rec*));
APR_DECLARE_OPTIONAL_FN(void, ap_dbd_close, (server_rec*, ap_dbd_t*));
APR_DECLARE_OPTIONAL_FN(ap_dbd_t*, ap_dbd_acquire, (request_rec*));
APR_DECLARE_OPTIONAL_FN(ap_dbd_t*, ap_dbd_cacquire, (conn_rec*));
APR_DECLARE_OPTIONAL_FN(void, ap_dbd_prepare, (server_rec*, const char*, const char*));

NS_SA_END

#endif	/* SASERVER_H */

