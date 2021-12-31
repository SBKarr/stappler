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

#include "STServer.h"
#include "STServerComponent.h"
#include "STRoot.h"
#include "STStorageInterface.h"
#include "STMemory.h"
#include "STVirtualFile.h"
#include "STTask.h"
#include "STPqHandle.h"

#include "SPJsonWebToken.h"
#include "SPugCache.h"

namespace stellator {

struct ServerComponentLoaderList {
	static constexpr size_t Max = 16;

	void addLoader(const mem::StringView &v, ServerComponent::Symbol s) {
		if (count < Max) {
			list[count] = pair(v, s);
			++ count;
		} else {
			std::cout << "[Server]: server component loader list overflow\n";
		}
	}

	ServerComponent::Symbol getLoader(const mem::StringView &v) {
		for (size_t i = 0; i < count; ++ i) {
			if (list[i].first == v) {
				return list[i].second;
			}
		}
		return nullptr;
	}

	std::array<stappler::Pair<mem::StringView, ServerComponent::Symbol>, Max> list;
	size_t count = 0;
};

static ServerComponentLoaderList s_componentLoaders;

ServerComponent::Loader::Loader(const mem::StringView &str, Symbol s) : name(str), loader(s) {
	s_componentLoaders.addLoader(str, s);
}

static constexpr auto SA_SERVER_FILE_SCHEME_NAME = "__files";
static constexpr auto SA_SERVER_USER_SCHEME_NAME = "__users";
static constexpr auto SA_SERVER_ERROR_SCHEME_NAME = "__error";

struct DbConnection : public mem::AllocBase {
	db::sql::Driver::Handle handle;
	DbConnection *next = nullptr;
};

struct DbConnList : public mem::AllocBase {
	struct Config {
		int nmin = 1;
		int nkeep = 2;
		int nmax = 10;
		int64_t exptime = 300;
		bool persistent = true;
	};

	mem::pool_t *pool = nullptr;
	mem::Vector<DbConnection> array;

	DbConnection *opened = nullptr;
	DbConnection *free = nullptr;

	Config _config;
	size_t count = 0;
	std::mutex mutex;

	db::sql::Driver *driver = nullptr;
	mem::Map<mem::StringView, mem::StringView> params;

	DbConnList(Root *, size_t nWorkers, const mem::Value &db);
	~DbConnList();

	db::sql::Driver::Handle open();
	void close(db::pq::Driver::Handle h);
};

struct Server::Config : public mem::AllocBase {
	struct ComponentScheme {
		mem::String name;
		mem::Value data;
		ServerComponent::Symbol symbol;
	};

	Config(Root *, size_t nWorkers, const mem::Value &config);

	void setSessionParams(mem::Value &val);
	void setForceHttps();

	void init(Server &serv);
	bool initKeyPair(Server &serv, const db::Adapter &a, mem::BytesView fp);
	void initServerKeys(Server &serv, const db::Adapter &a);
	void onChildInit(Server &serv);
	void onStorageTransaction(db::Transaction &t);
	void onError(const mem::StringView &str);

	void setDocumentRoot(const mem::StringView &);
	void addComponent(const mem::Value &);

	db::pq::Driver::Handle openDb();
	void closeDb(db::pq::Driver::Handle);

	db::sql::Driver *getDbDriver() const;
	mem::StringView getDbName() const;

	mem::pool_t *pool = nullptr;
	DbConnList dbConnList;

	mem::String name;
	mem::Vector<mem::String> aliases;

	uint16_t port = 8080;
	mem::String scheme;

	mem::String servAdmin;

	mem::TimeInterval timeout;
	mem::TimeInterval keepAlive;

	bool keepAliveEnabled = false;
	size_t keepAliveMax = 0;

	mem::String documentRoot;

	bool childInit = false;

	mem::Vector<ComponentScheme> componentLoaders;

