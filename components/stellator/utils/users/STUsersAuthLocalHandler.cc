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
#include "RequestHandler.h"

NS_SA_BEGIN

class AuthLocalHandlerMap : public HandlerMap {
public:
	AuthLocalHandlerMap();
};

class AuthLocalHandlerDoLogin : public AuthHandlerInterface {
public:
	AuthLocalHandlerDoLogin() { }

	virtual bool perform(data::Value &result) const override {
		auto &queryData = _request.getParsedQueryArgs();
		auto &name = queryData.getString("name");
		auto &passwd = queryData.getString("passwd");

		if (auto u = tryUserLogin(_request, name, passwd)) {
			if (auto userId = u->getInteger("external_id")) {
				if (_component->pushAuthUser(_request, userId)) {
					result.setInteger(userId, "user");
					result.setBool(true, "activated");
					if (u->getBool("isIncomplete")) {
						result.setBool(true, "incomplete");
					}
					return true;
				}
			} else if (!u->getBool("isActivated")) {
				result.setBool(false, "activated");
				return true;
			}
		}
		return false;
	}
};

class AuthLocalHandlerDoRegister : public AuthHandlerInterface {
public:
	AuthLocalHandlerDoRegister() { }

	virtual bool perform(data::Value &result) const override {
		auto &userScheme = _component->getLocalUserScheme();
		auto &queryData = _request.getParsedQueryArgs();

		enum FillFlags {
			None,
			Password = 1 << 0,
			Email = 1 << 1,
			GivenName = 1 << 2,
			FamilyName = 1 << 3,
			Captcha = 1 << 4,

			All =  Password | Email | GivenName | FamilyName | Captcha
		};

		FillFlags flags = None;
		String captchaToken;

		data::Value newUser(data::Value::Type::DICTIONARY); newUser.asDict().reserve(6);
		newUser.setBool(false, "isActivated");

		for (auto &it : queryData.asDict()) {
			if (it.first == "email" && it.second.isString()) {
				auto email = it.second.getString();
				if (valid::validateEmail(email)) {
					newUser.setString(move(email), "email");
					flags = FillFlags(flags | Email);
				}
			} else if (it.first == "password" && it.second.isString()) {
				auto password = it.second.getString();
				if (password.length() >= 6) {
					newUser.setString(move(password), "password");
					flags = FillFlags(flags | Password);
				}
			} else if (it.first == "captcha") {
				if (_component->verifyCaptcha(it.second.getString())) {
					flags = FillFlags(flags | Captcha);
				}
			} else if (it.first == "givenName" && it.second.isString() && it.second.size() > 0) {
				newUser.setValue(move(it.second), "givenName");
				flags = FillFlags(flags | GivenName);
			} else if (it.first == "familyName" && it.second.isString() && it.second.size() > 0) {
				newUser.setValue(move(it.second), "familyName");
				flags = FillFlags(flags | FamilyName);
			} else if (it.first == "middleName" && it.second.isString() && it.second.size() > 0) {
				newUser.setValue(move(it.second), "middleName");
			}
		}

		if (flags == All) {
			// make activation code
			auto &data = newUser.emplace("data");
			data.setBytes(valid::makeRandomBytes(24), "code");

			if (auto newUserObj = storage::Worker(userScheme, _request.storage()).asSystem().create(newUser)) {
				if (auto u = _component->getProvider("local")->initUser(0, _transaction, data::Value(newUserObj))) {

				}
				_component->sendActivationEmail(_request.getFullHostname(), newUserObj);

				newUserObj.erase("password");
				newUserObj.erase("data");
				result.setValue(move(newUserObj), "user");
				result.setBool(true, "OK");
				return true;
			} else {
				result.setBool(false, "OK");
				result.setBool(true, "userExist");
				return false;
			}
		} else {
			if ((flags & Password) == None) {
				result.setBool(true, "emptyPassword");
			}
			if ((flags & Email) == None) {
				result.setBool(true, "emptyEmail");
			}
			if ((flags & GivenName) == None) {
				result.setBool(true, "emptyGivenName");
			}
			if ((flags & FamilyName) == None) {
				result.setBool(true, "emptyFamilyName");
			}
			//if ((flags & Captcha) == None) {
			//	result.setBool(false, "captcha");
			//}

			result.setBool(false, "OK");
			return false;
		}

		return false;
	}
};

class AuthLocalHandlerDoActivate : public AuthHandlerInterface {
public:
	AuthLocalHandlerDoActivate() { }

