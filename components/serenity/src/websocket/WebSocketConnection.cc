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
#include "WebSocket.h"

#include <sys/eventfd.h>

#include "WebSocketWriter.cc"
#include "WebSocketReader.cc"
#include "WebSocketSsl.cc"

#define BRIGADE_NORMALIZE(b) \
do { \
    apr_bucket *e = APR_BRIGADE_FIRST(b); \
    do {  \
        if (e->length == 0 && !APR_BUCKET_IS_METADATA(e)) { \
            apr_bucket *d; \
            d = APR_BUCKET_NEXT(e); \
            apr_bucket_delete(e); \
            e = d; \
        } \
        else { \
            e = APR_BUCKET_NEXT(e); \
        } \
    } while (!APR_BRIGADE_EMPTY(b) && (e != APR_BRIGADE_SENTINEL(b))); \
} while (0)

NS_SA_EXT_BEGIN(websocket)

static apr_socket_t *Connection_duplicateSocket(apr_pool_t *pool, apr_socket_t *sock) {
	apr_os_sock_t osSock;
	if (apr_os_sock_get(&osSock, sock) == 0) {
		bool success = false;
		apr_socket_t *wsSock = nullptr;
		do {
			apr_os_sock_info_t info;
			apr_sockaddr_t *addrRemote = nullptr, *addrLocal = nullptr;
			if (apr_socket_addr_get(&addrRemote, APR_REMOTE, sock) != APR_SUCCESS) { break; }
			if (apr_socket_addr_get(&addrLocal, APR_LOCAL, sock) != APR_SUCCESS) { break; }
			if (apr_socket_type_get(sock, &info.type) != APR_SUCCESS) { break; }
			if (apr_socket_protocol_get(sock, &info.protocol) != APR_SUCCESS) { break; }

			info.family = addrRemote->family;

			info.local = (struct sockaddr *)::alloca(addrLocal->salen);
			info.remote = (struct sockaddr *)::alloca(addrRemote->salen);

	        memcpy((void *)info.local, (const void *)&addrLocal->sa.sin, addrLocal->salen);
	        memcpy((void *)info.remote, (const void *)&addrRemote->sa.sin, addrRemote->salen);

			info.os_sock = &osSock;

			if (apr_os_sock_make(&wsSock, &info, pool) == APR_SUCCESS) {
				apr_int32_t on = 0;
				apr_socket_opt_get(sock, APR_TCP_NODELAY, &on);
				if (on) {
					apr_socket_opt_set(wsSock, APR_TCP_NODELAY, 1);
				}

				success = true;
			}
		} while (0);

		if (success) {
			return wsSock;
		}
	}
	return nullptr;
}

Connection *Connection::create(apr_allocator_t *alloc, apr_pool_t *pool, const Request &rctx) {
	auto req = rctx.request();
	auto conn = req->connection;

	// search for SSL params
	void *sslOutputFilter = nullptr;

	ap_filter_t *output_filter = conn->output_filters;
	while (output_filter != NULL) {
		if ((output_filter->frec != NULL) && (output_filter->frec->name != NULL)) {
			if (!strcasecmp(output_filter->frec->name, "ssl/tls filter")) {
				sslOutputFilter = output_filter->ctx;
			}
		}
		output_filter = output_filter->next;
	}

	auto wsSock = Connection_duplicateSocket(pool, ap_get_conn_socket(conn));
	if (!wsSock) {
		return nullptr;
	}

	conn_rec *wsConn = (conn_rec *)apr_pcalloc(pool, sizeof(conn_rec));
	wsConn->pool = pool;
	wsConn->base_server = req->server;
	wsConn->vhost_lookup_data = nullptr;

	if (apr_socket_addr_get(&wsConn->local_addr, APR_LOCAL, wsSock) != APR_SUCCESS) { return nullptr; }
	if (apr_socket_addr_get(&wsConn->client_addr, APR_REMOTE, wsSock) != APR_SUCCESS) { return nullptr; }

	if (conn->client_ip) { wsConn->client_ip = apr_pstrdup(pool, conn->client_ip); }
	if (conn->remote_host) { wsConn->remote_host = apr_pstrdup(pool, conn->remote_host); }
	if (conn->remote_logname) { wsConn->remote_logname = apr_pstrdup(pool, conn->remote_logname); }
	if (conn->local_ip) { wsConn->local_ip = apr_pstrdup(pool, conn->local_ip); }
	if (conn->local_host) { wsConn->local_host = apr_pstrdup(pool, conn->local_host); }
	wsConn->id = -1;

	wsConn->conn_config = ap_create_conn_config(pool);
	wsConn->notes = apr_table_make(pool, 5);

	// create this later
	wsConn->input_filters = nullptr;
	wsConn->output_filters = nullptr;

	wsConn->sbh = nullptr;
	wsConn->bucket_alloc = apr_bucket_alloc_create(pool);
	wsConn->cs = nullptr;
	wsConn->data_in_input_filters = conn->data_in_input_filters;
	wsConn->data_in_output_filters = conn->data_in_output_filters;
	wsConn->clogging_input_filters = conn->clogging_input_filters;
	wsConn->double_reverse = conn->double_reverse;
	wsConn->aborted = 0;
	wsConn->keepalives = conn->keepalives;
	wsConn->log = nullptr;
	wsConn->log_id = nullptr;
	wsConn->current_thread = nullptr;
	wsConn->master = nullptr;

	ap_get_core_module_config(wsConn->conn_config) = (void *)wsSock;

	serenity::Connection::Config *ccfg = (serenity::Connection::Config *)serenity::Connection(wsConn).getConfig();
	ccfg->_accessRole = rctx.getAccessRole();

	Connection *ret = nullptr;
	mem::perform([&] {
		ret = new (pool) Connection(alloc, pool, wsConn, wsSock);

		if (sslOutputFilter) {
			if (!ret->setSslCtx(conn, sslOutputFilter)) {
				ret = nullptr;
			}
		}
	}, pool);
	return ret;
}

