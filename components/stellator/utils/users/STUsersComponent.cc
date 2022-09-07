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

#include "STUsersComponent.h"
#include "Networking.h"

NS_SA_BEGIN

String UsersComponent::makeStateToken() {
	StackBuffer<32> buf;
	size_t s = 24;
	auto ptr = buf.prepare(s);
	valid::makeRandomBytes(ptr, s);
	buf.save(ptr, s);

	auto t = Time::now().toMicroseconds();
	buf.put((const uint8_t *)&t, sizeof(int64_t));

	return base64url::encode(buf);
}

static String capitalize(StringView view) {
	String ret = view.str();
	if (!ret.empty()) {
		string::toupper_buf(ret.data(), unicode::utf8_length_data[uint8_t(ret.front())]);
	}
	return ret;
}

static int64_t getAgeFromBirthdate(StringView birthdateStr) {
	int64_t age = 0;
	sp_time_exp_t now(Time::now());
	sp_time_exp_t birthdate;
	if (birthdate.read(birthdateStr)) {
		birthdate = sp_time_exp_t(birthdate.geti()); // resolve missed components
		auto bAge = now.tm_year - birthdate.tm_year;
		if (birthdate.tm_yday > now.tm_yday) {
			-- bAge;
		}
		age = bAge;
	}
	return age;
}

data::Value UsersComponent::getFullName(const data::Value &value) {
	StringStream ret;
	if (value.isString("familyName") && value.isString("givenName")) {
		ret << capitalize(value.getString("familyName")) << " " << capitalize(value.getString("givenName"));
		if (value.isString("middleName") && !value.getString("middleName").empty()) {
			ret << " " << capitalize(value.getString("middleName"));
		}
	} else if (value.isString("firstName") && value.isString("lastName")) {
		ret << capitalize(value.getString("lastName")) << " " << capitalize(value.getString("firstName"));
		if (value.isString("middleName") && !value.getString("middleName").empty()) {
			ret << " " << capitalize(value.getString("middleName"));
		}
	}
	if (!ret.empty()) {
		return data::Value(ret.str());
	}
	return data::Value();
}

