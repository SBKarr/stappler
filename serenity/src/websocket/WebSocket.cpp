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

#include "Define.h"
#include "WebSocket.h"
#include "Root.h"

#define WEBSOCKET_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WEBSOCKET_GUID_LEN "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"_len

NS_SA_EXT_BEGIN(websocket)

static String makeAcceptKey(const String &key) {
	apr_byte_t digest[APR_SHA1_DIGESTSIZE];
	apr_sha1_ctx_t context;

	apr_sha1_init(&context);
	apr_sha1_update(&context, key.c_str(), key.size());
	apr_sha1_update(&context, (const char *)WEBSOCKET_GUID, (unsigned int)WEBSOCKET_GUID_LEN);
	apr_sha1_final(digest, &context);

	return base64::encode(digest, APR_SHA1_DIGESTSIZE);
}

template <typename B>
static size_t FrameReader_requiredBytes(const B &buffer, size_t max) {
	return (buffer.size() < max) ? (max - buffer.size()) : 0;
}

static void FrameReader_unmask(uint32_t mask, size_t offset, uint8_t *data, size_t nbytes) {
	uint8_t j = offset % 4;
	for (size_t i = 0; i < nbytes; ++i, ++j) {
		if (j >= 4) { j = 0; }
		data[i] ^= ((mask >> (j * 8)) & 0xFF);
	}
}

static Handler::FrameType FrameReader_getTypeFromOpcode(uint8_t opcode) {
	switch (opcode) {
	case 0x0: return Handler::FrameType::Continue; break;
	case 0x1: return Handler::FrameType::Text; break;
	case 0x2: return Handler::FrameType::Binary; break;
	case 0x8: return Handler::FrameType::Close; break;
	case 0x9: return Handler::FrameType::Ping; break;
	case 0xA: return Handler::FrameType::Pong; break;
	}
	return Handler::FrameType::None;
}

static bool FrameReader_isControl(Handler::FrameType t) {
	return t == Handler::FrameType::Close || t == Handler::FrameType::Continue
			|| t == Handler::FrameType::Ping || t == Handler::FrameType::Pong;
}

