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
#include "JsonWebToken.h"

NS_SA_BEGIN

static data::Value Session_getStorageData(const Request &rctx, const apr::uuid &t) {
	if (auto s = rctx.storage()) {
		return s.get(CoderSource(t.data(), t.size()));
	}
	return data::Value();
}

static bool Session_setStorageData(Request &rctx, const apr::uuid &t, const data::Value &d, TimeInterval maxAge) {
	if (auto s = rctx.storage()) {
		return s.set(CoderSource(t.data(), t.size()), d, maxAge);
	}
	return false;
}

static bool Session_clearStorageData(Request &rctx, const apr::uuid &t) {
	if (auto s = rctx.storage()) {
		return s.clear(CoderSource(t.data(), t.size()));
	}
	return false;
}


ExternalSession::Token ExternalSession::makeToken(const Request &rctx, const apr::uuid & uuid) {
	auto serv = rctx.server();
	string::Sha512 ctx;
	return ctx.update(uuid.data(), uuid.size())
		.update(serv.getSessionKey())
		.update(rctx.getHostname())
		.update(rctx.getProtocol())
		.update(rctx.getRequestHeaders().at("User-Agent"))
		.final();
}

ExternalSession *ExternalSession::get(const Request &rctx) {
	auto serv = rctx.server();
	return get(rctx, SessionKeyPair{serv.getSessionPublicKey(), serv.getSessionPrivateKey()});
}

ExternalSession *ExternalSession::get(const Request &rctx, const SessionKeyPair &keys) {
	if (auto obj = rctx.getObject<ExternalSession>("ExternalSession"_weak)) {
		return obj;
	} else {
		auto data = rctx.getCookie(SessionCookie);
		if (!data.empty()) {
			JsonWebToken token(data);
			if (token.validate(JsonWebToken::RS512, keys.pub)) {
				auto iss = rctx.getFullHostname();
				if (token.validatePayload(iss, iss)) {
					apr::uuid sessionId(token.payload.getBytes("sub"));
					Bytes secToken(token.payload.getBytes("tkn"));

					auto testToken = makeToken(rctx, sessionId);
					if (secToken.size() == testToken.size() && memcmp(secToken.data(), testToken.data(), testToken.size()) == 0) {
						if (auto d = Session_getStorageData(rctx, sessionId)) {
							obj = new (rctx.pool()) ExternalSession(rctx, keys, sessionId, move(d));
							if (obj->isValid()) {
								rctx.storeObject(obj, "ExternalSession"_weak);
								return obj;
							}
						}
					}
				}
			}
		}


		obj = new (rctx.pool()) ExternalSession(rctx, keys);
		if (obj->isValid()) {
			rctx.storeObject(obj, "ExternalSession"_weak);
			return obj;
		}
		return nullptr;
	}
}

ExternalSession *ExternalSession::acquire(const Request &rctx, TimeInterval maxage) {
	auto serv = rctx.server();
	return acquire(rctx, SessionKeyPair{serv.getSessionPublicKey(), serv.getSessionPrivateKey()}, maxage);
}

ExternalSession *ExternalSession::acquire(const Request &rctx, const SessionKeyPair &keys, TimeInterval maxage) {
	if (auto obj = rctx.getObject<ExternalSession>("ExternalSession"_weak)) {
		return obj;
	} else {
		obj = new (rctx.pool()) ExternalSession(rctx, keys, maxage);
		rctx.storeObject(obj, "ExternalSession"_weak);
		return obj;
	}
}

ExternalSession::ExternalSession(const Request &rctx, const SessionKeyPair &keys) : _request(rctx), _keys(keys) {
	auto data = rctx.getCookie(SessionCookie);
	if (!data.empty()) {
		JsonWebToken token(data);
		if (token.validate(JsonWebToken::RS512, keys.pub)) {
			auto iss = rctx.getFullHostname();
			if (token.validatePayload(iss, iss)) {
				apr::uuid sessionId(token.payload.getBytes("sub"));
				Bytes secToken(token.payload.getBytes("tkn"));

				auto testToken = makeToken(_request, sessionId);
				if (secToken.size() == testToken.size() && memcmp(secToken.data(), testToken.data(), testToken.size()) == 0) {
					if (auto d = Session_getStorageData(_request, sessionId)) {
						init(sessionId, move(d));
					}
				}
			}
		}
	}
}

ExternalSession::ExternalSession(const Request &rctx, const SessionKeyPair &keys, TimeInterval maxage) : ExternalSession(rctx, keys) {
	if (!_valid) {
		_maxAge = maxage;
		initAsNew(apr::uuid::generate());
	}
}

ExternalSession::ExternalSession(const Request &rctx, const SessionKeyPair &keys, const apr::uuid &uuid, data::Value &&d)
: _request(rctx), _keys(keys) {
	init(uuid, move(d));
}

ExternalSession::~ExternalSession() {
	if (isModified() && isValid() && _request) {
		save();
	}
}

