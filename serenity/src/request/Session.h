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

#ifndef SERENITY_SRC_REQUEST_SESSION_H_
#define SERENITY_SRC_REQUEST_SESSION_H_

#include "Request.h"
#include "SPDataWrapper.h"

NS_SA_BEGIN

class Session : public data::Wrapper {
public:
	using Token = string::Sha512::Buf;

	static Session *create(const Request &);

	Session(const Request &);
	Session(const Request &, User *user, TimeInterval maxAge);
	~Session();

	bool init(User *user, TimeInterval maxAge);
	bool init();

	bool isValid() const;
	const Token &getSessionToken() const;
	const Token &getCookieToken() const;
	const apr::uuid &getSessionUuid() const;

	User *getUser() const;
	TimeInterval getMaxAge() const;

	bool write();
	bool save();
	bool cancel();
	bool touch(TimeInterval maxAge = TimeInterval());

protected:
	static data::Value getStorageData(Request &, const Token &);
	static data::Value getStorageData(Request &, const Bytes &);
	static bool setStorageData(Request &, const Token &, const data::Value &, TimeInterval maxAge);
	static bool clearStorageData(Request &, const Token &);
	static User *getStorageUser(Request &, uint64_t);

	static void makeSessionToken(Request &rctx, Token &buf, const apr::uuid & uuid, const String & userName);
	static void makeCookieToken(Request &rctx, Token &buf, const apr::uuid & uuid, const String & userName, const Bytes & salt);

	Request _request;

	Token _sessionToken;
	Token _cookieToken;

	apr::uuid _uuid;
	TimeInterval _maxAge;

	User *_user = nullptr;
	bool _valid = false;

	//Binary *getKey(const char *token);
};

NS_SA_END

#endif /* SERENITY_SRC_REQUEST_SESSION_H_ */