	virtual bool perform(data::Value &result) const override {
		auto &userScheme = _component->getLocalUserScheme();
		auto &queryData = _request.getParsedQueryArgs();
		auto &activationCode = queryData.getString("code");

		if (activationCode.empty()) {
			return false;
		}

		auto d = data::read(base64::decode(activationCode));
		if (!d.isDictionary()) {
			return false;
		}

		auto oid = d.getInteger("id");
		auto &code = d.getBytes("code");

		auto userData = db::Worker(userScheme, _transaction).asSystem().get(oid);
		if (!userData) {
			return false;
		}

		auto isActivated = userData.getBool("isActivated");
		if (!isActivated) {
			auto &d = userData.getValue("data");
			auto &c = d.getBytes("code");
			if (c == code) {
				data::Value patch {
					pair("isActivated", data::Value(true)),
					pair("data", data::Value({
						pair("code", data::Value())
					}))
				};
				userData = db::Worker(userScheme, _transaction).asSystem().update(oid, patch, true);
				if (auto prov = _component->getProvider("local")) {
					if (auto authUser = prov->authUser(_request, toString("local-", userData.getInteger("__oid")), move(userData))) {
						uint64_t userId = authUser.getInteger("__oid");
						if (_component->pushAuthUser(_request, userId)) {
							result.setInteger(userId, "user");
							result.setBool(true, "activated");
							return true;
						}
					}
				}
			}
		}

		return false;
	}
};

class AuthLocalHandlerDoResetPassword : public AuthHandlerInterface {
public:
	AuthLocalHandlerDoResetPassword() { }

	virtual bool perform(data::Value &result) const override {
		auto &userScheme = _component->getLocalUserScheme();
		auto &activationCode = _inputFields.getString("code");

		if (activationCode.empty()) {
			return false;
		}

		auto d = data::read(base64::decode(activationCode));
		if (!d.isDictionary()) {
			return false;
		}

		auto oid = d.getInteger("id");
		auto &code = d.getBytes("code");

		uint64_t codeTime = 0;
		memcpy(&codeTime, code.data() + 12, sizeof(uint64_t));
		if (Time::now() - Time::microseconds(codeTime) > TimeInterval::seconds(30 * 60)) {
			result.setString("code_expired", "error");
			return false;
		}

		auto userData = db::Worker(userScheme, _transaction).asSystem().get(oid);
		if (!userData) {
			result.setString("user_not_found", "error");
			return false;
		}

		auto isActivated = userData.getBool("isActivated");
		if (!isActivated) {
			result.setString("not_activated", "error");
			return false;
		}

		auto &c = userData.getValue("data").getBytes("code");
		if (c == code) {
			data::Value patch {
				pair("password", data::Value(_inputFields.getString("password"))),
				pair("data", data::Value({
					pair("code", data::Value())
				}))
			};
			userData = db::Worker(userScheme, _transaction).asSystem().update(oid, patch, true);
			if (auto prov = _component->getProvider("local")) {
				if (auto authUser = prov->authUser(_request, toString("local-", userData.getInteger("__oid")), move(userData))) {
					uint64_t userId = authUser.getInteger("__oid");
					if (_component->pushAuthUser(_request, userId)) {
						result.setInteger(userId, "user");
						return true;
					}
				}
			}
		} else {
			result.setString("invalid_code", "error");
		}

		return false;
	}
};

class AuthLocalHandlerLogin : public AuthHandlerInterface {
public:
	AuthLocalHandlerLogin() { }

	virtual int onRequest() override {
		auto &args = _request.getParsedQueryArgs();
		auto &target = args.getValue("target");
		if (target.isString() && StringView(target.getString()).starts_with(StringView(_request.getFullHostname()))) {
			auto &name = _inputFields.getString("name");
			auto &passwd = _inputFields.getString("passwd");

			if (auto u = tryUserLogin(_request, name, passwd)) {
				if (auto userId = u->getInteger("external_id")) {
					if (_component->pushAuthUser(_request, userId)) {
						return _request.redirectTo(String(target.getString()));
					}
				}
			}
			if (auto session = ExternalSession::acquire(_request)) {
				session->setValue("invalid_user", "auth_error");
			}
			return _request.redirectTo(String(target.getString()));
		} else {
			_request.setContentType("text/html; charset=UTF-8");
			// print interface
			return DONE;
		}
	}
};

class AuthLocalHandlerUpdate : public AuthHandlerInterface {
public:
	AuthLocalHandlerUpdate() { }

