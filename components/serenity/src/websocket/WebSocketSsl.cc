/**
Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "WebSocketConnection.h"

#include <openssl/opensslv.h>
#if (OPENSSL_VERSION_NUMBER >= 0x10001000)
/* must be defined before including ssl.h */
#define OPENSSL_NO_SSL_INTERN
#endif
#include <openssl/ssl.h>

NS_SA_EXT_BEGIN(websocket)

static const char ssl_io_filter[] = "SSL/TLS Filter";

typedef struct SSLDirConfigRec SSLDirConfigRec;

enum ssl_shutdown_type_e {
	SSL_SHUTDOWN_TYPE_UNSET,
	SSL_SHUTDOWN_TYPE_STANDARD,
	SSL_SHUTDOWN_TYPE_UNCLEAN,
	SSL_SHUTDOWN_TYPE_ACCURATE
};

struct SSLConnRec {
	SSL *ssl;
	const char *client_dn;
	X509 *client_cert;
	ssl_shutdown_type_e shutdown_type;
	const char *verify_info;
	const char *verify_error;
	int verify_depth;
	int disabled;
	enum {
		NON_SSL_OK = 0, /* is SSL request, or error handling completed */
		NON_SSL_SEND_REQLINE, /* Need to send the fake request line */
		NON_SSL_SEND_HDR_SEP, /* Need to send the header separator */
		NON_SSL_SET_ERROR_MSG /* Need to set the error message */
	} non_ssl_request;

	server_rec *server;
	SSLDirConfigRec *dc;

	const char *cipher_suite; /* cipher suite used in last reneg */
	int service_unavailable; /* thouugh we negotiate SSL, no requests will be served */
	int vhost_found; /* whether we found vhost from SNI already */
};

struct ssl_filter_ctx_t {
	SSL *pssl;
	BIO *pbioRead;
	BIO *pbioWrite;
	ap_filter_t *pInputFilter;
	ap_filter_t *pOutputFilter;
	SSLConnRec *config;
};

struct char_buffer_t {
	int length;
	char *value;
};

struct bio_filter_in_ctx_t {
	SSL *ssl;
	BIO *bio_out;
	ap_filter_t *f;
	apr_status_t rc;
	ap_input_mode_t mode;
	apr_read_type_e block;
	apr_bucket_brigade *bb;
	char_buffer_t cbuf;
	apr_pool_t *pool;
	char buffer[AP_IOBUFSIZE];
	ssl_filter_ctx_t *filter_ctx;
};

struct bio_filter_out_ctx_t {
	ssl_filter_ctx_t *filter_ctx;
	conn_rec *c;
	apr_bucket_brigade *bb; /* Brigade used as a buffer. */
	apr_status_t rc;
};

