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

uint8_t Handler::getOpcodeFromType(Handler::FrameType opcode) {
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

bool Handler::isControl(Handler::FrameType t) {
	return t == Handler::FrameType::Close || t == Handler::FrameType::Continue
			|| t == Handler::FrameType::Ping || t == Handler::FrameType::Pong;
}

void Handler::makeHeader(StackBuffer<32> &buf, size_t dataSize, Handler::FrameType t) {
	size_t sizeSize = (dataSize <= 125) ? 0 : ((dataSize > (size_t)maxOf<uint16_t>())? 8 : 2);
	size_t frameSize = 2 + sizeSize;

	buf.prepare(frameSize);

	buf[0] = ((uint8_t)0b10000000 | getOpcodeFromType(t));
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

Handler::WriteSlot::WriteSlot(apr_pool_t *p) : pool(p) { }

bool Handler::WriteSlot::empty() const {
	return firstData == nullptr;
}

void Handler::WriteSlot::emplace(const uint8_t *data, size_t size) {
	auto mem = memory::pool::palloc(pool, sizeof(Slice) + size);
	Slice *next = (Slice *)mem;
	next->data = (uint8_t *)mem + sizeof(Slice);
	next->size = size;
	next->next = nullptr;

	memcpy(next->data, data, size);

	if (lastData) {
		lastData->next = next;
		lastData = next;
	} else {
		firstData = next;
		lastData = next;
	}

	alloc += size + sizeof(Slice);
}

void Handler::WriteSlot::pop(size_t size) {
	if (size >= firstData->size - offset) {
		firstData = firstData->next;
		if (!firstData) {
			lastData = nullptr;
		}
		offset = 0;
	} else {
		offset += size;
	}
}

uint8_t * Handler::WriteSlot::getNextBytes() const {
	return firstData->data + offset;
}
size_t Handler::WriteSlot::getNextLength() const {
	return firstData->size - offset;
}

Handler::FrameWriter::FrameWriter(const Request &req, apr_pool_t *pool) : pool(pool) {
	tmpbb = apr_brigade_create(pool, req.request()->connection->bucket_alloc);
}

bool Handler::FrameWriter::empty() const {
	return firstSlot == nullptr;
}

Handler::WriteSlot *Handler::FrameWriter::nextReadSlot() const {
	return firstSlot;
}

void Handler::FrameWriter::popReadSlot() {
	if (firstSlot->empty()) {
		memory::pool::destroy(firstSlot->pool);
		firstSlot = firstSlot->next;
		if (!firstSlot) {
			lastSlot = nullptr;
		}
	}
}

Handler::WriteSlot *Handler::FrameWriter::nextEmplaceSlot(size_t sizeOfData) {
	if (!lastSlot || lastSlot->alloc + sizeOfData > 16_KiB) {
		auto p = memory::pool::create(pool);
		WriteSlot *slot = new (p) WriteSlot(p);
		if (lastSlot) {
			lastSlot->next = slot;
			lastSlot = slot;
		} else {
			firstSlot = slot;
			lastSlot = slot;
		}
	}
	return lastSlot;
}

NS_SA_EXT_END(websocket)