void Connection::destroy(Connection *conn) {
	auto p = conn->_pool;
	auto a = conn->_allocator;

	apr_pool_destroy(p);
	apr_allocator_destroy(a);
}

apr_status_t Connection::outputFilterFunc(ap_filter_t *f, apr_bucket_brigade *bb) {
	if (APR_BRIGADE_EMPTY(bb)) {
		return APR_SUCCESS;
	}

	auto c = (websocket::Connection *)f->ctx;
	if (c->isEnabled()) {
		if (c->write(bb)) {
			return APR_SUCCESS;
		} else {
			return APR_EGENERAL;
		}
	} else {
		return ap_pass_brigade(f->next, bb);
	}
}

int Connection::outputFilterInit(ap_filter_t *f) {
	return OK;
}

apr_status_t Connection::inputFilterFunc(ap_filter_t *f, apr_bucket_brigade *b,
		ap_input_mode_t mode, apr_read_type_e block, apr_off_t readbytes) {
	apr_status_t rv;
	Connection *conn = (Connection *)f->ctx;
	NetworkReader *ctx = conn->_network;

	const char *str;
	apr_size_t len;

	if (mode == AP_MODE_INIT) {
		return APR_SUCCESS;
	}

	if (!ctx->tmpbb) {
		ctx->tmpbb = apr_brigade_create(ctx->pool, ctx->bucket_alloc);
	}

	if (!ctx->b) {
		ctx->b = apr_brigade_create(ctx->pool, ctx->bucket_alloc);
		rv = ap_run_insert_network_bucket(f->c, ctx->b, conn->_socket);
		if (rv != APR_SUCCESS) {
			return rv;
		}
	}

	/* ### This is bad. */
	BRIGADE_NORMALIZE(ctx->b);

	/* check for empty brigade again *AFTER* BRIGADE_NORMALIZE()
	 * If we have lost our socket bucket (see above), we are EOF.
	 *
	 * Ideally, this should be returning SUCCESS with EOS bucket, but
	 * some higher-up APIs (spec. read_request_line via ap_rgetline)
	 * want an error code. */
	if (APR_BRIGADE_EMPTY(ctx->b)) {
		return APR_EOF;
	}

	if (mode == AP_MODE_GETLINE || mode == AP_MODE_EATCRLF || mode == AP_MODE_EXHAUSTIVE) {
		return APR_EOF;
	}

	/* read up to the amount they specified. */
	if (mode == AP_MODE_READBYTES || mode == AP_MODE_SPECULATIVE) {
		apr_bucket *e = APR_BRIGADE_FIRST(ctx->b);
		rv = apr_bucket_read(e, &str, &len, block);

		if (APR_STATUS_IS_EAGAIN(rv) && block == APR_NONBLOCK_READ) {
			/* getting EAGAIN for a blocking read is an error; for a
			 * non-blocking read, return an empty brigade. */
			return APR_SUCCESS;
		} else if (rv != APR_SUCCESS) {
			return rv;
		} else if (block == APR_BLOCK_READ && len == 0) {
			if (e) {
				apr_bucket_delete(e);
			}

			if (mode == AP_MODE_READBYTES) {
				e = apr_bucket_eos_create(f->c->bucket_alloc);
				APR_BRIGADE_INSERT_TAIL(b, e);
			}
			return APR_SUCCESS;
		}

		/* Have we read as much data as we wanted (be greedy)? */
		if (apr_off_t(len) < readbytes) {
			apr_size_t bucket_len;

			rv = APR_SUCCESS;
			/* We already registered the data in e in len */
			e = APR_BUCKET_NEXT(e);
			while ((apr_off_t(len) < readbytes) && (rv == APR_SUCCESS) && (e != APR_BRIGADE_SENTINEL(ctx->b))) {
				/* Check for the availability of buckets with known length */
				if (e->length != (apr_size_t)-1) {
					len += e->length;
					e = APR_BUCKET_NEXT(e);
				} else {
					/*
					 * Read from bucket, but non blocking. If there isn't any
					 * more data, well than this is fine as well, we will
					 * not wait for more since we already got some and we are
					 * only checking if there isn't more.
					 */
					rv = apr_bucket_read(e, &str, &bucket_len, APR_NONBLOCK_READ);
					if (rv == APR_SUCCESS) {
						len += bucket_len;
						e = APR_BUCKET_NEXT(e);
					}
				}
			}
		}

		/* We can only return at most what we read. */
		if (apr_off_t(len) < readbytes) {
			readbytes = len;
		}

		rv = apr_brigade_partition(ctx->b, readbytes, &e);
		if (rv != APR_SUCCESS) {
			return rv;
		}

		/* Must do move before CONCAT */
		ctx->tmpbb = apr_brigade_split_ex(ctx->b, e, ctx->tmpbb);

		if (mode == AP_MODE_READBYTES) {
			APR_BRIGADE_CONCAT(b, ctx->b);
		} else if (mode == AP_MODE_SPECULATIVE) {
			apr_bucket *copy_bucket;

			for (e = APR_BRIGADE_FIRST(ctx->b); e != APR_BRIGADE_SENTINEL(ctx->b); e = APR_BUCKET_NEXT(e)) {
				rv = apr_bucket_copy(e, &copy_bucket);
				if (rv != APR_SUCCESS) {
					return rv;
				}
				APR_BRIGADE_INSERT_TAIL(b, copy_bucket);
			}
		}

		/* Take what was originally there and place it back on ctx->b */
		APR_BRIGADE_CONCAT(ctx->b, ctx->tmpbb);
	}
	return APR_SUCCESS;
}

