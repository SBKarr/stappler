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

#ifndef COMPONENTS_SERENITY_SRC_WEBSOCKET_WEBSOCKETCONNECTION_H_
#define COMPONENTS_SERENITY_SRC_WEBSOCKET_WEBSOCKETCONNECTION_H_

#include "Request.h"
#include "Connection.h"
#include "SPBuffer.h"

NS_SA_EXT_BEGIN(websocket)

struct FrameWriter;
struct FrameReader;
struct NetworkReader;

class Handler;

enum class FrameType : uint8_t {
	None,

	// User
	Text,
	Binary,

	// System
	Continue,
	Close,
	Ping,
	Pong,
};

enum class StatusCode : uint16_t {
	None = 0,
	Auto = 1,
	Ok = 1000,
	Away = 1001,
	ProtocolError = 1002,
	NotAcceptable = 1003,
	ExpectStatus = 1005,
	AbnormalClose = 1006,
	NotConsistent = 1007,
	PolicyViolated = 1008,
	TooLarge = 1009,
	NotNegotiated = 1010,
	UnexceptedCondition = 1011,
	SSLError = 1015,
};

class Connection : public AllocPool {
public:
	static Connection *create(apr_allocator_t *alloc, apr_pool_t *pool, const Request &);
	static void destroy(Connection *);

	static apr_status_t outputFilterFunc(ap_filter_t *f, apr_bucket_brigade *bb);
	static int outputFilterInit(ap_filter_t *f);

	static apr_status_t inputFilterFunc(ap_filter_t *f, apr_bucket_brigade *b,
            ap_input_mode_t mode, apr_read_type_e block, apr_off_t readbytes);
	static int inputFilterInit(ap_filter_t *f);

	void prepare(apr_socket_t *);
	void drop();

	// write ws protocol frame data into the filter chain
	bool write(FrameType t, const uint8_t *bytes = nullptr, size_t count = 0);

	// write data down into the filter chain
	bool write(apr_bucket_brigade *, const uint8_t *bytes, size_t &count);

	// write filtered bb into non-block socket and cache the rest
	bool write(apr_bucket_brigade *);

	// directly write to socket
	size_t send(const uint8_t *data, size_t len);

	bool cache(apr_bucket_brigade *bb, apr_bucket *e, size_t off);

	bool isEnabled() const { return _enabled; }

	bool run(Handler *);

	void cancel(StringView reason = StringView());

	void wakeup();

	void setStatusCode(StatusCode, StringView = StringView());

	void close();

	mem::pool_t *getPool() const { return _pool; }
	conn_rec * getConnection() const { return _connection; }

	mem::pool_t *getHandlePool() const;

protected:
	apr_status_t read(apr_bucket_brigade *bb, char *buf, size_t *len);

	bool processSocket(const apr_pollfd_t *fd, Handler *h);
	bool readSocket(const apr_pollfd_t *fd, Handler *h);
	bool writeSocket(const apr_pollfd_t *fd);

	Connection(apr_allocator_t *, apr_pool_t *, conn_rec *, apr_socket_t *);

	bool setSslCtx(conn_rec *, void *);
	void clearSslCtx();

	// updates with last read status
	StatusCode resolveStatus(StatusCode code);

	apr_allocator_t *_allocator;
	apr_pool_t *_pool;
	conn_rec *_connection;
	apr_socket_t *_socket;

	FrameWriter *_writer = nullptr;
	FrameReader *_reader = nullptr;
	NetworkReader *_network = nullptr;
	bool _enabled = false;
	bool _connected = true;

	apr_pollset_t *_poll = nullptr;
	apr_pollfd_t _pollfd;
	bool _fdchanged = false;

	String _serverReason;
	StatusCode _clientCloseCode = StatusCode::None;
	StatusCode _serverCloseCode = StatusCode::Auto;

	int _sslIdx = -1;
	void *_ssl = nullptr;
};

NS_SA_EXT_END(websocket)

#endif /* COMPONENTS_SERENITY_SRC_WEBSOCKET_WEBSOCKETCONNECTION_H_ */
