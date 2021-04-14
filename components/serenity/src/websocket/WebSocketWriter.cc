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

struct WriteSlot : AllocPool {
	struct Slice {
		uint8_t *data;
		size_t size;
		Slice *next;
	};

	apr_pool_t *pool;
	size_t alloc = 0;
	size_t offset = 0;
	Slice *firstData = nullptr;
	Slice *lastData = nullptr;

	WriteSlot *next = nullptr;

	WriteSlot(apr_pool_t *p) : pool(p) { }

	bool empty() const { return firstData == nullptr; }

	void emplace(const uint8_t *data, size_t size);
	void pop(size_t);

	uint8_t * getNextBytes() const { return firstData->data + offset; }
	size_t getNextLength() const { return firstData->size - offset; }
};

struct FrameWriter : AllocPool {
	apr_pool_t *pool = nullptr;
	apr_bucket_brigade *tmpbb = nullptr;
	WriteSlot *firstSlot = nullptr;
	WriteSlot *lastSlot = nullptr;

	FrameWriter(apr_pool_t *, apr_bucket_alloc_t *alloc);

	bool empty() const { return firstSlot == nullptr; }

	WriteSlot *nextReadSlot() const { return firstSlot; }
	void popReadSlot();

	WriteSlot *nextEmplaceSlot(size_t sizeOfData);
};

static uint8_t getOpcodeFromType(FrameType opcode) {
	switch (opcode) {
	case FrameType::Continue: return 0x0; break;
	case FrameType::Text: return 0x1; break;
	case FrameType::Binary: return 0x2; break;
	case FrameType::Close: return 0x8; break;
	case FrameType::Ping: return 0x9; break;
	case FrameType::Pong: return 0xA; break;
	default: break;
	}
	return 0;
}

static void makeHeader(StackBuffer<32> &buf, size_t dataSize, FrameType t) {
	size_t sizeSize = (dataSize <= 125) ? 0 : ((dataSize > (size_t)maxOf<uint16_t>())? 8 : 2);
	size_t frameSize = 2 + sizeSize;

	buf.prepare(frameSize);

	buf[0] = ((uint8_t)0b10000000 | getOpcodeFromType(t));
	if (sizeSize == 0) {
		buf[1] = ((uint8_t)dataSize);
	} else if (sizeSize == 2) {
		buf[1] = ((uint8_t)126);
		uint16_t size = byteorder::HostToNetwork((uint16_t)dataSize);
		memcpy(buf.data() + 2, &size, sizeof(uint16_t));
	} else if (sizeSize == 8) {
		buf[1] = ((uint8_t)127);
		uint64_t size = byteorder::HostToNetwork((uint64_t)dataSize);
		memcpy(buf.data() + 2, &size, sizeof(uint64_t));
	}

	buf.save(nullptr, frameSize);
}

void WriteSlot::emplace(const uint8_t *data, size_t size) {
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

void WriteSlot::pop(size_t size) {
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

FrameWriter::FrameWriter(apr_pool_t *p, apr_bucket_alloc_t *alloc) : pool(p) {
	tmpbb = apr_brigade_create(p, alloc);
}

void FrameWriter::popReadSlot() {
	if (firstSlot->empty()) {
		memory::pool::destroy(firstSlot->pool);
		firstSlot = firstSlot->next;
		if (!firstSlot) {
			lastSlot = nullptr;
		}
	}
}

WriteSlot *FrameWriter::nextEmplaceSlot(size_t sizeOfData) {
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
