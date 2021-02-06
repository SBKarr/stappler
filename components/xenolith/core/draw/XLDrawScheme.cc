/**
 Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#include "XLDrawScheme.h"
#include "XLDrawCommand.h"

namespace stappler::xenolith::draw {

DrawScheme *DrawScheme::create(memory::pool_t *p) {
	auto s = new (p) DrawScheme;
	s->pool = p;
	return s;
}

void DrawScheme::destroy(DrawScheme *s) {
	delete s;
}

static void appendToBuffer(memory::pool_t *p, Buffer &vec, BytesView b) {
	// dirty hack with low-level stappler memory types to bypass safety validation
	auto origSize = vec.size();
	auto newSize = origSize + b.size();
	auto newmem = max(newSize, vec.capacity() * 2);
	auto alloc = Buffer::allocator(p);
	vec.grow_alloc(alloc, newmem);
	memcpy(vec.data() + origSize, b.data(), b.size());
	vec.set_size(newSize);
}

void DrawScheme::pushDrawIndexed(CommandGroup *g, vk::Pipeline *p, SpanView<Vertex_V4F_C4F_T2F> vertexes, SpanView<uint16_t> indexes) {
	memory::pool::context ctx(pool);

	if (!g) {
		if (!group) {
			group = CommandGroup::create(pool);
		}
		g = group;
	}

	if (!g->last || g->last->type != CommandType::DrawIndexedIndirect || g->last->pipeline != p) {
		// add new command
		auto cmd = Command::create(pool, CommandType::DrawIndexedIndirect, p);
		auto cmdData = (CmdDrawIndexedIndirect *)cmd->data;
		cmdData->offset = draw.size(); // byte offset
		cmdData->drawCount = 1;
		cmdData->stride = sizeof(CmdDrawIndexedIndirectData);

		if (!g->last) {
			g->first = cmd;
		} else {
			g->last->next = cmd;
		}
		g->last = cmd;
	} else {
		// merge with existed command
		auto cmdData = (CmdDrawIndexedIndirect *)g->last->data;

		++ cmdData->drawCount;
	}

	auto it = vertex.find(VertexFormat::Vertex_V4F_C4F_T2F);
	if (it == vertex.end()) {
		it = vertex.emplace(VertexFormat::Vertex_V4F_C4F_T2F).first;
	}

	CmdDrawIndexedIndirectData data;
	data.indexCount = indexes.size();
	data.instanceCount = 1;
	data.firstIndex = index.size() * sizeof(uint16_t);
	data.vertexOffset = it->second.size() / sizeof(Vertex_V4F_C4F_T2F);
	data.firstInstance = 0;

	appendToBuffer(pool, index, indexes.bytes());
	appendToBuffer(pool, it->second, vertexes.bytes());
	appendToBuffer(pool, draw, BytesView((const uint8_t *)&data, sizeof(CmdDrawIndexedIndirectData)));
}

}