int Connection::inputFilterInit(ap_filter_t *f) {
	return OK;
}

bool Connection::write(FrameType t, const uint8_t *bytes, size_t count) {
	if (!_enabled) {
		return false;
	}

	size_t offset = 0;
	StackBuffer<32> buf;
	makeHeader(buf, count, t);

	offset = buf.size();

	auto bb = _writer->tmpbb;
	auto of = _connection->output_filters;

	_mutex.lock();
	auto err = ap_fwrite(of, bb, (const char *)buf.data(), offset);
	if (count > 0) {
		err = ap_fwrite(of, bb, (const char *)bytes, count);
	}

	err = ap_fflush(of, bb);
	apr_brigade_cleanup(bb);

	_mutex.unlock();
	if (err != APR_SUCCESS) {
		return false;
	}
	return true;
}

bool Connection::write(apr_bucket_brigade *bb, const uint8_t *bytes, size_t &count) {
	_mutex.lock();
	auto of = _connection->output_filters;
	ap_fwrite(of, bb, (const char *)bytes, count);
	ap_fflush(of, bb);
	apr_brigade_cleanup(bb);
	_mutex.unlock();
 	return true;
}

bool Connection::write(apr_bucket_brigade *bb) {
	bool ret = true;
	size_t offset = 0;
	auto e = APR_BRIGADE_FIRST(bb);
	if (_writer->empty()) {
		for (; e != APR_BRIGADE_SENTINEL(bb); e = APR_BUCKET_NEXT(e)) {
			if (!APR_BUCKET_IS_METADATA(e)) {
				const char * ptr = nullptr;
				apr_size_t len = 0;

				apr_bucket_read(e, &ptr, &len, APR_BLOCK_READ);

				auto written = send((const uint8_t *)ptr, size_t(len));
				if (_serverCloseCode == StatusCode::UnexceptedCondition) {
					return false;
				}
				if (written != len) {
					offset = written;
					break;
				}
			}
		}
	}

	if (e != APR_BRIGADE_SENTINEL(bb)) {
		ret = cache(bb, e, offset);
	}

    apr_brigade_cleanup(bb);
	return ret;
}

