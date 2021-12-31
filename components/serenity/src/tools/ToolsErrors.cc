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

	auto t = db::Transaction::acquire(req.storage());
	if (!t) {
		return HTTP_INTERNAL_SERVER_ERROR;
	}

	if (_subPath == "/fixTags") {
		auto errorScheme = req.server().getErrorScheme();
		auto tags = errorScheme->getField("tags");
		auto errors = errorScheme->select(t, db::Query().include("data"));
		for (auto &it : errors.asArray()) {
			auto objTags = tags->getDefault(it);
			errorScheme->update(t, it, data::Value({
				pair("tags", data::Value(objTags))
			}), db::UpdateFlags::NoReturn);
		}
		return HTTP_OK;
	}

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
			auto tag = d.getString("tag");

			StringStream url;
			url << req.getUri();

			if (!c.empty()) {
				auto token = ContinueToken(d.getString("c"));
				if (auto t = db::Transaction::acquire(req.storage())) {
					db::Query q;
					if (!tag.empty()) {
						q.select("tags", db::Comparation::Includes, data::Value(tag));
					}
					token.refresh(*errorScheme, t, q);

					url << "?c=" << token.encode();
				} else {
					url << "?c=" << c;
				}
			}
			if (!tag.empty()) {
				if (c.empty()) {
					url << "?tag=" << tag;
				} else {
					url << "&tag=" << tag;
				}
			}
			return req.redirectTo(url.str());
		}

		String selectedTag;
		if (d.isString("tag")) {
			selectedTag = d.getString("tag");
		}

		data::Value errorsData;
		auto token = d.isString("c") ? ContinueToken(d.getString("c")) : ContinueToken("__oid", 25, true);

		if (errorScheme) {
			db::Query q;
			if (!selectedTag.empty()) {
				q.select("tags", db::Comparation::Includes, data::Value(selectedTag));
			}
			errorsData = token.perform(*errorScheme, t, q);
		}

		req.runPug("virtual://html/errors.pug", [&] (pug::Context &exec, const pug::Template &tpl) -> bool {
			ServerGui::defineBasics(exec, req, u);

			if (!selectedTag.empty()) {
				exec.set("selectedTag", data::Value(selectedTag));
			}

			if (auto iface = dynamic_cast<db::pq::Handle *>(t.getAdapter().interface())) {
				data::Value ret;
				auto query = toString("SELECT tag, COUNT(*) FROM (SELECT __oid, unnest(tags) as tag FROM ", errorScheme->getName(), ") s GROUP BY tag;");
				iface->performSimpleSelect(query, [&] (db::Result &res) {
					for (auto it : res) {
						ret.addValue(data::Value({
							pair("tag", data::Value(it.toString(0))),
							pair("count", data::Value(it.toInteger(1))),
							pair("selected", data::Value(it.toString(0) == selectedTag))
						}));
					}
				});
				exec.set("tags", move(ret));
			}

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
			if (it.second.map) {
				auto base = StringView(it.first);
				if (base.ends_with("/")) {
					base = StringView(base, base.size() - 1);
				}
				auto &m = v.emplace("map");
				for (auto &iit : it.second.map->getHandlers()) {
					auto &mVal = m.emplace();
					mVal.setString(iit.getName(), "name");
					mVal.setString(toString(base, iit.getPattern()), "pattern");

					switch (iit.getMethod()) {
					case Request::Method::Get: mVal.setString("GET", "method"); break;
					case Request::Method::Put: mVal.setString("PUT", "method"); break;
					case Request::Method::Post: mVal.setString("POST", "method"); break;
					case Request::Method::Delete: mVal.setString("DELETE", "method"); break;
					case Request::Method::Connect: mVal.setString("CONNECT", "method"); break;
					case Request::Method::Options: mVal.setString("OPTIONS", "method"); break;
					case Request::Method::Trace: mVal.setString("TRACE", "method"); break;
					case Request::Method::Patch: mVal.setString("PATCH", "method"); break;
					default: break;
					}

					auto &qScheme = iit.getQueryScheme().getFields();
					if (!qScheme.empty()) {
						auto &qVal = mVal.emplace("query");
						for (auto &it : qScheme) {
							auto &v = qVal.addValue(it.second.getTypeDesc());
							v.setString(it.first, "name");
						}
					}

					auto &iScheme = iit.getInputScheme().getFields();
					if (!iScheme.empty()) {
						auto &qVal = mVal.emplace("input");
						for (auto &it : iScheme) {
							auto &v = qVal.addValue(it.second.getTypeDesc());
							v.setString(it.first, "name");
						}
					}
				}
			}
		}

		for (auto &it : servh.asArray()) {
			ret.addValue(move(it));
		}

		req.runPug("virtual://html/handlers.pug", [&] (pug::Context &exec, const pug::Template &tpl) -> bool {
			ServerGui::defineBasics(exec, req, u);
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

int ReportsGui::onTranslateName(Request &req) {
	req.getResponseHeaders().emplace("WWW-Authenticate", req.server().getServerHostname());

	auto u = req.getAuthorizedUser();
	if (u && u->isAdmin()) {
		auto reportsAddress = filesystem::writablePath(".serenity");

		auto readTime = [&] (StringView name) {
			if (name.starts_with("crash.")) {
				name.skipUntil<StringView::CharGroup<CharGroupId::Numbers>>();
				return Time::microseconds(name.readInteger(10).get());
			} else {
				name.skipUntil<StringView::CharGroup<CharGroupId::Numbers>>();
				return Time::milliseconds(name.readInteger(10).get());
			}
		};

		data::Value paths;
		data::Value file;
		if (_subPath.empty() || _subPath == "/") {
			filesystem::ftw(reportsAddress, [&] (StringView path, bool isFile) {
				if (isFile) {
					auto name = filepath::lastComponent(path);
					if (name.starts_with("crash.") || name.starts_with("update.")) {
						auto &info = paths.emplace();
						info.setString(name, "name");
						if (auto t = readTime(name)) {
							info.setInteger(t.toMicros(), "time");
							info.setString(t.toHttp(), "date");
						}
					}
				}
			}, 1);

			if (paths) {
				std::sort(paths.asArray().begin(), paths.asArray().end(), [&] (const data::Value &l, const data::Value &r) {
					return l.getInteger("time") > r.getInteger("time");
				});
			}
		} else if (_subPathVec.size() == 1) {
			auto name = _subPathVec.front();

			auto reportsAddress = filesystem::writablePath(filepath::merge(".serenity", name));
			if (filesystem::exists(reportsAddress) && !filesystem::isdir(reportsAddress)) {
				if (req.getParsedQueryArgs().getBool("remove")) {
					filesystem::remove(reportsAddress);
					return req.redirectTo(StringView(_originPath, _originPath.size() - _subPath.size()).str());
				}
				auto data = filesystem::readTextFile(reportsAddress);
				if (!data.empty()) {
					file.setString(move(data), "data");
					auto name = filepath::lastComponent(reportsAddress);
					file.setString(name, "name");
					if (auto t = readTime(name)) {
						file.setInteger(t.toMicros(), "time");
						file.setString(t.toHttp(), "date");
					}
				}
			}
		}

		req.runPug("virtual://html/reports.pug", [&] (pug::Context &exec, const pug::Template &tpl) -> bool {
			ServerGui::defineBasics(exec, req, u);
			if (paths) {
				exec.set("files", move(paths));
			} else if (file) {
				exec.set("file", move(file));
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
