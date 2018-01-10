/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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
		size_t len;
	};

	VirtualFilesystemHandle() : count(0) { }

	void add(const char *n, const char *c) {
		if (strncmp(n, "serenity/virtual", "serenity/virtual"_len) == 0) {
			n += "serenity/virtual"_len;
		}
		if (count < 255) {
			table[count].name = n;
			table[count].content = c;
			table[count].len = strlen(c);
			++ count;
		}
	}

	size_t count;
	Rec table[255] = { };
};

static VirtualFilesystemHandle s_handle;

StringView VirtualFile::get(const String &path) {
	return get(path.data());
}

StringView VirtualFile::get(const char *path) {
	for (size_t i = 0; i < s_handle.count; ++i) {
		if (strcmp(path, s_handle.table[i].name) == 0) {
			return StringView(s_handle.table[i].content, s_handle.table[i].len);
			break;
		}
	}
	return StringView();
}

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
			} else if (_subPath.compare(_subPath.length() - 5, 5, ".html") == 0) {
				rctx.setContentType("text/html;charset=UTF-8");
			}
			rctx << StringView(s_handle.table[i].content, s_handle.table[i].len);
			return DONE;
			break;
		}
	}

	return HTTP_NOT_FOUND;
}

NS_SA_EXT_END(tools)
