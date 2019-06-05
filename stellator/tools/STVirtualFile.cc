/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "STVirtualFile.h"

namespace stellator::tools {

struct VirtualFilesystemHandle {
	VirtualFilesystemHandle() : count(0) { }

	VirtualFile add(mem::StringView n, const mem::StringView &c) {
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

VirtualFile VirtualFile::add(const mem::StringView &n, const mem::StringView &c) {
	return s_handle.add(n, c);
}

mem::StringView VirtualFile::get(const mem::StringView &path) {
	for (size_t i = 0; i < s_handle.count; ++i) {
		if (path == s_handle.table[i].name) {
			return StringView(s_handle.table[i].content);
			break;
		}
	}
	return mem::StringView();
}

const VirtualFile *VirtualFile::getList(size_t &count) {
	count = s_handle.count;
	return s_handle.table;
}

}
