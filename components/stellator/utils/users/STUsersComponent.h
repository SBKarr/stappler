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

#ifndef STELLATOR_SERVER_STSERVERCOMPONENT_H_
#define STELLATOR_SERVER_STSERVERCOMPONENT_H_

#include "STServerComponent.h"
#include "STUsersProvider.h"
#include "ExternalSession.h"

NS_SA_BEGIN

class UsersComponent : public ServerComponent {
public:
	using String = mem::String;
	using StringView = mem::StringView;

	static String makeStateToken();
	static mem::Value getFullName(const mem::Value &);

	UsersComponent(Server &serv, const String &name, const mem::Value &dict);
	virtual ~UsersComponent() { }

	virtual void onChildInit(Server &) override;
	virtual void onStorageTransaction(db::Transaction &) override;

	String onLogin(const Request &rctx, const StringView &provider, const StringView &state, const StringView &redirectUrl) const;
	String onTry(const Request &rctx, const StringView &provider, const mem::Value &data, const StringView &state, const StringView &redirectUrl) const;
	uint64_t onToken(const Request &rctx, const StringView &provider, const StringView &token);
	uint64_t onResult(const Request &rctx, const StringView &provider, const StringView &redirectUrl) const;

	void sendActivationEmail(StringView hostname, const mem::Value &newUser) const;
	void sendDropPasswordEmail(StringView hostname, const mem::Value &user) const;

	bool isLongTermAuthorized(Request &rctx, uint64_t uid) const;
	bool isSessionAuthorized(Request &rctx, uint64_t uid) const;

	bool initLongTerm(const Request &rctx, uint64_t uid) const;

	bool pushAuthUser(const Request &rctx, uint64_t uid) const;

	Provider *getProvider(const StringView &) const;
	Provider *getLocalProvider() const;

	StringView getPrivateKey() const;
	StringView getPublicKey() const;
	SessionKeys getKeyPair() const;

	StringView getCaptchaSecret() const;

	String getBindAuth() const;
	String getBindLocal() const;
	mem::Value getMailConfig() const;

	int64_t isValidAppRequest(Request &) const; // returns application id
	mem::Value isValidSecret(Request &, const mem::Value &) const;

	const Scheme & getApplicationScheme() const;
	const Scheme & getExternalUserScheme() const;
	const Scheme & getProvidersScheme() const;
	const Scheme & getLocalUserScheme() const;
	const Scheme & getConnectionScheme() const;

	mem::Value getExternalUserByEmail(const StringView &) const;
	mem::Value getLocaUserByName(const StringView &) const;
	int64_t getLocalUserId(int64_t externalId) const;

	bool verifyCaptcha(const StringView &captchaToken) const;

	mem::Value makeConnection(Request &rctx, int64_t userId, int64_t appId, const StringView &token);

	void onNewUser(const mem::Value &);

protected:
	void updateDiscovery();

	mem::Value createUserCmd(const StringView &);
	mem::Value addApplicationCmd(const StringView &);

	mem::Value attachUser(int64_t local, int64_t extarnal);

	mem::Value updateUserEmail(const db::Transaction &, int64_t id, StringView email) const;
	bool findUsers(const db::Transaction &, StringView cmd, const mem::Callback<void(const mem::Value &)> &cb) const;
	bool removeUser(const db::Transaction &, int64_t, const mem::Callback<void(const mem::Value &)> &cb) const;

	Scheme _externalUsers = Scheme("users_external");
	Scheme _authProviders = Scheme("users_auth_providers");
	Scheme _localUsers = Scheme("users_local");
	Scheme _connections = Scheme("users_connections");
	Scheme _applications = Scheme("users_applications");

	String _privateKey;
	String _publicKey;
	String _captchaUrl;
	String _captchaSecret;
	String _prefix;

	mutable mem::Mutex _mutex;

	mem::Value _mailConfig;
	mem::Map<String, Provider *> _providers;
	Provider *_localProvider = nullptr;

	mem::Function<void(const mem::Value &)> _newUserCallback;
};

NS_SA_END

#endif /* COMPONENTS_SERENITY_SRC_UTILS_USERS_USERSCOMPONENT_H_ */
