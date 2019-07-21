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

#include "SPugCache.h"

namespace stellator {

static constexpr auto SA_SERVER_FILE_SCHEME_NAME = "__files";
static constexpr auto SA_SERVER_USER_SCHEME_NAME = "__users";
static constexpr auto SA_SERVER_ERROR_SCHEME_NAME = "__error";

struct Server::Config : public mem::AllocBase {
	Config(const mem::Value &config);

	void setSessionParams(mem::Value &val);
	void setForceHttps();

	void init(Server &serv);

	void onChildInit(Server &serv);
	void onStorageTransaction(db::Transaction &t);
	void onError(const mem::StringView &str);

	mem::String name;
	mem::Vector<mem::String> aliases;

	uint16_t port = 8080;
	mem::String scheme;

	mem::String servAdmin;
	mem::String servHostname;

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
	mem::Map<mem::String, const db::Scheme *> schemes;

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
	pug::Cache _pugCache;

	mem::String webHookUrl;
	mem::String webHookName;
	mem::String webHookFormat;
	mem::Value webHookExtra;

	CompressionConfig compression;
	mem::Set<mem::String> protectedList;

	mem::String publicSessionKey;
	mem::String privateSessionKey;

	db::Scheme userScheme = db::Scheme(SA_SERVER_USER_SCHEME_NAME, {
		db::Field::Text("name", db::Transform::Alias, db::Flags::Required),
		db::Field::Password("password", db::PasswordSalt(stellator::config::getDefaultPasswordSalt()), db::Flags::Required | db::Flags::Protected),
		db::Field::Boolean("isAdmin", mem::Value(false)),
		db::Field::Extra("data", mem::Vector<db::Field>{
			db::Field::Text("email", db::Transform::Email),
			db::Field::Text("public"),
			db::Field::Text("desc"),
		}),
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
	});
};



Server::Config::Config(const mem::Value &config)
#if DEBUG
: _pugCache(pug::Template::Options::getPretty(), [this] (const mem::StringView &str) { onError(str); })
#else
: _pugCache(pug::Template::Options::getDefault(), [this] (const mem::StringView &str) { onError(str); })
#endif
{
	// add virtual files to template engine
	size_t count = 0;
	auto d = tools::VirtualFile::getList(count);
	for (size_t i = 0; i < count; ++ i) {
		if (d[i].name.ends_with(".pug") || d[i].name.ends_with(".spug")) {
			_pugCache.addTemplate(mem::toString("virtual:/", d[i].name), d[i].content.str<mem::Interface>());
		} else {
			_pugCache.addContent(mem::toString("virtual:/", d[i].name), d[i].content.str<mem::Interface>());
		}
	}
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
	schemes.emplace(userScheme.getName().str<mem::Interface>(), &userScheme);
	schemes.emplace(fileScheme.getName().str<mem::Interface>(), &fileScheme);
	schemes.emplace(errorScheme.getName().str<mem::Interface>(), &errorScheme);

	for (auto &it : componentLoaders) {
		if (it.callback) {
			if (auto c = it.callback(serv, it.name, it.data)) {
				components.emplace(c->getName().str<mem::Interface>(), c);
				typedComponents.emplace(std::type_index(typeid(*c)), c);
			}
		}
	}
}

void Server::Config::onChildInit(Server &serv) {
	childInit = true;
	for (auto &it : components) {
		currentComponent = it.second->getName();
		it.second->onChildInit(serv);
		currentComponent = mem::StringView();
	}

	if (!loadingFalled) {
		db::Scheme::initSchemes(schemes);

		mem::perform([&] {
			auto root = Root::getInstance();
			auto pool = getCurrentPool();
			if (auto db = root->dbdOpen(pool, serv)) {
				db.init(db::Interface::Config{serv.getServerHostname().str<mem::Interface>(), serv.getFileScheme()}, schemes);

				for (auto &it : components) {
					currentComponent = it.second->getName();
					it.second->onStorageInit(serv, db);
					currentComponent = mem::String();
				}
				root->dbdClose(serv, db);
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
#if DEBUG
	std::cout << str << "\n";
#endif
	messages::error("Template", "Template compilation error", mem::Value(str));
}

}
