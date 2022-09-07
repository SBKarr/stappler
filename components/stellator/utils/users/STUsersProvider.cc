/**
 Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>

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

#include "STUsersProvider.h"
#include "STUsersComponent.h"
#include "Networking.h"

NS_SA_BEGIN

bool DiscoveryDocument::init(const data::Value &discoveryDoc) {
	if (discoveryDoc.isDictionary()) {
		issuer = discoveryDoc.getString("issuer");
		authorizationEndpoint = discoveryDoc.getString("authorization_endpoint");
		tokenEndpoint = discoveryDoc.getString("token_endpoint");
		jwksEndpoint = discoveryDoc.getString("jwks_uri");

		network::Handle h(network::Handle::Method::Get, jwksEndpoint);
		auto jwsDoc = h.performDataQuery();
		auto &keysArr = jwsDoc.getValue("keys");
		if (keysArr.isArray()) {
			for (auto &it : keysArr.asArray()) {
				JsonWebToken::SigAlg alg = JsonWebToken::SigAlg::RS256;
				String kid;
				Bytes n, e;
				for (auto &k_it : it.asDict()) {
					if (k_it.first == "alg") {
						alg = JsonWebToken::getAlg(k_it.second.getString());
					} else if (k_it.first == "n") {
						n = base64::decode(k_it.second.asString());
					} else if (k_it.first == "e") {
						e = base64::decode(k_it.second.asString());
					} else if (k_it.first == "kid") {
						kid = k_it.second.asString();
					}
				}

				if (!kid.empty()) {
					KeyData key(alg, n, e);
					keys.emplace(move(kid), move(key));
				}
			}
		}

		return true;
	}

	return false;
}

const String &DiscoveryDocument::getIssuer() const {
	return issuer;
}
const String &DiscoveryDocument::getTokenEndpoint() const {
	return tokenEndpoint;
}
const String &DiscoveryDocument::getAuthorizationEndpoint() const {
	return authorizationEndpoint;
}

const DiscoveryDocument::KeyData *DiscoveryDocument::getKey(const StringView &k) const {
	auto it = keys.find(k);
	if (it != keys.end()) {
		return &it->second;
	}
	return nullptr;
}

Provider *Provider::create(const StringView &name, const data::Value &data) {
	auto &type = data.getString("type");
	if (type == "oidc") {
		return new OidcProvider(name, data);
	} else if (type == "vk") {
		return new VkProvider(name, data);
	} else if (type == "facebook") {
		return new FacebookProvider(name, data);
	} else if (type == "local") {
		return new LocalProvider(name, data);
	}
	return nullptr;
}

void Provider::update(Mutex &) { }

const data::Value &Provider::getConfig() const {
	return _config;
}

data::Value Provider::makeUserPatch(const JsonWebToken &token) const {
	data::Value patch;
	for (auto &it : token.payload.asDict()) {
		if (it.first == "email") {
			if (it.second.isString()) {
				patch.setValue(move(it.second), it.first);
			}
		} else if (it.first == "family_name" || it.first == "familyName") {
			if (it.second.isString()) {
				patch.setValue(move(it.second), "familyName");
			}
		} else if (it.first == "given_name" || it.first == "givenName") {
			if (it.second.isString()) {
				patch.setValue(move(it.second), "givenName");
			}
		} else if (it.first == "name" || it.first == "fullName") {
			if (it.second.isString()) {
				patch.setValue(move(it.second), "fullName");
			}
		} else if (it.first == "picture") {
			if (it.second.isString()) {
				patch.setValue(move(it.second), "picture");
			}
		} else if (it.first == "locale") {
			if (it.second.isString()) {
				patch.setValue(move(it.second), "locale");
			}
		}
	}
	return patch;
}

Provider::Provider(const StringView &name, const data::Value &data) : _name(name.str()) {
	for (auto &it : data.asDict()) {
		if (it.first == "id") {
			_clientId = it.second.getString();
		} else if (it.first == "secret") {
			_clientSecret = it.second.getString();
		} else if (it.first == "type") {
			_type = it.second.getString();
		}
	}

	_config = data;
}

data::Value Provider::authUser(const Request &rctx, const StringView &sub, data::Value &&userPatch) const {
	auto storage = storage::Transaction::acquire(rctx.storage());
	auto serv = rctx.server();
	auto c = serv.getComponent<UsersComponent>();
	auto providerScheme = &c->getProvidersScheme();
	auto userScheme = &c->getExternalUserScheme();

	if (!storage || !providerScheme || !userScheme) {
		Request(rctx).setStatus(HTTP_INTERNAL_SERVER_ERROR);
		return data::Value();
	}

	int status = 0;
	int64_t currentUser = 0;
	if (auto session = ExternalSession::get()) {
		currentUser = session->getUser();
	}

	if (auto ret = initUser(currentUser, storage, sub, move(userPatch), &status)) {
		return ret;
	} else {
		if (status) {
			Request(rctx).setStatus(status);
		}
		return data::Value();
	}
}

String Provider::getUserId(const data::Value &data) const {
	return toString(data.getInteger("__oid"));
}

data::Value Provider::initUser(int64_t currentUser, const storage::Transaction &t, data::Value &&userPatch) const {
	return initUser(currentUser, t, getUserId(userPatch), std::move(userPatch));
}

data::Value Provider::initUser(int64_t currentUser, const storage::Transaction &t, const StringView &sub, data::Value &&val, int *status) const {
	auto serv = Server(mem::server());
	auto c = serv.getComponent<UsersComponent>();
	auto &providers = c->getProvidersScheme();
	auto &externals = c->getExternalUserScheme();

	data::Value ret;
	auto success = t.performAsSystem([&] {
		auto userPatch = makeProviderPatch(move(val));
		auto objs = providers.select(t, storage::Query().select("id", sub.str()).select("provider", _name));
		if (objs.size() > 1) {
			return false;
		} else if (objs.size() == 1) {
			auto &obj = objs.getValue(0);
			obj = storage::Worker(providers, t).asSystem().update(obj, userPatch);

			if (auto user = providers.getProperty(t, obj, "user")) {
				userPatch.setInteger(obj.getInteger("__oid"), "lastProvider");
				ret = storage::Worker(externals, t).asSystem().update(user, userPatch);
				return true;
			}

			if (status) { *status = HTTP_FORBIDDEN; }
		} else {
			// register user
			auto emailUsers = externals.select(t, storage::Query().select("email", userPatch.getString("email")));
			if (emailUsers.isArray() && emailUsers.size() > 0) {
				if (emailUsers.size() == 1) {
					auto user = emailUsers.getValue(0);
					if (currentUser == emailUsers.getValue(0).getInteger("__oid")) {
						userPatch.setInteger(user.getInteger("__oid"), "user");
						userPatch.setString(sub, "id");
						if (auto prov = providers.create(t, userPatch)) { // bind user account with new provider
							user = storage::Worker(externals, t).asSystem().update(user, data::Value({
								pair("lastProvider", data::Value(prov.getInteger("__oid")))
							}));
						}
						ret = move(user);
						return true;
					} else {
						if (status) { *status = HTTP_FORBIDDEN; }
					}
				}
			} else {
				// create new account
				if (data::Value user = storage::Worker(externals, t).asSystem().create(userPatch)) {
					userPatch.setInteger(user.getInteger("__oid"), "user");
					userPatch.setString(sub, "id");
					if (auto prov = providers.create(t, userPatch)) {
						storage::Worker(externals, t).asSystem().update(user, data::Value({
							pair("lastProvider", data::Value(prov.getInteger("__oid")))
						}));
						c->onNewUser(user);
						ret = move(user);
						return true;
					}
				}
			}
		}
		return false;
	});

	if (success) {
		return ret;
	}

	if (status && *status != HTTP_FORBIDDEN) { *status = HTTP_INTERNAL_SERVER_ERROR; }
	return data::Value();
}

data::Value Provider::makeProviderPatch(data::Value &&val) const {
	data::Value ret(move(val));
	ret.setString(_name, "provider");
	return ret;
}

bool AuthHandlerInterface::isPermitted() {
	_transaction = db::Transaction::acquire(_request.storage());
	_component = _request.server().getComponent<UsersComponent>();
	return _component && _transaction;
}

int AuthHandlerInterface::onRequest() {
	mem::Value data;
	auto result = perform(data);
	data.setBool(result, "OK");

	auto status = _request.getStatus();
	if (status >= 400) {
		return status;
	}

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

	_request.writeData(data, allowJsonP());
	return DONE;
}

bool AuthHandlerInterface::perform(data::Value &) const {
	return false;
}

User * AuthHandlerInterface::tryUserLogin(const Request &rctx, StringView name, StringView passwd) const {
	auto &userScheme = _component->getLocalUserScheme();

	if (name.empty() || passwd.empty()) {
		messages::error("Auth", "Name or password is not specified", data::Value({
			pair("Doc", data::Value("You should specify 'name' and 'passwd' variables in request"))
		}));
		return nullptr;
	}

	auto user = User::get(rctx.storage(), userScheme, name, passwd);
	if (!user) {
		messages::error("Auth", "Invalid username or password");
		return nullptr;
	}

	auto userData = user->getData();
	bool isActivated = user->getBool("isActivated");
	if (isActivated) {
		if (auto prov = _component->getProvider("local")) {
			if (auto authUser = prov->authUser(rctx, toString("local-", userData.getInteger("__oid")), move(userData))) {
				if (auto id = authUser.getInteger("__oid")) {
					user->setInteger(id, "external_id");
					return user;
				}
			}
		}
	}

	messages::error("Auth", "Fail to create session");
	return user;
}

data::Value AuthHandlerInterface::getLocalUser(int64_t u) const {
	auto &userScheme = _component->getExternalUserScheme();
	auto &localScheme = _component->getLocalUserScheme();

	if (!u) {
		return data::Value();
	}

	if (auto user = userScheme.get(_transaction, u)) {
		if (auto provs = userScheme.getProperty(_transaction, user, "providers")) {
			for (auto &it : provs.asArray()) {
				if (it.getValue("provider") == "local" && StringView(it.getString("id")).starts_with("local-")) {
					if (auto loc = localScheme.get(_transaction, StringView(it.getString("id")).sub(6).readInteger().get(0))) {
						loc.setInteger(it.getInteger("__oid"), "provider");
						return loc;
					}
				}
			}
		}
	}

	return data::Value();
}

String AuthHandlerInterface::getErrorUrl(const StringView &error) const {
	return toString(_originPath, "/error?error=", error);
}

String AuthHandlerInterface::getSuccessUrl(uint64_t user) const {
	return toString(_originPath, "/success?user=", user);
}

String AuthHandlerInterface::getExternalRedirectUrl() const {
	return toString(_request.getFullHostname(), _component->getBindAuth(), "callback");
}

bool AuthHandlerInterface::initStateToken(const StringView &token, const StringView &provider, bool isNew, StringView target) const {
	String tokenKey = StringView(token).starts_with("auth-") ? token.str() : toString("auth-", token);
	data::Value d;
	if (!isNew) {
		d = _transaction.getAdapter().get(tokenKey);
		if (!d.isDictionary()) {
			return false;
		}
	}

	d.setBool(true, "auth");
	d.setString(provider, "provider");
	d.setString(target, "target");

	_transaction.getAdapter().set(tokenKey, d, 720_sec);
	return true;
}

NS_SA_END
