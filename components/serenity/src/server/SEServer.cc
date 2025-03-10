/**
Copyright (c) 2016-2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "Networking.h"
#include "SPFilesystem.h"
#include "SPCrypto.h"
#include "Request.h"
#include "RequestHandler.h"
#include "Root.h"

#include "ServerComponent.h"
#include "STPqHandle.h"
#include "ResourceHandler.h"
#include "MultiResourceHandler.h"
#include "WebSocket.h"
#include "Tools.h"
#include "TemplateCache.h"
#include "SPugCache.h"
#include "ExternalSession.h"
#include "SEDbdModule.h"
#include "STFieldTextArray.h"

NS_SA_BEGIN

static constexpr auto SA_SERVER_FILE_SCHEME_NAME = "__files";
static constexpr auto SA_SERVER_USER_SCHEME_NAME = "__users";
static constexpr auto SA_SERVER_ERROR_SCHEME_NAME = "__error";

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
		rootPool = mem::pool::acquire();
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
		memset(serverKey.data(), 0, serverKey.size());
	}

	void onHandler(Server &serv, const StringView &name, const StringView &ifile, const StringView &symbol, const data::Value &handlerData) {
		String file;
		for (auto &it : sourceRoot) {
			auto path = filepath::absolute(filepath::merge(it, ifile));
			if (filesystem::exists(path)) {
				file = path;
			}
		}

		if (file.empty()) {
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
			err = apr_dso_sym(&sym, obj, symbol.data());
			if (err == APR_SUCCESS) {
				auto h = ((ServerComponent::Symbol) sym)(serv, name.str(), handlerData);
				if (h) {
					components.emplace(h->getName().str(), h);
					typedComponents.emplace(std::type_index(typeid(*h)), h);
				} else {
					log::format("Server", "DSO (%s) returns nullptr handler", name.data());
					serv.reportError(data::Value({
						pair("source", data::Value("Server")),
						pair("text", data::Value(toString("DSO (", name, ") returns nullptr handler")))
					}));
					loadingFalled = true;
				}
			} else {
				/* fail to load symbol, terminating process */
				char buf[256] = {0};
				log::format("Server", "DSO (%s) error: %s", name.data(), apr_dso_error(obj, buf, 255));
				serv.reportError(data::Value({
					pair("source", data::Value("Server")),
					pair("text", data::Value(toString("DSO (", name, ") error: ", apr_dso_error(obj, buf, 255))))
				}));
				apr_dso_unload(obj);
				loadingFalled = true;
			}
		} else {
			/* fail to load object, terminating process */
			char buf[256] = {0};
			log::format("Server", "Fail to load DSO (%s): %s", name.data(), apr_dso_error(obj, buf, 255));
			serv.reportError(data::Value({
				pair("source", data::Value("Server")),
				pair("text", data::Value(toString("Fail to load DSO (", name, "): ", apr_dso_error(obj, buf, 255))))
			}));
			loadingFalled = true;
		}
	}

	void initHandlers(Server &serv, const Vector<Pair<String, data::Value>> &val) {
		for (auto &it : val) {
			if (it.second.isDictionary()) {
				auto & name = it.second.getString("name");
				auto & file = it.second.getString("file");
				auto & symbol = it.second.getString("symbol");
				auto & handlerData = it.second.getValue("data");

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

	void setServerKey(StringView w) {
		serverKey = string::Sha512::hmac(w, w);
	}

	void addAllowed(StringView r) {
		auto p = valid::readIpRange(r);
		if (p.first && p.second) {
			allowedIps.emplace_back(p.first, p.second);
		}
	}

	void init(Server &serv) {
		schemes.emplace(userScheme.getName(), &userScheme);
		schemes.emplace(fileScheme.getName(), &fileScheme);
		schemes.emplace(errorScheme.getName(), &errorScheme);

		if (!handlers.empty()) {
			initHandlers(serv, handlers);
		}
	}

	bool initKeyPair(Server &serv, const db::Adapter &a, BytesView fp) {
		auto pers = serv.getServerHostname();

		crypto::PrivateKey pk;
		if (pk.generate()) {
			auto priv = pk.exportPem();
			auto pub = pk.exportPublic().exportPem();

			privateSessionKey = StringView((const char *)priv.data(), priv.size()).str();
			publicSessionKey = StringView((const char *)pub.data(), pub.size()).str();
		}

	    auto tok = AesToken::create(AesToken::Keys{ StringView(), StringView(config::INTERNAL_PRIVATE_KEY), BytesView(serverKey) });
		tok.setString(privateSessionKey, "priv");
		tok.setString(publicSessionKey, "pub");
		if (auto d = tok.exportData(fp)) {
			std::array<uint8_t, string::Sha512::Length + 4> data;
			memcpy(data.data(), "srv:", 4);
			memcpy(data.data() + 4, fp.data(), fp.size());

			a.set(data, d, TimeInterval::seconds(60 * 60 * 365 * 100)); // 100 years
			return true;
		}
	    return false;
	}

	void initServerKeys(Server &serv, const db::Adapter &a) {
		if (!privateSessionKey.empty() || !publicSessionKey.empty()) {
			return;
		}

		auto fp = string::Sha512::hmac(serv.getServerHostname(), serverKey);

		std::array<uint8_t, string::Sha512::Length + 4> data;
		memcpy(data.data(), "srv:", 4);
		memcpy(data.data() + 4, fp.data(), fp.size());

		if (auto d = a.get(data)) {
			if (auto tok = AesToken::parse(d, BytesView(fp), AesToken::Keys{ StringView(), StringView(config::INTERNAL_PRIVATE_KEY), BytesView(serverKey) })) {
				privateSessionKey = tok.getString("priv");
				publicSessionKey = tok.getString("pub");
			}
		}

		if (privateSessionKey.empty()) {
			initKeyPair(serv, a, fp);
		}
	}

	void onChildInit(Server &serv, mem::pool_t *p) {
		rootPool = p;
		for (auto &it : components) {
			currentComponent = it.second->getName();
			it.second->onChildInit(serv);
			currentComponent = String();
		}

		childInit = true;

		auto root = Root::getInstance();
		auto pool = getCurrentPool();

		db::sql::Driver::Handle db;
 		if (dbParams) {
			// run custom dbd
			customDbd = DbdModule::create(rootPool, dbParams);
			dbDriver = customDbd->getDriver();
			db = customDbd->openConnection(pool);
		} else {
			// setup apache httpd dbd
			db = root->dbdOpen(pool, serv);
			if (db.get()) {
				dbDriver = db::sql::Driver::open(p, apr_dbd_name(((ap_dbd_t *)db.get())->driver), ((ap_dbd_t *)db.get())->driver);
				dbDriver->init(db, mem::Vector<mem::StringView>());
			}
		}

 		if (!schemes.empty() && !db.get()) {
 			loadingFalled = true;
 		}

		if (!loadingFalled) {
			db::Scheme::initSchemes(schemes);

			apr::pool::perform([&] {
				dbDriver->performWithStorage(db, [&] (const db::Adapter &storage) {
					storage.init(db::Interface::Config{StringView(serv.server()->server_hostname), serv.getFileScheme()}, schemes);

					if (serverKey != string::Sha512::Buf{0}) {
						initServerKeys(serv, storage);
					}

					for (auto &it : components) {
						currentComponent = it.second->getName();
						it.second->onStorageInit(serv, storage);
						currentComponent = String();
					}
				});
			});
		}

		if (db.get()) {
			if (customDbd) {
				customDbd->closeConnection(db);
			} else {
				root->dbdClose(serv, db);
			}
		}

		if (loadingFalled) {
			abort();
		}
	}

	void onStorageTransaction(storage::Transaction &t) {
		for (auto &it : components) {
			it.second->onStorageTransaction(t);
		}
	}

	void setDbParams(StringView str) {
		mem::perform([&] {
			dbParams = new (rootPool) Map<StringView, StringView>;
			Root::parseParameterList(*dbParams, str);
		}, rootPool);
	}

	void onError(const StringView &str) {
#if DEBUG
		std::cout << "[Template]: " << str << "\n";
#endif
		messages::error("Template", "Template compilation error", data::Value(str));
	}

	bool childInit = false;

	db::Scheme userScheme = storage::Scheme(SA_SERVER_USER_SCHEME_NAME, {
		db::Field::Text("name", db::Transform::Alias, db::Flags::Required),
		db::Field::Bytes("pubkey", db::Transform::PublicKey, db::Flags::Indexed),
		db::Field::Password("password", db::PasswordSalt(config::getDefaultPasswordSalt()), db::Flags::Required | db::Flags::Protected),
		db::Field::Boolean("isAdmin", data::Value(false)),
		db::Field::Extra("data", Vector<db::Field>({
			db::Field::Text("email", db::Transform::Email),
			db::Field::Text("public"),
			db::Field::Text("desc"),
		})),
		db::Field::Text("email", db::Transform::Email, db::Flags::Unique),
	});

	db::Scheme fileScheme = storage::Scheme(SA_SERVER_FILE_SCHEME_NAME, {
		db::Field::Text("location", db::Transform::Url),
		db::Field::Text("type", db::Flags::ReadOnly),
		db::Field::Integer("size", db::Flags::ReadOnly),
		db::Field::Integer("mtime", db::Flags::AutoMTime | db::Flags::ReadOnly),
		db::Field::Extra("image", Vector<db::Field>{
			db::Field::Integer("width"),
			db::Field::Integer("height"),
		})
	});

	db::Scheme errorScheme = storage::Scheme(SA_SERVER_ERROR_SCHEME_NAME, {
		db::Field::Boolean("hidden", data::Value(false)),
		db::Field::Boolean("delivered", data::Value(false)),
		db::Field::Text("name"),
		db::Field::Text("documentRoot"),
		db::Field::Text("url"),
		db::Field::Text("request"),
		db::Field::Text("ip"),
		db::Field::Data("headers"),
		db::Field::Data("data"),
		db::Field::Integer("time"),
		db::Field::Custom(new db::FieldTextArray("tags", db::Flags::Indexed,
				db::DefaultFn([&] (const mem::Value &data) -> mem::Value {
			mem::Vector<mem::String> tags;
			for (auto &it : data.getArray("data")) {
				auto text = it.getString("source");
				if (!text.empty()) {
					mem::emplace_ordered(tags, text);
				}
			}

			mem::Value ret;
			for (auto &it : tags) {
				ret.addString(it);
			}
			return ret;
		})))
	});

	Vector<Pair<String, data::Value>> handlers;

	Vector<String> sourceRoot;
	StringView currentComponent;
	Vector<Function<int(Request &)>> preRequest;
	Map<String, ServerComponent *> components;
	Map<std::type_index, ServerComponent *> typedComponents;
	Map<String, RequestScheme> requests;
	Map<const storage::Scheme *, ResourceScheme> resources;
	Map<StringView, const storage::Scheme *> schemes;

	Map<String, websocket::Manager *> websockets;

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

	String publicSessionKey;
	String privateSessionKey;
	string::Sha512::Buf serverKey;

	Vector<Pair<uint32_t, uint32_t>> allowedIps;
	Map<StringView, StringView> *dbParams = nullptr;
	DbdModule *customDbd = nullptr;
	mem::pool_t *rootPool = nullptr;
	db::sql::Driver *dbDriver = nullptr;
};

void * Server::merge(void *base, void *add) {
	Config *baseCfg = (Config *)base;
	Config *addCfg = (Config *)add;

	if (!baseCfg->sourceRoot.empty()) {
		if (addCfg->sourceRoot.empty()) {
			addCfg->sourceRoot = baseCfg->sourceRoot;
		} else {
			auto pool = apr::pool::acquire();

			apr::pool::perform([&] {
				Set<String> strings;
				for (auto &it : baseCfg->sourceRoot) {
					strings.emplace(it);
				}
				for (auto &it : addCfg->sourceRoot) {
					strings.erase(it);
				}
				if (!strings.empty()) {
					for (auto &it : strings) {
						addCfg->sourceRoot.emplace_back(String(it, pool));
					}
				}
			});
		}
	}
	if (!baseCfg->webHookUrl.empty()) { addCfg->webHookUrl = baseCfg->webHookUrl; }
	if (!baseCfg->webHookName.empty()) { addCfg->webHookName = baseCfg->webHookName; }
	if (!baseCfg->webHookFormat.empty()) { addCfg->webHookFormat = baseCfg->webHookFormat; }

	if (!baseCfg->webHookExtra.empty()) {
		for (auto &it : baseCfg->webHookExtra.asDict()) {
			addCfg->webHookExtra.setValue(it.second, it.first);
		}
	}

	if (!baseCfg->handlers.empty()) {
		if (addCfg->handlers.empty()) {
			addCfg->handlers = baseCfg->handlers;
		} else {
			auto tmp = move(addCfg->handlers);
			addCfg->handlers = baseCfg->handlers;

			for (auto &it : tmp) {
				addCfg->handlers.emplace_back(move(it));
			}
		}
	}

	if (!baseCfg->allowedIps.empty()) {
		if (addCfg->allowedIps.empty()) {
			addCfg->allowedIps = baseCfg->allowedIps;
		} else {
			auto tmp = move(addCfg->allowedIps);
			addCfg->allowedIps = baseCfg->allowedIps;

			for (auto &it : tmp) {
				addCfg->allowedIps.emplace_back(move(it));
			}
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

void Server::onChildInit(mem::pool_t *rootPool) {
	_config->init(*this);
	_config->onChildInit(*this, rootPool);

	filesystem::mkdir(filepath::merge(getDocumentRoot(), "/.serenity"));
	filesystem::mkdir(filepath::merge(getDocumentRoot(), "/uploads"));

	_config->currentComponent = StringView("root");
	tools::registerTools(config::getServerToolsPrefix(), *this);
	_config->currentComponent = StringView();

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

void Server::performWithStorage(const Callback<void(const db::Transaction &)> &cb, bool openNewConnecton) const {
	if (!openNewConnecton) {
		if (auto t = db::Transaction::acquireIfExists()) {
			cb(t);
			return;
		}
	}

	auto targetPool = mem::pool::acquire();

	apr::pool::perform([&] {
		db::sql::Driver::Handle handle = openDbConnection(targetPool);
		if (handle.get()) {
			_config->dbDriver->performWithStorage(handle, [&] (const db::Adapter &a) {
				if (auto t = db::Transaction::acquire(a)) {
					cb(t);
					t.release();
				}
			});
			closeDbConnection(handle);
		}
	}, targetPool);
}

db::Interface * Server::acquireDbForRequest(request_rec *r) const {
	if (_config->customDbd) {
		auto handle = _config->customDbd->openConnection(r->pool);
		if (handle.get()) {
			mem::pool::cleanup_register(r->pool, [handle, dbd = _config->customDbd] {
				dbd->closeConnection(handle);
			});

			return _config->dbDriver->acquireInterface(handle, r->pool);
		}
	} else {
		auto handle = Root::getInstance()->dbdAcquire(r);
		if (handle.get()) {
			return _config->dbDriver->acquireInterface(handle, r->pool);
		}
	}

	return nullptr;
}

void Server::setSessionKeys(StringView pub, StringView priv) const {
	_config->publicSessionKey = pub.str();
	_config->privateSessionKey = priv.str();
}

StringView Server::getSessionPublicKey() const {
	return _config->publicSessionKey;
}

StringView Server::getSessionPrivateKey() const {
	return _config->privateSessionKey;
}

BytesView Server::getServerSecret() const {
	return _config->serverKey;
}

void Server::setSourceRoot(StringView file) {
	_config->sourceRoot.emplace_back(file.str());
}
void Server::addHanderSource(StringView str) {
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

				if (!n.empty()) {
					if (v.empty()) {
						data.setBool(true, n.str());
					} else {
						data.setString(v.str(), n.str());
					}
				}
			}
		}

		_config->handlers.emplace_back(args[idx - 2].str(), std::move(h));
	}
}

void Server::addAllow(StringView ips) {
	ips.split<StringView::CharGroup<CharGroupId::WhiteSpace>>([&] (StringView r) {
		_config->addAllowed(r);
	});

}

void Server::setSessionParams(StringView str) {
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

void Server::setServerKey(StringView w) {
	if (!w.empty()) {
		_config->setServerKey(w);
	}
}

void Server::setWebHookParams(StringView str) {
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

void Server::setProtectedList(StringView str) {
	str.split<StringView::Chars<' '>>([&] (StringView &value) {
		addProtectedLocation(value);
	});
}

void Server::setDbParams(StringView w) {
	_config->setDbParams(w);
}

void Server::addProtectedLocation(const StringView &value) {
	_config->protectedList.emplace(value.str());
}

void Server::setForceHttps() {
	_config->setForceHttps();
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

mem::pool_t *Server::getThreadPool() const {
	return _config->rootPool;
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

db::sql::Driver *Server::getDbDriver() const {
	return _config->dbDriver;
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

void Server::initHeartBeat(apr_pool_t *, int epoll) {
	auto p = _config->_pugCache.getNotify();
	if (p >= 0) {
		auto c = new (getProcessPool()) PollClient();
		c->type = PollClient::INotify;
		c->ptr = &_config->_pugCache;
		c->fd = p;
		c->server = Server(*this);
		c->event.data.ptr = c;
		c->event.events = EPOLLIN;

		auto err = epoll_ctl(epoll, EPOLL_CTL_ADD, p, &c->event);
		if (err == -1) {
			char buf[256] = { 0 };
			std::cout << "Failed to start thread worker with socket epoll_ctl("
					<< p << ", EPOLL_CTL_ADD): " << strerror_r(errno, buf, 255) << "\n";
		}
	}

	auto t = _config->_templateCache.getNotify();
	if (t >= 0) {
		auto c = new (getProcessPool()) PollClient();
		c->type = PollClient::TemplateINotify;
		c->ptr = &_config->_templateCache;
		c->fd = t;
		c->server = Server(*this);
		c->event.data.ptr = c;
		c->event.events = EPOLLIN;

		auto err = epoll_ctl(epoll, EPOLL_CTL_ADD, t, &c->event);
		if (err == -1) {
			char buf[256] = { 0 };
			std::cout << "Failed to start thread worker with socket epoll_ctl("
					<< p << ", EPOLL_CTL_ADD): " << strerror_r(errno, buf, 255) << "\n";
		}
	}

	if (_config->dbDriver && _config->dbDriver->isNotificationsSupported()) {
		auto handle = openDbConnection(getProcessPool());
		int sock = _config->dbDriver->listenForNotifications(handle);
		if (sock >= 0) {
			auto c = new (getProcessPool()) PollClient();
			c->type = PollClient::Postgres;
			c->ptr = handle.get();
			c->fd = sock;
			c->server = Server(*this);
			c->event.data.ptr = c;
			c->event.events = EPOLLIN | EPOLLERR | EPOLLET;

			auto err = epoll_ctl(epoll, EPOLL_CTL_ADD, sock, &c->event);
			if (err == -1) {
				char buf[256] = { 0 };
				std::cout << "Failed to start thread worker with socket epoll_ctl("
						<< sock << ", EPOLL_CTL_ADD): " << strerror_r(errno, buf, 255) << "\n";
				closeDbConnection(handle);
			}
		} else {
			closeDbConnection(handle);
		}
	}
}

void Server::onHeartBeat(apr_pool_t *pool) {
	memory::pool::store(pool, _server, "Apr.Server");
	apr::pool::perform([&] {
		auto now = Time::now();
		if (!_config->loadingFalled) {
			if (now - _config->lastDatabaseCleanup > config::getDefaultDatabaseCleanupInterval()) {
				db::sql::Driver::Handle handle = openDbConnection(pool);
				if (handle.get()) {
					_config->dbDriver->performWithStorage(handle, [&] (const db::Adapter &a) {
						_config->lastDatabaseCleanup = now;
						a.makeSessionsCleanup();
					});
					closeDbConnection(handle);
				}
			}

			for (auto &it : _config->components) {
				it.second->onHeartbeat(*this);
			}
		}
		if (now - _config->lastTemplateUpdate > config::getDefaultPugTemplateUpdateInterval()) {
			if (!_config->_templateCache.isNotifyAvailable()) {
				_config->_templateCache.update(pool);
			}
			if (!_config->_pugCache.isNotifyAvailable()) {
				_config->_pugCache.update(pool);
			}
			_config->lastTemplateUpdate = now;
		}
	}, pool);
	memory::pool::store(pool, nullptr, "Apr.Server");
}

void Server::checkBroadcasts() {
	Task::perform(*this, [&] (Task &task) {
		task.addExecuteFn([this] (const Task &task) -> bool {
			auto handle = openDbConnection(task.pool());
			if (handle.get()) {
				_config->dbDriver->performWithStorage(handle, [&] (const db::Adapter &a) {
					_config->broadcastId = a.interface()->processBroadcasts([&] (BytesView bytes) {
						onBroadcast(bytes);
					}, _config->broadcastId);
				});
				closeDbConnection(handle);
			}
			return true;
		});
	});
}

void Server::onBroadcast(const data::Value &val) {
	if (val.getBool("system")) {
		Root::getInstance()->onBroadcast(val);
		return;
	}

	if (!val.hasValue("data")) {
		return;
	}

	if (val.getBool("message") && !val.getBool("exclusive")) {
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

void Server::onBroadcast(const BytesView &bytes) {
	onBroadcast(data::read(bytes));
}

bool Server::isSecureAuthAllowed(const Request &rctx) const {
	auto userIp = rctx.getUseragentIp();
	if (rctx.isSecureConnection() || strncmp(userIp.data(), "127.", 4) == 0 || userIp == "::1") {
		return true;
	}

	if (auto ip = valid::readIp(userIp)) {
		for (auto &it : _config->allowedIps) {
			if (ip >= it.first && ip <= it.second) {
				return true;
			}
		}
	}

	return false;
}

static bool Server_processAuth(Request &rctx, StringView auth) {
	mem::StringView r(auth);
	r.skipChars<mem::StringView::CharGroup<stappler::CharGroupId::WhiteSpace>>();
	auto method = r.readUntil<mem::StringView::CharGroup<stappler::CharGroupId::WhiteSpace>>().str();
	stappler::string::tolower(method);
	if (method == "basic" && rctx.isSecureAuthAllowed()) {
		r.skipChars<mem::StringView::CharGroup<stappler::CharGroupId::WhiteSpace>>();
		auto str = stappler::base64::decode(r);
		mem::StringView source((const char *)str.data(), str.size());
		mem::StringView user = source.readUntil<mem::StringView::Chars<':'>>();
		if (source.is(':')) {
			++ source;

			if (!user.empty() && !source.empty()) {
				auto storage = rctx.storage();
				auto u = db::User::get(storage, mem::String::make_weak(user.data(), user.size()),
						mem::String::make_weak(source.data(), source.size()));
				if (u) {
					rctx.setUser(u);
					if (u->isAdmin()) {
						auto &args = rctx.getParsedQueryArgs();
						if (args.hasValue("__FORCE_ROLE__")) {
							auto role = args.getInteger("__FORCE_ROLE__");
							rctx.setAccessRole(db::AccessRoleId(role));
						}
					}
					return true;
				}
			}
		}
	} else if (method == "pkey") {
		r.skipChars<mem::StringView::CharGroup<stappler::CharGroupId::WhiteSpace>>();
		auto d = stappler::data::read(stappler::base64::decode(r));
		if (d.isArray() && d.size() == 2 && d.isBytes(0) && d.isBytes(1)) {
			auto &key = d.getBytes(0);
			auto &sig = d.getBytes(1);

			crypto::PublicKey pk;

			do {
				if (key.size() < 128 || sig.size() < 128) {
					break;
				}

				if (memcmp(key.data(), "ssh-", 4) == 0) {
					auto derKey = stappler::valid::convertOpenSSHKey(mem::StringView((const char *)key.data(), key.size()));
					if (!derKey.empty()) {
						if (!pk.import(derKey)) {
							break;
						}
					} else {
						break;
					}
				} else {
					if (!pk.import(key)) {
						break;
					}
				}

				if (!pk.verify(key, sig, crypto::SignAlgorithm::RSA_SHA512)) {
					break;
				}

				auto searchKey = pk.exportDer();
				if (auto u = db::User::get(rctx.storage(), *db::internals::getUserScheme(), searchKey)) {
					rctx.setUser(u);
					return true;
				}
			} while (0);
		}
	}
	return false;
}

static int Server_onRequestRecieved(Request &rctx, RequestHandler &h) {
	auto auth = rctx.getRequestHeaders().at("Authorization");
	if (!auth.empty()) {
		Server_processAuth(rctx, auth);
	}

	auto &origin = rctx.getRequestHeaders().at("Origin");
	if (origin.empty()) {
		return OK;
	}

	if (rctx.getMethod() != Request::Options) {
		// non-preflightted request
		if (h.isCorsPermitted(rctx, origin)) {
			rctx.getResponseHeaders().emplace("Access-Control-Allow-Origin", origin);
			rctx.getResponseHeaders().emplace("Access-Control-Allow-Credentials", "true");

			rctx.getErrorHeaders().emplace("Access-Control-Allow-Origin", origin);
			rctx.getErrorHeaders().emplace("Access-Control-Allow-Credentials", "true");
			return OK;
		} else {
			return HTTP_METHOD_NOT_ALLOWED;
		}
	} else {
		auto &method = rctx.getRequestHeaders().at("Access-Control-Request-Method");
		auto &headers = rctx.getRequestHeaders().at("Access-Control-Request-Headers");

		if (h.isCorsPermitted(rctx, origin, true, method, headers)) {
			rctx.getResponseHeaders().emplace("Access-Control-Allow-Origin", origin);
			rctx.getResponseHeaders().emplace("Access-Control-Allow-Credentials", "true");

			auto c_methods = h.getCorsAllowMethods(rctx);
			if (!c_methods.empty()) {
				rctx.getResponseHeaders().emplace("Access-Control-Allow-Methods", c_methods.str<mem::Interface>());
			} else if (!method.empty()) {
				rctx.getResponseHeaders().emplace("Access-Control-Allow-Methods", method);
			}

			auto c_headers = h.getCorsAllowHeaders(rctx);
			if (!c_headers.empty()) {
				rctx.getResponseHeaders().emplace("Access-Control-Allow-Headers", c_headers.str<mem::Interface>());
			} else if (!headers.empty()) {
				rctx.getResponseHeaders().emplace("Access-Control-Allow-Headers", headers);
			}

			auto c_maxAge = h.getCorsMaxAge(rctx);
			if (!c_maxAge.empty()) {
				rctx.getResponseHeaders().emplace("Access-Control-Max-Age", c_maxAge.str<mem::Interface>());
			}

			return DONE;
		} else {
			return HTTP_METHOD_NOT_ALLOWED;
		}
	}
}

int Server::onRequest(Request &req) {
	memory::pool::store(req.pool(), _server, "Apr.Server");
	if (_config->forceHttps) {
		StringView uri(req.getUnparsedUri());
		if (uri.starts_with("/.well-known/acme-challenge/")) {
			++ uri;
			if (filesystem::exists(uri)) {
				auto content = filesystem::readIntoMemory(uri);
				req.write((const char *)content.data(), content.size());
				return DONE;
			}
		}

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
			auto auth = req.getRequestHeaders().at("Authorization");
			if (!auth.empty()) {
				Server_processAuth(req, auth);
			}
			return it->second->accept(req);
		}
		return DECLINED;
	}

	for (auto &it : _config->preRequest) {
		auto ret = it(req);
		if (ret == DONE || ret > 0) {
			return ret;
		}
	}

	auto ret = Server_resolvePath(_config->requests, path);
	if (ret != _config->requests.end() && (ret->second.callback || ret->second.map)) {
		String subPath((ret->first.back() == '/')?path.substr(ret->first.size() - 1):"");
		String originPath = subPath.size() == 0 ? String(path) : String(ret->first);
		if (originPath.back() == '/' && !subPath.empty()) {
			originPath.pop_back();
		}

		RequestHandler *h = nullptr;
		if (ret->second.map) {
			h = ret->second.map->onRequest(req, subPath);
		} else if (ret->second.callback) {
			h = ret->second.callback();
		}
		if (h) {
			auto role = h->getAccessRole();
			if (role != storage::AccessRoleId::Nobody) {
				req.setAccessRole(role);
			}

			int preflight = h->onRequestRecieved(req, move(originPath), move(subPath), ret->second.data);
			if (preflight > 0 || preflight == DONE) {
				ap_send_interim_response(req.request(), 1);
				return preflight;
			}

			preflight = Server_onRequestRecieved(req, *h);
			if (preflight > 0 || preflight == DONE) {
				ap_send_interim_response(req.request(), 1);
				return preflight;
			}
			req.setRequestHandler(h);
		}
	} else {
		if (path.size() > 1 && path.back() == '/') {
			auto name = toString(path.substr(0, path.size() - 1));
			auto it = _config->requests.find(name);
			if (it != _config->requests.end()) {
				return req.redirectTo(String(name));
			}
		}
	}

	auto &data = req.getParsedQueryArgs();
	auto userIp = req.getUseragentIp();
	if (data.hasValue("basic_auth")) {
		if (req.isSecureAuthAllowed()) {
			if (req.getAuthorizedUser()) {
				return req.redirectTo(String(req.getUri()));
			}
			return HTTP_UNAUTHORIZED;
		}
	}

	return OK;
}

void Server::onStorageTransaction(storage::Transaction &t) {
	_config->onStorageTransaction(t);
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

void Server::addComponentWithName(const StringView &name, ServerComponent *comp) {
	_config->components.emplace(name.str(), comp);
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
		_config->requests.emplace(path, RequestScheme{_config->currentComponent.str(), cb, d});
	}
}
void Server::addResourceHandler(const String &path, const db::Scheme &scheme) {
	if (!path.empty() && path.front() == '/') {
		_config->requests.emplace(path, RequestScheme{_config->currentComponent.str(),
			[s = &scheme] () -> RequestHandler * { return new ResourceHandler(*s, data::Value()); },
			data::Value(), &scheme});
	}
	auto it = _config->resources.find(&scheme);
	if (it == _config->resources.end()) {
		_config->resources.emplace(&scheme, ResourceScheme{path, data::Value()});
	}
}

void Server::addResourceHandler(const String &path, const storage::Scheme &scheme, const data::Value &val) {
	if (!path.empty() && path.front() == '/') {
		_config->requests.emplace(path, RequestScheme{_config->currentComponent.str(),
			[s = &scheme, val] () -> RequestHandler * { return new ResourceHandler(*s, val); },
			data::Value(), &scheme});
	}
	auto it = _config->resources.find(&scheme);
	if (it == _config->resources.end()) {
		_config->resources.emplace(&scheme, ResourceScheme{path, val});
	}
}

void Server::addMultiResourceHandler(const String &path, std::initializer_list<Pair<const String, const storage::Scheme *>> &&schemes) {
	if (!path.empty() && path.front() == '/') {
		_config->requests.emplace(path, RequestScheme{_config->currentComponent.str(),
			[s = Map<String, const db::Scheme *>(move(schemes))] () -> RequestHandler * {
				return new MultiResourceHandler(s);
			}, data::Value()});
	}
}

void Server::addHandler(std::initializer_list<String> paths, const HandlerCallback &cb, const data::Value &d) {
	for (auto &it : paths) {
		if (!it.empty() && it.front() == '/') {
			_config->requests.emplace(std::move(const_cast<String &>(it)), RequestScheme{_config->currentComponent.str(), cb, d});
		}
	}
}

void Server::addHandler(const String &path, const HandlerMap *map) {
	if (!path.empty() && path.front() == '/') {
		_config->requests.emplace(path,
				RequestScheme{_config->currentComponent.str(), nullptr, mem::Value(), nullptr, map});
	}
}

void Server::addHandler(std::initializer_list<String> paths, const HandlerMap *map) {
	for (auto &it : paths) {
		if (!it.empty() && it.front() == '/') {
			_config->requests.emplace(std::move(const_cast<String &>(it)),
					RequestScheme{_config->currentComponent.str(), nullptr, mem::Value(), nullptr, map});
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

db::Scheme * Server::getMutable(const db::Scheme *s) const {
	if (!_config->childInit) {
		return const_cast<db::Scheme *>(s);
	}
	return nullptr;
}

String Server::getResourcePath(const storage::Scheme &scheme) const {
	auto it = _config->resources.find(&scheme);
	if (it != _config->resources.end()) {
		return it->second.path;
	}
	return String();
}

const Map<StringView, const storage::Scheme *> &Server::getSchemes() const {
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
		apr::pool::perform([&] {
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
		}, req->pool);
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
	if (errors.empty()) {
		return;
	}
	if (!_config->childInit) {
		for (auto &it : errors) {
			std::cout << "[Error]: " << data::EncodeFormat::Pretty << it << "\n";
		}
		return;
	}
	auto serv = req ? req->server : apr::pool::server();

	if (Server(serv)._config->loadingFalled) {
		return;
	}

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
		task.addExecuteFn([err, driver = _config->dbDriver] (const Task &task) -> bool {
			Server_ErrorReporterFlags *obj = apr::pool::get<Server_ErrorReporterFlags>("Server_ErrorReporterFlags");
			if (obj) {
				obj->isProtected = true;
			} else {
				obj = new Server_ErrorReporterFlags{true};
				apr::pool::store(obj, "Server_ErrorReporterFlags");
			}

			auto pool = apr::pool::acquire();
			auto serv = Server(mem::server());

			if (Server(serv)._config->loadingFalled) {
				return false;
			}

			auto handle = serv.openDbConnection(pool);
			if (handle.get()) {
				serv.getDbDriver()->performWithStorage(handle, [&] (const db::Adapter &storage) {
					auto serv = Server(apr::pool::server());
					if (auto t = db::Transaction::acquire(storage)) {
						t.performAsSystem([&] () -> bool {
							if (auto errScheme = serv.getErrorScheme()) {
								if (errScheme->create(storage, *err)) {
									return true;
								}
							}
							std::cout << "Fail to report error: " << *err << "\n";
							return false;
						});
						t.release();
					}
				});
				serv.closeDbConnection(handle);
			}
			return true;
		});
	});
}

db::sql::Driver::Handle Server::openDbConnection(mem::pool_t *pool) const {
	db::sql::Driver::Handle handle;
	if (_config->customDbd) {
		handle = _config->customDbd->openConnection(pool);
	} else {
		handle = Root::getInstance()->dbdOpen(pool, server());
	}
	return handle;
}

void Server::closeDbConnection(db::sql::Driver::Handle handle) const {
	if (_config->customDbd) {
		_config->customDbd->closeConnection(handle);
	} else {
		Root::getInstance()->dbdClose(server(), handle);
	}
}

NS_SA_END