UsersComponent::UsersComponent(Server &serv, const String &name, const data::Value &dict)
: ServerComponent(serv, name.empty() ? "Stellator-Users" : name, dict) {
	exportValues(_externalUsers, _authProviders, _localUsers, _applications, _connections);

	using namespace db;

	auto trimFilter = WriteFilterFn([&] (const Scheme &, const mem::Value &patch, mem::Value &value, bool isCreate) {
		if (value.isString()) {
			auto tmp = string::trim(value.getString());
			value.setString(capitalize(tmp));
		}
		return true;
	});

	auto birthdateFilter = WriteFilterFn([&] (const Scheme &, const mem::Value &patch, mem::Value &value, bool isCreate) {
		if (value.isString()) {
			int64_t age = getAgeFromBirthdate(value.getString());
			if (age >= 0 && age < 150) {
				return true;
			}
		}
		return false;
	});

	auto hashDefault = DefaultFn([&] (const mem::Value &value) -> data::Value {
		if (value.isString("familyName") && value.isString("givenName")) {
			auto data = toString(value.getString("birthdate"), ":", value.getString("givenName"), " ", value.getString("familyName"));
			return data::Value(hash::hash32(data.data(), data.size()));
		} else if (value.isString("firstName") && value.isString("lastName")) {
			auto data = toString(value.getString("birthdate"), ":", value.getString("firstName"), " ", value.getString("lastName"));
			return data::Value(hash::hash32(data.data(), data.size()));
		}
		return data::Value();
	});

	auto fullNameDefault = DefaultFn([&] (const mem::Value &value) -> data::Value {
		return getFullName(value);
	});

	_externalUsers.define(Vector<Field>({
		Field::Text("fullName", MinLength(3), MaxLength(1_KiB), trimFilter, fullNameDefault),
		Field::Text("familyName", MinLength(1), trimFilter),
		Field::Text("givenName", MinLength(1), trimFilter),
		Field::Text("middleName", MinLength(1), trimFilter),
		Field::Text("birthdate", birthdateFilter),
		Field::Text("email", MinLength(3), Transform::Email, Flags::Unique | Flags::Indexed),
		Field::Text("phone", MinLength(1)),
		Field::Integer("hash", Flags::Indexed, hashDefault),
		Field::Text("locale", MinLength(1)),
		Field::Text("picture", Transform::Url),
		Field::Boolean("emailValidated", data::Value(false)),
		Field::Integer("ctime", Flags::AutoCTime),
		Field::Integer("mtime", Flags::AutoMTime | storage::Flags::Indexed),
		Field::Integer("role", WriteFilterFn([&] (const Scheme &, const data::Value &patch, data::Value &val, bool isCreate) -> bool {
			if (val.isString()) {
				if (val.getString() == "admin") {
					val.setInteger(toInt(AccessRoleId::Admin));
					return true;
				} else if (val.getString() == "user") {
					val.setInteger(toInt(AccessRoleId::Authorized));
					return true;
				} else if (valid::validateNumber(val.getString())) {
					return true;
				}
			} else if (val.isInteger()) {
				return true;
			}
			return false;
		})),
		Field::Integer("origin"),
		Field::Set("providers", _authProviders),
		Field::Object("lastProvider", _authProviders, storage::Flags::Reference, Flags::Protected),
		Field::FullTextView("ts", FullTextViewFn([this] (const Scheme &scheme, const data::Value &obj) -> Vector<FullTextData> {
			Vector<FullTextData> ret;
			auto &objEmail = obj.getString("email");
			auto &objName = obj.getString("fullName");

			if (!objName.empty()) {
				ret.emplace_back(FullTextData{objName, FullTextData::Language::Simple, FullTextData::B});
			}

			if (!objEmail.empty()) {
				auto it = objEmail.find("@");
				if (it != String::npos) {
					ret.emplace_back(FullTextData{String(objEmail.data(), it), FullTextData::Language::Simple, FullTextData::A});
					ret.emplace_back(FullTextData{String(objEmail.data() + it + 1), FullTextData::Language::Simple, FullTextData::C});
				}
			}

			return ret;
		}), FullTextQueryFn([this] (const data::Value &input) -> Vector<FullTextData> {
			auto str = input.getString();
			auto it = str.find("@");
			if (it != String::npos) {
				return Vector<FullTextData>({
					FullTextData{toString(String(str.data(), it), " ", String(str.data() + it + 1)), FullTextData::Language::Simple}
				});
			} else {
				return Vector<FullTextData>({ FullTextData{input.getString(), FullTextData::Language::Simple} });
			}
		}), Vector<String>{"email", "fullName"}),
	}),
	AccessRole::Empty(AccessRoleId::Nobody));


	_authProviders.define(Vector<Field>({
		Field::Text("id", Transform::Alias, Flags::Indexed),
		Field::Text("fullName", MinLength(3), MaxLength(1_KiB), trimFilter, fullNameDefault),
		Field::Text("familyName", MinLength(1), trimFilter),
		Field::Text("givenName", MinLength(1), trimFilter),
		Field::Text("middleName", MinLength(1), trimFilter),
		Field::Text("birthdate", birthdateFilter),
		Field::Text("email", MinLength(3), Transform::Email),
		Field::Text("phone", MinLength(1)),
		Field::Integer("hash", Flags::Indexed, hashDefault),
		Field::Text("locale", MinLength(1)),
		Field::Text("picture", Transform::Url),
		Field::Object("user", _externalUsers, RemovePolicy::Cascade),
		Field::Text("provider", MinLength(1), storage::Flags::Indexed),
		Field::Data("data")
	}),
	AccessRole::Empty(AccessRoleId::Nobody));

	_localUsers.define(Vector<Field>({
		Field::Boolean("isActivated"),
		Field::Boolean("isIncomplete"),
		Field::Text("name", Flags::Indexed),
		Field::Text("email", Transform::Email, Flags::Unique | Flags::Required | Flags::Indexed),
		Field::Text("phone", MinLength(1)),
		Field::Text("fullName", MinLength(3), MaxLength(1_KiB), trimFilter, fullNameDefault),
		Field::Text("familyName", MinLength(1), trimFilter),
		Field::Text("givenName", MinLength(1), trimFilter),
		Field::Text("middleName", MinLength(1), trimFilter),
		Field::Text("birthdate", birthdateFilter),
		Field::Text("picture", Transform::Url),
		Field::Password("password", MinLength(4)),
		Field::Integer("hash", Flags::Indexed, hashDefault),
		Field::Extra("data", Vector<Field>({
			Field::Bytes("code", Flags::ReadOnly),
			Field::Text("locale", MinLength(1)),
			Field::Integer("origin"),
		})),
	}),
	AccessRole::Empty(AccessRoleId::Nobody));

	_applications.define({
		Field::Text("name"),
		Field::Text("bundleId", Flags::Indexed | Flags::Unique),
		Field::Set("connections", _connections),
		Field::Data("secret", Function<data::Value()>([] () -> data::Value {
			return data::Value(base64url::encode(string::Sha512::perform(Time::now().toHttp(), valid::makeRandomBytes(16))));
		})),
		Field::Integer("ctime", Flags::AutoCTime),
		Field::Integer("mtime", Flags::AutoMTime),
	});

	_connections.define({
		Field::Object("user", _externalUsers, RemovePolicy::Cascade),
		Field::Object("app", _applications, RemovePolicy::Cascade),
		Field::Bytes("secret"),
		Field::Text("sig", Flags::Indexed), // application signature, used for connection
		Field::Integer("time"),
		Field::Integer("ctime", Flags::AutoCTime),
		Field::Integer("mtime", Flags::AutoMTime),
		Field::Extra("data", Vector<Field>({
			Field::Text("agent"),
			Field::Text("application"),
			Field::Text("version"),
			Field::Text("token"),
			Field::Bytes("code"),
		})),
	});
}

