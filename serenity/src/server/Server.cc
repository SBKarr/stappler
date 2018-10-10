// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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
#include "Server.h"

#include "SPFilesystem.h"
#include "Request.h"
#include "RequestHandler.h"
#include "Root.h"

#include "ServerComponent.h"
#include "StorageField.h"
#include "StorageScheme.h"
#include "PGHandle.h"
#include "ResourceHandler.h"
#include "MultiResourceHandler.h"
#include "WebSocket.h"
#include "Tools.h"
#include "TemplateCache.h"
#include "SPugCache.h"

NS_SA_BEGIN

#define SA_SERVER_FILE_SCHEME_NAME "__files"
#define SA_SERVER_USER_SCHEME_NAME "__users"
#define SA_SERVER_ERROR_SCHEME_NAME "__error"

using namespace storage;

struct Server::Config : public AllocPool {
	static Config *get(server_rec *server) {
		if (!server) { return nullptr; }
		auto cfg = (Config *)ap_get_module_config(server->module_config, &serenity_module);
		if (cfg) { return cfg; }

		return apr::pool::perform([&] () -> Config * {
			return new Config(server);
		}, server);
	}

	Config(server_rec *server)
#if DEBUG
	: _pugCache(pug::Template::Options::getPretty(), [this] (const StringView &str) { onError(str); })
#else
	: _pugCache(pug::Template::Options::getDefault(), [this] (const StringView &str) { onError(str); })
#endif
	{
		// add virtual files to template engine
		size_t count = 0;
		auto d = tools::VirtualFile::getList(count);
		for (size_t i = 0; i < count; ++ i) {
			if (d[i].name.ends_with(".pug") || d[i].name.ends_with(".spug")) {
				_pugCache.addTemplate(toString("virtual:/", d[i].name), d[i].content.str());
			} else {
				_pugCache.addContent(toString("virtual:/", d[i].name), d[i].content.str());
			}
		}

		ap_set_module_config(server->module_config, &serenity_module, this);
	}

	void onHandler(Server &serv, const String &name, const String &ifile, const String &symbol, const data::Value &handlerData) {
		String file;
		if (!sourceRoot.empty() && ifile.front() != '/') {
			file = filepath::absolute(filepath::merge(sourceRoot, ifile));
		} else {
			file = filepath::absolute(ifile);
		}

		if (!filesystem::exists(file)) {
			log::format("Server", "No module file: %s", file.c_str());
			return;
		}

		auto pool = apr::pool::acquire();

		/* load new dynamic object */
		apr_dso_handle_t *obj = NULL;
		apr_dso_handle_sym_t sym = NULL;
		apr_status_t err = apr_dso_load(&obj, file.c_str(), pool);
		if (err == APR_SUCCESS) {
			/* loading was successful, export main symbol */
			err = apr_dso_sym(&sym, obj, symbol.c_str());
			if (err == APR_SUCCESS) {
				auto h = ((ServerComponent::Symbol) sym)(serv, name, handlerData);
				if (h) {
					components.emplace(h->getName(), h);
					typedComponents.emplace(std::type_index(typeid(*h)), h);
				} else {
					log::format("Server", "DSO (%s) returns nullptr handler", name.c_str());
					serv.reportError(data::Value {
						pair("source", data::Value("Server")),
						pair("text", data::Value(toString("DSO (", name, ") returns nullptr handler")))
					});
					loadingFalled = true;
				}
			} else {
				/* fail to load symbol, terminating process */
				char buf[256] = {0};
				log::format("Server", "DSO (%s) error: %s", name.c_str(), apr_dso_error(obj, buf, 255));
				serv.reportError(data::Value {
					pair("source", data::Value("Server")),
					pair("text", data::Value(toString("DSO (", name, ") error: ", apr_dso_error(obj, buf, 255))))
				});
				apr_dso_unload(obj);
				loadingFalled = true;
			}
		} else {
			/* fail to load object, terminating process */
			char buf[256] = {0};
			log::format("Server", "Fail to load DSO (%s): %s", name.c_str(), apr_dso_error(obj, buf, 255));
			serv.reportError(data::Value {
				pair("source", data::Value("Server")),
				pair("text", data::Value(toString("Fail to load DSO (", name, "): ", apr_dso_error(obj, buf, 255))))
			});
			loadingFalled = true;
		}
	}

