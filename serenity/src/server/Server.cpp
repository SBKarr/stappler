/*
   Copyright 2013 Roman "SBKarr" Katuntsev, LLC St.Appler

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "Define.h"
#include "Server.h"
#include "Couchbase.h"
#include "Request.h"
#include "RequestHandler.h"
#include "Root.h"

#include "SPFilesystem.h"

#include "ServerComponent.h"
#include "StorageField.h"
#include "StorageScheme.h"
#include "DatabaseHandle.h"
#include "ResourceHandler.h"
#include "WebSocket.h"
#include "Tools.h"
#include "TemplateCache.h"

APLOG_USE_MODULE(serenity);

NS_SA_BEGIN

#define SA_SERVER_FILE_SCHEME_NAME "__files"
#define SA_SERVER_USER_SCHEME_NAME "__users"

#ifndef NOCB
apr_status_t sa_server_cb_constructor(void **resource, void *params, apr_pool_t *pool) {
	auto config = (couchbase::Config *)(params);
	if (auto h = couchbase::Connection::create(pool, *config)) {
		(*resource) = h;
		return APR_SUCCESS;
	}
	return APR_EGENERAL;
}

apr_status_t sa_server_cb_destructor(void *resource, void *params, apr_pool_t *pool) {
	if (auto h = (couchbase::Connection *)(resource)) {
		delete h;
	}
	return APR_SUCCESS;
}
#endif

struct Server::Config : public AllocPool {
	static Config *get(server_rec *server) {
		if (!server) { return nullptr; }
		auto cfg = (Config *)ap_get_module_config(server->module_config, &serenity_module);
		if (cfg) { return cfg; }

		return apr::AllocStack::perform([&] () -> Config * {
			return new Config(server);
		}, server);
	}

	Config(server_rec *server) : serverNamespace("default") {
		ap_set_module_config(server->module_config, &serenity_module, this);
	}

	void initCouchbase(data::Value &val) {
#ifndef NOCB
		couchbaseConfig.init(std::move(val));
		apr_reslist_create(&_couchbaseConnections,
				couchbaseConfig.min,
				couchbaseConfig.softMax,
				couchbaseConfig.hardMax,
				couchbaseConfig.ttl,
				sa_server_cb_constructor, sa_server_cb_destructor, &couchbaseConfig, getCurrentPool());
#ifdef APR_RESLIST_CLEANUP_FIRST
		if (_couchbaseConnections) {
			apr_reslist_cleanup_order_set(_couchbaseConnections, APR_RESLIST_CLEANUP_FIRST);
		}
#endif
#endif
	}

	void onHandler(Server &serv, const String &name, const String &ifile, const String &symbol, const data::Value &data) {
		String file;
		if (!sourceRoot.empty()) {
			file = filepath::absolute(filepath::merge(sourceRoot, ifile));
		} else {
			file = filepath::absolute(ifile);
		}

		if (!filesystem::exists(file)) {
			log::format("Server", "No module file: %s", file.c_str());
			return;
		}

		auto pool = AllocStack::get().top();

		/* load new dynamic object */
		apr_dso_handle_t *obj = NULL;
		apr_dso_handle_sym_t sym = NULL;
		apr_status_t err = apr_dso_load(&obj, file.c_str(), pool);
		if (err == APR_SUCCESS) {
			/* loading was successful, export main symbol */
			err = apr_dso_sym(&sym, obj, symbol.c_str());
			if (err == APR_SUCCESS) {
				auto h = ((ServerComponent::Symbol) sym)(serv, name, data);
				if (h) {
					components.emplace(name, h);
				} else {
					log::format("Server", "DSO (%s) returns nullptr handler", name.c_str());
				}
			} else {
				/* fail to load symbol, terminating process */
				char buf[256] = {0};
				log::format("Server", "DSO (%s) error: %s", name.c_str(), apr_dso_error(obj, buf, 255));
				apr_dso_unload(obj);
				return;
			}
		} else {
			/* fail to load object, terminating process */
			char buf[256] = {0};
			log::format("Server", "Fail to load DSO (%s): %s", name.c_str(), apr_dso_error(obj, buf, 255));
			return;
		}
	}

	void initHandlers(Server &serv, data::Value &val) {
		for (auto &it : val.asArray()) {
			if (it.isDictionary()) {

				auto & name = it.getString("name");
				auto & file = it.getString("file");
				auto & symbol = it.getString("symbol");
				auto & data = it.getValue("data");

				onHandler(serv, name, file, symbol, data);
			}
		}
	}

	void initSession(data::Value &val) {
		sessionName = val.getString("name");
		sessionKey = val.getString("key");
		sessionMaxAge = val.getInteger("maxage");
		isSessionSecure = val.getBool("secure");
	}

	void setSessionParam(CharReaderBase &n, CharReaderBase &v) {
		if (n.is("name")) {
			sessionName = v.str();
		} else if (n.is("key")) {
			sessionKey = v.str();
		} else if (n.is("maxage")) {
			sessionMaxAge = v.readInteger();
		} else if (n.is("secure")) {
			if (v.is("true") || v.is("on") || v.is("On")) {
				isSessionSecure = true;
			} else if (v.is("false") || v.is("off") || v.is("Off")) {
				isSessionSecure = false;
			}
		}
	}

	void init(Server &serv) {
		using namespace storage;
		auto users = new storage::Scheme(SA_SERVER_USER_SCHEME_NAME, {
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

		auto files = new storage::Scheme(SA_SERVER_FILE_SCHEME_NAME, {
			Field::Text("location", Transform::Url),
			Field::Text("type", Flags::ReadOnly),
			Field::Integer("size", Flags::ReadOnly),
			Field::Integer("mtime", Flags::AutoMTime | Flags::ReadOnly),
			Field::Extra("image", Vector<Field>{
				Field::Integer("width"),
				Field::Integer("height"),
			})
		});

		schemes.emplace(users->getName(), users);
		schemes.emplace(files->getName(), files);

		if (data) {
			if (data.isDictionary("couchbase")) {
				initCouchbase(data.getValue("couchbase"));
			}

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
			it.second->onChildInit(serv);
		}

		auto root = Root::getInstance();
		auto pool = getCurrentPool();
		auto db = root->dbdOpen(pool, serv);
		if (db) {
			database::Handle h(pool, db);
			h.init(serv, schemes);
			root->dbdClose(serv, db);
		}
	}

	bool childInit = false;
	apr_reslist_t *_couchbaseConnections = nullptr;

#ifndef NOCB
	couchbase::Config couchbaseConfig;
#endif

	data::Value handlers;
	String handlerFile;
	String sourceRoot;
	String serverNamespace;
	Map<String, ServerComponent *> components;
	Map<String, std::pair<HandlerCallback, data::Value>> requests;
	Map<storage::Scheme *, ResourceScheme> resources;
	Map<String, storage::Scheme *> schemes;

	Map<String, websocket::Manager *> websockets;
	data::Value data;

	String sessionName = config::getDefaultSessionName();
	String sessionKey = config::getDefaultSessionKey();
	apr_time_t sessionMaxAge = 0;
	bool isSessionSecure = false;

	Time lastDatabaseCleanup;
	int64_t broadcastId = 0;
	tpl::Cache _templateCache;
};

void * Server::merge(void *base, void *add) {
	Config *baseCfg = (Config *)base;
	Config *addCfg = (Config *)add;

	if (!baseCfg->sourceRoot.empty()) {
		addCfg->sourceRoot = baseCfg->sourceRoot;
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

	filesystem::mkdir(filepath::merge(getDocumentRoot(), "/uploads"));

	tools::registerTools(config::getServerToolsPrefix(), *this);
}

void Server::setHandlerFile(const apr::string &file) {
	_config->handlerFile = file;
}
void Server::setSourceRoot(const apr::string &file) {
	_config->sourceRoot = file;
}
void Server::addHanderSource(const apr::string &str) {
	CharReaderBase r(str);
	r.skipChars<CharReaderBase::CharGroup<chars::CharGroupId::WhiteSpace>>();

	CharReaderBase handlerParams;
	if (r.is('"')) {
		++ r;
		handlerParams = r.readUntil<CharReaderBase::Chars<'"'>>();
		if (r.is('"')) {
			++ r;
		}
	} else {
		handlerParams = r.readUntil<CharReaderBase::CharGroup<chars::CharGroupId::WhiteSpace>>();
	}

	CharReaderBase name, file, symbol;
	name = handlerParams.readUntil<CharReaderBase::Chars<':'>>();
	++ handlerParams;
	file = handlerParams.readUntil<CharReaderBase::Chars<':'>>();
	++ handlerParams;
	symbol = handlerParams;

	if (!name.empty() && !file.empty() && !symbol.empty()) {
		data::Value h;
		h.setString(name.str(), "name");
		h.setString(file.str(), "file");
		h.setString(symbol.str(), "symbol");
		data::Value &data = h.emplace("data");

		while (!r.empty()) {
			r.skipChars<CharReaderBase::CharGroup<chars::CharGroupId::WhiteSpace>>();
			CharReaderBase params, n, v;
			if (r.is('"')) {
				++ r;
				params = r.readUntil<CharReaderBase::Chars<'"'>>();
				if (r.is('"')) {
					++ r;
				}
			} else {
				params = r.readUntil<CharReaderBase::CharGroup<chars::CharGroupId::WhiteSpace>>();
			}

			if (!params.empty()) {
				n = params.readUntil<CharReaderBase::Chars<'='>>();
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

void Server::setSessionParams(const apr::string &str) {
	CharReaderBase r(str);
	r.skipChars<CharReaderBase::CharGroup<chars::CharGroupId::WhiteSpace>>();
	while (!r.empty()) {
		CharReaderBase params, n, v;
		if (r.is('"')) {
			++ r;
			params = r.readUntil<CharReaderBase::Chars<'"'>>();
			if (r.is('"')) {
				++ r;
			}
		} else {
			params = r.readUntil<CharReaderBase::CharGroup<chars::CharGroupId::WhiteSpace>>();
		}

		if (!params.empty()) {
			n = params.readUntil<CharReaderBase::Chars<'='>>();
			++ params;
			v = params;

			if (!n.empty() && ! v.empty()) {
				_config->setSessionParam(n, v);
			}
		}

		r.skipChars<CharReaderBase::CharGroup<chars::CharGroupId::WhiteSpace>>();
	}
}

const apr::string &Server::getHandlerFile() const {
	return _config->handlerFile;
}

apr::weak_string Server::getDefaultName() const {
	return apr::string::make_weak(_server->defn_name, _server);
}

bool Server::isVirtual() const {
	return _server->is_virtual;
}

apr_port_t Server::getServerPort() const {
	return _server->port;
}
apr::weak_string Server::getServerScheme() const {
	return apr::string::make_weak(_server->server_scheme, _server);
}
apr::weak_string Server::getServerAdmin() const {
	return apr::string::make_weak(_server->server_admin, _server);
}
apr::weak_string Server::getServerHostname() const {
	return apr::string::make_weak(_server->server_hostname, _server);
}
apr::weak_string Server::getDocumentRoot() const {
	core_server_config *sconf = (core_server_config *)ap_get_core_module_config(_server->module_config);
    return apr::string::make_weak(sconf->ap_document_root, _server);
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
	return apr::string::make_weak(_server->path, _server->pathlen, _server);
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

const apr::string &Server::getSessionKey() const {
	return _config->sessionKey;
}
const apr::string &Server::getSessionName() const {
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

const apr::string &Server::getNamespace() const {
	return _config->serverNamespace;
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

void Server::onHeartBeat() {
	AllocStack::perform([&] {
		auto now = Time::now();
		auto root = Root::getInstance();
		auto pool = AllocStack::get().top();
		if (auto dbd = root->dbdOpen(pool, _server)) {
			database::Handle h(pool, dbd);
			if (now - _config->lastDatabaseCleanup > TimeInterval::seconds(60)) {
				_config->lastDatabaseCleanup = now;
				h.makeSessionsCleanup();
			}
			_config->broadcastId = h.processBroadcasts(*this, _config->broadcastId);
			root->dbdClose(_server, dbd);
		}
		_config->_templateCache.update();
	});
}

void Server::onBroadcast(const data::Value &val) {
	if (val.getBool("system")) {
		Root::getInstance()->onBroadcast(val);
		return;
	}

	if (!val.isString("url") || !val.hasValue("data")) {
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
	auto it = Server_resolvePath(_config->websockets, url);
	if (it != _config->websockets.end() && it->second) {
		it->second->receiveBroadcast(val.getValue("data"));
	}
}

void Server::onBroadcast(const Bytes &bytes) {
	onBroadcast(data::read(bytes));
}

int Server::onRequest(Request &req) {
	auto &path = req.getUri();

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

	auto ret = Server_resolvePath(_config->requests, path);
	if (ret != _config->requests.end() && ret->second.first) {
		RequestHandler *h = ret->second.first();
		if (h) {
			String subPath((ret->first.back() == '/')?path.substr(ret->first.size() - 1):"");
			// preflight request (for CORS implementation)
			int preflight = h->onRequestRecieved(req, subPath, ret->second.second);
			if (preflight > 0 || preflight == DONE) { // cors error or successful preflight
				ap_send_interim_response(req.request(), true);
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

#ifndef NOCB
couchbase::Connection *Server::acquireCouchbase() {
	if (_config->_couchbaseConnections) {
		couchbase::Connection *h;
		if (apr_reslist_acquire(_config->_couchbaseConnections, (void **)(&h)) == APR_SUCCESS) {
			return h;
		}
	}
	return NULL;
}

void Server::releaseCouchbase(couchbase::Connection *cb) {
	if (cb) {
		if (cb->shouldBeInvalidated()) {
			apr_reslist_invalidate(_config->_couchbaseConnections, cb);
		} else {
			apr_reslist_release(_config->_couchbaseConnections, cb);
		}
	}
}
#endif

ServerComponent *Server::getComponent(const String &name) const {
	auto it = _config->components.find(name);
	if (it != _config->components.end()) {
		return it->second;
	}
	return nullptr;
}

void Server::addComponent(const String &name, ServerComponent *comp) {
	_config->components.emplace(name, comp);
	if (_config->childInit) {
		comp->onChildInit(*this);
	}
}

void Server::addHandler(const String &path, const HandlerCallback &cb, const data::Value &d) {
	if (!path.empty() && path.front() == '/') {
		_config->requests.emplace(path, std::make_pair(cb, d));
	}
}
void Server::addResourceHandler(const String &path, storage::Scheme *scheme,
		data::TransformMap *transform, AccessControl *a, size_t priority) {
	addHandler(path, [scheme, transform, a] () -> RequestHandler * { return new ResourceHandler(scheme, transform, a, data::Value()); });
	auto it = _config->resources.find(scheme);
	if (it == _config->resources.end()) {
		_config->resources.emplace(scheme, ResourceScheme{path, priority, data::Value()});
	} else {
		if (it->second.priority <= priority) {
			it->second.path = path;
			it->second.priority = priority;
		}
	}
}

void Server::addResourceHandler(const String &path, storage::Scheme *scheme, const data::Value &val,
		data::TransformMap *transform, AccessControl *a, size_t priority) {
	addHandler(path, [scheme, transform, a, val] () -> RequestHandler * { return new ResourceHandler(scheme, transform, a, val); });
	auto it = _config->resources.find(scheme);
	if (it == _config->resources.end()) {
		_config->resources.emplace(scheme, ResourceScheme{path, priority, val});
	} else {
		if (it->second.priority <= priority) {
			it->second.path = path;
			it->second.priority = priority;
		}
	}
}


void Server::addHandler(std::initializer_list<String> paths, const HandlerCallback &cb, const data::Value &d) {
	for (auto &it : paths) {
		if (!it.empty() && it.front() == '/') {
			_config->requests.emplace(std::move(const_cast<String &>(it)), std::make_pair(cb, d));
		}
	}
}

void Server::addWebsocket(const String &str, websocket::Manager *m) {
	_config->websockets.emplace(str, m);
}

storage::Scheme * Server::exportScheme(storage::Scheme *scheme) {
	_config->schemes.emplace(scheme->getName(), scheme);
	return scheme;
}

storage::Scheme * Server::getScheme(const String &name) const {
	auto it = _config->schemes.find(name);
	if (it != _config->schemes.end()) {
		return it->second;
	}
	return nullptr;
}

storage::Scheme * Server::getFileScheme() const {
	return getScheme(SA_SERVER_FILE_SCHEME_NAME);
}

storage::Scheme * Server::getUserScheme() const {
	return getScheme(SA_SERVER_USER_SCHEME_NAME);
}

String Server::getResourcePath(storage::Scheme *scheme) const {
	auto it = _config->resources.find(scheme);
	if (it != _config->resources.end()) {
		return it->second.path;
	}
	return String();
}

const Map<String, storage::Scheme *> &Server::getSchemes() const {
	return _config->schemes;
}
const Map<storage::Scheme *, Server::ResourceScheme> &Server::getResources() const {
	return _config->resources;
}

const Map<String, std::pair<Server::HandlerCallback, data::Value>> &Server::getRequestHandlers() const {
	return _config->requests;
}

NS_SA_END
