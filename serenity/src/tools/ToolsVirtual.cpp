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
#include "Tools.h"

NS_SA_EXT_BEGIN(tools)

struct VirtualFilesystemHandle {
	struct Rec {
		const char *name;
		const char *content;
	};

	VirtualFilesystemHandle() : count(0) { }

	void add(const char *n, const char *c) {
		if (count < 255) {
			table[count].name = n;
			table[count].content = c;
			++ count;
		}
	}

	size_t count;
	Rec table[255] = { };
};

static VirtualFilesystemHandle s_handle;

VirtualFile::VirtualFile(const char *n, const char *c) : name(n), content(c) {
	s_handle.add(n, c);
}

int VirtualFilesystem::onPostReadRequest(Request &rctx) {
	if (rctx.getMethod() != Request::Get) {
		return DECLINED;
	}

	for (size_t i = 0; i < s_handle.count; ++i) {
		if (_subPath == s_handle.table[i].name) {
			if (_subPath.compare(_subPath.length() - 3, 3, ".js") == 0) {
				rctx.setContentType("application/javascript");
			} else if (_subPath.compare(_subPath.length() - 4, 4, ".css") == 0) {
				rctx.setContentType("text/css");
			}
			rctx << s_handle.table[i].content;
			return DONE;
			break;
		}
	}

	return HTTP_NOT_FOUND;
}

NS_SA_EXT_END(tools)
