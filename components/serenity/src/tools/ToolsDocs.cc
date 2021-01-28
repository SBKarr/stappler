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

int VirtualGui::onTranslateName(Request &req) {
	if (req.getMethod() == Request::Method::Post) {
		return _editable ? DECLINED : HTTP_FORBIDDEN;
	} else if (req.getMethod() != Request::Method::Get) {
		return HTTP_METHOD_NOT_ALLOWED;
	}

	req.getResponseHeaders().emplace("WWW-Authenticate", req.server().getServerHostname());

	auto u = req.getAuthorizedUser();
	if (u && u->isAdmin()) {
		String path;
		bool isSource = false;

		if (StringView(_subPath).starts_with("/_sources")) {
			isSource = true;
			auto tmp = StringView(_subPath); tmp += "/_sources"_len;
			tmp.backwardSkipChars<StringView::Chars<'/'>>();
			path = filepath::reconstructPath(toString(tools::getDocumentsPath(), "/../components", tmp));
		} else {
			path = toString(tools::getDocumentsPath(), _subPath);
			if (filesystem::isdir(path)) {
				return req.redirectTo(filepath::merge(req.getUri(), "/index"));
			} else {
				path += ".md";
			}
		}

		if (filesystem::exists(path)) {
			if (req.getParsedQueryArgs().getBool("delete")) {
				if (!_editable || isSource) {
					return HTTP_FORBIDDEN;
				}
				if (filepath::name(path) == "index") {
					filesystem::remove(filepath::root(path), true, true);
					return req.redirectTo(filepath::merge(filepath::root(filepath::root(req.getUri())), "/index"));
				} else {
					filesystem::remove(path);
					return req.redirectTo(filepath::merge(filepath::root(req.getUri()), "/index"));
				}
			}

			return req.runPug("html/docs.pug", [&] (pug::Context &exec, const pug::Template &tpl) -> bool {
				exec.set("version", data::Value(tools::getVersionString()));
				exec.set("hasDb", data::Value(true));
				exec.set("setup", data::Value(true));
				exec.set("virtual", data::Value(_virtual));
				exec.set("editable", data::Value(_editable && !isSource));
				exec.set("isSource", data::Value(isSource));
				exec.set("auth", data::Value({
					pair("id", data::Value(u->getObjectId())),
					pair("name", data::Value(u->getString("name"))),
					pair("cancel", data::Value(Tools_getCancelUrl(req)))
				}));
				exec.set("isCategory", data::Value(filepath::name(path) == "index"));

				do {
					if (!isSource) {
						StringStream out;
						auto data = filesystem::readTextFile(path);
						exec.set("mdsource", data::Value(data));
						auto ext = mmd::DefaultExtensions;
						ext.flags |= mmd::Extensions::Snippet;
						ext.metaCallback = [&] (StringView name) -> Pair<String, mmd::MetaType> {
							if (name == "sources") {
								StringView tmp(_originPath, 0, _originPath.size() - _subPath.size());
								return pair(String(toString(tmp, "/_sources")), mmd::MetaType::PlainString);
							}
							return pair(String(), mmd::MetaType::PlainString);
						};
						mmd::HtmlOutputProcessor::run(&out, req.pool(), data, ext);
						exec.set("source", data::Value(out.str()));
					} else {
						auto tmp = StringView(_subPath); tmp += "/_sources"_len;
						tmp.backwardSkipChars<StringView::Chars<'/'>>();
						if (filesystem::isdir(path)) {
							exec.set("isDir", data::Value(true));
							exec.set("dir", makeDirInfo(path));
						} else {
							auto name = filepath::name(path);
							StringView sourceClass("cpp");
							if (name == "Makefile" || name == "makefile" || name.ends_with(".mk")) {
								sourceClass = StringView("make");
							} else if (name.ends_with(".c")) {
								sourceClass = StringView("c");
							} else if (name.ends_with(".m") || name.ends_with(".mm")) {
								sourceClass = StringView("objc");
							} else if (name.ends_with(".java")) {
								sourceClass = StringView("java");
							} else if (name.ends_with(".sh")) {
								sourceClass = StringView("sh");
							} else if (name.ends_with(".js")) {
								sourceClass = StringView("js");
							} else if (name.ends_with(".css")) {
								sourceClass = StringView("css");
							} else if (name.ends_with(".html")) {
								sourceClass = StringView("html");
							} else if (name.ends_with(".pug")) {
								sourceClass = StringView("pug");
							} else if (name.ends_with(".conf")) {
								sourceClass = StringView("apacheconf");
							}

							auto source = filesystem::readTextFile(path);
							exec.set("isDir", data::Value(false));
							exec.set("mdsource", data::Value(source));
							exec.set("sourceClass", data::Value(sourceClass));
							exec.set("dir", makeDirInfo(filepath::root(path), true));
						}

						exec.set("title", data::Value(toString("$(STAPPLER_ROOT)/components", tmp)));
					}
				} while(0);

				if (isSource) {

				} else {
					makeMdContents(req, exec, path);
				}

				return true;
			});
		}
		return HTTP_NOT_FOUND;
	} else {
		return HTTP_UNAUTHORIZED;
	}
}