bool ExternalSession::initAsNew(const apr::uuid &sessionId) {
	data::Value d {
		pair("user", data::Value(0)),
		pair("maxage", data::Value(_maxAge.toMicroseconds()))
	};

	Session_setStorageData(_request, sessionId, d, _maxAge);

	_data = move(d);
	_uuid = sessionId;
	_valid = true;

	registerCleanupDestructor(this, _request.pool());

	setCookie();

	return true;
}

data::Value ExternalSession::getUserData() const {
	auto userScheme = _request.server().getScheme("external_users");
	if (_user && userScheme) {
		return storage::Worker(*userScheme, _request.storage()).asSystem().get(_user);
	}
	return data::Value();
}

bool ExternalSession::init(const apr::uuid &sessionId, data::Value &&d) {
	_data = move(d);
	_uuid = sessionId;
	_valid = true;
	_user = _data.getInteger("user");
	_maxAge = TimeInterval::microseconds(_data.getInteger("maxage"));

	registerCleanupDestructor(this, _request.pool());

	return true;
}

void ExternalSession::setUser(uint64_t id) {
	_user = id;
	setInteger(id, "user");
}

bool ExternalSession::isValid() const {
	return _valid;
}

const apr::uuid &ExternalSession::getUuid() const {
	return _uuid;
}

uint64_t ExternalSession::getUser() const {
	return _user;
}
TimeInterval ExternalSession::getMaxAge() const {
	return _maxAge;
}

bool ExternalSession::save() {
	setModified(false);
	return Session_setStorageData(_request, _uuid, _data, _maxAge);
}

bool ExternalSession::cancel() {
	Session_clearStorageData(_request, _uuid);
	_request.removeCookie(SessionCookie);
	_valid = false;
	return true;
}

bool ExternalSession::touch(TimeInterval maxAge) {
	if (maxAge) {
		_maxAge = maxAge;
	}

	return save();
}

void ExternalSession::setCookie() {
	auto secToken = makeToken(_request, _uuid);
	auto iss = _request.getFullHostname();

	JsonWebToken token = JsonWebToken::make(iss, iss);
	token.payload.setBytes(Bytes(_uuid.data(), _uuid.data() + _uuid.size()), "sub");
	token.payload.setBytes(Bytes(secToken.data(), secToken.data() + secToken.size()), "tkn");

	auto str = token.exportSigned(JsonWebToken::RS512, _keys.priv, CoderSource(), data::EncodeFormat::Cbor);
	_request.setCookie(SessionCookie, str);
}

LongSession *LongSession::acquire(const Request &rctx) {
	auto serv = rctx.server();
	return acquire(rctx, SessionKeyPair{serv.getSessionPublicKey(), serv.getSessionPrivateKey()});
}

LongSession *LongSession::acquire(const Request &rctx, const SessionKeyPair &keys) {
	if (auto obj = rctx.getObject<LongSession>("LongSession"_weak)) {
		return obj;
	} else {
		obj = new (rctx.pool()) LongSession(rctx, keys);
		rctx.storeObject(obj, "LongSession"_weak);
		return obj;
	}
}

LongSession *LongSession::create(const Request &rctx, uint64_t uid) {
	auto serv = rctx.server();
	return create(rctx, SessionKeyPair{serv.getSessionPublicKey(), serv.getSessionPrivateKey()}, uid);
}

LongSession *LongSession::create(const Request &rctx, const SessionKeyPair &keys, uint64_t uid) {
	if (auto obj = rctx.getObject<LongSession>("LongSession"_weak)) {
		obj->assignUser(uid);
		return obj;
	} else {
		obj = new (rctx.pool()) LongSession(rctx, keys, uid);
		rctx.storeObject(obj, "LongSession"_weak);
		return obj;
	}
}

LongSession::LongSession(const Request &rctx, const SessionKeyPair &keys): _request(rctx), _keys(keys) {
	auto str = _request.getCookie(SessionCookie);

	if (str.empty()) {
		return;
	}

	auto iss = rctx.getFullHostname();

	JsonWebToken cookieToken(str);
	if (cookieToken.validate(JsonWebToken::SigAlg::RS512, _keys.pub)) {
		if (cookieToken.validatePayload(iss, iss)) {
			_user = cookieToken.payload.getInteger("uid");
		}
	}
}

LongSession::LongSession(const Request &rctx, const SessionKeyPair &keys, uint64_t uid) : _request(rctx), _keys(keys) {
	assignUser(uid);
}

void LongSession::assignUser(uint64_t uid) {
	_user = uid;

	auto maxAge = TimeInterval::seconds(60 * 60 * 24 * 3650);
	auto iss = _request.getFullHostname();

	auto token = JsonWebToken::make(iss, iss, maxAge, toString(uid));
	token.payload.setInteger(uid, "uid");

	auto str = token.exportSigned(JsonWebToken::RS512, _keys.priv, CoderSource(), data::EncodeFormat::Cbor);
	if (!str.empty()) {
		_request.setCookie(SessionCookie, str, maxAge);
	}
}

void LongSession::cancel() {
	_request.removeCookie(SessionCookie);
}

uint64_t LongSession::getUser() const {
	return _user;
}

NS_SA_END
