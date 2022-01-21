// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017-2021 Roman Katuntsev <sbkarr@stappler.org>

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

NS_SA_EXT_BEGIN(websocket)

struct Frame {
	bool fin; // fin value inside current frame
	FrameType type; // opcode from first frame
	Bytes buffer; // common data buffer
	size_t block; // size of completely written block when segmented
	size_t offset; // offset inside current frame
};

struct FrameReader : AllocPool {
	enum class Status : uint8_t {
		Head,
		Size16,
		Size64,
		Mask,
		Body,
		Control
	};

	enum class Error : uint8_t {
		None,
		NotInitialized, // error in reader initialization
		ExtraIsNotEmpty,// rsv 1-3 is not empty
		NotMasked,// input frame is not masked
		UnknownOpcode,// unknown opcode in frame
		InvalidSegment,// invalid FIN or OPCODE sequence in segmented frames
		InvalidSize,// frame (or sequence) is larger then max size
		InvalidAction,// Handler tries to perform invalid reading action
	};

	bool fin = false;
	bool masked = false;

	Status status = Status::Head;
	Error error = Error::None;
	FrameType type = FrameType::None;
	uint8_t extra = 0;
	uint32_t mask = 0;
	size_t size = 0;
	size_t max = config::getDefaultWebsocketMax(); // absolute maximum (even for segmented frames)

	Frame frame;
	apr_pool_t *pool;
	StackBuffer<128> buffer;
	apr_bucket_alloc_t *bucket_alloc;
	apr_bucket_brigade *tmpbb;

	FrameReader(apr_pool_t *p, apr_bucket_alloc_t *alloc);

	operator bool() const {  return error == Error::None; }

	size_t getRequiredBytes() const;
	uint8_t * prepare(size_t &len);
	bool save(uint8_t *, size_t nbytes);

	bool isFrameReady() const;
	bool isControlReady() const;
	void popFrame();
	void clear();

	bool updateState();
};

struct NetworkReader : AllocPool {
	apr_pool_t *pool = nullptr;
	apr_bucket_alloc_t *bucket_alloc;
    apr_bucket_brigade *b = nullptr;
    apr_bucket_brigade *tmpbb = nullptr;

    NetworkReader(apr_pool_t *, apr_bucket_alloc_t *alloc);
    void drop();
};

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

static FrameType FrameReader_getTypeFromOpcode(uint8_t opcode) {
	switch (opcode) {
	case 0x0: return FrameType::Continue; break;
	case 0x1: return FrameType::Text; break;
	case 0x2: return FrameType::Binary; break;
	case 0x8: return FrameType::Close; break;
	case 0x9: return FrameType::Ping; break;
	case 0xA: return FrameType::Pong; break;
	}
	return FrameType::None;
}

static bool isControlFrameType(FrameType t) {
	switch (t) {
	case FrameType::Close:
	case FrameType::Continue:
	case FrameType::Ping:
	case FrameType::Pong:
		return true;
		break;
	default:
		return false;
		break;
	}
	return false;
}

StatusCode Connection::resolveStatus(StatusCode code) {
	if (code == StatusCode::Auto) {
		switch (_reader->error) {
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

FrameReader::FrameReader(apr_pool_t *p, apr_bucket_alloc_t *alloc)
: frame(Frame{false, FrameType::None, Bytes(), 0, 0})
, pool(memory::pool::create(p)), bucket_alloc(alloc) {
	if (!pool) {
		error = Error::NotInitialized;
	} else {
	    tmpbb = apr_brigade_create(pool, bucket_alloc);
		new (&frame.buffer) Bytes(pool); // switch allocator
	}
}

size_t FrameReader::getRequiredBytes() const {
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

uint8_t * FrameReader::prepare(size_t &len) {
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

bool FrameReader::save(uint8_t *b, size_t nbytes) {
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

bool FrameReader::updateState() {
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
			messages::error("Websocket", "Invalid control flow", mem::Value(toInt(error)));
			return false;
		}

		if (!frame.buffer.empty()) {
			if (!isControlFrameType(type)) {
				error = Error::InvalidSegment;
				messages::error("Websocket", "Invalid segment", mem::Value(toInt(error)));
				return false;
			}
		}

		if (size > max) {
			error = Error::InvalidSize;
			messages::error("Websocket", "Too large query", mem::Value{{
				pair("size", mem::Value(size)),
				pair("max", mem::Value(max)),
			}});
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
		size = buffer.get<BytesViewNetwork>().readUnsigned16();
		if (size > max) {
			error = Error::InvalidSize;
			messages::error("Websocket", "Too large query", mem::Value{{
				pair("size", mem::Value(size)),
				pair("max", mem::Value(max)),
			}});
			return false;
		}
		status = masked?Status::Mask:Status::Body;
		buffer.clear();
		shouldPrepareBody = true;
		break;
	case Status::Size64:
		size = buffer.get<BytesViewNetwork>().readUnsigned64();
		if (size > max) {
			error = Error::InvalidSize;
			messages::error("Websocket", "Too large query", mem::Value{{
				pair("size", mem::Value(size)),
				pair("max", mem::Value(max)),
			}});
			return false;
		}
		status = masked?Status::Mask:Status::Body;
		buffer.clear();
		shouldPrepareBody = true;
		break;
	case Status::Mask:
		mask = buffer.get<BytesView>().readUnsigned32();
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
		if (isControlFrameType(type)) {
			status = Status::Control;
		} else {
			if (size + frame.block > max) {
				error = Error::InvalidSize;
				messages::error("Websocket", "Too large query", mem::Value{{
					pair("size", mem::Value(size + frame.block)),
					pair("max", mem::Value(max)),
				}});
				return false;
			}
			frame.buffer.resize(size + frame.block);
		}
	}
	return true;
}

bool FrameReader::isControlReady() const {
	if (status == Status::Control && getRequiredBytes() == 0) {
		return true;
	}
	return false;
}
bool FrameReader::isFrameReady() const {
	if (status == Status::Body && getRequiredBytes() == 0 && frame.fin) {
		return true;
	}
	return false;
}
void FrameReader::popFrame() {
	switch (status) {
	case Status::Control:
		buffer.clear();
		status = Status::Head;
		break;
	case Status::Body:
		clear();
		break;
	default:
		error = Error::InvalidAction;
		break;
	}
}

void FrameReader::clear() {
	frame.buffer.force_clear();
	frame.buffer.clear();
	apr_brigade_destroy(tmpbb);
	memory::pool::clear(pool); // clear frame-related data

	// recreate cleared bb
    tmpbb = apr_brigade_create(pool, bucket_alloc);
	status = Status::Head;
	frame.block = 0;
	frame.offset = 0;
	frame.fin = true;
	frame.type = FrameType::None;
}

NetworkReader::NetworkReader(apr_pool_t *p, apr_bucket_alloc_t *alloc) : bucket_alloc(alloc) {
	pool = mem::pool::create(p);
}

void NetworkReader::drop() {
	if (tmpbb && b) {
		if (APR_BRIGADE_EMPTY(tmpbb) && APR_BRIGADE_FIRST(b) == APR_BRIGADE_LAST(b)) {
			mem::pool::clear(pool);
			b = nullptr;
			tmpbb = nullptr;
		}
	}
}

NS_SA_EXT_END(websocket)