static size_t Connection_getBrigadeSize(apr_bucket_brigade *bb, apr_bucket *e, size_t off) {
	size_t ret = 0;
	for (; e != APR_BRIGADE_SENTINEL(bb); e = APR_BUCKET_NEXT(e)) {
		if (!APR_BUCKET_IS_METADATA(e) && e->length != apr_size_t(-1)) {
			ret += e->length;
		}
	}
	return ret - off;
}

bool Connection::cache(apr_bucket_brigade *bb, apr_bucket *e, size_t off) {
	auto size = Connection_getBrigadeSize(bb, e, off);
	if (size == 0) {
		return true;
	}

	auto slot = _writer->nextEmplaceSlot(size);
	for (; e != APR_BRIGADE_SENTINEL(bb); e = APR_BUCKET_NEXT(e)) {
		if (!APR_BUCKET_IS_METADATA(e)) {
			const char * ptr = nullptr;
			apr_size_t len = 0;

			apr_bucket_read(e, &ptr, &len, APR_BLOCK_READ);
			if (off) {
				ptr += off;
				len -= off;
				off = 0;
			}

			slot->emplace((const uint8_t *)ptr, len);
		}
	}

	_pollfd.reqevents |= APR_POLLOUT;
	_fdchanged = true;

	return true;
}

size_t Connection::send(const uint8_t *data, size_t len) {
	auto err = apr_socket_send(_socket, (const char *)data, &len);
	if (err != APR_SUCCESS) {
		_serverReason = "Fail to send data";
		_serverCloseCode = StatusCode::UnexceptedCondition;
	}
	return len;
}

bool Connection::run(Handler *h, const Callback<void()> &beginCb, const Callback<void()> &endCb) {
	apr_pollset_create(&_poll, 1, _pool, APR_POLLSET_NOCOPY);

	if (!_socket || !_poll) {
		_serverReason = "Fail to initialize connection";
		_serverCloseCode = StatusCode::UnexceptedCondition;
		cancel();
		return false;
	}

	// override default with user-defined max
	_reader->max = h->getMaxInputFrameSize();

	memset((void *)&_pollfd, 0, sizeof(apr_pollfd_t));
	_pollfd.p = _pool;
	_pollfd.reqevents = APR_POLLIN;
	_pollfd.desc_type = APR_POLL_SOCKET;
	_pollfd.desc.s = _socket;
	apr_pollset_add(_poll, &_pollfd);

	apr_file_t *eventFile = nullptr;
	apr_os_file_put(&eventFile, &_eventFd, APR_FOPEN_READ | APR_FOPEN_WRITE, _pool);

	memset((void *)&_eventPollfd, 0, sizeof(apr_pollfd_t));
	_eventPollfd.p = _pool;
	_eventPollfd.reqevents = APR_POLLIN;
	_eventPollfd.desc_type = APR_POLL_FILE;
	_eventPollfd.desc.f = eventFile;
	apr_pollset_add(_poll, &_eventPollfd);

	apr_socket_opt_set(_socket, APR_SO_NONBLOCK, 1);
	apr_socket_timeout_set(_socket, 0);

	_enabled = true;

	_shouldTerminate.test_and_set();

	apr::pool::perform([&] {
		h->onBegin();
	}, _reader->pool, memory::pool::Socket);
	_reader->clear();

	auto ttl = h->getTtl().toMicroseconds();

	beginCb();
	while (true) {
		apr_int32_t num = 0;
		const apr_pollfd_t *fds = nullptr;
		_pollfd.rtnevents = 0;
		if (_fdchanged) {
			apr_pollset_remove(_poll, &_pollfd);
			apr_pollset_add(_poll, &_pollfd);
		}
		auto now = Time::now();
		auto err = apr_pollset_poll(_poll, ttl, &num, &fds);

		if (!_shouldTerminate.test_and_set()) {
			// terminated
			break;
		} else if (num == 0) {
			// timeout
			auto t = (Time::now() - now).toMicros();
			if (t > ttl) {
				_serverCloseCode = StatusCode::Away;
				break;
			} else {
				std::cout << "Error: " << err << "\n";
				break;
			}
		} else {
			for (int i = 0; i < num; ++ i) {
				if (fds[i].desc_type == APR_POLL_FILE) {
					// wakeup is pessimistic but accurate, can be called when no pending events or broadcasts
					_group.update();

					if (!h->processBroadcasts()) {
						_shouldTerminate.clear();
						break;
					}

					char buf[8] = { 0 };
					if (::read(_eventFd, buf, 8) > 0) { // decrement by 1 (EFD_SEMAPHORE)
						continue;
					}
				} else {
					if (!processSocket(&fds[i], h)) {
						_shouldTerminate.clear();
						break;
					}
				}
			}
		}

		if (!_shouldTerminate.test_and_set()) {
			break;
		}
	}

	_enabled = false;
	endCb();
	_group.waitForAll();
	apr::pool::perform([&] {
		h->onEnd();
	}, _reader->pool, memory::pool::Socket);
	memory::pool::clear(_reader->pool);

	cancel();

	return true;
}

