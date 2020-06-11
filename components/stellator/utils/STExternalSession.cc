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

#include "mbedtls/pk.h"
#include "mbedtls/aes.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

NS_SA_BEGIN

static data::Value Session_getStorageData(const Request &rctx, const mem::uuid &t) {
	if (auto s = rctx.storage()) {
		return s.get(CoderSource(t.data(), t.size()));
	}
	return data::Value();
}

static bool Session_setStorageData(Request &rctx, const mem::uuid &t, const data::Value &d, TimeInterval maxAge) {
	if (auto s = rctx.storage()) {
		return s.set(CoderSource(t.data(), t.size()), d, maxAge);
	}
	return false;
}

static bool Session_clearStorageData(Request &rctx, const mem::uuid &t) {
	if (auto s = rctx.storage()) {
		return s.clear(CoderSource(t.data(), t.size()));
	}
	return false;
}

ExternalSession::Token ExternalSession::makeToken(const Request &rctx, const mem::uuid & uuid) {
	auto serv = rctx.server();
	string::Sha512 ctx;
	return ctx.update(uuid.data(), uuid.size())
		.update(serv.getSessionKey())
		.update(rctx.getHostname())
		.update(rctx.getProtocol())
		.update(rctx.getRequestHeaders().at("User-Agent"))
		.final();
}

ExternalSession *ExternalSession::getFromData(const Request &rctx, const SessionKeys &keys, StringView data) {
	if (!data.empty()) {
		JsonWebToken token(data);
		if (token.validate(JsonWebToken::RS512, keys.pub)) {
			auto iss = rctx.getFullHostname();
			if (token.validatePayload(iss, iss)) {
				mem::uuid sessionId(token.payload.getBytes("sub"));
				Bytes secToken(token.payload.getBytes("tkn"));

				auto testToken = ExternalSession::makeToken(rctx, sessionId);
				if (secToken.size() == testToken.size() && memcmp(secToken.data(), testToken.data(), testToken.size()) == 0) {
					if (auto d = Session_getStorageData(rctx, sessionId)) {
						auto obj = new (rctx.pool()) ExternalSession(rctx, keys, sessionId, move(d));
						if (obj->isValid()) {
							rctx.storeObject(obj, "ExternalSession"_weak);
							mem::pool::pop();
							return obj;
						}
					}
				}
			}
		}
	}
	return nullptr;
}

ExternalSession *ExternalSession::get() {
	if (auto req = mem::request()) {
		return get(Request(req));
	}
	return nullptr;
}

ExternalSession *ExternalSession::get(const Request &rctx) {
	auto serv = rctx.server();
	return get(rctx, SessionKeys{serv.getSessionPublicKey(), serv.getSessionPrivateKey()});
}

ExternalSession *ExternalSession::get(const Request &rctx, const SessionKeys &keys) {
	if (auto obj = rctx.getObject<ExternalSession>("ExternalSession"_weak)) {
		return obj;
	} else {
		mem::pool::push(rctx.pool());
		auto v = rctx.getRequestHeaders().at("X-ExternalSession");
		if (!v.empty()) {
			return getFromData(rctx, keys, v);
		}

		auto data = rctx.getCookie(SessionCookie);
		if (!data.empty()) {
			return getFromData(rctx, keys, data);
		}

		mem::pool::pop();
		return nullptr;
	}
}

ExternalSession *ExternalSession::acquire(const Request &rctx, TimeInterval maxage) {
	auto serv = rctx.server();
	return acquire(rctx, SessionKeys{serv.getSessionPublicKey(), serv.getSessionPrivateKey()}, maxage);
}

ExternalSession *ExternalSession::acquire(const Request &rctx, const SessionKeys &keys, TimeInterval maxage) {
	if (auto obj = rctx.getObject<ExternalSession>("ExternalSession"_weak)) {
		return obj;
	} else {
		obj = new (rctx.pool()) ExternalSession(rctx, keys, maxage);
		rctx.storeObject(obj, "ExternalSession"_weak);
		return obj;
	}
}

