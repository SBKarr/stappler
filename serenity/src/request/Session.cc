// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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
#include "Session.h"
#include "User.h"
#include "StorageAdapter.h"

NS_SA_BEGIN

#define SA_SESSION_EXISTS(var, def) ((var == NULL)?def:var)
#define SA_SESSION_OBJECT_FLAG 0xFFF0

#define SA_SESSION_TOKEN_NAME "token"
#define SA_SESSION_UUID_KEY "uuid"
#define SA_SESSION_USER_NAME_KEY "userName"
#define SA_SESSION_USER_ID_KEY "userId"
#define SA_SESSION_SALT_KEY "salt"
#define SA_SESSION_MAX_AGE_KEY "maxAge"
#define SA_SESSION_TOKEN_LEN 64

void Session::makeSessionToken(Request &rctx, Token &buf, const apr::uuid & uuid, const StringView & userName) {
	auto serv = rctx.server();
	string::Sha512 ctx;
	ctx.update(uuid.data(), uuid.size())
			.update(userName)
			.update(serv.getSessionKey())
			.update(rctx.getHostname())
			.update(rctx.getProtocol())
			.update(rctx.getRequestHeaders().at("User-Agent"))
			.final(buf.data());
}

void Session::makeCookieToken(Request &rctx, Token &buf, const apr::uuid & uuid, const StringView & userName, const Bytes & salt) {
	auto serv = rctx.server();
	string::Sha512 ctx;
	ctx.update(uuid.data(), uuid.size())
			.update(userName)
			.update(serv.getSessionKey())
			.update(rctx.getHostname())
			.update(rctx.getProtocol())
			.update(salt)
			.update(rctx.getRequestHeaders().at("User-Agent"))
			.final(buf.data());

	/*Request::AddDebug("Session", "Cookie token components", data::Value{
				std::make_pair("uuid", data::Value(uuid.bytes())),
				std::make_pair("userName", data::Value(userName)),
				std::make_pair("getSessionKey", data::Value(serv.getSessionKey())),
				std::make_pair("getHostname", data::Value(rctx.getHostname())),
				std::make_pair("getProtocol", data::Value(rctx.getProtocol())),
				std::make_pair("salt", data::Value(salt)),
				std::make_pair("User-Agent", data::Value(rctx.getRequestHeaders().at("User-Agent"))),
	});*/
}

Session::~Session() {
	if (isModified() && isValid() && _request) {
		save();
	}
}

Session::Session(const Request &rctx, bool silent) : _request(rctx) {
	_valid = init(silent);
}

Session::Session(const Request &rctx, User *user, TimeInterval maxAge) : _request(rctx) {
	_valid = init(user, maxAge);
}

bool Session::init(User *user, TimeInterval maxAge) {
	_maxAge = maxAge;
	_uuid = apr::uuid::generate();
	_user = user;

	auto &data = newDict("data");
	data.setString(user ? user->getName() : apr::uuid::generate().str(), SA_SESSION_USER_NAME_KEY);
	data.setInteger(user ? user->getObjectId() : 0, SA_SESSION_USER_ID_KEY);
	data.setInteger(maxAge, SA_SESSION_MAX_AGE_KEY);
	data.setBytes(_uuid.bytes(), SA_SESSION_UUID_KEY);

	Bytes salt; salt.resize(32);
	ap_random_insecure_bytes(salt.data(), 32);

	makeSessionToken(_request, _sessionToken, _uuid, data.getString(SA_SESSION_USER_NAME_KEY));
	makeCookieToken(_request, _cookieToken, _uuid, user->getName(), salt);

	data.setBytes(std::move(salt), SA_SESSION_SALT_KEY);

	setModified(false);

	return write();
}

