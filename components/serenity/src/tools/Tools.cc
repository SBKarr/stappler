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

void registerTools(const String &prefix, Server &serv) {
	serv.addHandler(prefix, SA_HANDLER(tools::ServerGui));
	serv.addHandler(toString(prefix, config::getServerToolsShell(), "/"), SA_HANDLER(tools::ShellGui));
	serv.addHandler(toString(prefix, config::getServerToolsErrors(), "/"), SA_HANDLER(tools::ErrorsGui));
	serv.addHandler(toString(prefix, "/docs/"), SA_HANDLER(tools::DocsGui));
	serv.addHandler(toString(prefix, "/handlers"), SA_HANDLER(tools::HandlersGui));
	serv.addHandler(toString(prefix, "/reports/"), SA_HANDLER(tools::ReportsGui));
	serv.addWebsocket(prefix + config::getServerToolsShell(), new tools::ShellSocket());

	serv.addHandler(prefix + config::getServerToolsAuth(), SA_HANDLER(tools::AuthHandler));
	serv.addHandler(prefix + config::getServerVirtualFilesystem(), SA_HANDLER(tools::VirtualFilesystem));
}

static String Tools_getCancelUrl(Request &rctx) {
	StringStream cancelUrl;
	bool isSecure = rctx.isSecureConnection();
	cancelUrl << (isSecure?"https":"http") << "://nobody@" << rctx.getHostname();
	auto port = rctx.getParsedURI().port();
	if (port && ((isSecure && port != 443) || (!isSecure && port != 80))) {
		cancelUrl << ':' << port;
	}
	cancelUrl << "/__server";
	return cancelUrl.str();
}

NS_SA_EXT_END(tools)
