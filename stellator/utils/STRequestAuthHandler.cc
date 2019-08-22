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

#include "ExternalSession.h"
#include "RequestAuthHandler.h"

NS_SA_BEGIN

bool RequestAuthHandler::isRequestPermitted(Request & rctx) {
	if (!_userScheme) {
		_userScheme = acquireScheme(rctx);
	}
	_storage = storage::Transaction::acquire(rctx.storage());

	return _userScheme && _storage;
}

bool RequestAuthHandler::processDataHandler(Request & rctx, data::Value &result, data::Value &input) {
	if (_subPath == "/do_login") {
		return processLogin(rctx, result);
	} else if (_subPath == "/do_register") {
		return processRegister(rctx, result);
	} else if (_subPath == "/do_activate") {
		if (_allowActivation) {
			return processActivate(rctx, result);
		}
	} else if (_subPath == "/login") {
		auto &target = rctx.getParsedQueryArgs().getValue("target");
		auto &name = input.getString("name");
		auto &passwd = input.getString("passwd");

		if (target.isString() && StringView(target.getString()).starts_with(StringView(rctx.getFullHostname()))) {
			if (auto u = tryUserLogin(rctx, name, passwd)) {
				if (!isActivated(u->getData())) {
					if (auto session = ExternalSession::acquire(rctx)) {
						session->setValue("not_activated", "auth_error");
					}
					return false;
				}
				if (pushAuthUser(rctx, u->getObjectId())) {
					rctx.setStatus(rctx.redirectTo(String(target.getString())));
					return true;
				}
			}
			if (auto session = ExternalSession::acquire(rctx)) {
				session->setValue("invalid_user", "auth_error");
			}
			rctx.setStatus(rctx.redirectTo(String(target.getString())));
		}
	} else if (_subPath == "/touch") {
		return processTouch(rctx, result);
	} else if (_subPath == "/cancel") {
		if (auto s = ExternalSession::get(rctx)) {
			s->cancel();
		}
		if (auto s = LongSession::acquire(rctx)) {
			s->cancel();
		}
		return true;
	} else if (_subPath == "/update") {
		auto &target = rctx.getParsedQueryArgs().getValue("target");
		if (target.isString() && StringView(target.getString()).starts_with(StringView(rctx.getFullHostname()))) {
			rctx.setStatus(rctx.redirectTo(String(target.getString())));
			return processUpdate(rctx, input);
		}
	}
	return false;
}

int RequestAuthHandler::onTranslateName(Request &rctx) {
	if (_subPath == "/login") {
		auto &target = rctx.getParsedQueryArgs().getValue("target");
		if (target.isString()) {
			_maxRequestSize = 1_KiB;
			_maxVarSize = 256;
			return DataHandler::onTranslateName(rctx);
		} else {
			return HTTP_BAD_REQUEST;
		}
	} else if (_subPath == "/update") {
		auto &target = rctx.getParsedQueryArgs().getValue("target");
		if (target.isString()) {
			_maxRequestSize = 1_KiB;
			_maxVarSize = 256;
			return DataHandler::onTranslateName(rctx);
		} else {
			return HTTP_BAD_REQUEST;
		}
	} else if (_subPath == "/do_login"
			|| _subPath == "/do_register"
			|| _subPath == "/do_activate"
			|| _subPath == "/do_update"
			|| _subPath == "/touch"
			|| _subPath == "/cancel") {
		return DataHandler::onTranslateName(rctx);
	}

	return HTTP_NOT_FOUND;
}

storage::AccessRoleId RequestAuthHandler::getUserRole(const data::Value &uData) const {
	if (uData.isInteger("role")) {
		return storage::AccessRoleId(uData.getInteger("role"));
	}
	return storage::AccessRoleId::Nobody;
}

