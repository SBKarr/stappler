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

#include "Tools.h"
#include "Resource.h"
#include "SPugContext.h"
#include "ContinueToken.h"
#include "MMDHtmlOutputProcessor.h"

NS_SA_EXT_BEGIN(tools)

int DocsGui::onTranslateName(Request &req) {
	req.getResponseHeaders().emplace("WWW-Authenticate", req.server().getServerHostname());

	auto u = req.getAuthorizedUser();
	if (u && u->isAdmin()) {

		auto path = toString(getDocumentsPath(), _subPath);
		if (filesystem::isdir(path)) {
			return req.redirectTo(toString(req.getUri(), "/index"));
		} else {
			path += ".md";
		}

		if (filesystem::exists(path)) {
			auto content = filepath::root(path) + "/__content.md";
			return req.runPug("virtual://html/docs.pug", [&] (pug::Context &exec, const pug::Template &tpl) -> bool {
				exec.set("version", data::Value(getVersionString()));
				exec.set("hasDb", data::Value(true));
				exec.set("setup", data::Value(true));
				exec.set("auth", data::Value({
					pair("id", data::Value(u->getObjectId())),
					pair("name", data::Value(u->getString("name")))
				}));

				do {
					StringStream out;
					auto data = filesystem::readTextFile(path);
					mmd::HtmlOutputProcessor::run(&out, req.pool(), data);
					exec.set("source", data::Value(out.str()));
				} while(0);

				if (filesystem::exists(content)) {
					StringStream out;
					auto data = filesystem::readTextFile(content);
					mmd::HtmlOutputProcessor::run(&out, req.pool(), data);
					exec.set("contents", data::Value(out.str()));
				}

				if (_subPathVec.size() > 1) {
					auto url = filepath::root(filepath::root(req.getUri())) + "/index";
					exec.set("root", data::Value(url));
				}

				return true;
			});
		}
		return HTTP_NOT_FOUND;
	} else {
		return HTTP_UNAUTHORIZED;
	}
}

NS_SA_EXT_END(tools)
