/**
Copyright (c) 2017-2019 Roman Katuntsev <sbkarr@stappler.org>

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
#include "Output.h"
#include "Resource.h"
#include "TemplateExec.h"

NS_SA_EXT_BEGIN(tools)

int ServerGui::onTranslateName(Request &rctx) {
	if (rctx.getParsedQueryArgs().getBool("auth")) {
		if (rctx.getAuthorizedUser()) {
			return rctx.redirectTo(String(rctx.getUri()));
		} else {
			return HTTP_UNAUTHORIZED;
		}
	}

	if (rctx.getMethod() == Request::Get) {
		auto userScheme = rctx.server().getUserScheme();
		size_t count = 0;
		bool hasDb = false;
		if (rctx.storage() && userScheme) {
			count = storage::Worker(*userScheme, rctx.storage()).count();
			hasDb = true;
		}
		rctx.runPug("virtual://html/server.pug", [&] (pug::Context &exec, const pug::Template &) -> bool {
			exec.set("count", data::Value(count));
			exec.set("setup", data::Value(count != 0));
			exec.set("hasDb", data::Value(hasDb));
			exec.set("version", data::Value(getVersionString()));
			if (auto u = rctx.getAuthorizedUser()) {
				exec.set("auth", data::Value({
					pair("id", data::Value(u->getObjectId())),
					pair("name", data::Value(u->getString("name")))
				}));

				if (auto iface = dynamic_cast<db::sql::SqlHandle *>(rctx.storage().interface())) {
					iface->makeQuery([&] (db::sql::SqlQuery &query) {
						query << "SELECT current_database();";
						iface->selectQuery(query, [&] (db::sql::Result &qResult) {
							if (!qResult.empty()) {
								exec.set("dbName", data::Value(qResult.front().toString(0)));
							}
						});
					});
				}

				data::Value components;
				for (auto &it : rctx.server().getComponents()) {
					components.addValue(data::Value({
						pair("name", data::Value(it.second->getName())),
						pair("version", data::Value(it.second->getVersion())),
					}));
				}
				exec.set("components", move(components));
				exec.set("root", data::Value(rctx.server().getDocumentRoot()));
			}

			return true;
		});
		return DONE;
	} else {
		return DataHandler::onPostReadRequest(rctx);
	}
	return DECLINED;
}

void ServerGui::onFilterComplete(InputFilter *filter) {
	const auto data = filter->getData();
	Request rctx(filter->getRequest());
	_filter = filter;

	auto &name = data.getString("name");
	auto &passwd = data.getString("passwd");
	if (!name.empty() && !passwd.empty()) {
		auto t = storage::Transaction::acquire();
		t.performAsSystem([&] () -> bool {
			return User::setup(rctx.storage(), name, passwd) != nullptr;
		});
	}

	rctx.redirectTo(String(rctx.getUnparsedUri()));
	rctx.setStatus(HTTP_SEE_OTHER);
}

NS_SA_EXT_END(tools)