void Connection::prepare(apr_socket_t *sock) {
	apr_os_sock_t invalidSock = -1;
	apr_os_sock_put(&sock, &invalidSock, apr_socket_pool_get(sock));

	if (!_ssl) {
		return;
	}

	ssl_filter_ctx_t *origFilterCtx = (ssl_filter_ctx_t *)_ssl;

	SSLConnRec *connCtx = (SSLConnRec *)apr_palloc(_pool, sizeof(SSLConnRec));
	memcpy(connCtx, origFilterCtx->config, sizeof(SSLConnRec));

	if (connCtx->client_dn) { connCtx->client_dn = apr_pstrdup(_pool, connCtx->client_dn); }
	if (connCtx->verify_info) { connCtx->verify_info = apr_pstrdup(_pool, connCtx->verify_info); }
	if (connCtx->verify_error) { connCtx->verify_error = apr_pstrdup(_pool, connCtx->verify_error); }
	if (connCtx->cipher_suite) { connCtx->cipher_suite = apr_pstrdup(_pool, connCtx->cipher_suite); }

	ssl_filter_ctx_t *filterCtx = (ssl_filter_ctx_t *)apr_palloc(_pool, sizeof(ssl_filter_ctx_t));
	filterCtx->pssl = origFilterCtx->pssl;
	filterCtx->pbioRead = origFilterCtx->pbioRead;
	filterCtx->pbioWrite = origFilterCtx->pbioWrite;
	filterCtx->pInputFilter = nullptr; // setup later
	filterCtx->pOutputFilter = nullptr; // setup later
	filterCtx->config = connCtx;

	((void **)_connection->conn_config)[_sslIdx] = connCtx;

	bio_filter_in_ctx_t *origInCtx = (bio_filter_in_ctx_t *)BIO_get_data(filterCtx->pbioRead);
	bio_filter_in_ctx_t *inCtx = (bio_filter_in_ctx_t *)apr_palloc(_pool, sizeof(bio_filter_in_ctx_t));

    inCtx->ssl = filterCtx->pssl;
    inCtx->bio_out = filterCtx->pbioWrite;
    inCtx->f = filterCtx->pInputFilter; // setupLater
    inCtx->rc = origInCtx->rc;
    inCtx->mode = origInCtx->mode;
    inCtx->cbuf.length = origInCtx->cbuf.length;
    inCtx->cbuf.value = (inCtx->cbuf.length == 0) ? nullptr : (char *)apr_pmemdup(_pool, origInCtx->cbuf.value, origInCtx->cbuf.length);
    inCtx->bb = apr_brigade_create(_pool, _connection->bucket_alloc);
    if (!APR_BRIGADE_EMPTY(origInCtx->bb)) {
    	std::cout << "Input BB is not empty\n";
    }
    inCtx->block = origInCtx->block;
    inCtx->pool = _pool;
    inCtx->filter_ctx = filterCtx;

	BIO_set_data(filterCtx->pbioRead, (void *)inCtx);

	bio_filter_out_ctx_t *origOutCtx = (bio_filter_out_ctx_t *)BIO_get_data(filterCtx->pbioWrite);
	bio_filter_out_ctx_t *outCtx = (bio_filter_out_ctx_t *)apr_palloc(_pool, sizeof(bio_filter_out_ctx_t));

	outCtx->filter_ctx = filterCtx;
	outCtx->c = _connection;
	outCtx->rc = origOutCtx->rc;
	outCtx->bb = apr_brigade_create(_pool, _connection->bucket_alloc);
    if (!APR_BRIGADE_EMPTY(origOutCtx->bb)) {
    	std::cout << "Output BB is not empty\n";
    }
	BIO_set_data(filterCtx->pbioWrite, (void *)outCtx);

	filterCtx->pOutputFilter = ap_add_output_filter(ssl_io_filter, filterCtx, nullptr, _connection);
	inCtx->f = filterCtx->pInputFilter = ap_add_input_filter(ssl_io_filter, inCtx, nullptr, _connection);

	SSL_set_app_data(filterCtx->pssl, _connection);

	origFilterCtx->config->ssl = nullptr;
	origFilterCtx->config->client_cert = nullptr;

	origFilterCtx->pssl = nullptr;
	origFilterCtx->pbioRead = nullptr;
	origFilterCtx->pbioWrite = nullptr;

	origInCtx->ssl = nullptr;
	origInCtx->bio_out = nullptr;

	_ssl = filterCtx;
}

void Connection::drop() {
	apr_os_sock_t invalidSock = -1;
	apr_os_sock_put(&_socket, &invalidSock, _pool);
}

bool Connection::setSslCtx(conn_rec *c, void *ctx) {
	int modInd = serenity_module.module_index;

	ssl_filter_ctx_t *origFilterCtx = (ssl_filter_ctx_t *)ctx;
	auto vec = (void **)(c->conn_config);

	for (int i = 0; i < modInd; ++ i) {
		if (origFilterCtx->config == vec[i]) {
			_sslIdx = i;
			break;
		}
	}

	if (_sslIdx < 0) {
		return false;
	}

	_ssl = ctx;
	return true;
}

void Connection::clearSslCtx() {
	if (!_ssl) {
		return;
	}

	ssl_filter_ctx_t *filterCtx = (ssl_filter_ctx_t *)_ssl;
	if (filterCtx->pssl) {
		SSL_free(filterCtx->pssl);
		filterCtx->config->ssl = filterCtx->pssl = nullptr;
	}
}

NS_SA_EXT_END(websocket)
