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

#ifndef COMPONENTS_XENOLITH_CORE_DRAW_XLDRAWCOMMANDFLOW_H_
#define COMPONENTS_XENOLITH_CORE_DRAW_XLDRAWCOMMANDFLOW_H_

#include "XLDraw.h"

namespace stappler::xenolith::draw {

struct CmdCommandGroup {
	CommandGroup *group;
};

struct CmdDrawIndexedIndirectCount {
	BufferHandle *buffer;
	uint64_t offset;
	BufferHandle *countBuffer;
	uint64_t countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct Command {
	Command *next;
	CommandType type;
	void *cmd;
};

struct CommandGroup {
	CommandGroup *next;
	Command *cmd;
};

}



#endif /* COMPONENTS_XENOLITH_CORE_DRAW_XLDRAWCOMMANDFLOW_H_ */
