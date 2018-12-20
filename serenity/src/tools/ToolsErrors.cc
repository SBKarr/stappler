/**
Copyright (c) 2018-2019 Roman Katuntsev <sbkarr@stappler.org>

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
#include "User.h"
#include "Output.h"
#include "StorageScheme.h"
#include "Resource.h"
#include "TemplateExec.h"
#include "SPugContext.h"
#include "SPugTemplate.h"

NS_SA_EXT_BEGIN(tools)

int ErrorsGui::onTranslateName(Request &req) {
	req.getResponseHeaders().emplace("WWW-Authenticate", req.server().getServerHostname());

	auto &d = req.getParsedQueryArgs();
	if (d.hasValue("delete")) {
		if (auto id = StringView(d.getString("delete")).readInteger().get(0)) {
			auto u = req.getAuthorizedUser();
			if (u && u->isAdmin()) {
				auto s = req.server().getErrorScheme();
				auto a = req.storage();
				if (s && a) {
					storage::Worker(*s, a).remove(id);
				}
			}
		}
		return req.redirectTo(String(req.getUri()));
	}

	auto u = req.getAuthorizedUser();
	if (u && u->isAdmin()) {
		data::Value errorsData;
		auto s = req.server().getErrorScheme();
		auto a = req.storage();
		if (s && a) {
			errorsData = storage::Worker(*s, a).select(storage::Query().order("__oid", storage::Ordering::Descending));
		}

		req.runPug("virtual://html/errors.pug", [&] (pug::Context &exec, const pug::Template &tpl) -> bool {
			exec.set("version", data::Value(getVersionString()));
			exec.set("user", true, &u->getData());
			if (errorsData.size() > 0) {
				exec.set("errors", true, &errorsData);
			}
			return true;
		});
		return DONE;
	} else {
		req.runPug("virtual://html/errors_unauthorized.pug", [&] (pug::Context &exec, const pug::Template &) -> bool {
			exec.set("version", data::Value(getVersionString()));
			return true;
		});
		return HTTP_UNAUTHORIZED;
	}
}

NS_SA_EXT_END(tools)