void UsersComponent::onChildInit(Server &serv) {
	bool providersInConfig = false;
	if (_config.isDictionary()) {
		for (auto &it : _config.asDict()) {
			if (it.first == "private" && it.second.isString()) {
				_privateKey = it.second.getString();
			} else if (it.first == "public" && it.second.isString()) {
				_publicKey = it.second.getString();
			} else if (it.first == "providers" && it.second.isDictionary()) {
				providersInConfig = true;
				for (auto &p_it : it.second.asDict()) {
					if (auto provider = Provider::create(p_it.first, p_it.second)) {
						_providers.emplace(p_it.first, provider);
						if (p_it.first == "local") {
							_localProvider = provider;
						}
					}
				}
			} else if (it.first == "captcha" && it.second.isDictionary()) {
				for (auto &iit : it.second.asDict()) {
					if (iit.first == "secret") {
						_captchaSecret = it.second.getString();
					} else if (it.first == "url") {
						_captchaUrl = it.second.getString();
					}
				}
			} else if (it.first == "mail" && it.second.isDictionary()) {
				_mailConfig = it.second;
			} else if (it.first == "prefix" && it.second.isString()) {
				_prefix = it.second.getString();
			}
		}
	}

	if (!_publicKey.empty() && !_privateKey.empty()) {
		serv.setSessionKeys(_publicKey, _privateKey);
	}

	if (!providersInConfig) {
		if (auto localProvider = Provider::create("local", data::Value({ pair("type", data::Value("local")) }))) {
			_providers.emplace("local", localProvider);
			_localProvider = localProvider;
		}
	}

	serv.addHandler(getBindAuth(), new AuthExternalHandlerMap);
	serv.addHandler(getBindLocal(), new AuthLocalHandlerMap);
	//serv.addHandler(toString(_prefix, "/auth/connections/"), SA_HANDLER(DeviceConnectionHandler));

	addCommand("users-create", [this] (const StringView &str) -> data::Value {
		return createUserCmd(str);
	}, "<name> <password> - creates local login pair for new user");

	addCommand("users-add-application", [this] (const StringView &str) -> data::Value {
		return addApplicationCmd(str);
	}, "<name> <bundle-id> [<secret>] - Add application for connections");

	addCommand("users-attach-local", [this] (const StringView &str) -> data::Value {
		StringView r(str);
		if (auto local = r.readInteger().get(0)) {
			if (r.is<StringView::CharGroup<CharGroupId::WhiteSpace>>()) {
				r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
				if (auto external = r.readInteger().get(0)) {
					return attachUser(local, external);
				}
			}
		}
		return data::Value(true);
	}, "<local-id> <external-id> - binds local and external users");

	addCommand("users-rebuild-index", [this] (const StringView &str) -> data::Value {
		if (auto t = storage::Transaction::acquire()) {
			auto p = mem::pool::create(mem::pool::acquire());
			mem::perform([&] {
				auto allExt = _externalUsers.select(t, storage::Query());
				for (auto &it : allExt.asArray()) {
					auto email = it.getString("email");
					auto fullName = getFullName(it);
					_externalUsers.update(t, it, data::Value({
						pair("email", data::Value(email)),
						pair("fullName", data::Value(fullName)),
					}), storage::UpdateFlags::NoReturn);
				}
			}, p);
			mem::pool::clear(p);
			mem::perform([&] {
				auto allExt = _authProviders.select(t, storage::Query());
				for (auto &it : allExt.asArray()) {
					auto email = it.getString("email");
					auto fullName = getFullName(it);
					_authProviders.update(t, it, data::Value({
						pair("email", data::Value(email)),
						pair("fullName", data::Value(fullName)),
					}), storage::UpdateFlags::NoReturn);
				}
			}, p);
			mem::pool::clear(p);
			mem::perform([&] {
				auto allExt = _localUsers.select(t, storage::Query());
				for (auto &it : allExt.asArray()) {
					auto email = it.getString("email");
					auto fullName = getFullName(it);
					_localUsers.update(t, it, data::Value({
						pair("email", data::Value(email)),
						pair("fullName", data::Value(fullName)),
					}), storage::UpdateFlags::NoReturn);
				}
			}, p);
			mem::pool::clear(p);
		}
		return data::Value(true);
	}, " - rebuild external_users full text view");

	addOutputCommand("users-search", [this] (mem::StringView cmd, const mem::Callback<void(const mem::Value &)> &cb) {
		if (auto t = db::Transaction::acquire()) {
			cmd.skipChars<StringView::WhiteSpace>();
			return findUsers(t, cmd, cb);
		}
		return true;
	}, " <email or name> - find user in db");

	addOutputCommand("users-remove", [this] (mem::StringView cmd, const mem::Callback<void(const mem::Value &)> &cb) {
		if (auto t = db::Transaction::acquire()) {
			cmd.skipChars<StringView::WhiteSpace>();
			return removeUser(t, cmd.readInteger(10).get(0), cb);
		}
		return true;
	}, " <local_id or external_id> - completely remove user");

	addOutputCommand("users-update-email", [this] (mem::StringView cmd, const mem::Callback<void(const mem::Value &)> &cb) {
		if (auto t = db::Transaction::acquire()) {
			cmd.skipChars<StringView::WhiteSpace>();
			auto id = cmd.readInteger(10).get();
			cmd.skipChars<StringView::WhiteSpace>();
			cb(updateUserEmail(t, id, cmd));
		}
		return true;
	}, " <local_id> <new_email> - update user email");

	Task::perform(_server, [&] (Task &task) {
		task.addExecuteFn([this] (const Task &task) -> bool {
			updateDiscovery();
			return true;
		});
	});
}