	void initHandlers(Server &serv, data::Value &val) {
		for (auto &it : val.asArray()) {
			if (it.isDictionary()) {
				auto & name = it.getString("name");
				auto & file = it.getString("file");
				auto & symbol = it.getString("symbol");
				auto & handlerData = it.getValue("data");

				onHandler(serv, name, file, symbol, handlerData);
			}
		}
	}

	void initSession(data::Value &val) {
		sessionName = val.getString("name");
		sessionKey = val.getString("key");
		sessionMaxAge = val.getInteger("maxage");
		isSessionSecure = val.getBool("secure");
	}

	void setSessionParam(StringView &n, StringView &v) {
		if (n.is("name")) {
			sessionName = v.str();
		} else if (n.is("key")) {
			sessionKey = v.str();
		} else if (n.is("maxage")) {
			sessionMaxAge = v.readInteger().get(0);
		} else if (n.is("secure")) {
			if (v.is("true") || v.is("on") || v.is("On")) {
				isSessionSecure = true;
			} else if (v.is("false") || v.is("off") || v.is("Off")) {
				isSessionSecure = false;
			}
		}
	}

	void setWebHookParam(StringView &n, StringView &v) {
		if (n.is("name")) {
			webHookName = v.str();
		} else if (n.is("url")) {
			webHookUrl = v.str();
		} else if (n.is("format")) {
			webHookFormat = v.str();
		} else {
			webHookExtra.setString(v.str(), n.str());
		}
	}

	void setForceHttps() {
		forceHttps = true;
	}

	void init(Server &serv) {
		schemes.emplace(userScheme.getName(), &userScheme);
		schemes.emplace(fileScheme.getName(), &fileScheme);
		schemes.emplace(errorScheme.getName(), &errorScheme);

		if (data) {
			if (data.isArray("handlers")) {
				initHandlers(serv, data.getValue("handlers"));
			}

			if (data.isDictionary("session")) {
				initSession(data.getValue("session"));
			}
		}

		if (handlers.isArray()) {
			initHandlers(serv, handlers);
		}
	}

	void onChildInit(Server &serv) {
		childInit = true;
		for (auto &it : components) {
			currentComponent = it.second->getName();
			it.second->onChildInit(serv);
			currentComponent = String();
		}

		if (!loadingFalled) {
			Scheme::initSchemes(serv, schemes);

			auto root = Root::getInstance();
			auto pool = getCurrentPool();
			auto db = root->dbdOpen(pool, serv);
			if (db) {
				pg::Handle h(pool, db);
				h.init(serv, schemes);

				for (auto &it : components) {
					currentComponent = it.second->getName();
					it.second->onStorageInit(serv, &h);
					currentComponent = String();
				}
				root->dbdClose(serv, db);
			}
		}
	}

	void onError(const StringView &str) {
#if DEBUG
		std::cout << str << "\n";
#endif
		messages::error("Template", "Template compilation error", data::Value(str));
	}

	bool childInit = false;

	storage::Scheme userScheme = storage::Scheme(SA_SERVER_USER_SCHEME_NAME, {
		Field::Text("name", Transform::Alias, Flags::Required),
		Field::Password("password", PasswordSalt(config::getDefaultPasswordSalt()), Flags::Required | Flags::Protected),
		Field::Boolean("isAdmin", data::Value(false)),
		Field::Extra("data", Vector<Field>{
			Field::Text("email", Transform::Email),
			Field::Text("public"),
			Field::Text("desc"),
		}),
		Field::Text("email", Transform::Email, Flags::Unique),
	});

	storage::Scheme fileScheme = storage::Scheme(SA_SERVER_FILE_SCHEME_NAME, {
		Field::Text("location", Transform::Url),
		Field::Text("type", Flags::ReadOnly),
		Field::Integer("size", Flags::ReadOnly),
		Field::Integer("mtime", Flags::AutoMTime | Flags::ReadOnly),
		Field::Extra("image", Vector<Field>{
			Field::Integer("width"),
			Field::Integer("height"),
		})
	});