static uint8_t FrameWriter_getOpcodeFromType(Handler::FrameType opcode) {
	switch (opcode) {
	case Handler::FrameType::Continue: return 0x0; break;
	case Handler::FrameType::Text: return 0x1; break;
	case Handler::FrameType::Binary: return 0x2; break;
	case Handler::FrameType::Close: return 0x8; break;
	case Handler::FrameType::Ping: return 0x9; break;
	case Handler::FrameType::Pong: return 0xA; break;
	default: break;
	}
	return 0;
}

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

	_ssl = _request.isSecureConnection();
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

	AllocStack::perform([&] {
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

	AllocStack::perform([&] {
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
	if (_writer.empty()) {
		return trySend(FrameType::Text, (const uint8_t *)str.data(), str.size());
	} else {
		return _writer.addFrame(FrameType::Text, (const uint8_t *)str.data(), str.size());
	}
}
bool Handler::send(const Bytes &bytes) {
	if (_writer.empty()) {
		return trySend(FrameType::Binary, bytes.data(), bytes.size());
	} else {
		return _writer.addFrame(FrameType::Binary, bytes.data(), bytes.size());
	}
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

static void Handler_makeHeader(StackBuffer<32> &buf, size_t dataSize, Handler::FrameType t) {
	size_t sizeSize = (dataSize < 125) ? 0 : ((dataSize > (size_t)maxOf<uint16_t>())? 8 : 2);
	size_t frameSize = 2 + sizeSize;

	buf.prepare(frameSize);

	buf[0] = ((uint8_t)0b10000000 | FrameWriter_getOpcodeFromType(t));
	if (sizeSize == 0) {
		buf[1] = ((uint8_t)dataSize);
	} else if (sizeSize == 2) {
		buf[1] = ((uint8_t)126);
		uint16_t size = ByteOrder::HostToNetwork((uint16_t)dataSize);
		memcpy(buf.data() + 2, &size, sizeof(uint16_t));
	} else if (sizeSize == 8) {
		buf[1] = ((uint8_t)127);
		uint64_t size = ByteOrder::HostToNetwork((uint64_t)dataSize);
		memcpy(buf.data() + 2, &size, sizeof(uint64_t));
	}

	buf.save(nullptr, frameSize);
}

bool Handler::trySend(FrameType t, const uint8_t *bytes, size_t count) {
	if (_writer.empty()) {
		size_t offset = 0;
		StackBuffer<32> buf;
		Handler_makeHeader(buf, count, FrameType::Text);

		offset = buf.size();

		auto bb = _writer.tmpbb;
		auto r = _request.request();
		auto of = r->connection->output_filters;

		ap_fwrite(of, bb, (const char *)buf.data(), offset);
		if (count > 0) {
			ap_fwrite(of, bb, (const char *)bytes, count);
		}

		ap_fflush(of, bb);
		apr_brigade_cleanup(bb);
		return true;
	}
	return false;
}

storage::Adapter *Handler::storage() const {
	auto pool = AllocStack::get().top();

	database::Handle *db = nullptr;
	apr_pool_userdata_get((void **)&db, (const char *)config::getSerenityWebsocketDatabaseName(), pool);
	if (!db) {
		db = new (pool) database::Handle(pool, Root::getInstance()->dbdPoolAcquire(_request.server(), pool));
		apr_pool_userdata_set(db, (const char *)config::getSerenityWebsocketDatabaseName(), NULL, pool);
	}
	return db;
}

void Handler::receiveBroadcast(const data::Value &data) {
	if (_valid) {
		_broadcastMutex.lock();
		if (!_broadcastsPool) {
			apr_pool_create(&_broadcastsPool, _connection.pool());
		}
		if (_broadcastsPool) {
			AllocStack::perform([&] {
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
	apr_pool_t *pool = nullptr;
	Vector<data::Value> * vec = nullptr;

	_broadcastMutex.lock();

	pool = _broadcastsPool;
	vec = _broadcastsMessages;

	_broadcastsPool = nullptr;
	_broadcastsMessages = nullptr;

	_broadcastMutex.unlock();

	bool ret = true;
	if (pool) {
		apr_pool_userdata_set(this, config::getSerenityWebsocketHandleName(), nullptr, pool);
		AllocStack::perform([&] {
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
	if ((fd->rtnevents & APR_POLLIN) != 0) {
		if (!readSocket(fd)) {
			return false;
		}
	} else if ((fd->rtnevents & APR_POLLOUT) != 0) {
		if (!writeSocket(fd)) {
			return false;
		}
	}
	_pollfd.rtnevents = 0;
	return true;
}

static apr_status_t Handler_readSocket_request(Request &req, apr_bucket_brigade *bb, apr_pool_t *pool, char *buf, size_t *len) {
	auto r = req.request();
    apr_status_t rv;
    apr_size_t readbufsiz = *len;

    ap_filter_t *f = r->input_filters;
    if (f->frec && f->frec->name && !strcasecmp(f->frec->name, "reqtimeout")) {
    	f = f->next;
    }

    if (bb != NULL) {
        if ((rv = ap_get_brigade(f, bb, AP_MODE_READBYTES, APR_NONBLOCK_READ, readbufsiz)) == APR_SUCCESS) {
            if ((rv = apr_brigade_flatten(bb, buf, len)) == APR_SUCCESS) {
                readbufsiz = *len;
            }
        }
        apr_brigade_cleanup(bb);
    }
    if (readbufsiz == 0) {
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
			}

			if (_reader.isControlReady()) {
				auto ret = onControlFrame(_reader.type, _reader.buffer);
				_reader.popFrame();
				if (!ret) {
					return false;
				}
			} else if (_reader.isFrameReady()) {
				auto ret = AllocStack::perform([&] {
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
	if (!_writer.empty()) {
		apr_status_t err = APR_SUCCESS;
		while (err == APR_SUCCESS && !_writer.empty()) {
			size_t len = _writer.getReadyLength();
			uint8_t *buf = _writer.getReadyBytes();
			auto bb = _writer.getReadyBrigade(_request);

			writeToSocket(bb, buf, len);

			_writer.drop(buf, len);
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
	AllocStack::perform([&] {
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
	// drain all data
	StackBuffer<(size_t)1_KiB> sbuf;

	if (_valid) {
		apr_status_t err = APR_SUCCESS;
		while (err == APR_SUCCESS) {
			size_t len = 1_KiB;
			err = Handler_readSocket_request(_request, _reader.tmpbb, _reader.pool, (char *)sbuf.data(), &len);
			sbuf.clear();
		}

		_writer.skipBuffered();

		apr_socket_opt_set(_socket, APR_SO_NONBLOCK, 0);
		apr_socket_timeout_set(_socket, -1);

		while (!_writer.empty()) {
			size_t len = _writer.getReadyLength();
			uint8_t *buf = _writer.getReadyBytes();
			auto bb = _writer.getReadyBrigade(_request);

			writeToSocket(bb, buf, len);
			_writer.drop(buf, len);
		}
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
		_writer.addControlFrame(FrameType::Pong);
		writeSocket(&_pollfd);
	}
	return true;
}

Handler::FrameReader::FrameReader(const Request &req, apr_pool_t *p, size_t maxFrameSize)
: fin(false), masked(false), status(Status::Head), error(Error::None), type(FrameType::None), extra(0)
, mask(0) , size(0), max(maxFrameSize), pool(nullptr), bucket_alloc(nullptr), tmpbb(nullptr) {
	apr_pool_create(&pool, p);
	if (!pool) {
		error = Error::NotInitialized;
	} else {
		bucket_alloc = req.request()->connection->bucket_alloc;
	    tmpbb = apr_brigade_create(pool, bucket_alloc);
		new (&frame.buffer) Bytes(pool); // switch allocator
	}
}

Handler::FrameReader::operator bool() const {
	return error == Error::None;
}

size_t Handler::FrameReader::getRequiredBytes() const {
	switch (status) {
	case Status::Head: return FrameReader_requiredBytes(buffer, 2); break;
	case Status::Size16: return FrameReader_requiredBytes(buffer, 2); break;
	case Status::Size64: return FrameReader_requiredBytes(buffer, 8); break;
	case Status::Mask: return FrameReader_requiredBytes(buffer, 4); break;
	case Status::Body: return (frame.offset < size) ? (size - frame.offset) : 0; break;
	case Status::Control: return FrameReader_requiredBytes(buffer, size); break;
	default: break;
	}
	return 0;
}
uint8_t * Handler::FrameReader::prepare(size_t &len) {
	switch (status) {
	case Status::Head:
	case Status::Size16:
	case Status::Size64:
	case Status::Mask:
	case Status::Control:
		return buffer.prepare_preserve(len); break;
	case Status::Body:
		return frame.buffer.data() + frame.block + frame.offset; break;
	default: break;
	}
	return nullptr;
}
bool Handler::FrameReader::save(uint8_t *b, size_t nbytes) {
	switch (status) {
	case Status::Head:
	case Status::Size16:
	case Status::Size64:
	case Status::Mask:
	case Status::Control:
		buffer.save(b, nbytes); break;
	case Status::Body:
		FrameReader_unmask(mask, frame.offset, b, nbytes);
		frame.offset += nbytes;
		break;
	default: break;
	}

	if (getRequiredBytes() == 0) {
		return updateState();
	}
	return true;
}

bool Handler::FrameReader::updateState() {
	bool shouldPrepareBody = false;
	switch (status) {
	case Status::Head:
		size = 0;
		mask = 0;
		type = FrameType::None;

		fin =		(buffer[0] & 0b10000000) != 0;
		extra =		(buffer[0] & 0b01110000);
		type = FrameReader_getTypeFromOpcode
					(buffer[0] & 0b00001111);
		masked =	(buffer[1] & 0b10000000) != 0;
		size =		(buffer[1] & 0b01111111);

		if (extra != 0 || !masked || type == FrameType::None) {
			if (extra != 0) {
				error = Error::ExtraIsNotEmpty;
			} else if (!masked) {
				error = Error::NotMasked;
			} else {
				error = Error::UnknownOpcode;
			}
			return false;
		}

		if (!frame.buffer.empty()) {
			if (!FrameReader_isControl(type)) {
				error = Error::InvalidSegment;
				return false;
			}
		}

		if (size > max) {
			error = Error::InvalidSize;
			return false;
		}

		if (size == 126) {
			size = 0;
			status = Status::Size16;
		} else if (size == 127) {
			size = 0;
			status = Status::Size64;
		} else {
			status = Status::Mask;
		}

		buffer.clear();
		return true;
		break;
	case Status::Size16:
		size = buffer.get<DataReader<ByteOrder::Network>>().readUnsigned16();
		if (size > max) {
			error = Error::InvalidSize;
			return false;
		}
		status = masked?Status::Mask:Status::Body;
		buffer.clear();
		shouldPrepareBody = true;
		break;
	case Status::Size64:
		size = buffer.get<DataReader<ByteOrder::Network>>().readUnsigned64();
		if (size > max) {
			error = Error::InvalidSize;
			return false;
		}
		status = masked?Status::Mask:Status::Body;
		buffer.clear();
		shouldPrepareBody = true;
		break;
	case Status::Mask:
		mask = buffer.get<DataReader<ByteOrder::Host>>().readUnsigned32();
		status = Status::Body;
		buffer.clear();
		shouldPrepareBody = true;
		break;
	case Status::Control:
		break;
	case Status::Body:
		frame.fin = fin;
		frame.block += size;
		if (type != FrameType::Continue) {
			frame.type = type;
		}
		break;
	default:
		break;
	}

	if (shouldPrepareBody && status == Status::Body) {
		if (FrameReader_isControl(type)) {
			status = Status::Control;
		} else {
			if (size + frame.block > max) {
				error = Error::InvalidSize;
				return false;
			}
			frame.buffer.resize(size + frame.block);
		}
	}
	return true;
}

bool Handler::FrameReader::isControlReady() const {
	if (status == Status::Control && getRequiredBytes() == 0) {
		return true;
	}
	return false;
}
bool Handler::FrameReader::isFrameReady() const {
	if (status == Status::Body && getRequiredBytes() == 0 && frame.fin) {
		return true;
	}
	return false;
}
void Handler::FrameReader::popFrame() {
	switch (status) {
	case Status::Control:
		buffer.clear();
		status = Status::Head;
		break;
	case Status::Body:
		frame.buffer.force_clear();
		apr_brigade_cleanup(tmpbb);
		apr_pool_clear(pool); // clear frame-related data
	    tmpbb = apr_brigade_create(pool, bucket_alloc);
		status = Status::Head;
		frame.block = 0;
		frame.offset = 0;
		frame.fin = true;
		frame.type = FrameType::None;
		break;
	default:
		error = Error::InvalidAction;
		break;
	}
}

Handler::FrameWriter::FrameWriter(const Request &req, apr_pool_t *pool) : pool(pool), tmpbb(nullptr) {
	frames.reserve(config::getWebsocketBufferSlots());
	tmpbb = apr_brigade_create(pool, req.request()->connection->bucket_alloc);
}

bool Handler::FrameWriter::empty() const {
	return frames.empty();
}
size_t Handler::FrameWriter::size() const {
	return frames.size();
}

size_t Handler::FrameWriter::getReadyLength() {
	if (!frames.empty()) {
		auto &front = frames.front();
		auto &item = front.storage.at(front.item);
		return item.size() - front.offset;
	} else {
		return 0;
	}
}
uint8_t * Handler::FrameWriter::getReadyBytes() {
	if (!frames.empty()) {
		auto &front = frames.front();
		auto &item = front.storage.at(front.item);
		return item.data() + front.offset;
	} else {
		return nullptr;
	}
}

apr_bucket_brigade *Handler::FrameWriter::getReadyBrigade(const Request &req) {
	if (!frames.empty()) {
		auto &front = frames.front();
		if (!front.bb) {
			front.bb = apr_brigade_create(front.pool, req.request()->connection->bucket_alloc);
		}
		return front.bb;
	} else {
		return nullptr;
	}
}

void Handler::FrameWriter::drop(uint8_t *, size_t nbytes) {
	if (!frames.empty()) {
		auto &front = frames.front();
		auto &item = front.storage.at(front.item);
		front.offset += nbytes;
		if (front.offset == item.size()) {
			++ front.item;
			front.offset = 0;
			if (front.item >= front.storage.size()) {
				apr_pool_destroy(front.pool);
				frames.erase(frames.begin());
			}
		}
	}
}

void Handler::FrameWriter::skipBuffered() {
	if (!frames.empty()) {
		auto &front = frames.front();
		if (front.offset == 0 && front.item == 0) {
			// drop all frames
			for (auto &it : frames) {
				apr_pool_destroy(it.pool);
			}
			frames.clear();
		} else {
			if (front.offset == 0) {
				if (front.item < front.storage.size()) {
					for (size_t i = front.storage.size(); i != front.item; -- i) {
						front.storage.pop_back();
					}
				}
			} else {
				if (front.item < front.storage.size() - 1) {
					for (size_t i = front.storage.size(); i != front.item + 1; -- i) {
						front.storage.pop_back();
					}
				}
			}

			for (size_t i = frames.size(); i != 1; -- i) {
				apr_pool_destroy(frames.back().pool);
				frames.pop_back();
			}
		}
	}
}

uint8_t *Handler::FrameWriter::emplaceFrame(size_t size, bool first, size_t offset) {
	if (frames.empty() || (!first && frames.back().size + size > config::getWebsocketMaxBufferSlotSize())) {
		// create new block
		if (frames.size() >= config::getWebsocketBufferSlots()) {
			return nullptr;
		}

		apr_pool_t *npool = nullptr;
		apr_pool_create(&npool, pool);
		if (npool) {
			frames.emplace_back(npool);
		} else {
			return nullptr;
		}
	}

	if (first) {
		auto &frame = frames.front();
		auto it = frame.storage.emplace(frame.storage.begin() + frame.item + 1, frame.pool);
		it->resize(size);
		frame.size += size;
		return it->data();
	} else {
		auto &frame = frames.back();
		frame.storage.emplace_back(frame.pool);

		auto &item = frame.storage.back();
		item.resize(size);
		frame.size += size;
		if (offset && frame.storage.size() == 0) {
			frame.offset = offset;
		}

		return item.data();
	}
}

bool Handler::FrameWriter::addControlFrame(FrameType t, const String &str) {
	if (!FrameReader_isControl(t) || t == FrameType::Continue) {
		return false;
	}

	size_t dataSize = std::min((size_t)125, str.length());
	size_t frameSize = 2 + dataSize;

	auto buf = emplaceFrame(frameSize, true);
	if (buf) {
		buf[0] = (0b10000000 | FrameWriter_getOpcodeFromType(t));
		buf[1] = ((uint8_t)dataSize);
		memcpy(buf + 2, str.data(), dataSize);
		return true;
	}
	return false;
}

bool Handler::FrameWriter::addFrame(FrameType t, const uint8_t *dbuf, size_t dataSize) {
	size_t sizeSize = (dataSize < 125) ? 0 : ((dataSize > (size_t)maxOf<uint16_t>())? 8 : 2);
	size_t frameSize = 2 + sizeSize + dataSize;

	auto buf = emplaceFrame(frameSize, false);
	if (buf) {
		buf[0] = ((uint8_t)0b10000000 | FrameWriter_getOpcodeFromType(t));
		if (sizeSize == 0) {
			buf[1] = ((uint8_t)dataSize);
		} else if (sizeSize == 2) {
			buf[1] = ((uint8_t)126);
			uint16_t size = ByteOrder::HostToNetwork((uint16_t)dataSize);
			memcpy(buf + 2, &size, sizeof(uint16_t));
		} else if (sizeSize == 8) {
			buf[1] = ((uint8_t)127);
			uint64_t size = ByteOrder::HostToNetwork((uint64_t)dataSize);
			memcpy(buf + 2, &size, sizeof(uint64_t));
		}

		memcpy(buf + 2 + sizeSize, dbuf, dataSize);
		return true;
	}
	return false;
}

bool Handler::FrameWriter::addFrameOffset(const uint8_t *head, size_t nhead, const uint8_t *body, size_t nbody, size_t offset) {
	if (!empty()) {
		return false;
	}

	auto buf = emplaceFrame(nhead + nbody - offset + 1, false, 1);
	if (buf) {
		buf[0] = 0;
		if (offset < nhead) {
			memcpy(buf + 1, head + offset, nhead - offset);
			memcpy(buf + 1 + nhead - offset, body, nbody);
		} else {
			memcpy(buf + 1, body, nbody - offset + nhead);
		}
		return true;
	}
	return false;
}

Manager::Manager() : _pool(getCurrentPool()), _mutex(_pool) { }
Manager::~Manager() { }

Handler * Manager::onAccept(const Request &req) {
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

int Manager::accept(Request &req) {
	auto h = req.getRequestHeaders();
	auto &version = h.at("sec-websocket-version");
	auto &key = h.at("sec-websocket-key");
	auto decKey = base64::decode(key);
	if (decKey.size() != 16 || version != "13") {
		req.getErrorHeaders().emplace("Sec-WebSocket-Version", "13");
		return HTTP_BAD_REQUEST;
	}

	auto handler = onAccept(req);
	if (handler) {
		auto hout = req.getResponseHeaders();

		hout.clear();
		hout.emplace("Upgrade", "websocket");
		hout.emplace("Connection", "Upgrade");
		hout.emplace("Sec-WebSocket-Accept", makeAcceptKey(key));

		apr_socket_timeout_set((apr_socket_t *)ap_get_module_config (req.request()->connection->conn_config, &core_module), -1);
		req.setStatus(HTTP_SWITCHING_PROTOCOLS);
		ap_send_interim_response(req.request(), 1);

		ap_filter_t *input_filter = req.request()->input_filters;
		while (input_filter != NULL) {
			if ((input_filter->frec != NULL) && (input_filter->frec->name != NULL)) {
				if (!strcasecmp(input_filter->frec->name, "http_in")
						|| !strcasecmp(input_filter->frec->name, "reqtimeout")) {
					auto next = input_filter->next;
					ap_remove_input_filter(input_filter);
					input_filter = next;
					continue;
				}
			}
			input_filter = input_filter->next;
		}

		addHandler(handler);
		handler->run();
		removeHandler(handler);

	    ap_lingering_close(req.request()->connection);

	    return DONE;
	}
	if (req.getStatus() == HTTP_OK) {
		return HTTP_BAD_REQUEST;
	}
	return req.getStatus();
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

NS_SA_EXT_END(websocket)
