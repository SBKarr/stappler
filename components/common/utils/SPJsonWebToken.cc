/**
Copyright (c) 2019-2020 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPJsonWebToken.h"
#include "SPCrypto.h"

namespace stappler {

JsonWebToken::KeyData::KeyData(SigAlg a, const Bytes &n, const Bytes &e) : alg(a) {
	crypto::PublicKey pk;
	if (pk.import(n, e)) {
		key = pk.exportDer();
	}
}

JsonWebToken::SigAlg JsonWebToken::getAlg(const StringView &name) {
	if (name == "HS256") {
		return HS256;
	} else if (name == "HS512") {
		return HS512;
	} else if (name == "RS256") {
		return RS256;
	} else if (name == "RS512") {
		return RS512;
	} else if (name == "ES256") {
		return ES256;
	} else if (name == "ES512") {
		return ES512;
	}
	return JsonWebToken::None;
}

String JsonWebToken::getAlgName(const SigAlg &alg) {
	switch (alg) {
	case None: return "none"; break;
	case HS256: return "HS256"; break;
	case HS512: return "HS512"; break;
	case RS256: return "RS256"; break;
	case RS512: return "RS512"; break;
	case ES256: return "ES256"; break;
	case ES512: return "ES512"; break;
	}
	return String();
}

JsonWebToken JsonWebToken::make(const StringView &iss, const StringView &aud, TimeInterval maxage, const StringView &sub) {
	data::Value payload;
	payload.setString(iss, "iss");
	if (!sub.empty()) {
		payload.setString(sub, "sub");
	}

	if (!aud.empty()) {
		payload.setString(aud, "aud");
	}

	return JsonWebToken(move(payload), maxage);
}

JsonWebToken::JsonWebToken(data::Value &&val, TimeInterval maxage) : payload(move(val)) {
	if (maxage) {
		payload.setInteger((Time::now() + maxage).toSeconds(), "exp");
	}
}

JsonWebToken::JsonWebToken(const StringView &token) {
	StringView r(token);
	auto head = r.readUntil<StringView::Chars<'.'>>();
	if (r.is('.')) {
		++ r;
	}

	header = data::read(base64::decode(head));
	for (auto &it : header.asDict()) {
		if (it.first == "alg") {
			alg = getAlg(it.second.getString());
		} else if (it.first == "kid") {
			kid = it.second.asString();
		}
	}

	auto pl = r.readUntil<StringView::Chars<'.'>>();

	message = String(token.data(), token.size() - r.size());

	if (r.is('.')) {
		++ r;
	}

	payload = data::read(base64::decode(pl));
	sig = base64::decode(r);
}

void JsonWebToken::setMaxAge(TimeInterval maxage) {
	payload.setInteger((Time::now() + maxage).toSeconds(), "exp");
}

bool JsonWebToken::validate(const KeyData &key) {
	return validate(key.alg, key.key);
}

bool JsonWebToken::validate(SigAlg a, StringView key) {
	return validate(a, BytesView((const uint8_t *)key.data(), key.size() + 1));
}

bool JsonWebToken::validate(SigAlg a, BytesView key) {
	if (key.empty()) {
		return false;
	}

	if (a != alg) {
		return false;
	}

	switch (alg) {
	case HS256: {
		auto keySig = string::Sha256::hmac(message, key);
		if (sig.size() == keySig.size() && memcmp(sig.data(), keySig.data(), sig.size()) == 0) {
			return true;
		}
		break;
	}
	case HS512: {
		auto keySig = string::Sha512::hmac(message, key);
		if (sig.size() == keySig.size() && memcmp(sig.data(), keySig.data(), sig.size()) == 0) {
			return true;
		}
		break;
	}
	default: {
		crypto::PublicKey pk(key);
		if (!pk) {
			return false;
		}

		crypto::SignAlgorithm algo = crypto::SignAlgorithm::RSA_SHA512;
		switch (alg) {
		case RS256: algo = crypto::SignAlgorithm::RSA_SHA256; break;
		case ES256: algo = crypto::SignAlgorithm::ECDSA_SHA256; break;
		case RS512: algo = crypto::SignAlgorithm::RSA_SHA512; break;
		case ES512: algo = crypto::SignAlgorithm::ECDSA_SHA512; break;
		default: break;
		}

		if (pk.verify(BytesView((const uint8_t *)message.data(), message.size()), sig, algo)) {
			return true;
		}

		break;
	}
	}

	return false;
}

bool JsonWebToken::validatePayload(const StringView &issuer, const StringView &aud) {
	auto exp = payload.getInteger("exp");
	if (issuer == payload.getString("iss") && aud == payload.getString("aud")
			&& (exp == 0 || exp > int64_t(Time::now().toSeconds()))) {
		return true;
	}
	return false;
}

bool JsonWebToken::validatePayload() {
	auto exp = payload.getInteger("exp");
	if (exp == 0 || exp > int64_t(Time::now().toSeconds())) {
		return true;
	}
	return false;
}

data::Value JsonWebToken::data() const {
	data::Value data;
	data.setValue(header, "header");
	data.setValue(payload, "payload");
	data.setBytes(sig, "sig");
	return data;
}

String JsonWebToken::exportPlain(data::EncodeFormat format) const {
	StringStream out;
	out << base64url::encode(data::write(header, format))
		<< "." << base64url::encode(data::write(payload, format));

	return out.str();
}

String JsonWebToken::exportSigned(SigAlg alg, const StringView &key, const CoderSource &passwd, data::EncodeFormat format) const {
	if (key.empty()) {
		return String();
	}

	data::Value hederData(header);
	hederData.setString(getAlgName(alg), "alg");

	memory::ostringstream out;
	out << base64url::encode(data::write(hederData, format))
		<< "." << base64url::encode(data::write(payload, format));

	switch (alg) {
	case HS256:
		out << "." << base64url::encode(string::Sha256::hmac(out.weak(), key));
		break;

	case HS512:
		out << "." << base64url::encode(string::Sha512::hmac(out.weak(), key));
		break;

	default: {
		crypto::PrivateKey pk(BytesView((const uint8_t *)key.data(), key.size()), passwd);
		auto mes = out.weak();

		crypto::SignAlgorithm algo = crypto::SignAlgorithm::RSA_SHA512;
		switch (alg) {
		case RS256: algo = crypto::SignAlgorithm::RSA_SHA256; break;
		case ES256: algo = crypto::SignAlgorithm::ECDSA_SHA256; break;
		case RS512: algo = crypto::SignAlgorithm::RSA_SHA512; break;
		case ES512: algo = crypto::SignAlgorithm::ECDSA_SHA512; break;
		default: break;
		}

		auto data = pk.sign(BytesView((const uint8_t *)mes.data(), mes.size()), algo);
		if (!data.empty()) {
			out << "." << base64url::encode(data);
		} else {
			return String();
		}
		break;
	}
	}

	return StringView(out.weak()).str();
}

AesToken AesToken::parse(StringView token, const Fingerprint &fpb, StringView iss, StringView aud, Keys keys) {
	if (aud.empty()) {
		aud = iss;
	}

	JsonWebToken input(token);
	if (input.validate(JsonWebToken::RS512, keys.pub) && input.validatePayload(iss, aud)) {
		Time tf = Time::microseconds(input.payload.getInteger("tf"));
		auto fp = getFingerprint(fpb, tf, keys.secret);

		if (BytesView(fp.data(), fp.size()) == BytesView(input.payload.getBytes("fp"))) {
			auto v = crypto::getAesVersion(input.payload.getBytes("p"));
			auto aesKey = makeAesKey(BytesView(fp.data(), fp.size()), keys.priv, v);
			auto p = decryptAes(aesKey, input.payload.getBytes("p"));
			if (p) {
				return AesToken(move(p), keys);
			}
		}
	}
	return AesToken();
}

AesToken AesToken::parse(const data::Value &payload, const Fingerprint &fpb, Keys keys) {
	Time tf = Time::microseconds(payload.getInteger("tf"));
	auto fp = getFingerprint(fpb, tf, keys.secret);

	if (BytesView(fp.data(), fp.size()) == BytesView(payload.getBytes("fp"))) {
		auto v = crypto::getAesVersion(payload.getBytes("p"));
		auto aesKey = makeAesKey(BytesView(fp.data(), fp.size()), keys.priv, v);
		auto p = decryptAes(aesKey, payload.getBytes("p"));
		if (p) {
			return AesToken(move(p), keys);
		}
	}
	return AesToken();
}

AesToken AesToken::create(Keys keys) {
	return AesToken(keys);
}

AesToken::operator bool () const {
	return !_data.isNull() && !_keys.priv.empty() && !_keys.secret.empty();
}

String AesToken::exportToken(StringView iss, const Fingerprint &fpb, TimeInterval maxage, StringView sub) const {
	auto t = Time::now();

	JsonWebToken token = JsonWebToken::make(iss, iss, maxage, sub);
	auto fp = getFingerprint(fpb, t, _keys.secret);

	token.payload.setBytes(BytesView(fp.data(), fp.size()), "fp");
	token.payload.setInteger(t.toMicros(), "tf");

	auto aesKey = makeAesKey(BytesView(fp.data(), fp.size()), _keys.priv, 1);

	token.payload.setBytes(encryptAes(aesKey, _data), "p");
	return token.exportSigned(JsonWebToken::RS512, _keys.priv, CoderSource(), data::EncodeFormat::Cbor);
}

data::Value AesToken::exportData(const Fingerprint &fpb) const {
	auto t = Time::now();
	auto fp = getFingerprint(fpb, t, _keys.secret);

	data::Value payload;
	payload.setBytes(BytesView(fp.data(), fp.size()), "fp");
	payload.setInteger(t.toMicros(), "tf");

	auto aesKey = makeAesKey(BytesView(fp.data(), fp.size()), _keys.priv, 1);

	payload.setBytes(encryptAes(aesKey, _data), "p");
	return payload;
}

string::Sha512::Buf AesToken::getFingerprint(const Fingerprint &fp, Time t, BytesView secret) {
	auto v = t.toMicros();
	if (!fp.fpb.empty()) {
		return string::Sha512()
			.update(secret)
			.update(fp.fpb)
			.update(CoderSource((const uint8_t *)&v, sizeof(v)))
			.final();
	} else if (fp.cb) {
		auto ctx = string::Sha512()
				.update(secret)
				.update(CoderSource((const uint8_t *)&v, sizeof(v)));
		fp.cb(ctx);
		return ctx.final();
	} else {
		return string::Sha512()
			.update(secret)
			.update(CoderSource((const uint8_t *)&v, sizeof(v)))
			.final();
	}
}

Bytes AesToken::encryptAes(const string::Sha256::Buf &key, const data::Value &val) const {
	auto d = data::write(val, data::EncodeFormat::CborCompressed);
	return crypto::encryptAes(key, d, 1);
}

data::Value AesToken::decryptAes(const string::Sha256::Buf &key, BytesView val) {
	auto output = crypto::decryptAes(key, val);
	return data::read(output);
}

string::Sha256::Buf AesToken::makeAesKey(BytesView hash, StringView priv, uint32_t version) {
	return crypto::makeAesKey(BytesView((const uint8_t *)priv.data(), priv.size()), hash, version);
}

AesToken::AesToken() { }

AesToken::AesToken(Keys keys) : _keys(keys) { }

AesToken::AesToken(data::Value &&v, Keys keys) : data::Wrapper(std::move(v)), _keys(keys) { }

}