void Connection::wakeup() {
	uint64_t value = 1;
	if (::write(_eventFd, &value, sizeof(uint64_t)) > 0) {
		return;
	}
}

void Connection::terminate() {
	_shouldTerminate.clear();
	wakeup();
}

void Connection::cancel(StringView reason) {
	// drain all data
	StackBuffer<(size_t)1_KiB> sbuf;

	if (_connected) {
		apr_status_t err = APR_SUCCESS;
		while (err == APR_SUCCESS) {
			size_t len = 1_KiB;
			err = read(_reader->tmpbb, (char *)sbuf.data(), &len);
			sbuf.clear();
		}

		apr_socket_opt_set(_socket, APR_SO_NONBLOCK, 0);
		apr_socket_timeout_set(_socket, -1);

		writeSocket(&_pollfd);
	}

	sbuf.clear();

	StatusCode nstatus = resolveStatus(_serverCloseCode);
	uint16_t status = byteorder::HostToNetwork(_clientCloseCode==StatusCode::None?(uint16_t)nstatus:(uint16_t)_clientCloseCode);
	size_t memSize = std::min((size_t)123, _serverReason.size());
	size_t frameSize = 4 + memSize;
	sbuf[0] = (0b10000000 | 0x8);
	sbuf[1] = ((uint8_t)(memSize + 2));
	memcpy(sbuf.data() + 2, &status, sizeof(uint16_t));
	if (!_serverReason.empty()) {
		memcpy(sbuf.data() + 4, _serverReason.data(), memSize);
	}

	write(_writer->tmpbb, sbuf.data(), frameSize);
	ap_shutdown_conn(_connection, 1);
	_connected = false;

	clearSslCtx();
	close();
}

void Connection::close() {
	apr_socket_close(_socket);
	::close(_eventFd);
}

mem::pool_t *Connection::getHandlePool() const {
	return _reader->pool;
}

bool Connection::performAsync(const Callback<void(Task &)> &cb) const {
	if (!_enabled) {
		return false;
	}
	return _group.perform(cb);
}

void Connection::setStatusCode(StatusCode s, StringView r) {
	_serverCloseCode = s;
	if (!r.empty()) {
		_serverReason = r.str();
	}
}

bool Connection::processSocket(const apr_pollfd_t *fd, Handler *h) {
	if ((fd->rtnevents & APR_POLLOUT) != 0) {
		if (!writeSocket(fd)) {
			return false;
		}
	}
	if ((fd->rtnevents & APR_POLLIN) != 0) {
		if (!readSocket(fd, h)) {
			return false;
		}
	}
	if ((fd->rtnevents & APR_POLLHUP) != 0) {
		return false;
	}
	_pollfd.rtnevents = 0;
	return true;
}

