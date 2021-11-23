// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "Define.h"
#include "WebSocket.h"
#include "Root.h"
#include "STPqHandle.h"

NS_SA_EXT_BEGIN(websocket)

constexpr auto WEBSOCKET_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
constexpr auto WEBSOCKET_GUID_LEN = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"_len;

static String makeAcceptKey(const String &key) {
	apr_byte_t digest[APR_SHA1_DIGESTSIZE];
	apr_sha1_ctx_t context;

	apr_sha1_init(&context);
	apr_sha1_update(&context, key.c_str(), key.size());
	apr_sha1_update(&context, (const char *)WEBSOCKET_GUID, (unsigned int)WEBSOCKET_GUID_LEN);
	apr_sha1_final(digest, &context);

	return base64::encode(CoderSource(digest, APR_SHA1_DIGESTSIZE));
}

apr_status_t Manager::filterFunc(ap_filter_t *f, apr_bucket_brigade *bb) {
	// Block any output
	return APR_SUCCESS;
}

int Manager::filterInit(ap_filter_t *f) {
	return apr::pool::perform([&] () -> apr_status_t {
		return OK;
	}, f->c);
}

void Manager::filterRegister() {
	ap_register_output_filter(WEBSOCKET_FILTER, &(filterFunc),
			&(filterInit), ap_filter_type(AP_FTYPE_NETWORK - 1));
	ap_register_output_filter(WEBSOCKET_FILTER_OUT, &(Connection::outputFilterFunc),
			&(Connection::outputFilterInit), ap_filter_type(AP_FTYPE_NETWORK - 1));
	ap_register_input_filter(WEBSOCKET_FILTER_IN, &(Connection::inputFilterFunc),
			&(Connection::inputFilterInit), ap_filter_type(AP_FTYPE_NETWORK - 1));
}

Manager::Manager(Server serv) : _pool(getCurrentPool()), _mutex(_pool), _server(serv) { }
Manager::~Manager() { }

Handler * Manager::onAccept(const Request &req, mem::pool_t *) {
	return nullptr;
}
bool Manager::onBroadcast(const data::Value & val) {
	return false;
}

size_t Manager::size() const {
	return _count.load();
}

void Manager::receiveBroadcast(const data::Value &val) {
	if (onBroadcast(val)) {
		_mutex.lock();
		for (auto &it : _handlers) {
			it->receiveBroadcast(val);
		}
		_mutex.unlock();
	}
}

void *Manager_spawnThread(apr_thread_t *self, void *data) {
#if LINUX
	pthread_setname_np(pthread_self(), "WebSocketThread");
#endif

	Handler *h = (Handler *)data;
	Manager *m = h->manager();
	mem::perform([&] {
		mem::perform([&] {
			m->run(h);
		}, h->connection()->getConnection());
	}, m->server());

	Connection::destroy(h->connection());

	return NULL;
}

static int Manager_abortfn(int retcode) {
	std::cout << "WebSocket Handle allocation failed with code: " << retcode << "\n";
	return retcode;
}

int Manager::accept(Request &req) {
	auto h = req.getRequestHeaders();
	auto &version = h.at("sec-websocket-version");
	auto &key = h.at("sec-websocket-key");
	auto decKey = base64::decode(key);
	if (decKey.size() != 16 || version != "13") {
		req.getErrorHeaders().emplace("Sec-WebSocket-Version", "13");
		return HTTP_BAD_REQUEST;
	}

	apr_allocator_t *alloc = nullptr;
	apr_pool_t *pool = nullptr;

	auto FailCleanup = [&] (int code) -> int {
		if (pool) {
			apr_pool_destroy(pool);
		}

		if (alloc) {
			apr_allocator_destroy(alloc);
		}

		return code;
	};

	if (apr_allocator_create(&alloc) != APR_SUCCESS) { return FailCleanup(HTTP_INTERNAL_SERVER_ERROR); }
	if (apr_pool_create_unmanaged_ex(&pool, Manager_abortfn, alloc) != APR_SUCCESS) { return FailCleanup(HTTP_INTERNAL_SERVER_ERROR); }

	apr_allocator_max_free_set(alloc, 20_MiB);

	auto orig = mem::pool::acquire();
	auto handler = onAccept(req, pool);

	if (handler) {
		auto hout = req.getResponseHeaders();

		hout.clear();
		hout.emplace("Upgrade", "websocket");
		hout.emplace("Connection", "Upgrade");
		hout.emplace("Sec-WebSocket-Accept", makeAcceptKey(key));

		auto r = req.request();
		auto sock = ap_get_conn_socket(r->connection);

		// send HTTP_SWITCHING_PROTOCOLS right f*cking NOW!
		apr_socket_timeout_set(sock, -1);
		req.setStatus(HTTP_SWITCHING_PROTOCOLS);
		ap_send_interim_response(req.request(), 1);

		// block any other output
	    ap_add_output_filter(WEBSOCKET_FILTER, (void *)handler, r, r->connection);

	    // duplicate connection
	    if (auto conn = Connection::create(alloc, pool, req)) {
			handler->setConnection(conn);
			apr_thread_t *thread = nullptr;
			apr_threadattr_t *attr = nullptr;
			apr_status_t error = apr_threadattr_create(&attr, orig);
			if (error == APR_SUCCESS) {
				apr_threadattr_detach_set(attr, 1);
				conn->prepare(sock);
				if (apr_thread_create(&thread, attr,
						Manager_spawnThread, handler, orig) == APR_SUCCESS) {
				    return HTTP_OK;
				}
			} else {
				conn->drop();
			}
	    }
	}
	if (req.getStatus() == HTTP_OK) {
		return FailCleanup(HTTP_BAD_REQUEST);
	}
	return FailCleanup(req.getStatus());
}

