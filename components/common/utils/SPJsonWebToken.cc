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

#include "mbedtls/config.h"
#include "mbedtls/pk.h"
#include "mbedtls/error.h"
#include "mbedtls/pem.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

namespace stappler {

static size_t asn1_size(size_t s) {
	if (s < 0x80) {
		return 1;
	} else if (s <= 0xFF) {
		return 2;
	} else {
		return 3;
	}
}

static size_t asn1_writeSize(uint8_t **ptr, size_t s) {
	if (s < 0x80) {
		**ptr = uint8_t(s);
		++ (*ptr);
		return 1;
	} else if (s <= 0xFF) {
		**ptr = uint8_t(0x81);
		++ (*ptr);
		**ptr = uint8_t(s);
		++ (*ptr);
		return 2;
	} else {
		**ptr = uint8_t(0x82);
		++ (*ptr);
		uint16_t d = stappler::byteorder::bswap16(uint16_t(s));
		memcpy(*ptr, &d, sizeof(uint16_t));
		(*ptr) += 2;
		return 3;
	}
}

static size_t asn1_writeLongInt(uint8_t **ptr, size_t s, const uint8_t *data, bool prefix = false) {
	size_t ret = 1;
	**ptr = uint8_t(0x02);
	++ (*ptr);
	ret += asn1_writeSize(ptr, s + (prefix?1:0));
	if (prefix) {
		**ptr = 0;
		++ (*ptr);
	}
	memcpy(*ptr, data, s);
	(*ptr) += s;
	return ret + s;
}


static uint8_t asn1_rsaConstant[] = { 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05, 0x00 };

JsonWebToken::KeyData::KeyData(SigAlg a, const Bytes &n, const Bytes &e) : alg(a) {
	auto keySize = n.size() + asn1_size(n.size()) + e.size() + asn1_size(e.size()) + 3;
	auto keySeqSize = keySize + 1 + asn1_size(keySize);
	auto keyStringSize = keySeqSize + 2 + asn1_size(keySeqSize);

	auto rsaSize = keyStringSize + sizeof(asn1_rsaConstant);
	auto fullSize = rsaSize + 1 + asn1_size(rsaSize);
	key.resize(fullSize);

	auto ptr = key.data();

	*ptr = uint8_t(0x30); ++ ptr;
	asn1_writeSize(&ptr, rsaSize);
	memcpy(ptr, asn1_rsaConstant, sizeof(asn1_rsaConstant));
	ptr += sizeof(asn1_rsaConstant);

	*ptr = uint8_t(0x03); ++ ptr;
	asn1_writeSize(&ptr, keySeqSize + 1);
	*ptr = uint8_t(0x00); ++ ptr;

	*ptr = uint8_t(0x30); ++ ptr;
	asn1_writeSize(&ptr, keySize);
	asn1_writeLongInt(&ptr, n.size(), n.data(), true);
	asn1_writeLongInt(&ptr, e.size(), e.data());
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
	return validate(key.alg, StringView((const char *)key.key.data(), key.key.size() - 1));
}

bool JsonWebToken::validate(SigAlg a, const StringView &key) {
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
		mbedtls_pk_context ctx;
		mbedtls_pk_init(&ctx);

		auto err = mbedtls_pk_parse_public_key(&ctx, (const unsigned char *)key.data(), key.size() + 1);
		if (err == 0) {
			switch (alg) {
			case RS256:
			case ES256: {
				auto hash = string::Sha256().update(message).final();
				if (mbedtls_pk_verify(&ctx, MBEDTLS_MD_SHA256, hash.data(), hash.size(), sig.data(), sig.size()) == 0) {
					mbedtls_pk_free(&ctx);
					return true;
				}
				break;
			}
			case RS512:
			case ES512: {
				auto hash = string::Sha512().update(message).final();
				if (mbedtls_pk_verify(&ctx, MBEDTLS_MD_SHA512, hash.data(), hash.size(), sig.data(), sig.size()) == 0) {
					mbedtls_pk_free(&ctx);
					return true;
				}
				break;
			}
			default:
				mbedtls_pk_free(&ctx);
				return false;
				break;
			}
		} else {
			char buf[256] = { 0 };
			mbedtls_strerror(err, buf, 255);
			std::cout << buf << "\n";
		}

		mbedtls_pk_free(&ctx);
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
		bool success = false;
		mbedtls_pk_context ctx;
		mbedtls_entropy_context entropy;
		mbedtls_ctr_drbg_context ctr_drbg;

		mbedtls_pk_init(&ctx);
		mbedtls_entropy_init( &entropy );
		mbedtls_ctr_drbg_init( &ctr_drbg );

		auto pers = valid::makeRandomBytes(16);

		if (mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy, pers.data(), pers.size()) == 0) {
			auto err = passwd.empty()
					? mbedtls_pk_parse_key(&ctx, (const unsigned char *)key.data(), key.size() + 1, nullptr, 0)
					: mbedtls_pk_parse_key(&ctx, (const unsigned char *)key.data(), key.size() + 1, passwd.data(), passwd.size());

			if (err == 0) {
				switch (alg) {
				case RS256:
				case ES256: {
					std::array<uint8_t, 1_KiB> buf;
					size_t writeLen = buf.size();
					auto hash = string::Sha256().update(out.weak()).final();
					if (mbedtls_pk_sign(&ctx, MBEDTLS_MD_SHA256, hash.data(), hash.size(), buf.data(), &writeLen, mbedtls_ctr_drbg_random, &ctr_drbg) == 0) {
						out << "." << base64url::encode(BytesView(buf.data(), writeLen));
						success = true;
					}
					break;
				}
				case RS512:
				case ES512: {
					std::array<uint8_t, 1_KiB> buf;
					size_t writeLen = buf.size();
					auto hash = string::Sha512().update(out.weak()).final();
					if (mbedtls_pk_sign(&ctx, MBEDTLS_MD_SHA512, hash.data(), hash.size(), buf.data(), &writeLen, mbedtls_ctr_drbg_random, &ctr_drbg) == 0) {
						out << "." << base64url::encode(BytesView(buf.data(), writeLen));
						success = true;
					}
					break;
				}
				default:
					break;
				}
			} else {
				char buf[1_KiB] = { 0 };
				mbedtls_strerror(err, buf, 1_KiB);
				std::cout << buf << "\n";
			}
		}

