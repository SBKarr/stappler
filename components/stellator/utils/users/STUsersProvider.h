/**
 Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef STELLATOR_UTILS_USERS_STUSERSPROVIDER_H_
#define STELLATOR_UTILS_USERS_STUSERSPROVIDER_H_

#include "JsonWebToken.h"
#include "Task.h"
#include "RequestHandler.h"
#include "STStorageUser.h"

NS_SA_BEGIN

class UsersComponent;

class DiscoveryDocument : public SharedObject {
public:
	using String = mem::String;
	using KeyData = stappler::JsonWebToken::KeyData;

	virtual bool init(const mem::Value &);

	const String &getIssuer() const;
	const String &getTokenEndpoint() const;
	const String &getAuthorizationEndpoint() const;

	const KeyData *getKey(const mem::StringView &) const;

	DiscoveryDocument(mem::pool_t *p) : SharedObject(p) { }

protected:
	String issuer;
	String authorizationEndpoint;
	String tokenEndpoint;
	String jwksEndpoint;

	mem::Map<String, KeyData> keys;
	uint64_t mtime = 0;
};

class Provider : public mem::AllocBase {
public:
	using Scheme = db::Scheme;
	using Transaction = db::Transaction;
	using String = mem::String;
	using StringView = mem::StringView;

	static Provider *create(const mem::StringView &name, const mem::Value &);

	virtual ~Provider() { }

	virtual String onLogin(const Request &rctx, const StringView &state, const StringView &redirectUrl) const = 0;
	virtual String onTry(const Request &rctx, const mem::Value &data, const StringView &state, const StringView &redirectUrl) const = 0;
	virtual uint64_t onResult(const Request &rctx, const StringView &redirectUrl) const = 0;
	virtual uint64_t onToken(const Request &rctx, const StringView &tokenStr) const = 0;

	virtual mem::Value authUser(const Request &rctx, const StringView &sub, mem::Value &&userPatch) const;

	virtual void update(mem::Mutex &);

	const mem::Value &getConfig() const;

	virtual String getUserId(const mem::Value &) const;

	virtual mem::Value initUser(int64_t currentUser, const Transaction &t, mem::Value &&userPatch) const;
	virtual mem::Value initUser(int64_t currentUser, const Transaction &t, const StringView &, mem::Value &&userPatch, int *status = nullptr) const;
	virtual mem::Value makeProviderPatch(mem::Value &&) const;

protected:
	mem::Value makeUserPatch(const stappler::JsonWebToken &token) const;

	Provider(const StringView &, const mem::Value &);

	mem::Value _config;
	String _name;
	String _type;
	String _clientId;
	String _clientSecret;
};

class AuthHandlerInterface : public HandlerMap::Handler {
public:
	using String = mem::String;
	using StringView = mem::StringView;

	AuthHandlerInterface() { }

	virtual bool isPermitted() override;
	virtual int onRequest() override;
	virtual bool perform(mem::Value &) const;

protected:
	db::User * tryUserLogin(const Request &rctx, StringView, StringView) const;
	mem::Value getLocalUser(int64_t u) const;

	String getErrorUrl(const StringView &error) const;
	String getSuccessUrl(uint64_t user) const;
	String getExternalRedirectUrl() const;

	bool initStateToken(const StringView &token, const StringView &provider, bool isNew, StringView target) const;

	const UsersComponent *_component = nullptr;
	db::Transaction _transaction = nullptr;
};

NS_SA_END

#endif /* COMPONENTS_STELLATOR_UTILS_USERS_STUSERSPROVIDER_H_ */