apr_status_t Connection::read(apr_bucket_brigade *bb, char *buf, size_t *len) {
	if (_connection->input_filters) {
		auto f = _connection->input_filters;
	    apr_status_t rv = APR_SUCCESS;
	    apr_size_t readbufsiz = *len;

		if (bb != NULL) {
			rv = ap_get_brigade(f, bb, AP_MODE_READBYTES, APR_NONBLOCK_READ, readbufsiz);
			if (rv == APR_SUCCESS) {
				if ((rv = apr_brigade_flatten(bb, buf, len)) == APR_SUCCESS) {
					readbufsiz = *len;
				}
			}
			apr_brigade_cleanup(bb);
		}
		_network->drop();

	    if (readbufsiz == 0 && (rv == APR_SUCCESS || APR_STATUS_IS_EAGAIN(rv))) {
	    	return APR_EAGAIN;
	    }
	    return rv;
	} else {
		return apr_socket_recv(_socket, buf, len);
	}
}

bool Connection::readSocket(const apr_pollfd_t *fd, Handler *h) {
	apr_status_t err = APR_SUCCESS;
	while (err == APR_SUCCESS) {
		if (_reader) {
			size_t len = _reader->getRequiredBytes();
			uint8_t *buf = _reader->prepare(len);

			err = read(_reader->tmpbb, (char *)buf, &len);
			if (err == APR_SUCCESS && !_reader->save(buf, len)) {
				return false;
			} else if (err != APR_SUCCESS && !APR_STATUS_IS_EAGAIN(err)) {
				return false;
			}

			if (_reader->isControlReady()) {
				if (_reader->type == FrameType::Close) {
					_clientCloseCode = (StatusCode)(_reader->buffer.get<BytesViewNetwork>().readUnsigned16());
					_reader->popFrame();
					return false;
				} else if (_reader->type == FrameType::Ping) {
					write(FrameType::Pong, nullptr, 0);
					_reader->popFrame();
				}
			} else if (_reader->isFrameReady()) {
				auto tmp = _connection->pool;
				_connection->pool = _reader->pool;
				auto ret = apr::pool::perform([&] {
					_connection->pool = tmp;
					if (h) {
						h->sendPendingNotifications(_reader->pool);
					}
					if (h && !h->onFrame(_reader->type, _reader->frame.buffer)) {
						return false;
					}
					return true;
				}, _connection);
				_connection->pool = tmp;
				_reader->popFrame();
				if (!ret) {
					return false;
				}
			}
		} else {
			return false;
		}
	}
	_pollfd.reqevents |= APR_POLLIN;
	return true;
}

bool Connection::writeSocket(const apr_pollfd_t *fd) {
	while (!_writer->empty()) {
		auto slot = _writer->nextReadSlot();

		while (!slot->empty()) {
			auto len = slot->getNextLength();
			auto ptr = slot->getNextBytes();

			auto written = send(ptr, len);
			if (_serverCloseCode == StatusCode::UnexceptedCondition) {
				return false;
			}
			if (written > 0) {
				slot->pop(written);
			}
			if (written != len) {
				break;
			}
		}

		if (slot->empty()) {
			_writer->popReadSlot();
		} else {
			break;
		}
	}

	if (_writer->empty()) {
		if (_pollfd.reqevents & APR_POLLOUT) {
			_pollfd.reqevents &= ~APR_POLLOUT;
			_fdchanged = true;
		}
	} else {
		if ((_pollfd.reqevents & APR_POLLOUT) == 0) {
			_pollfd.reqevents |= APR_POLLOUT;
			_fdchanged = true;
		}
	}

	return true;
}

Connection::Connection(apr_allocator_t *a, apr_pool_t *p, conn_rec *c, apr_socket_t *s)
: _allocator(a), _pool(p), _connection(c), _socket(s), _group(Server(c->base_server), [this] {
	wakeup();
}) {
	ap_add_output_filter(WEBSOCKET_FILTER_OUT, (void *)this, nullptr, c);
	ap_add_input_filter(WEBSOCKET_FILTER_IN, (void *)this, nullptr, c);

	_writer = new (_pool) FrameWriter(_pool, _connection->bucket_alloc);
	_reader = new (_pool) FrameReader(_pool, _connection->bucket_alloc);
	_network = new (_pool) NetworkReader(_pool, _connection->bucket_alloc);

	serenity::Connection cctx(_connection);
	serenity::Connection::Config *ccfg = (serenity::Connection::Config *)cctx.getConfig();
	ccfg->_websocket = this;

	_eventFd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);
}

NS_SA_EXT_END(websocket)
