/*
 * ToolsVirtual.cpp
 *
 *  Created on: 21 апр. 2016 г.
 *      Author: sbkarr
 */

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
	Rec table[255];
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
