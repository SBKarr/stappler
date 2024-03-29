// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include <sys/epoll.h>
#include <sys/inotify.h>

#include "Server.h"

APLOG_USE_MODULE(serenity);

NS_SA_BEGIN

struct PollClient : mem::AllocBase {
	enum Type {
		None,
		INotify,
		Postgres,
		TemplateINotify,
	};

	Server server;
	void *ptr = nullptr;
	Type type = None;
	int fd = -1;
    struct epoll_event event;
};

NS_SA_END

#include "SPAprAllocator.cc"
#include "SPAprFileStream.cc"
#include "SPAprFilesystem.cc"
#include "SEForward.cc"
#include "SEServer.cc"
#include "SERoot.cc"
#include "SERootDebug.cc"
#include "SERootMime.cc"
#include "SEDbdModule.cc"

#include "SEConnection.cc"
#include "SERequest.cc"
#include "SEInputFilter.cc"
#include "OutputFilter.cc"
#include "MultipartParser.cc"
#include "UrlEncodeParser.cc"

#include "WebSocketConnection.cc"
#include "WebSocket.cc"

#include "brotli_compress.cc"
#include "mod_serenity.cc"
