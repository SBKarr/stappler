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

#ifndef SERENITY_SRC_UTILS_JSONWEBTOKEN_H_
#define SERENITY_SRC_UTILS_JSONWEBTOKEN_H_

#include "Define.h"

NS_SA_BEGIN

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

	static JsonWebToken make(const String &iss, const String &aud, TimeInterval maxage = TimeInterval(), const String &sub = String());

	bool validate(const KeyData &key);
	bool validate(SigAlg, const StringView &key);

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

NS_SA_END

#endif /* SERENITY_SRC_UTILS_JSONWEBTOKEN_H_ */
