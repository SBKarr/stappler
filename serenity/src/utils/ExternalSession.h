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

#ifndef SERENITY_SRC_UTILS_EXTERNALSESSION_H_
#define SERENITY_SRC_UTILS_EXTERNALSESSION_H_

#include "Request.h"
#include "SPDataWrapper.h"
#include "StorageTransaction.h"

NS_SA_BEGIN

struct SessionKeyPair {
	StringView pub;
	StringView priv;
};

class ExternalSession : public data::Wrapper {
public:
	using Token = string::Sha512::Buf;
	using AccessRoleId = storage::AccessRoleId;

	static constexpr auto SessionCookie = "SessionToken";

	static ExternalSession *get();
	static ExternalSession *get(const Request &);
	static ExternalSession *get(const Request &, const SessionKeyPair &keys);

	static ExternalSession *acquire(const Request &, TimeInterval = TimeInterval::seconds(60 * 60 * 12));
	static ExternalSession *acquire(const Request &, const SessionKeyPair &keys, TimeInterval = TimeInterval::seconds(60 * 60 * 12));

	~ExternalSession();

	bool initAsNew(const apr::uuid &);
	bool init(const apr::uuid &, data::Value &&);

	void setUser(uint64_t, AccessRoleId);

	bool isValid() const;
	const apr::uuid &getUuid() const;

	uint64_t getUser() const;
	AccessRoleId getRole() const;
	data::Value getUserData() const;
	TimeInterval getMaxAge() const;

	bool save();
	bool cancel();
	bool touch(TimeInterval maxAge = TimeInterval());

protected:
	ExternalSession(const Request &, const SessionKeyPair &key);
	ExternalSession(const Request &, const SessionKeyPair &key, TimeInterval);
	ExternalSession(const Request &, const SessionKeyPair &key, const apr::uuid &, data::Value &&);

	void setCookie();

	static Token makeToken(const Request &rctx, const apr::uuid & uuid);

	Request _request;

	SessionKeyPair _keys;

	Token _token;
	apr::uuid _uuid;
	TimeInterval _maxAge;

	uint64_t _user = 0;
	AccessRoleId _role = AccessRoleId::Nobody;
	bool _valid = false;
};

class LongSession :  public data::Wrapper {
public:
	static constexpr auto SessionCookie = "LongToken";

	static LongSession *acquire(const Request &rctx);
	static LongSession *acquire(const Request &rctx, const SessionKeyPair &);

	static LongSession *create(const Request &rctx, uint64_t uid);
	static LongSession *create(const Request &rctx, const SessionKeyPair &, uint64_t uid);

	void assignUser(uint64_t);
	void cancel();

	uint64_t getUser() const;
	const Bytes &getSignature() const;

protected:
	LongSession(const Request &, const SessionKeyPair &);
	LongSession(const Request &, const SessionKeyPair &, uint64_t uid);

	Request _request;
	SessionKeyPair _keys;
	uint64_t _user = 0;
	Bytes _sig;
};

NS_SA_END

#endif /* SERENITY_SRC_UTILS_EXTERNALSESSION_H_ */
