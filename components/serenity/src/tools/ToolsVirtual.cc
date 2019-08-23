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
	VirtualFilesystemHandle() : count(0) { }

	VirtualFile add(StringView n, const StringView &c) {
		if (n.starts_with("serenity/virtual")) {
			n += "serenity/virtual"_len;
		}
		if (count < 255) {
			table[count].name = n;
			table[count].content = c;
			++ count;
		}
		return table[count - 1];
	}

	size_t count = 0;
	VirtualFile table[255] = { };
};

static VirtualFilesystemHandle s_handle;

VirtualFile VirtualFile::add(const StringView &n, const StringView &c) {
	return s_handle.add(n, c);
}

StringView VirtualFile::get(const StringView &path) {
	for (size_t i = 0; i < s_handle.count; ++i) {
		if (path == s_handle.table[i].name) {
			return StringView(s_handle.table[i].content);
			break;
		}
	}
	return StringView();
}

const VirtualFile *VirtualFile::getList(size_t &count) {
	count = s_handle.count;
	return s_handle.table;
}

int VirtualFilesystem::onTranslateName(Request &rctx) {
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

			if (output::checkCacheHeaders(rctx, getCompileUnixTime(), hash::hash32(s_handle.table[i].name.data(), s_handle.table[i].name.size()))) {
				return HTTP_NOT_MODIFIED;
			}

			rctx << s_handle.table[i].content;
			return DONE;
			break;
		}
	}

	return HTTP_NOT_FOUND;
}

NS_SA_EXT_END(tools)