bool RequestAuthHandler::isActivated(const data::Value &uData) const {
	if (uData.getBool("isActive")) {
		return true;
	}

	if (uData.isDictionary("data") && uData.getValue("data").getBool("emailValidated")) {
		return true;
	}

	return false;
}

bool RequestAuthHandler::validateActivationCode(const data::Value &uData, const BytesView &code) {
	if (uData.isDictionary("data") && uData.getValue("data").isBytes("validationCode")) {
		return BytesView(uData.getValue("data").getBytes("validationCode")) == code;
	}

	if (uData.isDictionary("codes") && uData.getValue("codes").isBytes("activation")) {
		return BytesView(uData.getValue("codes").getBytes("activation")) == code;
	}

	return false;
}

const storage::Scheme *RequestAuthHandler::acquireScheme(Request & rctx) const {
	auto serv = rctx.server();
	return serv.getScheme("users");
}

storage::Auth RequestAuthHandler::acquireAuth(Request & rctx) const {
	return storage::Auth(*_userScheme);
}

/*
 * Default google recatcha validator looks like this:
 *
bool RequestAuthHandler::validateCaptcha(const StringView &v) {
	static constexpr auto CAPTCHA_URL = "https://www.google.com/recaptcha/api/siteverify";
	static constexpr auto CAPTCHA_SECRET = "yoursecret";

	network::Handle h(NetworkHandle::Method::Post, CAPTCHA_URL);
	h.addHeader("Content-Type: application/x-www-form-urlencoded");
	h.setSendData(toString("secret=", CAPTCHA_SECRET, "&response=", v));

	auto data = h.performDataQuery();
	if (data.getBool("success")) {
		return false;
	}
	return true;
}
*/

bool RequestAuthHandler::validateCaptcha(const StringView &v) const {
	return false;
}

bool RequestAuthHandler::pushAuthUser(Request & rctx, int64_t userId) const {
	if (auto s = ExternalSession::acquire(rctx)) {
		return _storage.performAsSystem([&] () -> bool {
			data::Value providers;
			if (auto user = _userScheme->get(_storage, userId)) {

				auto initLongTerm = [&] () -> bool {
					if (auto longSession = LongSession::acquire(rctx)) {
						if (int64_t(longSession->getUser()) == userId) {
							return true;
						} else {
							longSession->assignUser(userId);
							return true;
						}
					}

					return false;
				};

				if (!initLongTerm()) {
					return false;
				}

				s->setUser(userId, getUserRole(user));
				return true;
			}
			return false;
		});
	}
	return false;
}

void RequestAuthHandler::notifyCreatedUser(Request & rctx, const data::Value &) const { }

void RequestAuthHandler::notifyActivatedUser(Request & rctx, const data::Value &) const { }

User * RequestAuthHandler::tryUserLogin(Request &rctx, const StringView &name, const StringView &passwd) {
	if (name.empty() || passwd.empty()) {
		messages::error("Auth", "Name or password is not specified", data::Value{
			pair("Doc", data::Value("You should specify 'name' and 'passwd' variables in request"))
		});
		return nullptr;
	}

	auto user = rctx.storage().authorizeUser(acquireAuth(rctx), name, passwd);
	if (!user) {
		messages::error("Auth", "Invalid username or password");
		return nullptr;
	}

	return user;
}

bool RequestAuthHandler::processLogin(Request &rctx, data::Value &result) {
	auto &queryData = rctx.getParsedQueryArgs();
	auto &name = queryData.getString("name");
	auto &passwd = queryData.getString("passwd");

	if (auto u = tryUserLogin(rctx, name, passwd)) {
		auto userId = u->getObjectId();
		if (isActivated(u->getData())) {
			if (pushAuthUser(rctx, userId)) {
				result.setInteger(userId, "user");
				result.setBool(true, "activated");
				result.setBool(true, "session");
				return true;
			} else {
				result.setBool(true, "activated");
				result.setBool(false, "session");
			}
		} else {
			result.setBool(false, "activated");
		}
	}
	return false;
}