	mem::StringView currentComponent;
	mem::Vector<mem::Function<int(Request &)>> preRequest;
	mem::Map<mem::String, ServerComponent *> components;
	mem::Map<std::type_index, ServerComponent *> typedComponents;
	mem::Map<mem::String, RequestScheme> requests;
	mem::Map<const db::Scheme *, ResourceScheme> resources;
	mem::Map<mem::StringView, const db::Scheme *> schemes;

	mem::Map<mem::String, websocket::Manager *> websockets;

	mem::String sessionName = stellator::config::getDefaultSessionName();
	mem::String sessionKey = stellator::config::getDefaultSessionKey();
	mem::TimeInterval sessionMaxAge = mem::TimeInterval::seconds(0);
	bool isSessionSecure = false;
	bool loadingFalled = false;
	bool forceHttps = false;

	mem::Time lastDatabaseCleanup;
	mem::Time lastTemplateUpdate;
	int64_t broadcastId = 0;
	pug::Cache pugCache;

	mem::String webHookUrl;
	mem::String webHookName;
	mem::String webHookFormat;
	mem::Value webHookExtra;

	CompressionConfig compression;
	mem::Set<mem::String> protectedList;

	mem::String publicSessionKey;
	mem::String privateSessionKey;
	stappler::string::Sha512::Buf serverKey;

	db::Scheme userScheme = db::Scheme(SA_SERVER_USER_SCHEME_NAME, {
		db::Field::Text("name", db::Transform::Alias, db::Flags::Required),
		db::Field::Bytes("pubkey", db::Transform::PublicKey, db::Flags::Indexed),
		db::Field::Password("password", db::PasswordSalt(stellator::config::getDefaultPasswordSalt()), db::Flags::Required | db::Flags::Protected),
		db::Field::Boolean("isAdmin", mem::Value(false)),
		db::Field::Extra("data", mem::Vector<db::Field>({
			db::Field::Text("email", db::Transform::Email),
			db::Field::Text("public"),
			db::Field::Text("desc"),
		})),
		db::Field::Text("email", db::Transform::Email, db::Flags::Unique),
	});

	db::Scheme fileScheme = db::Scheme(SA_SERVER_FILE_SCHEME_NAME, {
		db::Field::Text("location", db::Transform::Url),
		db::Field::Text("type", db::Flags::ReadOnly),
		db::Field::Integer("size", db::Flags::ReadOnly),
		db::Field::Integer("mtime", db::Flags::AutoMTime | db::Flags::ReadOnly),
		db::Field::Extra("image", mem::Vector<db::Field>{
			db::Field::Integer("width"),
			db::Field::Integer("height"),
		})
	});