	storage::Scheme errorScheme = storage::Scheme(SA_SERVER_ERROR_SCHEME_NAME, {
		Field::Boolean("hidden", data::Value(false)),
		Field::Boolean("delivered", data::Value(false)),
		Field::Text("name"),
		Field::Text("documentRoot"),
		Field::Text("url"),
		Field::Text("request"),
		Field::Text("ip"),
		Field::Data("headers"),
		Field::Data("data"),
		Field::Integer("time"),
	});

	data::Value handlers;
	String handlerFile;
	String sourceRoot;
	String currentComponent;
	Vector<Function<int(Request &)>> preRequest;
	Map<String, ServerComponent *> components;
	Map<std::type_index, ServerComponent *> typedComponents;
	Map<String, RequestScheme> requests;
	Map<const storage::Scheme *, ResourceScheme> resources;
	Map<String, const storage::Scheme *> schemes;

	Map<String, websocket::Manager *> websockets;
	data::Value data;

	String sessionName = config::getDefaultSessionName();
	String sessionKey = config::getDefaultSessionKey();
	apr_time_t sessionMaxAge = 0;
	bool isSessionSecure = false;
	bool loadingFalled = false;
	bool forceHttps = false;

	Time lastDatabaseCleanup;
	Time lastTemplateUpdate;
	int64_t broadcastId = 0;
	tpl::Cache _templateCache;
	pug::Cache _pugCache;

	String webHookUrl;
	String webHookName;
	String webHookFormat;
	data::Value webHookExtra;

	CompressionConfig compression;
	Set<String> protectedList;
};

void * Server::merge(void *base, void *add) {
	Config *baseCfg = (Config *)base;
	Config *addCfg = (Config *)add;

	if (!baseCfg->sourceRoot.empty()) { addCfg->sourceRoot = baseCfg->sourceRoot; }
	if (!baseCfg->webHookUrl.empty()) { addCfg->webHookUrl = baseCfg->webHookUrl; }
	if (!baseCfg->webHookName.empty()) { addCfg->webHookName = baseCfg->webHookName; }
	if (!baseCfg->webHookFormat.empty()) { addCfg->webHookFormat = baseCfg->webHookFormat; }

	if (!baseCfg->webHookExtra.empty()) {
		for (auto &it : baseCfg->webHookExtra.asDict()) {
			addCfg->webHookExtra.setValue(it.second, it.first);
		}
	}

	return addCfg;
}

Server::Server() : _server(nullptr), _config(nullptr) { }
Server::Server(server_rec *server) : _server(server), _config(Config::get(server)) { }

Server & Server::operator =(server_rec *server) {
	_server = server;
	_config = Config::get(_server);
	return *this;
}

Server::Server(Server &&other) : _server(other._server), _config(other._config) { }
Server & Server::operator =(Server &&other) {
	_server = other._server;
	_config = other._config;
	return *this;
}

Server::Server(const Server &other) : _server(other._server), _config(other._config) { }
Server & Server::operator =(const Server &other) {
	_server = other._server;
	_config = other._config;
	return *this;
}

void Server::onChildInit() {
	if (!_config->handlerFile.empty()) {
		_config->data = data::readFile(_config->handlerFile);
	}

	_config->init(*this);
	_config->onChildInit(*this);

	filesystem::mkdir(filepath::merge(getDocumentRoot(), "/.serenity"));
	filesystem::mkdir(filepath::merge(getDocumentRoot(), "/uploads"));

	_config->currentComponent = "root";
	tools::registerTools(config::getServerToolsPrefix(), *this);
	_config->currentComponent = String();

	addProtectedLocation("/.serenity");
	addProtectedLocation("/uploads");

	auto serv = server();
	Task::perform(*this, [&] (Task &task) {
		task.addExecuteFn([serv] (const Task &task) -> bool {
			Server(serv).processReports();
			return true;
		});
	});
}

enum class ServerReportType {
	Crash,
	Update,
	Error,
};

