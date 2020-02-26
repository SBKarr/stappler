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

#ifndef STELLATOR_UTILS_STEXTERNALSESSION_H_
#define STELLATOR_UTILS_STEXTERNALSESSION_H_

#include "Request.h"
#include "SPDataWrapper.h"

NS_SA_BEGIN

struct SessionKeys {
	StringView pub;
	StringView priv;
	StringView secret;
};

class ExternalSession : public stappler::data::WrapperTemplate<mem::Interface> {
public:
	using Token = string::Sha512::Buf;
	using AccessRoleId = storage::AccessRoleId;

	static constexpr auto SessionCookie = "SessionToken";

	static ExternalSession *get();
	static ExternalSession *get(const Request &);
	static ExternalSession *get(const Request &, const SessionKeys &keys);

	static ExternalSession *acquire(const Request &, TimeInterval = TimeInterval::seconds(60 * 60 * 12));
	static ExternalSession *acquire(const Request &, const SessionKeys &keys, TimeInterval = TimeInterval::seconds(60 * 60 * 12));

	~ExternalSession();

	bool initAsNew(const mem::uuid &);
	bool init(const mem::uuid &, data::Value &&);

	void setUser(uint64_t, AccessRoleId);

	bool isValid() const;
	const mem::uuid &getUuid() const;

	uint64_t getUser() const;
	AccessRoleId getRole() const;
	data::Value getUserData() const;
	TimeInterval getMaxAge() const;

	bool save();
	bool cancel();
	bool touch(TimeInterval maxAge = TimeInterval());

protected:
	ExternalSession(const Request &, const SessionKeys &key);
	ExternalSession(const Request &, const SessionKeys &key, TimeInterval);
	ExternalSession(const Request &, const SessionKeys &key, const mem::uuid &, data::Value &&);

	void setCookie();

	static Token makeToken(const Request &rctx, const mem::uuid & uuid);

	Request _request;

	SessionKeys _keys;

	Token _token;
	mem::uuid _uuid;
	TimeInterval _maxAge;

	uint64_t _user = 0;
	AccessRoleId _role = AccessRoleId::Nobody;
	bool _valid = false;
};

class LongSession : public stappler::data::WrapperTemplate<mem::Interface> {
public:
	static constexpr auto SessionCookie = "LongToken";

	static LongSession *acquire(const Request &rctx);
	static LongSession *acquire(const Request &rctx, const SessionKeys &);

	static LongSession *create(const Request &rctx, uint64_t uid);
	static LongSession *create(const Request &rctx, const SessionKeys &, uint64_t uid);

	void assignUser(uint64_t);
	void cancel();

	uint64_t getUser() const;
	const Bytes &getSignature() const;

protected:
	string::Sha512::Buf getUserFingerprint(const Request &, Time);

	LongSession(const Request &, const SessionKeys &);
	LongSession(const Request &, const SessionKeys &, uint64_t uid);

	Request _request;
	SessionKeys _keys;
	uint64_t _user = 0;
	Bytes _sig;
};


class AuthToken : public stappler::data::WrapperTemplate<mem::Interface> {
public:
	static AuthToken *parse(StringView token, Request, SessionKeys = SessionKeys());
	static AuthToken *parse(StringView token, BytesView fp, StringView iss, StringView aud = StringView(), SessionKeys = SessionKeys());

	static AuthToken *create(SessionKeys = SessionKeys());
	//static AuthToken *create(BytesView fp, StringView iss, StringView aud, TimeInterval maxage = config::getInternalsStorageTime(), SessionKeys = SessionKeys(), StringView sub = StringView());

	String exportToken(Request &req, TimeInterval maxage, StringView sub) const;

protected:
	static string::Sha512::Buf getFingerprint(const Request &rctx, Time t, StringView secret);
	static string::Sha512::Buf getFingerprint(BytesView, Time t, StringView secret);

	Bytes encryptAes(const string::Sha256::Buf &, const data::Value &) const;
	static data::Value decryptAes(const string::Sha256::Buf &, BytesView);
	static string::Sha256::Buf makeAesKey(BytesView, StringView priv);

	AuthToken(SessionKeys keys);
	AuthToken(data::Value &&, SessionKeys keys);

	SessionKeys _keys;
};

NS_SA_END

#endif /* STELLATOR_UTILS_STEXTERNALSESSION_H_ */