int VirtualGui::onHandler(Request &) {
	return OK;
}

void VirtualGui::onInsertFilter(Request &req) {
	if (!_editable) {
		return;
	}

	if (req.getMethod() == Request::Method::Post) {
		req.setRequiredData(db::InputConfig::Require::Body | db::InputConfig::Require::Data | db::InputConfig::Require::Files);
		req.setMaxRequestSize(20_MiB);
		req.setMaxVarSize(20_MiB);
		req.setMaxFileSize(20_MiB);

		auto ex = InputFilter::insert(req);
		if (ex != InputFilter::Exception::None) {
			if (ex == InputFilter::Exception::TooLarge) {
				req.setStatus(HTTP_REQUEST_ENTITY_TOO_LARGE);
			} else if (ex == InputFilter::Exception::Unrecognized) {
				req.setStatus(HTTP_UNSUPPORTED_MEDIA_TYPE);
			}
		}
	}
}

void VirtualGui::onFilterComplete(InputFilter *filter) {
	if (!_editable) {
		return;
	}

	Request rctx(filter->getRequest());

	auto &args = _request.getParsedQueryArgs();
	if (args.getBool("createArticle")) {
		auto data = filter->getData();
		if (!createArticle(data)) {
			_request.setStatus(_request.redirectTo(String(_request.getUri())));
		}
	} else if (args.getBool("createCategory")) {
		auto data = filter->getData();
		if (!createCategory(data)) {
			_request.setStatus(_request.redirectTo(String(_request.getUri())));
		}
	} else {
		bool result = true;
		auto &body = filter->getBody();

		auto path = toString(tools::getDocumentsPath(), _subPath);
		if (filesystem::isdir(path)) {
			result = false;
		} else {
			path += ".md";
		}

		StringView r(body.weak());
		r.trimChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		if (filesystem::exists(path)) {
			StringStream tmp; tmp << r << "\n";
			filesystem::remove(path);
			filesystem::write(path, tmp.weak());
		}

		data::Value data;
		data.setBool(result, "OK");
		writeData(data);
	}
}

data::Value VirtualGui::readMeta(StringView r) const {
	data::Value ret;
	bool isMeta = true;
	do {
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		auto tmp = r;
		auto key = r.readUntil<StringView::Chars<':', '\r', '\n'>>();
		key.trimChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		if (key.empty()) {
			isMeta = false;
			break;
		}
		if (r.is(':')) {
			++ r;
			auto value = r.readUntil<StringView::Chars<'\r', '\n'>>();
			value.trimChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			if (!value.empty()) {
				ret.setString(value, key);
			}
		} else {
			r = tmp;
			isMeta = false;
			break;
		}
	} while (isMeta);

	if (!ret.isString("Title")) {
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		if (r.is('#')) {
			r.skipChars<StringView::Chars<'#'>>();
			auto v = r.readUntil<StringView::Chars<'\r', '\n'>>();
			v.trimChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			ret.setString(v, "Title");
		}
	}
	return ret;
}

void VirtualGui::writeData(data::Value &data) const {
	data.setInteger(mem::Time::now().toMicros(), "date");
#if DEBUG
	auto &debug = _request.getDebugMessages();
	if (!debug.empty()) {
		data.setArray(debug, "debug");
	}
#endif
	auto &error = _request.getErrorMessages();
	if (!error.empty()) {
		data.setArray(error, "errors");
	}

	Request(_request).writeData(data, true);
}

bool VirtualGui::createArticle(const data::Value &data) {
	if (valid::validateIdentifier(data.getString("name"))) {
		auto path = filepath::root(toString(tools::getDocumentsPath(), _subPath));

		StringStream content;
		content << "Title: " << data.getString("title") << "\n";
		content << "Priority: 0\n";
		content << "# " << data.getString("title") << "\n\n{{TOC}}\n";

		auto target = toString(path, "/", data.getString("name"), ".md");
		if (filesystem::exists(target)) {
			return false;
		}

		filesystem::write(target, content.weak());
		_request.setStatus(_request.redirectTo(toString(filepath::root(_request.getUri()), "/", data.getString("name"))));
		return true;
	}
	return false;
}

bool VirtualGui::createCategory(const data::Value &data) {
	if (valid::validateIdentifier(data.getString("name"))) {
		auto path = filepath::root(toString(tools::getDocumentsPath(), _subPath));
		auto target = toString(path, "/", data.getString("name"));
		if (filesystem::exists(target)) {
			return false;
		}

		filesystem::mkdir(target);
		target = toString(target, "/index.md");

		StringStream content;
		content << "Title: " << data.getString("title") << "\n";
		content << "Priority: 0\n";
		content << "# " << data.getString("title") << "\n\n{{TOC}}\n";

		filesystem::write(target, content.weak());
		_request.setStatus(_request.redirectTo(toString(filepath::root(_request.getUri()), "/", data.getString("name"))));
		return true;
	}
	return false;
}