void Manager::run(Handler *h) {
	auto c = h->connection();
	c->run(h, [&] {
		addHandler(h);
	}, [&] {
		removeHandler(h);
	});
}

void Manager::addHandler(Handler * h) {
	_mutex.lock();
	_handlers.emplace_back(h);
	++ _count;
	_mutex.unlock();
}

void Manager::removeHandler(Handler * h) {
	_mutex.lock();
	auto it = _handlers.begin();
	while (it != _handlers.end() && *it != h) {
		++ it;
	}
	if (it != _handlers.end()) {
		_handlers.erase(it);
	}
	-- _count;
	_mutex.unlock();
}

Handler::Handler(Manager *m, mem::pool_t *p, StringView url, TimeInterval ttl, size_t max)
: _pool(p), _manager(m), _url(url.pdup(_pool)), _ttl(ttl), _maxInputFrameSize(max), _broadcastMutex(_pool) {
}

Handler::~Handler() { }

// Data frame was received from network
bool Handler::onFrame(FrameType, const Bytes &) { return true; }

// Message was received from broadcast
bool Handler::onMessage(const data::Value &) { return true; }

void Handler::sendBroadcast(data::Value &&val) const {
	data::Value bcast {
		std::make_pair("server", data::Value(_manager->server().getDefaultName())),
		std::make_pair("url", data::Value(_url)),
		std::make_pair("data", data::Value(std::move(val))),
	};

	performWithStorage([&] (const db::Transaction &t) {
		t.getAdapter().broadcast(bcast);
	});
}

void Handler::setEncodeFormat(const data::EncodeFormat &fmt) {
	_format = fmt;
}

bool Handler::send(StringView str) {
	return _conn->write(FrameType::Text, (const uint8_t *)str.data(), str.size());
}
bool Handler::send(BytesView bytes) {
	return _conn->write(FrameType::Binary, bytes.data(), bytes.size());
}
bool Handler::send(const data::Value &data) {
	if (_format.isTextual()) {
		apr::ostringstream stream;
		stream << _format << data;
		return send(stream.weak());
	} else {
		return send(data::write(data, _format));
	}
}

void Handler::performWithStorage(const Callback<void(const db::Transaction &)> &cb) const {
	_manager->server().performWithStorage(cb);
}

bool Handler::performAsync(const Callback<void(Task &)> &cb) const {
	return _conn->performAsync(cb);
}

mem::pool_t *Handler::pool() const {
	return _conn->getHandlePool();
}

void Handler::setConnection(Connection *c) {
	_conn = c;
	serenity::Connection cctx(c->getConnection());
	serenity::Connection::Config *ccfg = (serenity::Connection::Config *)cctx.getConfig();
	ccfg->_handler = this;
}

void Handler::receiveBroadcast(const data::Value &data) {
	if (_conn->isEnabled()) {
		_broadcastMutex.lock();
		if (!_broadcastsPool) {
			_broadcastsPool = memory::pool::create(_pool);
		}
		if (_broadcastsPool) {
			apr::pool::perform([&] {
				if (!_broadcastsMessages) {
					_broadcastsMessages = new (_broadcastsPool) Vector<data::Value>(_broadcastsPool);
				}

				_broadcastsMessages->emplace_back(data);
			}, _broadcastsPool, memory::pool::Broadcast);
		}
		_broadcastMutex.unlock();
		_conn->wakeup();
	}
}

bool Handler::processBroadcasts() {
	apr_pool_t *pool;
	Vector<data::Value> * vec;

	_broadcastMutex.lock();

	pool = _broadcastsPool;
	vec = _broadcastsMessages;

	_broadcastsPool = nullptr;
	_broadcastsMessages = nullptr;

	_broadcastMutex.unlock();

	bool ret = true;
	if (pool) {
		auto conn = _conn->getConnection();
		// temporary replace connection pool to hack serenity's pool stack
		auto tmp = conn->pool;
		conn->pool = pool;
		apr::pool::perform([&] {
			conn->pool = tmp;
			sendPendingNotifications(pool);
			if (vec) {
				for (auto & it : (*vec)) {
					if (!onMessage(it)) {
						ret = false;
						break;
					}
				}
			}
		}, conn);
		conn->pool = tmp;
		apr_pool_destroy(pool);
	}

	return ret;
}

void Handler::sendPendingNotifications(apr_pool_t *pool) {
	apr::pool::perform([&] {
		messages::setNotifications(pool, [this] (data::Value && data) {
			send(data::Value({
				std::make_pair("error", data::Value(std::move(data)))
			}));
		}, [this] (data::Value && data) {
			send(data::Value({
				std::make_pair("debug", data::Value(std::move(data)))
			}));
		});
	}, pool, memory::pool::Broadcast);
}

NS_SA_EXT_END(websocket)
