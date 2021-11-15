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

#ifndef COMPONENTS_COMMON_UTILS_SPJSONWEBTOKEN_H_
#define COMPONENTS_COMMON_UTILS_SPJSONWEBTOKEN_H_

#include "SPCommon.h"
#include "SPString.h"
#include "SPTime.h"
#include "SPData.h"
#include "SPDataWrapper.h"

namespace stappler {

struct JsonWebToken {
	enum SigAlg {
		None,
		HS256,
		HS512,
		RS256,
		RS512,
		ES256,
		ES512,
	};

	struct KeyData {
		SigAlg alg = None;
		Bytes key;

		KeyData(SigAlg, const Bytes &n, const Bytes &e);
	};

	static SigAlg getAlg(const StringView &);
	static String getAlgName(const SigAlg &);

	static JsonWebToken make(const StringView &iss, const StringView &aud, TimeInterval maxage = TimeInterval(), const StringView &sub = StringView());

	bool validate(const KeyData &key);
	bool validate(SigAlg, StringView key);
	bool validate(SigAlg, BytesView key);

	bool validatePayload(const StringView &issuer, const StringView &aud);
	bool validatePayload();

	void setMaxAge(TimeInterval maxage);

	data::Value data() const;

	String exportPlain(data::EncodeFormat = data::EncodeFormat::Json) const;
	String exportSigned(SigAlg, const StringView &key,
			const CoderSource &passwd = CoderSource(), data::EncodeFormat = data::EncodeFormat::Json) const;

	JsonWebToken(data::Value &&payload, TimeInterval maxage = TimeInterval());
	JsonWebToken(const StringView &);

	String message;
	data::Value header;
	data::Value payload;
	Bytes sig;

	SigAlg alg = None;
	String kid;
};

class AesToken : public data::Wrapper {
public:
	struct Keys {
		StringView pub;
		StringView priv;
		BytesView secret;
	};

	// struct to pass identity data to fingerprinting algorithm
	struct Fingerprint {
		BytesView fpb;
		Function<void(string::Sha512 &ctx)> cb = nullptr;

		Fingerprint(BytesView v) : fpb(v) { }
		Fingerprint(Function<void(string::Sha512 &ctx)> &&cb) : cb(move(cb)) { }
	};

	// parse from JsonWebToken source
	static AesToken parse(StringView token, const Fingerprint &, StringView iss, StringView aud = StringView(), Keys = Keys());

	// parse from data::Value source
	static AesToken parse(const data::Value &, const Fingerprint &, Keys = Keys());

	static AesToken create(Keys = Keys());

	operator bool () const;

	String exportToken(StringView iss, const Fingerprint &fpb, TimeInterval maxage, StringView sub) const;
	data::Value exportData(const Fingerprint &fpb) const;

protected:
	static string::Sha512::Buf getFingerprint(const Fingerprint &, Time t, BytesView secret);

	Bytes encryptAes(const string::Sha256::Buf &, const data::Value &) const;
	static data::Value decryptAes(const string::Sha256::Buf &, BytesView);
	static string::Sha256::Buf makeAesKey(BytesView, StringView priv, uint32_t version);

	AesToken();
	AesToken(Keys keys);
	AesToken(data::Value &&, Keys keys);

	Keys _keys;
};

}

#endif /* COMPONENTS_COMMON_UTILS_SPJSONWEBTOKEN_H_ */
