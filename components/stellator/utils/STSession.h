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

#ifndef STELLATOR_UTILS_STSESSION_H_
#define STELLATOR_UTILS_STSESSION_H_

#include "Request.h"
#include "SPDataWrapper.h"

NS_SA_ST_BEGIN

class Session : public stappler::data::WrapperTemplate<mem::Interface> {
public:
	using Token = stappler::string::Sha512::Buf;

	static Session *create(const Request &);

	Session(const Request &, bool silent = false);
	Session(const Request &, db::User *user, mem::TimeInterval maxAge);
	~Session();

	bool init(db::User *user, mem::TimeInterval maxAge);
	bool init(bool silent);

	bool isValid() const;
	const Token &getSessionToken() const;
	const Token &getCookieToken() const;
	const mem::uuid &getSessionUuid() const;

	db::User *getUser() const;
	mem::TimeInterval getMaxAge() const;

	bool write();
	bool save();
	bool cancel();
	bool touch(mem::TimeInterval maxAge = mem::TimeInterval());

protected:
	static mem::Value getStorageData(Request &, const Token &);
	static mem::Value getStorageData(Request &, const Bytes &);
	static bool setStorageData(Request &, const Token &, const mem::Value &, mem::TimeInterval maxAge);
	static bool clearStorageData(Request &, const Token &);
	static db::User *getStorageUser(Request &, uint64_t);

	static void makeSessionToken(Request &rctx, Token &buf, const mem::uuid & uuid, const mem::StringView & userName);
	static void makeCookieToken(Request &rctx, Token &buf, const mem::uuid & uuid, const mem::StringView & userName, const Bytes & salt);

	Request _request;

	Token _sessionToken;
	Token _cookieToken;

	mem::uuid _uuid;
	mem::TimeInterval _maxAge;

	db::User *_user = nullptr;
	bool _valid = false;
};

NS_SA_ST_END

#endif /* STELLATOR_UTILS_STSESSION_H_ */