		mbedtls_pk_free(&ctx);
		mbedtls_entropy_free( &entropy );
		mbedtls_ctr_drbg_free( &ctr_drbg );

		if (!success) {
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
			auto aesKey = makeAesKey(BytesView(fp.data(), fp.size()), keys.priv);
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
		auto aesKey = makeAesKey(BytesView(fp.data(), fp.size()), keys.priv);
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

	auto aesKey = makeAesKey(BytesView(fp.data(), fp.size()), _keys.priv);

	token.payload.setBytes(encryptAes(aesKey, _data), "p");
	return token.exportSigned(JsonWebToken::RS512, _keys.priv, CoderSource(), data::EncodeFormat::Cbor);
}

data::Value AesToken::exportData(const Fingerprint &fpb) const {
	auto t = Time::now();
	auto fp = getFingerprint(fpb, t, _keys.secret);

	data::Value payload;
	payload.setBytes(BytesView(fp.data(), fp.size()), "fp");
	payload.setInteger(t.toMicros(), "tf");

	auto aesKey = makeAesKey(BytesView(fp.data(), fp.size()), _keys.priv);

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

static constexpr size_t SPALIGN(size_t size) { return (((size) + ((16) - 1)) & ~((16) - 1)); }

Bytes AesToken::encryptAes(const string::Sha256::Buf &key, const data::Value &val) const {
	auto d = data::write(val, data::EncodeFormat::CborCompressed);
	auto dataSize = d.size();
	auto blockSize = SPALIGN(dataSize);

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

data::Value AesToken::decryptAes(const string::Sha256::Buf &key, BytesView val) {
	auto dataSize = val.readUnsigned64();
	auto blockSize = SPALIGN(dataSize);

	val.offset(8);

	Bytes output; output.resize(blockSize);

	mbedtls_aes_context aes;
	unsigned char iv[16] = { 0 };

	mbedtls_aes_setkey_dec( &aes, key.data(), 256 );
	mbedtls_aes_crypt_cbc( &aes, MBEDTLS_AES_DECRYPT, blockSize, iv, val.data(), output.data() );

	return data::read(BytesView(output.data(), dataSize));
}

string::Sha256::Buf AesToken::makeAesKey(BytesView hash, StringView priv) {
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

AesToken::AesToken() { }

AesToken::AesToken(Keys keys) : _keys(keys) { }

AesToken::AesToken(data::Value &&v, Keys keys) : data::Wrapper(std::move(v)), _keys(keys) { }

}
