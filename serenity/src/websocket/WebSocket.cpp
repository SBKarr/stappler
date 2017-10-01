// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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
#include "PGHandle.h"

NS_SA_EXT_BEGIN(websocket)

Handler::Handler(Manager *m, const Request &req, TimeInterval ttl, size_t max)
: _request(req), _connection(req.connection()), _manager(m), _ttl(ttl)
, _reader(req, _connection.pool(), max), _writer(req, _connection.pool())
, _clientCloseCode(StatusCode::None), _serverCloseCode(StatusCode::Auto)
, _broadcastMutex(_connection.pool()) {
	_socket = (apr_socket_t *)ap_get_module_config (req.request()->connection->conn_config, &core_module);

	apr_pool_t *pool = _connection.pool();
	apr_pollset_create(&_poll, 1, pool, APR_POLLSET_WAKEABLE | APR_POLLSET_NOCOPY);

	if (_poll && _socket && _manager && _broadcastMutex) {
		memset(&_pollfd, 0, sizeof(apr_pollfd_t));
		_pollfd.p = pool;
		_pollfd.reqevents = APR_POLLIN;
		_pollfd.desc_type = APR_POLL_SOCKET;
		_pollfd.desc.s = _socket;
		apr_pollset_add(_poll, &_pollfd);
		_valid = true;
	}
}

Handler::~Handler() { }

void Handler::run() {
	if (!_valid) {
		_serverReason = "Fail to initialize connection";
		_serverCloseCode = StatusCode::UnexceptedCondition;
		cancel();
		return;
	}

	apr_socket_opt_set(_socket, APR_SO_NONBLOCK, 1);
	apr_socket_timeout_set(_socket, 0);

	apr::pool::perform([&] {
		onBegin();
	}, _reader.pool);

	while (true) {
		apr_int32_t num = 0;
		const apr_pollfd_t *fds = nullptr;
		_pollfd.rtnevents = 0;
		if (_fdchanged) {
			apr_pollset_remove(_poll, &_pollfd);
			apr_pollset_add(_poll, &_pollfd);
		}
		auto err = apr_pollset_poll(_poll, _ttl.toMicroseconds(), &num, &fds);
		if (err == APR_EINTR) {
			// wakeup was used
			if (!processBroadcasts()) {
				break;
			}
		} else if (num == 0) {
			// timeout
			_serverCloseCode = StatusCode::Away;
			break;
		} else {
			if (!processSocket(fds)) {
				break;
			}
		}
	}

	apr::pool::perform([&] {
		onEnd();
	}, _reader.pool);
	apr_pool_clear(_reader.pool);

	cancel();
}

// Data frame was received from network
bool Handler::onFrame(FrameType, const Bytes &) { return true; }

// Message was received from broadcast
bool Handler::onMessage(const data::Value &) { return true; }

void Handler::sendBroadcast(data::Value &&val) const {
	data::Value bcast {
		std::make_pair("server", data::Value(_request.server().getDefaultName())),
		std::make_pair("url", data::Value(_request.getUri())),
		std::make_pair("data", data::Value(std::move(val))),
	};

	auto s = storage();
	s->broadcast(bcast);
}

void Handler::setStatusCode(StatusCode s, const String &r) {
	_serverCloseCode = s;
	if (!r.empty()) {
		_serverReason = r;
	}
}
void Handler::setEncodeFormat(const data::EncodeFormat &fmt) {
	_format = fmt;
}