ExternalSession::ExternalSession(const Request &rctx, const SessionKeys &keys) : _request(rctx), _keys(keys) {
	auto data = rctx.getCookie(SessionCookie);
	if (!data.empty()) {
		JsonWebToken token(data);
		if (token.validate(JsonWebToken::RS512, keys.pub)) {
			auto iss = rctx.getFullHostname();
			if (token.validatePayload(iss, iss)) {
				mem::uuid sessionId(token.payload.getBytes("sub"));
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

ExternalSession::ExternalSession(const Request &rctx, const SessionKeys &keys, TimeInterval maxage) : ExternalSession(rctx, keys) {
	if (!_valid) {
		_maxAge = maxage;
		initAsNew(mem::uuid::generate());
	}
}

ExternalSession::ExternalSession(const Request &rctx, const SessionKeys &keys, const mem::uuid &uuid, data::Value &&d)
: _request(rctx), _keys(keys) {
	init(uuid, move(d));
}

ExternalSession::~ExternalSession() {
	if (isModified() && isValid() && _request) {
		save();
	}
}

bool ExternalSession::initAsNew(const mem::uuid &sessionId) {
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

bool ExternalSession::init(const mem::uuid &sessionId, data::Value &&d) {
	_data = move(d);
	_uuid = sessionId;
	_valid = true;
	_user = _data.getInteger("user");
	_role = AccessRoleId(_data.getInteger("role"));
	_maxAge = TimeInterval::microseconds(_data.getInteger("maxage"));

	registerCleanupDestructor(this, _request.pool());

	if (_user) {
		_request.setAltUserId(_user);
	}

	return true;
}

void ExternalSession::setUser(uint64_t id, AccessRoleId role) {
	_user = id;
	_role = role;
	setInteger(id, "user");
	setInteger(toInt(_role), "role");
}

bool ExternalSession::isValid() const {
	return _valid;
}

const mem::uuid &ExternalSession::getUuid() const {
	return _uuid;
}

uint64_t ExternalSession::getUser() const {
	return _user;
}
storage::AccessRoleId ExternalSession::getRole() const {
	return _role;
}
TimeInterval ExternalSession::getMaxAge() const {
	return _maxAge;
}

mem::String ExternalSession::exportToken() const {
	auto secToken = makeToken(_request, _uuid);
	auto iss = _request.getFullHostname();

	JsonWebToken token = JsonWebToken::make(iss, iss);
	token.payload.setBytes(Bytes(_uuid.data(), _uuid.data() + _uuid.size()), "sub");
	token.payload.setBytes(Bytes(secToken.data(), secToken.data() + secToken.size()), "tkn");

	return token.exportSigned(JsonWebToken::RS512, _keys.priv, CoderSource(), data::EncodeFormat::Cbor);
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
	_request.setCookie(SessionCookie, exportToken());
}

LongSession *LongSession::acquire(const Request &rctx) {
	auto serv = rctx.server();
	return acquire(rctx, SessionKeys{serv.getSessionPublicKey(), serv.getSessionPrivateKey()});
}

LongSession *LongSession::acquire(const Request &rctx, const SessionKeys &keys) {
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
	return create(rctx, SessionKeys{serv.getSessionPublicKey(), serv.getSessionPrivateKey()}, uid);
}

LongSession *LongSession::create(const Request &rctx, const SessionKeys &keys, uint64_t uid) {
	if (auto obj = rctx.getObject<LongSession>("LongSession"_weak)) {
		obj->assignUser(uid);
		return obj;
	} else {
		obj = new (rctx.pool()) LongSession(rctx, keys, uid);
		rctx.storeObject(obj, "LongSession"_weak);
		return obj;
	}
}

LongSession::LongSession(const Request &rctx, const SessionKeys &keys): _request(rctx), _keys(keys) {
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

LongSession::LongSession(const Request &rctx, const SessionKeys &keys, uint64_t uid) : _request(rctx), _keys(keys) {
	auto str = _request.getCookie(SessionCookie);
	if (!str.empty()) {
		auto iss = rctx.getFullHostname();
		JsonWebToken cookieToken(str);
		if (cookieToken.validate(JsonWebToken::SigAlg::RS512, _keys.pub)) {
			if (cookieToken.validatePayload(iss, iss)) {
				_user = cookieToken.payload.getInteger("uid");
				_sig = cookieToken.payload.getBytes("signature");
				auto diff = Time::now() - Time::microseconds(cookieToken.payload.getInteger("exp"));
				if (_user != uid || diff > TimeInterval::seconds(60 * 60 * 24 * 15)) {
					assignUser(uid ? uid : _user);
				}
				return;
			}
		}
	}

	assignUser(uid);
}

void LongSession::assignUser(uint64_t uid) {
	if (_sig.empty()) {
		const String ua = _request.getRequestHeaders().at("User-Agent");
		auto userIp = _request.getUseragentIp();
		auto hashBuff = string::Sha256::perform(ua, userIp);

		_sig = Bytes(hashBuff.data(), hashBuff.data() + hashBuff.size());
	}

	_user = uid;

	auto maxAge = TimeInterval::seconds(60 * 60 * 24 * 3650);
	auto iss = _request.getFullHostname();

	auto token = JsonWebToken::make(iss, iss, maxAge, toString(uid));
	token.payload.setInteger(uid, "uid");
	token.payload.setBytes(_sig, "signature");

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

const Bytes &LongSession::getSignature() const {
	return _sig;
}

AuthToken *AuthToken::parse(StringView token, Request req, SessionKeys keys) {
	auto serv = Server(mem::server());
	if (keys.pub.empty()) { keys.pub = serv.getSessionPublicKey(); }
	if (keys.priv.empty()) { keys.priv = serv.getSessionPrivateKey(); }
	if (keys.secret.empty()) { keys.secret = serv.getServerSecret(); }

	auto iss = req.getFullHostname();
	JsonWebToken input(token);
	if (input.validate(JsonWebToken::RS512, keys.pub) && input.validatePayload(iss, iss)) {
		Time tf = Time::microseconds(input.payload.getInteger("tf"));
		auto fp = getFingerprint(req, tf, keys.secret);

		if (BytesView(fp.data(), fp.size()) == BytesView(input.payload.getBytes("fp"))) {
			auto aesKey = makeAesKey(BytesView(fp.data(), fp.size()), keys.priv);
			auto p = decryptAes(aesKey, input.payload.getBytes("p"));
			if (p) {
				return new (mem::pool::acquire()) AuthToken(move(p), keys);
			}
		}
	}
	return nullptr;
}

AuthToken *AuthToken::parse(StringView token, BytesView fpb, StringView iss, StringView aud, SessionKeys keys) {
	auto serv = Server(mem::server());
	if (keys.pub.empty()) { keys.pub = serv.getSessionPublicKey(); }
	if (keys.priv.empty()) { keys.priv = serv.getSessionPrivateKey(); }
	if (keys.secret.empty()) { keys.secret = serv.getServerSecret(); }

	if (aud.empty()) {
		aud = iss;
	}

	JsonWebToken input(token);
	if (input.validate(JsonWebToken::RS512, keys.pub) && input.validatePayload(iss, aud)) {
		Time tf = Time::microseconds(input.payload.getInteger("tf"));
		auto fp = getFingerprint(fpb, tf, keys.secret);

		if (BytesView(fp.data(), fp.size()) == BytesView(input.payload.getBytes("fp"))) {
			auto aesKey = makeAesKey(BytesView(fp.data(), fp.size()), keys.priv);
			auto p = decryptAes(aesKey, input.payload.getBytes("p"));
			if (p) {
				return new (mem::pool::acquire()) AuthToken(move(p), keys);
			}
		}
	}
	return nullptr;
}

AuthToken *AuthToken::create(SessionKeys keys) {
	auto serv = Server(mem::server());
	if (keys.pub.empty()) { keys.pub = serv.getSessionPublicKey(); }
	if (keys.priv.empty()) { keys.priv = serv.getSessionPrivateKey(); }
	if (keys.secret.empty()) { keys.secret = serv.getServerSecret(); }
	return new (mem::pool::acquire()) AuthToken(keys);
}

String AuthToken::exportToken(Request &req, TimeInterval maxage, StringView sub) const {
	auto t = Time::now();
	auto iss = req.getFullHostname();

	JsonWebToken token = JsonWebToken::make(iss, iss, maxage);
	auto fp = getFingerprint(req, t, _keys.secret);

	token.payload.setBytes(BytesView(fp.data(), fp.size()), "fp");
	token.payload.setInteger(t.toMicros(), "tf");

	auto aesKey = makeAesKey(BytesView(fp.data(), fp.size()), _keys.priv);

	token.payload.setBytes(encryptAes(aesKey, _data), "p");
	return token.exportSigned(JsonWebToken::RS512, _keys.priv, CoderSource(), data::EncodeFormat::Cbor);
}

string::Sha512::Buf AuthToken::getFingerprint(const Request &rctx, Time t, StringView secret) {
	auto v = t.toMicros();
	return string::Sha512()
		.update(secret)
		.update(rctx.getHostname())
		.update(rctx.getProtocol())
		.update(rctx.getUseragentIp())
		.update(CoderSource((const uint8_t *)&v, sizeof(v)))
		.update(rctx.getRequestHeaders().at("user-agent"))
		.final();
}

string::Sha512::Buf AuthToken::getFingerprint(BytesView fp, Time t, StringView secret) {
	auto v = t.toMicros();
	return string::Sha512()
		.update(secret)
		.update(fp)
		.update(CoderSource((const uint8_t *)&v, sizeof(v)))
		.final();
}

static constexpr size_t ALIGN(size_t size) { return (((size) + ((16) - 1)) & ~((16) - 1)); }

Bytes AuthToken::encryptAes(const string::Sha256::Buf &key, const data::Value &val) const {
	auto d = data::write(val, data::EncodeFormat::CborCompressed);
	auto dataSize = d.size();
	auto blockSize = ALIGN(dataSize);

	Bytes input; input.resize(blockSize);
	Bytes output; output.resize(blockSize + 16);
	memcpy(input.data(), d.data(), d.size());
	memcpy(output.data(), &dataSize, sizeof(dataSize));

	mbedtls_aes_context aes;
	unsigned char iv[16] = { 0 };

	mbedtls_aes_setkey_enc( &aes, key.data(), 256 );
	mbedtls_aes_crypt_cbc( &aes, MBEDTLS_AES_ENCRYPT, blockSize, iv, input.data(), output.data() + 16 );

	return output;
}

data::Value AuthToken::decryptAes(const string::Sha256::Buf &key, BytesView val) {
	auto dataSize = val.readUnsigned64();
	auto blockSize = ALIGN(dataSize);

	val.offset(8);

	Bytes output; output.resize(blockSize);

	mbedtls_aes_context aes;
	unsigned char iv[16] = { 0 };

	mbedtls_aes_setkey_dec( &aes, key.data(), 256 );
	mbedtls_aes_crypt_cbc( &aes, MBEDTLS_AES_DECRYPT, blockSize, iv, val.data(), output.data() );

	return data::read(BytesView(output.data(), dataSize));
}

string::Sha256::Buf AuthToken::makeAesKey(BytesView hash, StringView priv) {
	string::Sha256::Buf ret;
	mbedtls_pk_context ctx;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;

	mbedtls_pk_init(&ctx);
	mbedtls_entropy_init( &entropy );
	mbedtls_ctr_drbg_init( &ctr_drbg );

	bool success = false;
	StringView key(priv);
	auto pers = valid::makeRandomBytes(16);

	if (mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy, pers.data(), pers.size()) == 0) {
		auto err = mbedtls_pk_parse_key(&ctx, (const unsigned char *)key.data(), key.size() + 1, nullptr, 0);
		if (err == 0) {
			std::array<uint8_t, 256> buf;
			size_t writeLen = buf.size();
			if (mbedtls_pk_sign(&ctx, MBEDTLS_MD_SHA512, hash.data(), hash.size(), buf.data(), &writeLen, mbedtls_ctr_drbg_random, &ctr_drbg) == 0) {
				ret = string::Sha256().update(CoderSource(buf.data(), writeLen)).final();
				success = true;
			}
		}
	}

	if (!success) {
		ret = string::Sha256().update(hash).update(key).final();
	}

	mbedtls_pk_free(&ctx);
	mbedtls_entropy_free( &entropy );
	mbedtls_ctr_drbg_free( &ctr_drbg );

	return ret;
}

AuthToken::AuthToken(SessionKeys keys) : _keys(keys) { }

AuthToken::AuthToken(data::Value &&v, SessionKeys keys) : WrapperTemplate(std::move(v)), _keys(keys) { }

NS_SA_END