void UsersComponent::onStorageTransaction(storage::Transaction &t) {
	if (auto s = ExternalSession::get()) {
		t.performAsSystem([&] {
			if (auto user = _externalUsers.get(t, s->getUser())) {
				if (user.getInteger("profile") != s->getInteger("profile")) {
					s->setInteger(user.getInteger("profile"), "profile");
				}
				if (user.getInteger("patient") != s->getInteger("patient")) {
					s->setInteger(user.getInteger("patient"), "patient");
				}
			}
			return true;
		});
		if (toInt(t.getRole()) < toInt(s->getRole())) {
			t.setRole(s->getRole());
		}
	}
}

StringView UsersComponent::getPrivateKey() const {
	return _privateKey;
}

StringView UsersComponent::getPublicKey() const {
	return _publicKey;
}

SessionKeys UsersComponent::getKeyPair() const {
	return SessionKeys{_publicKey, _privateKey};
}

StringView UsersComponent::getCaptchaSecret() const {
	return _captchaSecret;
}

String UsersComponent::getBindAuth() const {
	return toString(_prefix, "/auth/external/");
}

String UsersComponent::getBindLocal() const {
	return toString(_prefix, "/auth/local/");
}

data::Value UsersComponent::getMailConfig() const {
	return _mailConfig;
}

String UsersComponent::onLogin(const Request &rctx, const StringView &providerName, const StringView &state, const StringView &redirectUrl) const {
	if (auto provider = getProvider(providerName)) {
		return provider->onLogin(rctx, state, redirectUrl);
	}
	return String();
}

String UsersComponent::onTry(const Request &rctx, const StringView &providerName, const data::Value &data, const StringView &state, const StringView &redirectUrl) const {
	if (auto provider = getProvider(providerName)) {
		return provider->onTry(rctx, data, state, redirectUrl);
	}
	return String();
}

uint64_t UsersComponent::onToken(const Request &rctx, const StringView &providerName, const StringView &token) {
	if (auto provider = getProvider(providerName)) {
		return provider->onToken(rctx, token);
	} else {
		messages::error("UserDatabase", "No provider with name", data::Value(providerName));
	}
	return 0;
}

uint64_t UsersComponent::onResult(const Request &rctx, const StringView &providerName, const StringView &redirectUrl) const {
	uint64_t ret = 0;
	if (auto provider = getProvider(providerName)) {
		ret = provider->onResult(rctx, redirectUrl);
	}
	return ret;
}

Provider *UsersComponent::getProvider(const StringView &name) const {
	auto it = _providers.find(name);
	if (it != _providers.end()) {
		return it->second;
	}
	return nullptr;
}

Provider *UsersComponent::getLocalProvider() const {
	return _localProvider;
}

int64_t UsersComponent::isValidAppRequest(Request &rctx) const {
	auto h = rctx.getRequestHeaders();

	auto spkey = h.at("X-Stappler-Sign");
	if (!spkey.empty()) {
		StringStream message;
		message << rctx.getFullHostname() << rctx.getUnparsedUri() << "\r\n";
		message << "X-ApplicationName: " << h.at("X-ApplicationName") << "\r\n";
		message << "X-ApplicationVersion: " << h.at("X-ApplicationVersion") << "\r\n";
		message << "X-ClientDate: " << h.at("X-ClientDate") << "\r\n";
		message << "User-Agent: " << h.at("User-Agent") << "\r\n";

		auto d = base64::decode(spkey);
		auto now = Time::now();
		CoderSource source(message.data(), message.size());

		auto validateKey = [&] (const StringView &key) -> bool {
			auto sig = string::Sha512::hmac(source, key);
			if (d.size() == sig.size() && memcmp(d.data(), sig.data(), sig.size()) == 0) {
				if (now - Time::fromHttp(h.at("X-ClientDate")) < 10_sec) {
					return true;
				}
			}
			return false;
		};

		data::Value apps = storage::Worker(_applications, rctx.storage()).asSystem().select(storage::Query().select("bundleId", h.at("X-ApplicationName")).include(storage::Query::Field("secret")));
		if (apps.size() == 1) {
			auto &app = apps.getValue(0);
			auto &secret = app.getValue("secret");
			if (secret.isString()) {
				if (validateKey(secret.getString())) {
					return app.getInteger("__oid");
				}
			} else if (secret.isArray()) {
				for (auto &it : secret.asArray()) {
					if (it.isString() && validateKey(it.getString())) {
						return app.getInteger("__oid");
					}
				}
			}
		}
	}

	return false;
}