bool Session::init(bool silent) {
	auto serv = _request.server();
	auto &args = _request.getParsedQueryArgs();

	auto &sessionTokenString = args.getString(SA_SESSION_TOKEN_NAME);

	/* token is a base64 encoded hash from sha512, so, it must have 88 bytes */
	if (sessionTokenString.size() != 88) {
		if (!silent) { messages::debug("Session", "Session token format is invalid"); }
		return false;
	}

	Bytes sessionToken(base64url::decode(sessionTokenString));
	auto sessionData = getStorageData(_request, sessionToken);
	auto &data = sessionData.getValue("data");
	if (!data) {
		if (!silent) { messages::debug("Session", "Fail to extract session from storage"); }
	}

	auto &uuidData = data.getBytes(SA_SESSION_UUID_KEY);
	auto &userName = data.getString(SA_SESSION_USER_NAME_KEY);
	auto &salt = data.getBytes(SA_SESSION_SALT_KEY);
	if (uuidData.empty() || userName.empty()) {
		if (!silent) { messages::error("Session", "Wrong authority data in session"); }
		return false;
	}

	apr::uuid sessionUuid(uuidData);

	Token buf;
	makeSessionToken(_request, buf, sessionUuid, userName);

	if (memcmp(buf.data(), sessionToken.data(), sizeof(Token)) != 0) {
		if (!silent) { messages::error("Session", "Session token is invalid"); }
		return false;
	}

	Bytes cookieToken(base64url::decode(_request.getCookie(serv.getSessionName(), true)));
	if (cookieToken.empty() || cookieToken.size() != 64) {
		if (!silent) { messages::error("Session", "Fail to read token from cookie", data::Value{
			std::make_pair("token", data::Value(cookieToken))
		}); }
		return false;
	}

	makeCookieToken(_request, buf, sessionUuid, userName, salt);

	if (memcmp(buf.data(), cookieToken.data(), 64) != 0) {
		if (!silent) { messages::error("Session", "Cookie token is invalid", data::Value{
			std::make_pair("token", data::Value(cookieToken)),
			std::make_pair("check", data::Value(Bytes(buf.begin(), buf.end())))
		}); }
		return false;
	}

	memcpy(_cookieToken.data(), cookieToken.data(), SA_SESSION_TOKEN_LEN);
	memcpy(_sessionToken.data(), sessionToken.data(), SA_SESSION_TOKEN_LEN);
	_uuid = sessionUuid;
	_maxAge = TimeInterval::seconds(data.getInteger(SA_SESSION_MAX_AGE_KEY));

	uint64_t id = (uint64_t)data.getInteger(SA_SESSION_USER_ID_KEY);
	if (id) {
		_user = getStorageUser(_request, id);
		if (!_user) {
			if (!silent) { messages::error("Session", "Invalid user id in session data"); }
			return false;
		}
	}

	_data = std::move(sessionData);
	return _user != nullptr;
}

const Session::Token & Session::getCookieToken() const {
	return _cookieToken;
}

bool Session::isValid() const {
	return _valid;
}
const Session::Token & Session::getSessionToken() const {
	return _sessionToken;
}

const apr::uuid &Session::getSessionUuid() const {
	return _uuid;
}

bool Session::write() {
	if (!save()) {
		return false;
	}

	_request.setCookie(_request.server().getSessionName(), base64url::encode(_cookieToken), _maxAge);
	return true;
}

bool Session::save() {
	setModified(false);
	return setStorageData(_request, _sessionToken, _data, _maxAge);
}

bool Session::cancel() {
	clearStorageData(_request, _sessionToken);
	_request.removeCookie(_request.server().getSessionName());
	_valid = false;
	return true;
}

bool Session::touch(TimeInterval maxAge) {
	if (maxAge) {
		_maxAge = maxAge;
		setInteger(maxAge.toSeconds(), SA_SESSION_MAX_AGE_KEY);
	}

	return write();
}

User *Session::getUser() const {
	return _user;
}

TimeInterval Session::getMaxAge() const {
	return _maxAge;
}


data::Value Session::getStorageData(Request &rctx, const Token &t) {
	if (auto s = rctx.storage()) {
		return s.get(t);
	}
	return data::Value();
}

data::Value Session::getStorageData(Request &rctx, const Bytes &b) {
	if (auto s = rctx.storage()) {
		return s.get(b);
	}
	return data::Value();
}

bool Session::setStorageData(Request &rctx, const Token &t, const data::Value &d, TimeInterval maxAge) {
	if (auto s = rctx.storage()) {
		return s.set(t, d, maxAge);
	}
	return false;
}

bool Session::clearStorageData(Request &rctx, const Token &t) {
	if (auto s = rctx.storage()) {
		return s.clear(t);
	}
	return false;
}

User *Session::getStorageUser(Request &rctx, uint64_t oid) {
	if (auto s = rctx.storage()) {
		return User::get(s, oid);
	}
	return nullptr;
}

NS_SA_END