bool Handler::send(const String &str) {
	return trySend(FrameType::Text, (const uint8_t *)str.data(), str.size());
}
bool Handler::send(const Bytes &bytes) {
	return trySend(FrameType::Binary, bytes.data(), bytes.size());
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

bool Handler::trySend(FrameType t, const uint8_t *bytes, size_t count) {
	size_t offset = 0;
	StackBuffer<32> buf;
	makeHeader(buf, count, FrameType::Text);

	offset = buf.size();

	auto bb = _writer.tmpbb;
	auto r = _request.request();
	auto of = r->connection->output_filters;

	auto err = ap_fwrite(of, bb, (const char *)buf.data(), offset);
	if (count > 0) {
		err = ap_fwrite(of, bb, (const char *)bytes, count);
	}

	err = ap_fflush(of, bb);
	apr_brigade_cleanup(bb);

	if (err != APR_SUCCESS) {
		return false;
	}
	return true;
}

storage::Adapter *Handler::storage() const {
	auto pool = apr::pool::acquire();

	pg::Handle *db = nullptr;
	apr_pool_userdata_get((void **)&db, (const char *)config::getSerenityWebsocketDatabaseName(), pool);
	if (!db) {
		db = new (pool) pg::Handle(pool, Root::getInstance()->dbdPoolAcquire(_request.server(), pool));
		apr_pool_userdata_set(db, (const char *)config::getSerenityWebsocketDatabaseName(), NULL, pool);
	}
	return db;
}

const Request &Handler::request() const {
	return _request;
}
Manager *Handler::manager() const {
	return _manager;
}
bool Handler::isEnabled() const {
	return _valid && !_ended;
}

void Handler::receiveBroadcast(const data::Value &data) {
	if (_valid) {
		_broadcastMutex.lock();
		if (!_broadcastsPool) {
			apr_pool_create(&_broadcastsPool, _connection.pool());
		}
		if (_broadcastsPool) {
			apr::pool::perform([&] {
				if (!_broadcastsMessages) {
					_broadcastsMessages = new (_broadcastsPool) Vector<data::Value>(_broadcastsPool);
				}

				_broadcastsMessages->emplace_back(data);
			}, _broadcastsPool);
		}
		_broadcastMutex.unlock();
		apr_pollset_wakeup(_poll);
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
		apr_pool_userdata_set(this, config::getSerenityWebsocketHandleName(), nullptr, pool);
		apr::pool::perform([&] {
			pushNotificator(pool);
			if (vec) {
				for (auto & it : (*vec)) {
					if (!onMessage(it)) {
						ret = false;
						break;
					}
				}
			}
		}, pool);
		apr_pool_destroy(pool);
	}

	return ret;
}

bool Handler::processSocket(const apr_pollfd_t *fd) {
	if ((fd->rtnevents & APR_POLLOUT) != 0) {
		if (!writeSocket(fd)) {
			return false;
		}
	}
	if ((fd->rtnevents & APR_POLLIN) != 0) {
		if (!readSocket(fd)) {
			return false;
		}
	}
	_pollfd.rtnevents = 0;
	return true;
}

static apr_status_t Handler_readSocket_request(Request &req, apr_bucket_brigade *bb, apr_pool_t *pool, char *buf, size_t *len) {
	auto r = req.request();
    apr_status_t rv = APR_SUCCESS;
    apr_size_t readbufsiz = *len;

    ap_filter_t *f = r->input_filters;
    if (f->frec && f->frec->name && !strcasecmp(f->frec->name, "reqtimeout")) {
    	f = f->next;
    }

    if (bb != NULL) {
    	rv = ap_get_brigade(f, bb, AP_MODE_READBYTES, APR_NONBLOCK_READ, readbufsiz);
        if (rv == APR_SUCCESS) {
            if ((rv = apr_brigade_flatten(bb, buf, len)) == APR_SUCCESS) {
                readbufsiz = *len;
            }
        }
        apr_brigade_cleanup(bb);
    }
    if (readbufsiz == 0 && (rv == APR_SUCCESS || APR_STATUS_IS_EAGAIN(rv))) {
    	return APR_EAGAIN;
    }
    return rv;
}

bool Handler::readSocket(const apr_pollfd_t *fd) {
	apr_status_t err = APR_SUCCESS;
	while (err == APR_SUCCESS) {
		if (_reader) {
			size_t len = _reader.getRequiredBytes();
			uint8_t *buf = _reader.prepare(len);

			err = Handler_readSocket_request(_request, _reader.tmpbb, _reader.pool, (char *)buf, &len);
			if (err == APR_SUCCESS && !_reader.save(buf, len)) {
				return false;
			} else if (err != APR_SUCCESS && !APR_STATUS_IS_EAGAIN(err)) {
				return false;
			}

			if (_reader.isControlReady()) {
				auto ret = onControlFrame(_reader.type, _reader.buffer);
				_reader.popFrame();
				if (!ret) {
					return false;
				}
			} else if (_reader.isFrameReady()) {
				auto ret = apr::pool::perform([&] {
					pushNotificator(_reader.pool);
					apr_pool_userdata_set(this, config::getSerenityWebsocketHandleName(), nullptr, _reader.pool);
					if (!onFrame(_reader.type, _reader.frame.buffer)) {
						return false;
					}
					return true;
				}, _reader.pool);
				_reader.popFrame();
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

bool Handler::writeSocket(const apr_pollfd_t *fd) {
	while (!_writer.empty()) {
		auto slot = _writer.nextReadSlot();

		while (!slot->empty()) {
			auto len = slot->getNextLength();
			auto ptr = slot->getNextBytes();

			auto written = writeNonBlock(ptr, len);
			if (written > 0) {
				slot->pop(written);
			}
			if (written != len) {
				break;
			}
		}

		if (slot->empty()) {
			_writer.popReadSlot();
		} else {
			break;
		}
	}

	if (_writer.empty()) {
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

bool Handler::writeToSocket(apr_bucket_brigade *bb, const uint8_t *bytes, size_t &count) {
	auto r = _request.request();
	auto of = r->connection->output_filters;

	ap_fwrite(of, bb, (const char *)bytes, count);
	ap_fflush(of, bb);
	apr_brigade_cleanup(bb);

 	return true;
}

bool Handler::writeBrigade(apr_bucket_brigade *bb) {
	bool ret = true;
	size_t offset = 0;
	auto e = APR_BRIGADE_FIRST(bb);
	if (_writer.empty()) {
		for (; e != APR_BRIGADE_SENTINEL(bb); e = APR_BUCKET_NEXT(e)) {
			if (!APR_BUCKET_IS_METADATA(e)) {
				const char * ptr = nullptr;
				apr_size_t len = 0;

				apr_bucket_read(e, &ptr, &len, APR_BLOCK_READ);

				auto written = writeNonBlock((const uint8_t *)ptr, len);
				if (written != len) {
					offset = written;
					break;
				}
			}
		}
	}

	if (e != APR_BRIGADE_SENTINEL(bb)) {
		ret = writeBrigadeCache(bb, e, offset);
	}

    apr_brigade_cleanup(bb);
	return ret;
}

size_t Handler::writeNonBlock(const uint8_t *data, size_t len) {
	auto err = apr_socket_send(_socket, (const char *)data, &len);

	return len;
}

static size_t Handler_writeBrigadeSize(apr_bucket_brigade *bb, apr_bucket *e, size_t off) {
	size_t ret = 0;
	for (; e != APR_BRIGADE_SENTINEL(bb); e = APR_BUCKET_NEXT(e)) {
		if (!APR_BUCKET_IS_METADATA(e) && e->length != apr_size_t(-1)) {
			ret += e->length;
		}
	}
	return ret - off;
}

bool Handler::writeBrigadeCache(apr_bucket_brigade *bb, apr_bucket *e, size_t off) {
	auto size = Handler_writeBrigadeSize(bb, e, off);
	if (size == 0) {
		return true;
	}

	auto slot = _writer.nextEmplaceSlot(size);
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

Handler::StatusCode Handler::resolveStatus(StatusCode code) {
	if (code == StatusCode::Auto) {
		switch (_reader.error) {
		case FrameReader::Error::NotInitialized: return StatusCode::UnexceptedCondition; break;
		case FrameReader::Error::ExtraIsNotEmpty: return StatusCode::ProtocolError; break;
		case FrameReader::Error::NotMasked: return StatusCode::ProtocolError; break;
		case FrameReader::Error::UnknownOpcode: return StatusCode::ProtocolError; break;
		case FrameReader::Error::InvalidSegment: return StatusCode::ProtocolError; break;
		case FrameReader::Error::InvalidSize: return StatusCode::TooLarge; break;
		case FrameReader::Error::InvalidAction: return StatusCode::UnexceptedCondition; break;
		default: return StatusCode::Ok; break;
		}
	} else if (code == StatusCode::None) {
		return StatusCode::Ok;
	}
	return code;
}

void Handler::pushNotificator(apr_pool_t *pool) {
	apr::pool::perform([&] {
		messages::setNotifications(pool, [this] (data::Value && data) {
			send(data::Value{
				std::make_pair("error", data::Value(std::move(data)))
			});
		}, [this] (data::Value && data) {
			send(data::Value{
				std::make_pair("debug", data::Value(std::move(data)))
			});
		});
	}, pool);
}

void Handler::cancel() {
	_ended = true;
	// drain all data
	StackBuffer<(size_t)1_KiB> sbuf;

	if (_valid) {
		apr_status_t err = APR_SUCCESS;
		while (err == APR_SUCCESS) {
			size_t len = 1_KiB;
			err = Handler_readSocket_request(_request, _reader.tmpbb, _reader.pool, (char *)sbuf.data(), &len);
			sbuf.clear();
		}

		apr_socket_opt_set(_socket, APR_SO_NONBLOCK, 0);
		apr_socket_timeout_set(_socket, -1);

		writeSocket(&_pollfd);
	}

	sbuf.clear();

	StatusCode nstatus = resolveStatus(_serverCloseCode);
	uint16_t status = ByteOrder::HostToNetwork(_clientCloseCode==StatusCode::None?(uint16_t)nstatus:(uint16_t)_clientCloseCode);
	size_t memSize = std::min((size_t)123, _serverReason.size());
	size_t frameSize = 4 + memSize;
	sbuf[0] = (0b10000000 | 0x8);
	sbuf[1] = ((uint8_t)(memSize + 2));
	memcpy(sbuf.data() + 2, &status, sizeof(uint16_t));
	if (!_serverReason.empty()) {
		memcpy(sbuf.data() + 4, _serverReason.data(), memSize);
	}

	writeToSocket(_writer.tmpbb, sbuf.data(), frameSize);
}

bool Handler::onControlFrame(FrameType type, const StackBuffer<128> &b) {
	if (type == FrameType::Close) {
		_clientCloseCode = (StatusCode)(b.get<DataReader<ByteOrder::Network>>().readUnsigned16());
		return false;
	} else if (type == FrameType::Ping) {
		trySend(FrameType::Pong, nullptr, 0);
	}
	return true;
}

NS_SA_EXT_END(websocket)