data::Value UsersComponent::isValidSecret(Request &rctx, const data::Value &secret) const {
	if (!secret) {
		return data::Value();
	}

	int64_t secretConnId = 0;
	int64_t secretAppId = 0;
	Bytes secretBytes;

	if (secret.isBytes()) {
		auto secretData = data::read(secret.getBytes());
		secretConnId = secretData.getInteger("id");
		secretAppId = secretData.getInteger("app");
		secretBytes = secretData.getBytes("secret");
	} else if (secret.isString()) {
		auto secretData = data::read(base64::decode(secret.getString()));
		secretConnId = secretData.getInteger("id");
		secretAppId = secretData.getInteger("app");
		secretBytes = secretData.getBytes("secret");
	}

	if (!secretConnId || !secretAppId || secretBytes.empty()) {
		return data::Value();
	}

	auto _appId = isValidAppRequest(rctx);
	if (_appId != secretAppId) {
		return data::Value();
	}

	if (auto conn = storage::Worker(_connections, rctx.storage()).asSystem().get(secretConnId, {"app", "user", "secret", "sig"})) {
		int64_t userId = conn.getInteger("user");
		int64_t appId = conn.getInteger("app");
		if (conn.getBytes("secret") == secretBytes && _appId == appId) {
			auto userData = storage::Worker(_externalUsers, rctx.storage()).asSystem().get(userId, {"ctime"});
			auto appData = storage::Worker(_applications, rctx.storage()).asSystem().get(appId, {"secret"});

			if (!userData || !appData) {
				return data::Value();
			}

			auto &sig = conn.getString("sig");
			auto &sec = appData.getValue("secret");
			if (sec.isString()) {
				if (sig != sec.getString()) {
					return data::Value();
				}
			} else if (sec.isArray()) {
				bool found = false;
				for (auto &it : sec.asArray()) {
					if (it.getString() == sig) {
						found = true;
						break;
					}
				}

				if (!found) {
					return data::Value();
				}
			}

			return data::Value({
				pair("user", data::Value(userId)),
				pair("app", data::Value(appId)),
				pair("conn", data::Value(secretConnId)),
			});
		}
	}
	return data::Value();
}

const storage::Scheme & UsersComponent::getApplicationScheme() const {
	return _applications;
}

const storage::Scheme & UsersComponent::getExternalUserScheme() const {
	return _externalUsers;
}

const storage::Scheme & UsersComponent::getProvidersScheme() const {
	return _authProviders;
}

const storage::Scheme & UsersComponent::getLocalUserScheme() const {
	return _localUsers;
}

const storage::Scheme & UsersComponent::getConnectionScheme() const {
	return _connections;
}

data::Value UsersComponent::getExternalUserByEmail(const StringView &str) const {
	auto users = storage::Worker(_externalUsers).asSystem().select(storage::Query().select("email", str.str()));
	if (users.size() == 1) {
		return users.getValue(0);
	}
	return data::Value();
}

data::Value UsersComponent::getLocaUserByName(const StringView &str) const {
	auto users = db::Worker(_localUsers).asSystem().select(storage::Query().select("name", str.str()));
	if (users.size() == 1) {
		return users.getValue(0);
	}
	return data::Value();
}

int64_t UsersComponent::getLocalUserId(int64_t externalId) const {
	int64_t ret = 0;
	if (auto t = db::Transaction::acquireIfExists()) {
		t.performAsSystem([&] {
			auto provs = _authProviders.select(t, db::Query().select("user", data::Value(externalId)));
			for (auto &it : provs.asArray()) {
				if (it.getString("provider") == "local") {
					auto id = StringView(it.getString("id"));
					if (id.starts_with("local-")) {
						id += "local-"_len;
						if (auto u =_localUsers.get(t, id.readInteger(10).get(0))) {
							ret = u.getInteger("__oid");
						}
					}
				}
			}
			return true;
		});

	}
	return ret;
}

bool UsersComponent::verifyCaptcha(const StringView &captchaToken) const {
	network::Handle h(network::Handle::Method::Post, _captchaUrl);
	h.setSendData(toString("secret=", string::urlencode(_captchaSecret), "&response=", string::urldecode(captchaToken)));

	auto captchaData = h.performDataQuery();
	return captchaData.getBool("success");
}

void UsersComponent::updateDiscovery() {
	for (auto &it : _providers) {
		it.second->update(_mutex);
	}
}