	virtual int onRequest() override {
		auto &args = _request.getParsedQueryArgs();
		auto &target = args.getValue("target");
		if (target.isString() && StringView(target.getString()).starts_with(StringView(_request.getFullHostname()))) {
			auto &userScheme = _component->getLocalUserScheme();
			auto s = ExternalSession::acquire(_request);

			data::Value patch;

			if (_inputFields.isString("name")) { patch.setValue(_inputFields.getValue("name"), "name"); }
			if (_inputFields.isString("email")) { patch.setValue(_inputFields.getValue("email"), "email"); }
			if (_inputFields.isString("familyName")) { patch.setValue(_inputFields.getValue("familyName"), "familyName"); }
			if (_inputFields.isString("givenName")) { patch.setValue(_inputFields.getValue("givenName"), "givenName"); }
			if (_inputFields.isString("middleName")) { patch.setValue(_inputFields.getValue("middleName"), "middleName"); }
			if (_inputFields.isString("picture")) { patch.setValue(_inputFields.getValue("picture"), "picture"); }

			if (patch.empty()) {
				return false;
			}

			if (auto prov = _component->getProvider("local")) {
				if (auto user = getLocalUser(s->getUser())) {
					if (auto userData = userScheme.update(_transaction, user, patch)) {
						if (prov->authUser(_request, toString("local-", userData.getInteger("__oid")), move(userData))) {
							return _request.redirectTo(String(target.getString()));
						}
					}
				}
			}

			return HTTP_BAD_REQUEST;
		} else {
			_request.setContentType("text/html; charset=UTF-8");
			// print interface
			return DONE;
		}
	}
};

class AuthLocalHandlerActivate : public AuthHandlerInterface {
public:
	AuthLocalHandlerActivate() { }

	virtual int onRequest() override {
		auto &queryData = _request.getParsedQueryArgs();
		auto &activationCode = queryData.getString("code");

		if (auto provider = _component->getProvider("local")) {
			auto &url = provider->getConfig().getString("activationUrl");
			if (!url.empty()) {
				if (!activationCode.empty()) {
					return _request.redirectTo(toString(url, "?code=", activationCode));
				} else {
					return _request.redirectTo(String(url));
				}
			}
		}

		_request.setContentType("text/html; charset=UTF-8");
		// print interface
		return DONE;
	}
};

class AuthLocalHandlerResetPassword : public AuthHandlerInterface {
public:
	AuthLocalHandlerResetPassword() { }

	virtual int onRequest() override {
		auto &queryData = _request.getParsedQueryArgs();
		auto &activationCode = queryData.getString("code");

		if (auto provider = _component->getProvider("local")) {
			auto &url = provider->getConfig().getString("resetPasswordUrl");
			if (!url.empty()) {
				if (!activationCode.empty()) {
					return _request.redirectTo(toString(url, "?code=", activationCode));
				} else {
					return _request.redirectTo(String(url));
				}
			}
		}

		_request.setContentType("text/html; charset=UTF-8");
		// print interface
		return DONE;
	}
};

class AuthLocalHandlerPushLogin : public AuthHandlerInterface {
public:
	AuthLocalHandlerPushLogin() { }

	virtual int onRequest() override {
		if (auto d = _request.storage().get(_params.getString("state"))) {
			auto &success = d.getString("success");
			auto &failure = d.getString("failure");

			auto &queryData = _request.getParsedQueryArgs();
			auto &name = queryData.getString("name");
			auto &passwd = queryData.getString("passwd");

			if (auto u = tryUserLogin(_request, name, passwd)) {
				if (auto userId = u->getInteger("external_id")) {
					d.setInteger(userId, "user");
					_request.storage().set(_params.getString("state"), d, 300_sec);
					return _request.redirectTo(String(success));
				}
			}
			return _request.redirectTo(String(failure));
		}

		return HTTP_NOT_FOUND;
	}
};

class AuthLocalHandlerUpdatePassword : public AuthHandlerInterface {
public:
	AuthLocalHandlerUpdatePassword() { }

	virtual bool perform(data::Value &result) const override {
		auto s = ExternalSession::acquire(_request);
		auto &localScheme = _component->getLocalUserScheme();

		if (auto user = getLocalUser(s->getUser())) {
			if (!user.getBool("isActivated")) {
				return false;
			}

			if (valid::validatePassord(_inputFields.getString("old"), user.getBytes("password"),
					localScheme.getField("password")->getSlot<db::FieldPassword>()->salt)) {
				localScheme.update(_transaction, user, data::Value({
					pair("password", data::Value(_inputFields.getString("new")))
				}));
				return true;
			}
		}

		return false;
	}
};