	db::Scheme errorScheme = db::Scheme(SA_SERVER_ERROR_SCHEME_NAME, {
		db::Field::Boolean("hidden", mem::Value(false)),
		db::Field::Boolean("delivered", mem::Value(false)),
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

	mem::Vector<mem::Pair<uint32_t, db::Interface::StorageType>> storageTypes;
	mem::Vector<mem::Pair<uint32_t, mem::String>> customTypes;
};

DbConnList::DbConnList(Root *root, size_t nWorkers, const mem::Value &db)
: pool(mem::pool::acquire()) {
	array.resize(nWorkers);

	memset((void *)array.data(), 0, sizeof(DbConnection) * array.size());

	DbConnection *target = nullptr;
	for (auto &it : array) {
		it.next = target;
		target = &it;
	}
	free = target;

	for (auto &it : db.asDict()) {
		if (it.first == "nmin") {
			if (int v = it.second.getInteger()) {
				_config.nmin = stappler::math::clamp(v, 1, config::getMaxDatabaseConnections());
			} else {
				std::cout << "[DbdModule] invalid value for nmin: " << it.second << "\n";
			}
		} else if (it.first == "nkeep") {
			if (int v = it.second.getInteger()) {
				_config.nkeep = stappler::math::clamp(v, 1, config::getMaxDatabaseConnections());
			} else {
				std::cout << "[DbdModule] invalid value for nkeep: " << it.second << "\n";
			}
		} else if (it.first == "nmax") {
			if (int v = it.second.getInteger()) {
				_config.nmax = stappler::math::clamp(v, 1, config::getMaxDatabaseConnections());
			} else {
				std::cout << "[DbdModule] invalid value for nmax: " << it.second << "\n";
			}
		} else if (it.first == "exptime") {
			if (auto v = it.second.getInteger()) {
				_config.exptime = v;
			} else {
				std::cout << "[DbdModule] invalid value for exptime: " << it.second << "\n";
			}
		} else if (it.first == "persistent") {
			if (it.second == "1" || it.second == "yes") {
				_config.persistent = true;
			} else if (it.second == "0" || it.second == "no") {
				_config.persistent = false;
			} else {
				std::cout << "[DbdModule] invalid value for persistent: " << it.second << "\n";
			}
		} else if (it.first == "driver") {
			driver = root->getDbDriver(it.second.getString());
		} else {
			params.emplace(mem::StringView(it.first).pdup(), mem::StringView(it.second.getString()).pdup());
		}
	}

	if (!driver) {
		driver = root->getRootDbDriver();
	}
}

DbConnList::~DbConnList() { }

db::sql::Driver::Handle DbConnList::open() {
	db::pq::Driver::Handle ret(nullptr);
	mutex.lock();
	while (opened) {
		auto tmpOpened = opened;
		ret = opened->handle;
		opened->handle = db::pq::Driver::Handle(nullptr);
		opened = opened->next;

		tmpOpened->next = free;
		free = tmpOpened;

		auto conn = driver->getConnection(ret);
		if (driver->isValid(conn)) {
			break;
		} else {
			driver->finish(ret);
			ret = db::pq::Driver::Handle(nullptr);
		}
	}
	mutex.unlock();

	if (ret.get()) {
		return ret;
	} else {
		db::sql::Driver::Handle ret;
		mem::perform([&] {
			ret = driver->connect(params);
		}, pool);
		if (ret.get()) {
			auto key = mem::toString("pq", uintptr_t(ret.get()));
			mem::pool::store(ret.get(), key, [d = driver, ret] () {
				d->finish(ret);
			});
		}
		return ret;
	}
}

void DbConnList::close(db::pq::Driver::Handle h) {
	if (h.get()) {
		auto key = mem::toString("pq", uintptr_t(h.get()));
		mem::pool::store(h.get(), key, nullptr);
	}

	auto conn = driver->getConnection(h);
	bool valid = driver->isValid(conn) && driver->isIdle(conn);
	if (!valid) {
		mem::perform([&] {
			driver->finish(h);
		}, pool);
	} else {
		mutex.lock();
		if (free) {
			auto tmpfree = free;
			free->handle = h;
			free = free->next;

			tmpfree->next = opened;
			opened = tmpfree;
			mutex.unlock();
		} else {
			mutex.unlock();
			mem::perform([&] {
				driver->finish(h);
			}, pool);
		}
	}
}

Server::Config::Config(Root *root, size_t nWorkers, const mem::Value &config)
: pool(mem::pool::acquire())
, dbConnList(root, nWorkers, config.getValue("db"))
#if DEBUG
, pugCache(pug::Template::Options::getPretty(), [this] (const mem::StringView &str) { onError(str); })
#else
, pugCache(pug::Template::Options::getDefault(), [this] (const mem::StringView &str) { onError(str); })
#endif
{
	for (auto &it : config.asDict()) {
		if (it.first == "name") {
			name = it.second.getString();
		} else if (it.first == "admin") {
			servAdmin = it.second.getString();
		} else if (it.first == "root") {
			setDocumentRoot(it.second.getString());
		} else if (it.first == "components" && it.second.isArray()) {
			for (auto &iit : it.second.asArray()) {
				addComponent(iit);
			}
		}
	}

	// add virtual files to template engine
	size_t count = 0;
	auto d = tools::VirtualFile::getList(count);
	for (size_t i = 0; i < count; ++ i) {
		if (d[i].name.ends_with(".pug") || d[i].name.ends_with(".spug")) {
			pugCache.addTemplate(mem::toString("virtual:/", d[i].name), d[i].content.str<mem::Interface>());
		} else {
			pugCache.addContent(mem::toString("virtual:/", d[i].name), d[i].content.str<mem::Interface>());
		}
	}

	registerCleanupDestructor(this, mem::pool::acquire());
}

void Server::Config::setSessionParams(mem::Value &val) {
	sessionName = val.getString("name");
	sessionKey = val.getString("key");
	sessionMaxAge = stappler::TimeInterval::seconds(val.getInteger("maxage"));
	isSessionSecure = val.getBool("secure");
}

void Server::Config::setForceHttps() {
	forceHttps = true;
}

void Server::Config::init(Server &serv) {
	schemes.emplace(userScheme.getName(), &userScheme);
	schemes.emplace(fileScheme.getName(), &fileScheme);
	schemes.emplace(errorScheme.getName(), &errorScheme);

	for (auto &it : componentLoaders) {
		if (it.symbol) {
			if (auto c = it.symbol(serv, it.name, it.data)) {
				components.emplace(c->getName().str<mem::Interface>(), c);
				typedComponents.emplace(std::type_index(typeid(*c)), c);
			}
		}
	}
}

bool Server::Config::initKeyPair(Server &serv, const db::Adapter &a, mem::BytesView fp) {
	stappler::crypto::PrivateKey pk;
	if (pk.generate()) {
		auto priv = pk.exportPem();
		auto pub = pk.exportPublic().exportPem();

		privateSessionKey = mem::StringView((const char *)priv.data(), priv.size()).str();
		publicSessionKey = mem::StringView((const char *)pub.data(), pub.size()).str();
	}

    auto tok = stappler::AesToken::create(stappler::AesToken::Keys{ mem::StringView(), mem::StringView(config::INTERNAL_PRIVATE_KEY), mem::BytesView(serverKey) });
	tok.setString(privateSessionKey, "priv");
	tok.setString(publicSessionKey, "pub");
	if (auto d = tok.exportData(fp)) {
		std::array<uint8_t, stappler::string::Sha512::Length + 4> data;
		memcpy(data.data(), "srv:", 4);
		memcpy(data.data() + 4, fp.data(), fp.size());

		a.set(data, d, mem::TimeInterval::seconds(60 * 60 * 365 * 100)); // 100 years
		return true;
	}
    return false;
}

void Server::Config::initServerKeys(Server &serv, const db::Adapter &a) {
	if (!privateSessionKey.empty() || !publicSessionKey.empty()) {
		return;
	}

	auto fp = stappler::string::Sha512::hmac(serv.getServerHostname(), serverKey);

	std::array<uint8_t, stappler::string::Sha512::Length + 4> data;
	memcpy(data.data(), "srv:", 4);
	memcpy(data.data() + 4, fp.data(), fp.size());

	if (auto d = a.get(data)) {
		if (auto tok = stappler::AesToken::parse(d, mem::BytesView(fp), stappler::AesToken::Keys{
				mem::StringView(), mem::StringView(config::INTERNAL_PRIVATE_KEY), mem::BytesView(serverKey) })) {
			privateSessionKey = tok.getString("priv");
			publicSessionKey = tok.getString("pub");
		}
	}

	if (privateSessionKey.empty()) {
		initKeyPair(serv, a, fp);
	}
}

void Server::Config::onChildInit(Server &serv) {
	for (auto &it : components) {
		currentComponent = it.second->getName();
		it.second->onChildInit(serv);
		currentComponent = mem::StringView();
	}

	childInit = true;

	if (!loadingFalled) {
		if (!db::Scheme::initSchemes(schemes)) {
			loadingFalled = true;
			return;
		}

		mem::perform([&] {
			auto db = dbConnList.open();
			if (!db.empty()) {
				dbConnList.driver->init(db, mem::Vector<mem::StringView>());
				dbConnList.driver->performWithStorage(db, [&] (const db::Adapter &storage) {
					storage.init(db::Interface::Config{serv.getServerHostname(), serv.getFileScheme()}, schemes);

					if (serverKey != stappler::string::Sha512::Buf{0}) {
						initServerKeys(serv, storage);
					}

					for (auto &it : components) {
						currentComponent = it.second->getName();
						it.second->onStorageInit(serv, storage);
						currentComponent = mem::String();
					}
				});
				dbConnList.close(db);
			}
		});
	}
}

void Server::Config::onStorageTransaction(db::Transaction &t) {
	for (auto &it : components) {
		it.second->onStorageTransaction(t);
	}
}

void Server::Config::onError(const mem::StringView &str) {
	messages::error("Template", "Template compilation error", mem::Value(str));
}

void Server::Config::setDocumentRoot(const mem::StringView &str) {
	documentRoot = stappler::filepath::reconstructPath(stappler::filepath::absolute(str));

	std::cout << "DocumentRoot for " << name << ": " << documentRoot << "\n";
}

void Server::Config::addComponent(const mem::Value &d) {
	mem::String name;
	mem::Value data;

	for (auto &it : d.asDict()) {
		if (it.first == "name") {
			name = it.second.getString();
		} else if (it.first == "data") {
			data = std::move(it.second);
		}
	}

	if (auto sym = s_componentLoaders.getLoader(name)) {
		componentLoaders.emplace_back(ComponentScheme{std::move(name), std::move(data), sym});
	}
}

db::pq::Driver::Handle Server::Config::openDb() {
	return dbConnList.open();
}
void Server::Config::closeDb(db::pq::Driver::Handle h) {
	dbConnList.close(h);
}

db::sql::Driver *Server::Config::getDbDriver() const {
	return dbConnList.driver;
}

mem::StringView Server::Config::getDbName() const {
	auto it = dbConnList.params.find(mem::StringView("dbname"));
	if (it != dbConnList.params.end()) {
		return it->second;
	}
	return mem::StringView();
}

Server::Server() : _config(nullptr) { }
Server::Server(Config *cfg) : _config(cfg) { }
Server & Server::operator =(Config *c) { _config = c; return *this; }

Server::Server(Server &&s) : _config(s._config) { }
Server & Server::operator =(Server &&s) { _config = s._config; return *this; }

Server::Server(const Server &s) : _config(s._config) { }
Server & Server::operator =(const Server &s) { _config = s._config; return *this; }

void Server::onServerInit() {
	_config->init(*this);
}

void Server::onChildInit() {
	_config->onChildInit(*this);

	stappler::filesystem::mkdir(getDocumentRoot());
	stappler::filesystem::mkdir(stappler::filepath::merge(getDocumentRoot(), "/.serenity"));
	stappler::filesystem::mkdir(stappler::filepath::merge(getDocumentRoot(), "/uploads"));

	_config->currentComponent = mem::StringView("root");
	//tools::registerTools(config::getServerToolsPrefix(), *this);
	_config->currentComponent = mem::StringView();

	addProtectedLocation("/.serenity");
	addProtectedLocation("/uploads");

	auto cfg = _config;
	Task::perform(*this, [&] (Task &task) {
		task.addExecuteFn([cfg] (const Task &task) -> bool {
			Server(cfg).processReports();
			return true;
		});
	});
}

void Server::onHeartBeat(mem::pool_t *pool) {
	mem::pool::store(pool, _config, "Apr.Server");
	mem::perform([&] {
		auto now = mem::Time::now();
		if (!_config->loadingFalled) {

			auto db = _config->dbConnList.open();
			if (!db.empty()) {
				_config->dbConnList.driver->performWithStorage(db, [&] (const db::Adapter &storage) {
					if (now - _config->lastDatabaseCleanup > config::getDefaultDatabaseCleanupInterval()) {
						_config->lastDatabaseCleanup = now;
						storage.makeSessionsCleanup();
					}

					_config->broadcastId = storage.interface()->processBroadcasts([&] (mem::BytesView bytes) {
						onBroadcast(bytes);
					}, _config->broadcastId);

					for (auto &it : _config->components) {
						it.second->onHeartbeat(*this);
					}
				});
				_config->dbConnList.close(db);
			}
		}
		if (now - _config->lastTemplateUpdate > config::getDefaultPugTemplateUpdateInterval()) {
			_config->pugCache.update(pool);
			_config->lastTemplateUpdate = now;
		}
	}, pool);
	mem::pool::store(pool, nullptr, "Apr.Server");
}

void Server::onBroadcast(const mem::Value &val) {
	if (val.getBool("system")) {
		Root::getInstance()->onBroadcast(val);
		return;
	}

	if (!val.hasValue("data")) {
		return;
	}

	if (val.getBool("message")) {
		/*mem::String url = mem::String(config::getServerToolsPrefix()) + config::getServerToolsShell();
		auto it = Server_resolvePath(_config->websockets, url);
		if (it != _config->websockets.end() && it->second) {
			it->second->receiveBroadcast(val);
		}*/
	}

	auto &url = val.getString("url");
	if (!url.empty()) {
		/*auto it = Server_resolvePath(_config->websockets, url);
		if (it != _config->websockets.end() && it->second) {
			it->second->receiveBroadcast(val.getValue("data"));
		}*/
	}
}

void Server::onBroadcast(const mem::BytesView &bytes) {
	onBroadcast(stappler::data::read<stappler::BytesView, mem::Interface>(bytes));
}

void Server::onStorageTransaction(db::Transaction &t) {
	_config->onStorageTransaction(t);
}

void Server::setForceHttps() {
	_config->setForceHttps();
}

void Server::addProtectedLocation(const mem::StringView &value) {
	_config->protectedList.emplace(value.str<mem::Interface>());
}

const mem::Map<mem::String, ServerComponent *> &Server::getComponents() const {
	return _config->components;
}

int Server::onRequest(Request &) { return DECLINED; }
void Server::addPreRequest(mem::Function<int(Request &)> &&) { }

void Server::addHandler(const mem::String &, const HandlerCallback &, const mem::Value &) { }
void Server::addHandler(std::initializer_list<mem::String>, const HandlerCallback &, const mem::Value &) { }
void Server::addHandler(const mem::String &, const HandlerMap *) { }
void Server::addHandler(std::initializer_list<mem::String>, const HandlerMap *) { }

void Server::addResourceHandler(const mem::String &, const db::Scheme &) { }
void Server::addResourceHandler(const mem::String &, const db::Scheme &, const mem::Value &) { }
void Server::addMultiResourceHandler(const mem::String &, std::initializer_list<stappler::Pair<const mem::String, const db::Scheme *>> &&) { }

void Server::addWebsocket(const mem::String &, websocket::Manager *) { }

const db::Scheme * Server::exportScheme(const db::Scheme &scheme) {
	_config->schemes.emplace(scheme.getName(), &scheme);
	return &scheme;
}

const db::Scheme * Server::getScheme(const mem::StringView &name) const {
	auto it = _config->schemes.find(name);
	if (it != _config->schemes.end()) {
		return it->second;
	}
	return nullptr;
}

const db::Scheme * Server::getFileScheme() const {
	return &_config->fileScheme;
}

const db::Scheme * Server::getUserScheme() const {
	return &_config->userScheme;
}

const db::Scheme * Server::getErrorScheme() const {
	return &_config->errorScheme;
}

const db::Scheme * Server::defineUserScheme(std::initializer_list<db::Field> il) {
	_config->userScheme.define(il);
	return &_config->userScheme;
}

db::Scheme * Server::getMutable(const db::Scheme *scheme) const {
	if (!_config->childInit) {
		return const_cast<db::Scheme *>(scheme);
	}
	return nullptr;
}

bool Server::performTask(Task *task, bool performFirst) const {
	return Root::getInstance()->performTask(*this, task, performFirst);
}

bool Server::scheduleTask(Task *task, mem::TimeInterval t) const {
	return Root::getInstance()->scheduleTask(*this, task, t);
}

mem::StringView Server::getDefaultName() const {
	return _config->name;
}

mem::StringView Server::getServerScheme() const {
	return _config->scheme;
}

mem::StringView Server::getServerAdmin() const {
	return _config->servAdmin;
}

mem::StringView Server::getServerHostname() const {
	return _config->name;
}

mem::StringView Server::getDocumentRoot() const {
	return _config->documentRoot;
}

uint16_t Server::getServerPort() const {
	return _config->port;
}

mem::StringView Server::getSessionKey() const {
	return _config->sessionKey;
}
mem::StringView Server::getSessionName() const {
	return _config->sessionName;
}
mem::TimeInterval Server::getSessionMaxAge() const {
	return _config->sessionMaxAge;
}
bool Server::isSessionSecure() const {
	return _config->isSessionSecure;
}

mem::pool_t *Server::getThreadPool() const {
	return _config->pool;
}

mem::pool_t *Server::getProcessPool() const {
	return _config->pool;
}

stappler::pug::Cache *Server::getPugCache() const {
	return &_config->pugCache;
}

void Server::processReports() {

}


void Server::performWithStorage(const mem::Callback<void(const db::Transaction &)> &cb, bool openNewConnecton) const {
	if (!openNewConnecton) {
		if (auto t = db::Transaction::acquireIfExists()) {
			cb(t);
			return;
		}
	}

	auto targetPool = mem::pool::acquire();
	mem::perform([&] {
		db::sql::Driver::Handle handle = _config->dbConnList.open();
		if (handle.get()) {
			_config->dbConnList.driver->performWithStorage(handle, [&] (const db::Adapter &a) {
				if (auto t = db::Transaction::acquire(a)) {
					cb(t);
					t.release();
				}
			});
			_config->dbConnList.close(handle);
		}
	}, targetPool);
}

void Server::setSessionKeys(mem::StringView pub, mem::StringView priv) const {
	_config->publicSessionKey = pub.str<mem::Interface>();
	_config->privateSessionKey = priv.str<mem::Interface>();
}

mem::StringView Server::getSessionPublicKey() const {
	return _config->publicSessionKey;
}

mem::StringView Server::getSessionPrivateKey() const {
	return _config->privateSessionKey;
}

mem::BytesView Server::getServerSecret() const {
	return _config->serverKey;
}

Server Server::next() const {
	return Root::getInstance()->getNextServer(*this);
}

ServerComponent *Server::getServerComponent(const mem::StringView &name) const {
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

const mem::Map<mem::StringView, const db::Scheme *> &Server::getSchemes() const {
	return _config->schemes;
}

const mem::Map<const db::Scheme *, Server::ResourceScheme> &Server::getResources() const {
	return _config->resources;
}

const mem::Map<mem::String, Server::RequestScheme> &Server::getRequestHandlers() const {
	return _config->requests;
}

void Server::addComponentWithName(const mem::StringView &name, ServerComponent *comp) {
	_config->components.emplace(name.str(), comp);
	_config->typedComponents.emplace(std::type_index(typeid(*comp)), comp);
	if (_config->childInit) {
		comp->onChildInit(*this);
	}
}

}
