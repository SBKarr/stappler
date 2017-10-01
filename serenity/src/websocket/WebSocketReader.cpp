// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

Handler::FrameReader::FrameReader(const Request &req, apr_pool_t *p, size_t maxFrameSize)
: fin(false), masked(false), status(Status::Head), error(Error::None), type(FrameType::None), extra(0)
, mask(0) , size(0), max(maxFrameSize), frame(Frame{false, FrameType::None, Bytes(), 0, 0})
, pool(nullptr), bucket_alloc(nullptr), tmpbb(nullptr) {
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
			if (!isControl(type)) {
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
		if (isControl(type)) {
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
		frame.buffer.clear();
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

NS_SA_EXT_END(websocket)