class AuthLocalHandlerDropPassword : public AuthHandlerInterface {
public:
	AuthLocalHandlerDropPassword() { }

	virtual bool perform(data::Value &result) const override {
		auto &localScheme = _component->getLocalUserScheme();

		if (auto user = db::Worker(localScheme, _transaction).asSystem()
				.select(db::Query().select("email", data::Value(_inputFields.getString("email")))).getValue(0)) {
			if (!user.getBool("isActivated")) {
				return false;
			}

			auto t = Time::now().toMicros();
			uint8_t buf[sizeof(uint64_t) + 12] = { 0 };
			valid::makeRandomBytes(buf, 12);
			memcpy(buf + 12, &t, sizeof(uint64_t));

			BytesView code(buf, sizeof(buf));

			db::Worker(localScheme, _transaction).asSystem().update(user, data::Value({
				pair("data", data::Value({
					pair("code", data::Value(code))
				}))
			}));

			user.setValue(data::Value({
				pair("code", data::Value(code))
			}), "data");

			_component->sendDropPasswordEmail(_request.getFullHostname(), user);
			return true;
		}
		return false;
	}
};

AuthLocalHandlerMap::AuthLocalHandlerMap() {
	addHandler("DoLogin", Request::Method::Get, "/do_login", SA_HANDLER(AuthLocalHandlerDoLogin));
	addHandler("DoRegister", Request::Method::Get, "/do_register", SA_HANDLER(AuthLocalHandlerDoRegister));
	addHandler("DoActivate", Request::Method::Get, "/do_activate", SA_HANDLER(AuthLocalHandlerDoActivate));
	addHandler("DoResetPassword", Request::Method::Post, "/do_reset_password", SA_HANDLER(AuthLocalHandlerDoResetPassword))
			.addInputFields({
		db::Field::Text("code", db::MinLength(1), db::MaxLength(1_KiB), db::Flags::Required),
		db::Field::Text("password", db::MinLength(6), db::MaxLength(1_KiB), db::Flags::Required),
	});

	addHandler("Login", Request::Method::Post, "/login", SA_HANDLER(AuthLocalHandlerLogin))
			.addInputFields({
		db::Field::Text("name", db::MinLength(1), db::MaxLength(1_KiB), db::Flags::Required),
		db::Field::Text("passwd", db::MinLength(1), db::MaxLength(1_KiB), db::Flags::Required),
	});

	addHandler("Update", Request::Method::Post, "/update", SA_HANDLER(AuthLocalHandlerUpdate))
			.addInputFields({
		db::Field::Text("name", db::MinLength(1), db::MaxLength(1_KiB)),
		db::Field::Text("email", db::MinLength(1), db::MaxLength(1_KiB), db::Transform::Email),
		db::Field::Text("familyName", db::MinLength(1), db::MaxLength(1_KiB)),
		db::Field::Text("givenName", db::MinLength(1), db::MaxLength(1_KiB)),
		db::Field::Text("middleName", db::MinLength(1), db::MaxLength(1_KiB)),
		db::Field::Text("picture", db::MinLength(1), db::MaxLength(10_KiB), db::Transform::Url),
	});

	addHandler("Activate", Request::Method::Get, "/activate", SA_HANDLER(AuthLocalHandlerActivate));
	addHandler("ResetPassword", Request::Method::Get, "/reset_password", SA_HANDLER(AuthLocalHandlerResetPassword));

	addHandler("PushLogin", Request::Method::Get, "/push_login/:state", SA_HANDLER(AuthLocalHandlerPushLogin));

	addHandler("UpdatePassword", Request::Method::Post, "/update_password", SA_HANDLER(AuthLocalHandlerUpdatePassword))
			.addInputFields({
		db::Field::Text("old", db::MinLength(6), db::MaxLength(1_KiB), db::Flags::Required),
		db::Field::Text("new", db::MinLength(6), db::MaxLength(1_KiB), db::Flags::Required),
	});

	addHandler("DropPassword", Request::Method::Post, "/drop_password", SA_HANDLER(AuthLocalHandlerDropPassword))
			.addInputFields({
		db::Field::Text("email", db::MinLength(1), db::MaxLength(1_KiB), db::Transform::Email, db::Flags::Required),
	});
}

NS_SA_END