static bool replaceString(String& str, const String& from, const String& to) {
    size_t start_pos = str.find(from);
    if (start_pos == String::npos) {
        return false;
    }
    str.replace(start_pos, from.length(), to);
    return true;
}

void UsersComponent::sendActivationEmail(StringView hostname, const data::Value &user) const {
	String registerMail = filesystem::readTextFile("private/emails/register.html");

	uint64_t id = user.getInteger("__oid");
	auto &data = user.getValue("data");
	auto &code = data.getBytes("code");
	auto activationCode = base64url::encode(data::write(data::Value({
		pair("id", data::Value(id)),
		pair("code", data::Value(code))
	}), data::EncodeFormat::Cbor));

	auto &email = user.getString("email");
	auto &givenName = user.getString("givenName");
	auto &familyName = user.getString("familyName");

	auto url = toString(hostname, getBindLocal(), "activate?code=", activationCode);

	replaceString(registerMail, "{{activationCode}}", activationCode);
	replaceString(registerMail, "{{userFullName}}", givenName);
	replaceString(registerMail, "{{activationUrl}}", url);

	StringStream msg;
	msg << "From: " << _mailConfig.getString("name") << " <" << _mailConfig.getString("from") << ">\r\n"
			<< "Content-Type: text/html; charset=utf-8\r\n"
            << "Mime-version: 1.0\r\n"
			<< "To: " << givenName << " " << familyName << " <" << email << ">\r\n"
			<< "Subject: Активация учетной записи\r\n\r\n"
			<< registerMail;

	network::Mail notify(_mailConfig);
	notify.addMailTo(email);
	notify.send(msg);
}

void UsersComponent::sendDropPasswordEmail(StringView hostname, const data::Value &user) const {
	String registerMail = filesystem::readTextFile("private/emails/dropPassword.html");

	uint64_t id = user.getInteger("__oid");
	auto &data = user.getValue("data");
	auto &code = data.getBytes("code");
	auto activationCode = base64url::encode(data::write(data::Value({
		pair("id", data::Value(id)),
		pair("code", data::Value(code))
	}), data::EncodeFormat::Cbor));

	auto &email = user.getString("email");
	auto &givenName = user.getString("givenName");
	auto &familyName = user.getString("familyName");

	auto url = toString(hostname, getBindLocal(), "reset_password?code=", activationCode);

	replaceString(registerMail, "{{activationCode}}", activationCode);
	replaceString(registerMail, "{{userFullName}}", givenName);
	replaceString(registerMail, "{{activationUrl}}", url);

	StringStream msg;
	msg << "From: " << _mailConfig.getString("name") << " <" << _mailConfig.getString("from") << ">\r\n"
			<< "Content-Type: text/html; charset=utf-8\r\n"
            << "Mime-version: 1.0\r\n"
			<< "To: " << givenName << " " << familyName << " <" << email << ">\r\n"
			<< "Subject: Восстановление пароля\r\n\r\n"
			<< registerMail;

	network::Mail notify(_mailConfig);
	notify.addMailTo(email);
	notify.send(msg);
}

bool UsersComponent::isLongTermAuthorized(Request &rctx, uint64_t uid) const {
	if (auto longSession = LongSession::acquire(rctx)) {
		return longSession->getUser() == uid;
	}

	return false;
}

bool UsersComponent::initLongTerm(const Request &rctx, uint64_t uid) const {
	if (auto longSession = LongSession::acquire(rctx)) {
		if (longSession->getUser() == uid) {
			return true;
		} else {
			longSession->assignUser(uid);
			return true;
		}
	}

	return false;
}

bool UsersComponent::isSessionAuthorized(Request &rctx, uint64_t uid) const {
	if (auto session = ExternalSession::get(rctx)) {
		return session->getUser() > 0 && session->getUser() == uid;
	}
	return false;
}

bool UsersComponent::pushAuthUser(const Request &rctx, uint64_t userId) const {
	if (auto s = ExternalSession::acquire(rctx)) {
		if (auto t = storage::Transaction::acquire(rctx.storage())) {
			return t.performAsSystem([&] () -> bool {
				if (auto user = _externalUsers.get(t, userId)) {
					if (!initLongTerm(rctx, userId)) {
						return false;
					}

					s->setUser(userId, storage::AccessRoleId( std::max(user.getInteger("role"), int64_t(toInt(storage::AccessRoleId::Authorized))) ));

					data::Value providers;
					if (auto provs = _externalUsers.getProperty(t, user, "providers")) {
						for (auto &it : provs.asArray()) {
							providers.addValue(data::Value({
								pair("__oid", data::Value(it.getInteger("__oid"))),
								pair("provider", data::Value(it.getString("provider"))),
								pair("id", data::Value(it.getString("id"))),
							}));
							if (it.getValue("provider") == "local") {
								auto &id = it.getString("id");
								if (StringView(id).starts_with("local-")) {
									if (auto loc = _localUsers.get(t, StringView(id).sub(6).readInteger().get(0))) {
										s->setInteger(loc.getInteger("__oid"), "local_user");
									}
								}
							}
						}
					}

					if (user.isInteger("profile")) {
						s->setInteger(user.getInteger("profile"), "profile");
					}
					if (user.isInteger("patient")) {
						s->setInteger(user.getInteger("patient"), "patient");
					}

					if (!providers.empty()) {
						s->setValue(move(providers), "providers");
					}
					return true;
				}
				return false;
			});
		}
	}

	// take or make user token
	return false;
}

