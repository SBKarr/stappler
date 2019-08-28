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

#include "Tools.h"
#include "Resource.h"
#include "SPugContext.h"
#include "ContinueToken.h"

NS_SA_EXT_BEGIN(tools)

int ErrorsGui::onTranslateName(Request &req) {
	req.getResponseHeaders().emplace("WWW-Authenticate", req.server().getServerHostname());

	auto u = req.getAuthorizedUser();
	if (u && u->isAdmin()) {
		auto errorScheme = req.server().getErrorScheme();
		auto &d = req.getParsedQueryArgs();
		if (d.hasValue("delete")) {
			if (auto id = StringView(d.getString("delete")).readInteger().get(0)) {
				auto u = req.getAuthorizedUser();
				if (u && u->isAdmin()) {
					auto a = req.storage();
					if (errorScheme && a) {
						storage::Worker(*errorScheme, a).remove(id);
					}
				}
			}

			auto c = d.getString("c");
			if (!c.empty()) {
				auto token = ContinueToken(d.getString("c"));
				if (auto t = db::Transaction::acquire(req.storage())) {
					db::Query q;
					token.refresh(*errorScheme, t, q);

					return req.redirectTo(toString(req.getUri(), "?c=", token.encode()));
				} else {
					return req.redirectTo(toString(req.getUri(), "?c=", c));
				}
			} else {
				return req.redirectTo(String(req.getUri()));
			}
		}

		data::Value errorsData;
		auto token = d.isString("c") ? ContinueToken(d.getString("c")) : ContinueToken("__oid", 25, true);
		if (auto t = db::Transaction::acquire(req.storage())) {
			if (errorScheme) {
				db::Query q;
				errorsData = token.perform(*errorScheme, t, q);
			}
		}

		req.runPug("virtual://html/errors.pug", [&] (pug::Context &exec, const pug::Template &tpl) -> bool {
			exec.set("version", data::Value(getVersionString()));
			exec.set("user", true, &u->getData());
			exec.set("hasDb", data::Value(true));
			exec.set("setup", data::Value(true));
			exec.set("auth", data::Value({
				pair("id", data::Value(u->getObjectId())),
				pair("name", data::Value(u->getString("name")))
			}));
			if (errorsData.size() > 0) {
				exec.set("errors", true, &errorsData);
			}

			data::Value cursor({
				pair("start", data::Value(token.getStart())),
				pair("end", data::Value(token.getEnd())),
				pair("current", data::Value(d.getString("c"))),
				pair("total", data::Value(token.getTotal())),
			});

			if (token.hasNext()) {
				cursor.setString(token.encodeNext(), "next");
			}

			if (token.hasPrev()) {
				cursor.setString(token.encodePrev(), "prev");
			}

			exec.set("cursor", move(cursor));
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

int HandlersGui::onTranslateName(Request &req) {
	req.getResponseHeaders().emplace("WWW-Authenticate", req.server().getServerHostname());

	auto u = req.getAuthorizedUser();
	if (u && u->isAdmin()) {

		auto &hdl = req.server().getRequestHandlers();

		data::Value servh;
		data::Value ret;
		for (auto &it : hdl) {
			auto &v = (it.second.component == "root") ? servh.emplace() : ret.emplace();
			v.setValue(it.first, "name");
			if (!it.second.data.isNull()) {
				v.setValue(it.second.data, "data");
			}
			if (it.second.scheme) {
				v.setString(it.second.scheme->getName(), "scheme");
			}
			if (!it.second.component.empty()) {
				if (it.second.component == "root") {
					v.setBool(true, "server");
				}
				v.setString(it.second.component, "component");
			}
			if (it.first.back() == '/') {
				v.setBool(true, "forSubPaths");
			}
		}

		for (auto &it : servh.asArray()) {
			ret.addValue(move(it));
		}

		req.runPug("virtual://html/handlers.pug", [&] (pug::Context &exec, const pug::Template &tpl) -> bool {
			exec.set("version", data::Value(getVersionString()));
			exec.set("user", true, &u->getData());
			exec.set("hasDb", data::Value(true));
			exec.set("setup", data::Value(true));
			exec.set("auth", data::Value({
				pair("id", data::Value(u->getObjectId())),
				pair("name", data::Value(u->getString("name")))
			}));

			if (auto iface = dynamic_cast<db::sql::SqlHandle *>(req.storage().interface())) {
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
			for (auto &it : req.server().getComponents()) {
				components.addValue(data::Value({
					pair("name", data::Value(it.second->getName())),
					pair("version", data::Value(it.second->getVersion())),
				}));
			}
			exec.set("components", move(components));
			exec.set("root", data::Value(req.server().getDocumentRoot()));

			exec.set("handlers", std::move(ret));
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
