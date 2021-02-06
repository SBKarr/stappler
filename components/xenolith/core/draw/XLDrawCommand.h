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

#ifndef COMPONENTS_XENOLITH_CORE_DRAW_XLDRAWCOMMAND_H_
#define COMPONENTS_XENOLITH_CORE_DRAW_XLDRAWCOMMAND_H_

#include "XLDraw.h"

namespace stappler::xenolith::draw {

struct CmdCommandGroup {
	CommandGroup *group;
};

struct CmdDrawIndexedIndirectData {
	uint32_t indexCount; // indexCount is the number of vertices to draw
	uint32_t instanceCount; // instanceCount is the number of instances to draw
	uint32_t firstIndex; // firstIndex is the base index within the index buffer
	int32_t vertexOffset; // vertexOffset is the value added to the vertex index before indexing into the vertex buffer
	uint32_t firstInstance; // firstInstance is the instance ID of the first instance to draw
};

struct CmdDrawIndexedIndirect {
	uint64_t offset; // offset is the byte offset into buffer where parameters begin
	uint32_t drawCount; // drawCount is the number of draws to execute, and can be zero
	uint32_t stride; // stride is the byte stride between successive sets of draw parameters
};

struct Command {
	static Command *create(memory::pool_t *, CommandType t, vk::Pipeline *);

	Command *next;
	CommandType type;
	vk::Pipeline *pipeline;
	void *data;
};

struct CommandGroup {
	static CommandGroup *create(memory::pool_t *);

	CommandGroup *next;
	Command *first;
	Command *last;
};

}

#endif /* COMPONENTS_XENOLITH_CORE_DRAW_XLDRAWCOMMAND_H_ */