data::Value UsersComponent::createUserCmd(const StringView &str) {
	if (auto t = storage::Transaction::acquire()) {
		StringView r(str);
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		auto email = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		auto passwd = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		String emailValue(email.str());
		if (!valid::validateEmail(emailValue)) {
			return data::Value("invalidEmail");
		}

		if (passwd.empty()) {
			return data::Value("invalidPassword");
		}

		auto name = StringView(emailValue).readUntil<StringView::Chars<'@'>>();

		data::Value patch{
			pair("isActivated", data::Value(true)),
			pair("email", data::Value(emailValue)),
			pair("name", data::Value(name)),
			pair("password", data::Value(passwd)),
		};

		data::Value ret;
		if (t.perform([&] {
			if (auto localUser = _localUsers.create(t, patch)) {
				if (auto prov = getProvider("local")) {
					int64_t currentUser = 0;
					if (auto session = ExternalSession::get()) {
						currentUser = session->getUser();
					}

					if (auto authUser = prov->initUser(currentUser, t, toString("local-", localUser.getInteger("__oid")), move(localUser))) {
						ret = _externalUsers.update(t, authUser, data::Value({ pair("role", data::Value("admin")) }));
						return true;
					}
				}
			}
			return false;
		})) {
			return ret;
		}
	}
	return data::Value("Invalid input params");
}

data::Value UsersComponent::addApplicationCmd(const StringView &str) {
	StringView r(str);
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	auto appName = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	auto appId = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	auto secret = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	if (appName.empty() || appId.empty()) {
		return data::Value({ pair("error", data::Value("Invalid application bundle id")) });
	}

	String secretData;
	if (!secret.empty()) {
		secretData = secret.str();
	} else {
		secretData = base64url::encode(string::Sha512::perform(appName, Time::now().toHttp(), valid::makeRandomBytes(16)));
	}

	if (auto storage = storage::Adapter::FromContext()) {
		return storage::Worker(_applications, storage).asSystem().create(data::Value({
			pair("name", data::Value(appName)),
			pair("bundleId", data::Value(appId)),
			pair("secret", data::Value(secretData))
		}));
	}
	return data::Value(true);
}

data::Value UsersComponent::attachUser(int64_t local, int64_t extarnal) {
	if (auto t = storage::Transaction::acquire(storage::Adapter::FromContext())) {
		auto userData = _localUsers.get(t, local);
		if (!userData) {
			return data::Value(true);
		}

		auto externalData = _externalUsers.get(t, extarnal);
		if (!externalData) {
			return data::Value(true);
		}

		if (externalData.getString("email") != userData.getString("email")) {
			return data::Value("Incompatible users");
		}

		auto provId = toString("local-", userData.getInteger("__oid"));
		auto objs =  _authProviders.select(t, storage::Query().select("id", provId).select("provider", "local"));

		if (objs.empty()) {
			// register user
			auto patch = _providers.at("local")->makeProviderPatch(move(userData));
			patch.setInteger(externalData.getInteger("__oid"), "user");
			patch.setString(provId, "id");
			if (auto prov = _authProviders.create(t, patch)) { // bind user account with new provider
				externalData = _externalUsers.update(t, externalData, data::Value({ pair("lastProvider", data::Value(prov.getInteger("__oid")))}));

				if (auto provs = _externalUsers.getProperty(t, externalData, "providers")) {
					externalData.setValue(move(provs), "providers");
				}

				return externalData;
			}
		} else {
			return data::Value("Already binded");
		}
	}
	return data::Value(true);
}

data::Value UsersComponent::updateUserEmail(const db::Transaction &t, int64_t id, StringView iemail) const {
	auto userData = _localUsers.get(t, id);
	if (!userData) {
		return data::Value("user not found");
	}

	String email; email = iemail.str();
	if (!valid::validateEmail(email)) {
		return data::Value("invalid email");
	}

	auto password = valid::generatePassword(8);
	auto ret = _localUsers.update(t, userData, data::Value({
		pair("email", data::Value(email)),
		pair("password", data::Value(password)),
	}));

	if (auto prov = getProvider("local")) {
		if (auto external = prov->initUser(0, t, data::Value(ret))) {
			return ret;
		}
	}
	return data::Value();
}