template <typename Callback> static
void Server_prepareEmail(Server::Config *cfg, Callback &&cb, ServerReportType type) {
	StringStream data;
	if (!cfg->webHookUrl.empty() && !cfg->webHookName.empty()) {
		auto &from = cfg->webHookName;
		auto &to = cfg->webHookExtra.getString("to");
		auto &title = cfg->webHookExtra.getString("title");

		network::Mail notify(cfg->webHookUrl, cfg->webHookName, cfg->webHookExtra.getString("password"));
		notify.setMailFrom(from);
		notify.addMailTo(to);

		data << "From: " << from << " <" << from << ">\r\n"
			<< "Content-Type: text/plain; charset=utf-8\r\n"
			<< "To: " << to << " <" << to << ">\r\n";

		switch (type) {
		case ServerReportType::Crash:
			data << "Subject: Serenity Crash report";
			break;
		case ServerReportType::Update:
			data << "Subject: Serenity Update report";
			break;
		case ServerReportType::Error:
			data << "Subject: Serenity Error report";
			break;
		}

		if (!title.empty()) {
			data << " (" << title << ")";
		}
		data << "\r\n\r\n";

		cb(data);

		notify.send(data);
	}
}

void Server::processReports() {
	if (_config->webHookFormat != "email") {
		return;
	}

	Vector<Pair<String, ServerReportType>> crashFiles;
	String path = filepath::absolute(".serenity", true);
	filesystem::ftw(path, [&] (const StringView &view, bool isFile) {
		if (isFile) {
			StringView r(view);
			r.skipString(path);
			if (r.starts_with("/crash.")) {
				crashFiles.emplace_back(view.str(), ServerReportType::Crash);
			} else if (r.starts_with("/update.")) {
				crashFiles.emplace_back(view.str(), ServerReportType::Update);
			}
		}
	});

	Vector<Pair<String, ServerReportType>> crashData;
	for (auto &it : crashFiles) {
		crashData.emplace_back(filesystem::readTextFile(it.first), it.second);
		filesystem::remove(it.first);
	}

	for (auto &it : crashData) {
		Server_prepareEmail(_config, [&] (StringStream &data) {
			data << it.first << "\r\n";
		}, it.second);
	}
}

void Server::setHandlerFile(const StringView &file) {
	_config->handlerFile = file.str();
}
void Server::setSourceRoot(const StringView &file) {
	_config->sourceRoot = file.str();
}
void Server::addHanderSource(const StringView &str) {
	StringView r(str);
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	StringView handlerParams;
	if (r.is('"')) {
		++ r;
		handlerParams = r.readUntil<StringView::Chars<'"'>>();
		if (r.is('"')) {
			++ r;
		}
	} else {
		handlerParams = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	}

	StringView args[3];
	int64_t idx = 0;
	while (!handlerParams.empty() && idx < 3) {
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		args[idx] = handlerParams.readUntil<StringView::Chars<':'>>();
		if (handlerParams.is(':')) {
			++ handlerParams;
		}
		++ idx;
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	}

	if (idx >= 2) {
		data::Value h;
		h.setString(args[idx - 1].str(), "symbol");
		h.setString(args[idx - 2].str(), "file");
		if (idx == 3) {
			h.setString(args[0].str(), "name");
		}
		data::Value &data = h.emplace("data");

		while (!r.empty()) {
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			StringView params, n, v;
			if (r.is('"')) {
				++ r;
				params = r.readUntil<StringView::Chars<'"'>>();
				if (r.is('"')) {
					++ r;
				}
			} else {
				params = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			}

			if (!params.empty()) {
				n = params.readUntil<StringView::Chars<'='>>();
				++ params;
				v = params;

				if (!n.empty() && ! v.empty()) {
					data.setString(v.str(), n.str());
				}
			}
		}

		_config->handlers.addValue(std::move(h));
	}
}