bool RequestAuthHandler::processRegister(Request &rctx, data::Value &result) {
	auto userScheme = rctx.server().getScheme("users");
	auto &queryData = rctx.getParsedQueryArgs();
	auto t = storage::Transaction::acquire(rctx.storage());

	if (!userScheme) {
		return false;
	}

	enum FillFlags {
		None,
		Password = 1 << 0,
		Email = 1 << 1,
		GivenName = 1 << 2,
		FamilyName = 1 << 3,
		Captcha = 1 << 4,

		All = 0b1111
	};

	FillFlags flags = None;
	String captchaToken;

	data::Value newUser(data::Value::Type::DICTIONARY); newUser.asDict().reserve(6);

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
		} else if (it.first == "captcha" && _useCaptcha) {
			if (validateCaptcha(it.second.getString())) {
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

	auto successValue = All;
	if (!_useCaptcha) {
		successValue = FillFlags(toInt(successValue) & ~toInt(Captcha));
	}

	if (flags == successValue) {
		// make activation code
		auto &data = newUser.emplace("data");
		data.setBytes(valid::makeRandomBytes(6), "validationCode");

		if (auto newUserObj = db::Worker(*userScheme, t).asSystem().create(newUser, true)) {
			notifyCreatedUser(rctx, newUserObj);

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
		if (_useCaptcha && (flags & Captcha) == None) {
			result.setBool(false, "captcha");
		}

		result.setBool(false, "OK");
		return false;
	}

	return false;
}

bool RequestAuthHandler::processActivate(Request &rctx, data::Value &result) {
	auto &queryData = rctx.getParsedQueryArgs();
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

	auto userData = storage::Worker(*_userScheme, _storage).asSystem().get(oid);
	if (!userData) {
		return false;
	}

	if (!isActivated(userData)) {
		if (validateActivationCode(userData, code)) {
			data::Value patch;
			if (_userScheme->getField("isActive")->getType() == db::Type::Boolean) {
				patch.setBool(true, "isActive");
			} else if (_userScheme->getField("data")->getType() == db::Type::Extra) {
				patch = data::Value({ pair( "data", data::Value { pair("emailValidated", data::Value(true)) } ) });
			}
			userData = storage::Worker(*_userScheme, _storage).asSystem().update(oid, patch, true);
			auto userId = userData.getInteger("__oid");
			if (pushAuthUser(rctx, userId)) {
				result.setInteger(userId, "user");
				result.setBool(true, "activated");

				notifyActivatedUser(rctx, userData);
				return true;
			}
		}
	}

	return false;
}

bool RequestAuthHandler::processUpdate(Request &rctx, data::Value &input) {
	auto s = ExternalSession::acquire(rctx);

	data::Value patch;

	if (input.isString("email")) { patch.setValue(input.getValue("email"), "email"); }
	if (input.isString("familyName")) { patch.setValue(input.getValue("familyName"), "familyName"); }
	if (input.isString("givenName")) { patch.setValue(input.getValue("givenName"), "givenName"); }
	if (input.isString("middleName")) { patch.setValue(input.getValue("middleName"), "middleName"); }

	if (patch.empty()) {
		return false;
	}

	if (auto userData = _userScheme->update(_storage, s->getUser(), patch)) {
		return true;
	}

	return false;
}

bool RequestAuthHandler::processTouch(Request &rctx, data::Value &result) {
	if (auto s = ExternalSession::get(rctx)) {
		if (auto u = s->getUser()) {
			if (auto uData = _userScheme->get(_storage, u)) {
				auto uRole = getUserRole(uData);
				if (toInt(s->getRole()) != toInt(uRole)) {
					s->setUser(u, uRole);
					s->save();
				}
				result.setInteger(toInt(uRole), "role");
				result.setInteger(u, "user");
				return true;
			}
		}
	}
	return false;
}

NS_SA_END