bool UsersComponent::findUsers(const db::Transaction &t, StringView cmd, const mem::Callback<void(const mem::Value &)> &cb) const {
	bool found = false;
	auto resolveUser = [&] (data::Value &it) {
		auto provs = _authProviders.select(t, db::Query().select("user", data::Value(it.getInteger("__oid"))));
		for (auto &it : provs.asArray()) {
			if (it.getString("provider") == "local") {
				auto id = StringView(it.getString("id"));
				if (id.starts_with("local-")) {
					id += "local-"_len;
					if (auto u = _localUsers.get(t, id.readInteger(10).get(0))) {
						it.setValue(move(u), "local_user");
					}
				}
			}
		}
		it.setValue(move(provs), "providers");
		cb(it);
		found = true;
	};

	String str(cmd.str());
	if (valid::validateEmail(str)) {
		auto u = _externalUsers.select(t, db::Query().select("email", data::Value(str)));
		for (auto &it : u.asArray()) {
			resolveUser(it);
		}
	} else {
		auto u = _externalUsers.select(t, db::Query().select("ts", data::Value(cmd)));
		for (auto &it : u.asArray()) {
			resolveUser(it);
		}
	}
	if (!found) {
		cb(data::Value("Not found"));
	}
	return true;
}

bool UsersComponent::removeUser(const db::Transaction &t, int64_t id, const mem::Callback<void(const mem::Value &)> &cb) const {
	auto external = _externalUsers.get(t, id);
	if (!external) {
		auto local = _localUsers.get(t, id);
		if (!local) {
			cb(data::Value("Not found"));
			return true;
		} else {
			if (auto prov = _authProviders.select(t, db::Query().select("id", data::Value(toString("local-", id)))).getValue(0)) {
				if (prov.isInteger("user")) {
					external = _externalUsers.get(t, prov.getInteger("id"));
				}
			}
		}
	}
	if (!external) {
		cb(data::Value("Not found"));
		return true;
	}

	auto provs = _authProviders.select(t, db::Query().select("user", data::Value(external.getInteger("__oid"))));
	for (auto &it : provs.asArray()) {
		if (it.getString("provider") == "local") {
			auto id = StringView(it.getString("id"));
			if (id.starts_with("local-")) {
				id += "local-"_len;
				if (auto u = _localUsers.get(t, id.readInteger(10).get(0))) {
					cb(data::Value("users_local"));
					cb(u);
					_localUsers.remove(t, u.getInteger("__oid"));
				}
			}
		}
		cb(data::Value("users_auth_providers"));
		cb(it);
		_authProviders.remove(t, it.getInteger("__oid"));
	}

	cb(data::Value("users_external"));
	cb(external);
	_externalUsers.remove(t, external.getInteger("__oid"));

	return true;
}

data::Value UsersComponent::makeConnection(Request &rctx, int64_t userId, int64_t appId, const StringView &token) {
	auto headers = rctx.getRequestHeaders();
	auto agent = headers.at("user-agent");
	auto app = headers.at("X-ApplicationName");
	auto version = headers.at("X-ApplicationVersion");

	auto userData = storage::Worker(_externalUsers, rctx.storage()).asSystem().get(userId, {"ctime", "email", "familyName", "fullName", "givenName", "locale", "picture"});
	auto appData = storage::Worker(_applications, rctx.storage()).asSystem().get(appId, {"name", "secret"});
	auto &secret = appData.getValue("secret");

	String sig;
	if (secret.isString()) {
		sig = secret.getString();
	} else if (secret.isArray() && !secret.empty()) {
		sig = secret.getString(secret.size() - 1);
	}

	if (userData && appData && !sig.empty()) {
		Bytes code = valid::makeRandomBytes(16);
		Time t = Time::now();

		StringStream data;
		data << agent << "\n" << app << "\n" << t.toHttp() << "\n" << base64url::encode(code) << "\n";

		auto secretBuf = string::Sha512::hmac(CoderSource(data.data(), data.size()), sig);

		data::Value patch({
			pair("user", data::Value(userId)),
			pair("app", data::Value(appId)),
			pair("secret", data::Value(Bytes(secretBuf.data(), secretBuf.data() + secretBuf.size()))),
			pair("sig", data::Value(sig)),
			pair("time", data::Value(t.toMicros())),
			pair("data", data::Value({
				pair("agent", data::Value(agent)),
				pair("application", data::Value(app)),
				pair("version", data::Value(version)),
				pair("token", data::Value(token)),
				pair("code", data::Value(code)),
			}))
		});

		if (auto ret = storage::Worker(_connections, rctx.storage()).asSystem().create(patch)) {
			auto secret = data::write(data::Value({
				pair("id", ret.getValue("__oid")),
				pair("app", ret.getValue("app")),
				pair("secret", ret.getValue("secret")),
			}), data::EncodeFormat::Cbor);

			userData.setBytes(move(secret), "secret");
			return userData;
		}
	}
	return data::Value();
}

void UsersComponent::onNewUser(const data::Value &val) {
	if (_newUserCallback) {
		_newUserCallback(val);
	}
}

NS_SA_END