void Server::setSessionParams(const StringView &str) {
	StringView r(str);
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	while (!r.empty()) {
		StringView params, n, v;
		if (r.is('"')) {
			++ r;
			params = r.readUntil<StringView::Chars<'"'>>();
			if (r.is('"')) {
				++ r;
			}
		} else {
			params = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		if (!params.empty()) {
			n = params.readUntil<StringView::Chars<'='>>();
			++ params;
			v = params;

			if (!n.empty() && ! v.empty()) {
				_config->setSessionParam(n, v);
			}
		}

		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	}
}

void Server::setWebHookParams(const StringView &str) {
	StringView r(str);
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	while (!r.empty()) {
		StringView params, n, v;
		if (r.is('"')) {
			++ r;
			params = r.readUntil<StringView::Chars<'"'>>();
			if (r.is('"')) {
				++ r;
			}
		} else {
			params = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		if (!params.empty()) {
			n = params.readUntil<StringView::Chars<'='>>();
			++ params;
			v = params;

			if (!n.empty() && ! v.empty()) {
				_config->setWebHookParam(n, v);
			}
		}

		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	}
}

void Server::setProtectedList(const StringView &str) {
	str.split<StringView::Chars<' '>>([&] (StringView &value) {
		addProtectedLocation(value);
	});
}

void Server::addProtectedLocation(const StringView &value) {
	_config->protectedList.emplace(value.str());
}

void Server::setForceHttps() {
	_config->setForceHttps();
}

StringView Server::getHandlerFile() const {
	return _config->handlerFile;
}

apr::weak_string Server::getDefaultName() const {
	return String::make_weak(_server->defn_name, _server->process->pconf);
}

bool Server::isVirtual() const {
	return _server->is_virtual;
}

apr_port_t Server::getServerPort() const {
	return _server->port;
}
apr::weak_string Server::getServerScheme() const {
	return apr::string::make_weak(_server->server_scheme, _server->process->pconf);
}
apr::weak_string Server::getServerAdmin() const {
	return apr::string::make_weak(_server->server_admin, _server->process->pconf);
}
apr::weak_string Server::getServerHostname() const {
	return apr::string::make_weak(_server->server_hostname, _server->process->pconf);
}
apr::weak_string Server::getDocumentRoot() const {
	core_server_config *sconf = (core_server_config *)ap_get_core_module_config(_server->module_config);
    return apr::string::make_weak(sconf->ap_document_root, _server->process->pconf);
}

apr_interval_time_t Server::getTimeout() const {
	return _server->timeout;
}
apr_interval_time_t Server::getKeepAliveTimeout() const {
	return _server->keep_alive_timeout;
}
int Server::getMaxKeepAlives() const {
	return _server->keep_alive_max;
}
bool Server::isUsingKeepAlive() const {
	return _server->keep_alive;
}

apr::weak_string Server::getServerPath() const {
	return apr::string::make_weak(_server->path, _server->pathlen, _server->process->pconf);
}

int Server::getMaxRequestLineSize() const {
	return _server->limit_req_line;
}
int Server::getMaxHeaderSize() const {
	return _server->limit_req_fieldsize;
}
int Server::getMaxHeaders() const {
	return _server->limit_req_fields;
}

const String &Server::getSessionKey() const {
	return _config->sessionKey;
}
const String &Server::getSessionName() const {
	return _config->sessionName;
}
apr_time_t Server::getSessionMaxAge() const {
	return _config->sessionMaxAge;
}
bool Server::isSessionSecure() const {
	return _config->isSessionSecure;
}

tpl::Cache *Server::getTemplateCache() const {
	return &_config->_templateCache;
}

pug::Cache *Server::getPugCache() const {
	return &_config->_pugCache;
}

template <typename T>
auto Server_resolvePath(Map<String, T> &map, const String &path) -> typename Map<String, T>::iterator {
	auto it = map.begin();
	auto ret = map.end();
	for (; it != map.end(); it ++) {
		auto &p = it->first;
		if (p.size() - 1 <= path.size()) {
			if (p.back() == '/') {
				if (p.size() == 1 || (path.compare(0, p.size() - 1, p, 0, p.size() - 1) == 0
						&& (path.size() == p.size() - 1 || path.at(p.size() - 1) == '/' ))) {
					if (ret == map.end() || ret->first.size() < p.size()) {
						ret = it;
					}
				}
			} else if (p == path) {
				ret = it;
				break;
			}
		}
	}
	return ret;
}

void Server::onHeartBeat(apr_pool_t *pool) {
	apr::pool::perform([&] {
		auto now = Time::now();
		if (!_config->loadingFalled) {
			auto root = Root::getInstance();
			if (auto dbd = root->dbdOpen(pool, _server)) {
				pg::Handle h(pool, dbd);
				if (now - _config->lastDatabaseCleanup > TimeInterval::seconds(60)) {
					_config->lastDatabaseCleanup = now;
					h.makeSessionsCleanup();
				}
				_config->broadcastId = h.processBroadcasts(*this, _config->broadcastId);
				root->dbdClose(_server, dbd);
			}
		}
		if (now - _config->lastTemplateUpdate > TimeInterval::seconds(10)) {
			_config->_templateCache.update(pool);
			_config->_pugCache.update(pool);
			_config->lastDatabaseCleanup = now;
		}
	}, pool);
}

void Server::onBroadcast(const data::Value &val) {
	if (val.getBool("system")) {
		Root::getInstance()->onBroadcast(val);
		return;
	}

	if (!val.hasValue("data")) {
		return;
	}

	if (val.getBool("message")) {
		String url = String(config::getServerToolsPrefix()) + config::getServerToolsShell();
		auto it = Server_resolvePath(_config->websockets, url);
		if (it != _config->websockets.end() && it->second) {
			it->second->receiveBroadcast(val);
		}
	}

	auto &url = val.getString("url");
	if (!url.empty()) {
		auto it = Server_resolvePath(_config->websockets, url);
		if (it != _config->websockets.end() && it->second) {
			it->second->receiveBroadcast(val.getValue("data"));
		}
	}
}

void Server::onBroadcast(const Bytes &bytes) {
	onBroadcast(data::read(bytes));
}

int Server::onRequest(Request &req) {
	if (_config->forceHttps) {
		if (!req.isSecureConnection()) {
			auto p = req.request()->parsed_uri.port;
			if (!p || p == 80) {
				return req.redirectTo(toString("https://", req.getHostname(), req.getUnparsedUri()));
			} else if (p == 8080) {
				return req.redirectTo(toString("https://", req.getHostname(), ":8443", req.getUnparsedUri()));
			} else {
				return req.redirectTo(toString("https://", req.getHostname(), ":", p, req.getUnparsedUri()));
			}
		}
	}

	if (_config->loadingFalled) {
		return HTTP_SERVICE_UNAVAILABLE;
	}


	auto &path = req.getUri();

	if (!_config->protectedList.empty()) {
		StringView path_v(path);
		auto lb_it = _config->protectedList.lower_bound(path);
		if (lb_it != _config->protectedList.end() && path_v == *lb_it) {
			return HTTP_NOT_FOUND;
		} else {
			-- lb_it;
			StringView lb_v(*lb_it);
			if (path_v.is(lb_v)) {
				if (path_v.size() == lb_v.size() || lb_v.back() == '/' || (path_v.size() > lb_v.size() && path_v[lb_v.size()] == '/')) {
					return HTTP_NOT_FOUND;
				}
			}
		}
	}

	// Websocket handshake
	auto h = req.getRequestHeaders();
	auto connection = string::tolower(h.at("connection"));
	auto & upgrade = h.at("upgrade");
	if (connection.find("upgrade") != String::npos && upgrade == "websocket") {
		// try websocket
		auto it = Server_resolvePath(_config->websockets, path);
		if (it != _config->websockets.end() && it->second) {
			return it->second->accept(req);
		}
		return HTTP_NOT_FOUND;
	}

	for (auto &it : _config->preRequest) {
		auto ret = it(req);
		if (ret == DONE || ret > 0) {
			return ret;
		}
	}

	auto ret = Server_resolvePath(_config->requests, path);
	if (ret != _config->requests.end() && ret->second.callback) {
		RequestHandler *h = ret->second.callback();
		if (h) {
			String subPath((ret->first.back() == '/')?path.substr(ret->first.size() - 1):"");
			String originPath = subPath.size() == 0 ? String(path) : String(ret->first);
			if (originPath.back() == '/' && !subPath.empty()) {
				originPath.pop_back();
			}
			// preflight request (for CORS implementation)
			int preflight = h->onRequestRecieved(req, std::move(originPath), std::move(subPath), ret->second.data);
			if (preflight > 0 || preflight == DONE) { // cors error or successful preflight
				ap_send_interim_response(req.request(), 1);
				return preflight;
			}
			req.setRequestHandler(h);
		}
	}

	auto &data = req.getParsedQueryArgs();
	auto userIp = req.getUseragentIp();
	if (data.hasValue("basic_auth")) {
		if (req.isSecureConnection() || strncmp(userIp.c_str(), "127.", 4) == 0 || userIp == "::1") {
			if (req.getAuthorizedUser()) {
				return req.redirectTo(String(req.getUri()));
			}
			return HTTP_UNAUTHORIZED;
		}
	}

	return OK;
}

Server::CompressionConfig *Server::getCompressionConfig() const {
	return &_config->compression;
}

ServerComponent *Server::getServerComponent(const StringView &name) const {
	auto it = _config->components.find(name);
	if (it != _config->components.end()) {
		return it->second;
	}
	return nullptr;
}

ServerComponent *Server::getServerComponent(std::type_index name) const {
	auto it = _config->typedComponents.find(name);
	if (it != _config->typedComponents.end()) {
		return it->second;
	}
	return nullptr;
}

void Server::addComponentWithName(const String &name, ServerComponent *comp) {
	_config->components.emplace(name, comp);
	_config->typedComponents.emplace(std::type_index(typeid(*comp)), comp);
	if (_config->childInit) {
		comp->onChildInit(*this);
	}
}

const Map<String, ServerComponent *> &Server::getComponents() const {
	return _config->components;
}

void Server::addPreRequest(Function<int(Request &)> &&req) {
	_config->preRequest.emplace_back(std::move(req));
}

void Server::addHandler(const String &path, const HandlerCallback &cb, const data::Value &d) {
	if (!path.empty() && path.front() == '/') {
		_config->requests.emplace(path, RequestScheme{_config->currentComponent, cb, d});
	}
}
void Server::addResourceHandler(const String &path, const storage::Scheme &scheme,
		const data::TransformMap *transform, const AccessControl *a, size_t priority) {
	if (!path.empty() && path.front() == '/') {
		_config->requests.emplace(path, RequestScheme{_config->currentComponent,
			[s = &scheme, transform, a] () -> RequestHandler * { return new ResourceHandler(*s, transform, a, data::Value()); },
			data::Value(), &scheme});
	}
	auto it = _config->resources.find(&scheme);
	if (it == _config->resources.end()) {
		_config->resources.emplace(&scheme, ResourceScheme{priority, path, data::Value()});
	} else {
		if (it->second.priority <= priority) {
			it->second.path = path;
			it->second.priority = priority;
		}
	}
}

void Server::addResourceHandler(const String &path, const storage::Scheme &scheme, const data::Value &val,
		const data::TransformMap *transform, const AccessControl *a, size_t priority) {
	if (!path.empty() && path.front() == '/') {
		_config->requests.emplace(path, RequestScheme{_config->currentComponent,
			[s = &scheme, transform, a, val] () -> RequestHandler * { return new ResourceHandler(*s, transform, a, val); },
			data::Value(), &scheme});
	}
	auto it = _config->resources.find(&scheme);
	if (it == _config->resources.end()) {
		_config->resources.emplace(&scheme, ResourceScheme{priority, path, val});
	} else {
		if (it->second.priority <= priority) {
			it->second.path = path;
			it->second.priority = priority;
		}
	}
}

void Server::addMultiResourceHandler(const String &path, std::initializer_list<Pair<const String, const storage::Scheme *>> &&schemes,
		const data::TransformMap *transform, const AccessControl *a) {
	if (!path.empty() && path.front() == '/') {
		_config->requests.emplace(path, RequestScheme{_config->currentComponent,
			[s = Map<String, const storage::Scheme *>(move(schemes)), transform, a] () -> RequestHandler * {
				return new MultiResourceHandler(s, transform, a);
			}, data::Value()});
	}
}

void Server::addHandler(std::initializer_list<String> paths, const HandlerCallback &cb, const data::Value &d) {
	for (auto &it : paths) {
		if (!it.empty() && it.front() == '/') {
			_config->requests.emplace(std::move(const_cast<String &>(it)), RequestScheme{_config->currentComponent, cb, d});
		}
	}
}

void Server::addWebsocket(const String &str, websocket::Manager *m) {
	_config->websockets.emplace(str, m);
}

const storage::Scheme * Server::exportScheme(const storage::Scheme &scheme) {
	_config->schemes.emplace(scheme.getName(), &scheme);
	return &scheme;
}

const storage::Scheme * Server::getScheme(const StringView &name) const {
	auto it = _config->schemes.find(name);
	if (it != _config->schemes.end()) {
		return it->second;
	}
	return nullptr;
}

const storage::Scheme * Server::getFileScheme() const {
	return getScheme(SA_SERVER_FILE_SCHEME_NAME);
}

const storage::Scheme * Server::getUserScheme() const {
	return getScheme(SA_SERVER_USER_SCHEME_NAME);
}

const storage::Scheme * Server::getErrorScheme() const {
	return getScheme(SA_SERVER_ERROR_SCHEME_NAME);
}

const storage::Scheme * Server::defineUserScheme(std::initializer_list<storage::Field> il) {
	_config->userScheme.define(il);
	return &_config->userScheme;
}

String Server::getResourcePath(const storage::Scheme &scheme) const {
	auto it = _config->resources.find(&scheme);
	if (it != _config->resources.end()) {
		return it->second.path;
	}
	return String();
}

const Map<String, const storage::Scheme *> &Server::getSchemes() const {
	return _config->schemes;
}
const Map<const storage::Scheme *, Server::ResourceScheme> &Server::getResources() const {
	return _config->resources;
}

const Map<String, Server::RequestScheme> &Server::getRequestHandlers() const {
	return _config->requests;
}

struct Server_ErrorList {
	request_rec *request;
	Vector<data::Value> errors;
};

struct Server_ErrorReporterFlags {
	bool isProtected = false;
};

void Server::reportError(const data::Value &d) {
	if (auto obj = apr::pool::get<Server_ErrorReporterFlags>("Server_ErrorReporterFlags")) {
		if (obj->isProtected) {
			return;
		}
	}
	if (auto req = apr::pool::request()) {
		if (auto objVal = Request(req).getObject<Server_ErrorList>("Server_ErrorList")) {
			objVal->errors.emplace_back(d);
		} else {
			auto obj = new Server_ErrorList{req};
			obj->errors.emplace_back(d);
			Request rctx(req);
			rctx.storeObject(obj, "Server_ErrorList", [obj] {
				Server(obj->request->server).runErrorReportTask(obj->request, obj->errors);
			});
		}
	} else {
		runErrorReportTask(nullptr, Vector<data::Value>{d});
	}
}

bool Server::performTask(Task *task, bool performFirst) const {
	return Root::getInstance()->performTask(*this, task, performFirst);
}

bool Server::scheduleTask(Task *task, TimeInterval t) const {
	return Root::getInstance()->scheduleTask(*this, task, t);
}

void Server::runErrorReportTask(request_rec *req, const Vector<data::Value> &errors) {
	auto serv = req->server;
	Task::perform(Server(serv), [&] (Task &task) {
		data::Value *err = nullptr;
		if (req) {
			err = new data::Value {
				pair("documentRoot", data::Value(Server(serv).getDocumentRoot())),
				pair("name", data::Value(serv->server_hostname)),
				pair("url", data::Value(toString(req->hostname, req->unparsed_uri))),
				pair("request", data::Value(req->the_request)),
				pair("ip", data::Value(req->useragent_ip)),
				pair("time", data::Value(Time::now().toMicros()))
			};
			apr::table t(req->headers_in);
			if (!t.empty()) {
				auto &headers = err->emplace("headers");
				for (auto &it : t) {
					headers.setString(it.val, it.key);
				}
			}
		} else {
			err = new data::Value {
				pair("documentRoot", data::Value(Server(serv).getDocumentRoot())),
				pair("name", data::Value(serv->server_hostname)),
				pair("time", data::Value(Time::now().toMicros()))
			};
		}
		auto &d = err->emplace("data");
		for (auto &it : errors) {
			d.addValue(it);
		}
		task.addExecuteFn([err] (const Task &task) -> bool {
			Server_ErrorReporterFlags *obj = apr::pool::get<Server_ErrorReporterFlags>("Server_ErrorReporterFlags");
			if (obj) {
				obj->isProtected = true;
			} else {
				obj = new Server_ErrorReporterFlags{true};
				apr::pool::store(obj, "Server_ErrorReporterFlags");
			}

			auto root = Root::getInstance();
			auto pool = apr::pool::acquire();
			auto serv = apr::pool::server();
			if (auto dbd = root->dbdOpen(pool, serv)) {
				pg::Handle h(pool, dbd);
				storage::Adapter *storage = &h;

				auto serv = Server(apr::pool::server());
				serv.getErrorScheme()->create(storage, *err);

				root->dbdClose(serv, dbd);
			}
			return true;
		});
	});
}

NS_SA_END