void VirtualGui::makeMdContents(Request &req, pug::Context &exec, StringView path) const {
	// build contents
	data::Value rootMeta;
	Vector<data::Value> contentData;
	auto root = filepath::root(path);
	auto self = path;
	filesystem::ftw(root, [&] (StringView path, bool isFile) {
		if (filepath::name(path) != "index") {
			StringView target = path;
			if (!isFile) {
				auto p = toString(path, "/index.md");
				if (filesystem::exists(p)) {
					target = StringView(p).pdup();
					isFile = true;
				}
			}
			if (isFile && target.ends_with(".md")) {
				auto data = filesystem::readTextFile(target);
				if (auto meta = readMeta(data)) {
					if (root == path) {
						rootMeta = move(meta);
					} else {
						meta.setString(filepath::name(path), "Url");
						if (self == path) {
							meta.setBool(true, "selected");
						}
						contentData.emplace_back(move(meta));
					}
				}
			}
		}
	}, 1);

	std::sort(contentData.begin(), contentData.end(), [&] (const data::Value &l, const data::Value &r) {
		return l.getInteger("Priority") > r.getInteger("Priority");
	});

	StringStream contents;
	contents << "# Content\n\n";
	if (rootMeta && rootMeta.isString("Title")) {
		contents << "[" << rootMeta.getString("Title") << "](index" <<
				((filepath::name(self) == "index")?StringView(" class=\"selected\""):StringView()) << ")\n\n";
	}

	for (auto &meta : contentData) {
		if (meta.isString("Title")) {
			contents << "[" << meta.getString("Title") << "](" << meta.getString("Url")
				<< (meta.getBool("selected")?StringView(" class=\"selected\""):StringView()) << ")\n\n";
		}
	}

	if (!contents.empty()) {
		StringStream out;
		mmd::HtmlOutputProcessor::run(&out, req.pool(), contents.weak());
		exec.set("contents", data::Value(out.str()));
	}

	Vector<data::Value> breadcrumbs;
	auto docPath = StringView(tools::getDocumentsPath());
	auto selfDir = filepath::root(self);
	size_t level = 0;
	do {
		if (selfDir != filepath::root(self)) {
			auto target = toString(selfDir, "/index.md");
			auto data = filesystem::readTextFile(target);
			if (auto meta = readMeta(data)) {
				StringStream tmp;
				for (size_t i = 0; i < level; ++ i) { if (tmp.empty()) { tmp << ".."; } else { tmp << "/.."; } }
				meta.setString(tmp.weak(), "Url");
				breadcrumbs.emplace_back(move(meta));
			}
		}
		selfDir = filepath::root(selfDir);
		++ level;
	} while (selfDir.size() >= docPath.size());

	std::reverse(breadcrumbs.begin(), breadcrumbs.end());
	exec.set("breadcrumbs", data::Value(move(breadcrumbs)));

	if (_subPathVec.size() > 1) {
		auto url = filepath::root(filepath::root(req.getUri())) + "/index";
		exec.set("root", data::Value(url));
	}
}

data::Value VirtualGui::makeDirInfo(StringView path, bool forFile) const {
	data::Value ret;
	bool isRoot = false;

	auto tmpSub = StringView(_subPath);
	tmpSub.backwardSkipChars<StringView::Chars<'/'>>();
	if (tmpSub != "/_sources") {
		ret.setString((_subPath.back() == '/') ? String("..") : String("."), "back");
	} else {
		isRoot = true;
	}


	Vector<StringView> dirs;
	Vector<StringView> files;

	filesystem::ftw(path, [&] (StringView p, bool isFile) {
		if (isFile) {
			files.emplace_back(StringView(p, path.size() + 1, p.size() - path.size()).pdup());
		} else if (p != path) {
			dirs.emplace_back(StringView(p, path.size() + 1, p.size() - path.size()).pdup());
		}
	}, 1);

	std::sort(dirs.begin(), dirs.end());
	std::sort(files.begin(), files.end());

	auto rootName = (_subPath.back() == '/' || forFile) ? String() : (isRoot ? "_sources/" : toString(filepath::name(path), "/"));

	data::Value &dirsData = ret.emplace("dirs");
	for (auto &it : dirs) {
		dirsData.addValue(data::Value({
			pair("name", data::Value(it)),
			pair("url", data::Value(toString(rootName, it))),
		}));
	}

	data::Value &filesData = ret.emplace("files");
	for (auto &it : files) {
		filesData.addValue(data::Value({
			pair("name", data::Value(it)),
			pair("url", data::Value(toString(rootName, it))),
		}));
	}

	return ret;
}

NS_SA_EXT_END(tools)
