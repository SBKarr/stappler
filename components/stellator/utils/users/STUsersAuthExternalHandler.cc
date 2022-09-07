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

#include "STUsersComponent.h"
#include "Output.h"

NS_SA_BEGIN

class AuthExternalHandlerMap : public HandlerMap {
public:
	AuthExternalHandlerMap();
};

class AuthExternalHandlerLogin : public AuthHandlerInterface {
public:
	AuthExternalHandlerLogin() { }

	virtual int onRequest() override {
		auto &args = _request.getParsedQueryArgs();
		auto &prov = args.getString("provider");
		auto &storage = args.getString("storage");

		auto t = args.getString("target");
		if (!t.empty()) {
			stappler::UrlView v(t);
			if (v.host == _request.getHostname() || v.host.empty()) {
				t = v.get();
			} else {
				t.clear();
			}
		}

		String state(storage.empty() ? UsersComponent::makeStateToken() : StringView(storage).starts_with("auth-") ? storage.substr("auth-"_len) : storage);
		String redirectUrl = _component->onLogin(_request, prov, state, getExternalRedirectUrl());

		if (!redirectUrl.empty()) {
			if (!initStateToken(state, prov, storage.empty(), t)) {
				return HTTP_NOT_FOUND;
			}
			return _request.redirectTo(move(redirectUrl));
		} else {
			return _request.redirectTo(move(getErrorUrl("auth_invalid_provider")));
		}
	}
};

class AuthExternalHandlerTry : public AuthHandlerInterface {
public:
	AuthExternalHandlerTry() { }

	virtual int onRequest() override {
		String state(UsersComponent::makeStateToken());
		String provider;
		String redirectUrl;
		if (auto session = ExternalSession::get(_request, _component->getKeyPair())) {
			if (auto user = session->getUser()) {
				return _request.redirectTo(move(getSuccessUrl(user)));
			}
		}

		if (auto longSession = LongSession::acquire(_request, _component->getKeyPair())) {
			if (auto user = db::Worker(_component->getExternalUserScheme(), _transaction).asSystem().get(longSession->getUser())) {
				if (auto prov = db::Worker(_component->getProvidersScheme(), _transaction).asSystem().get(user.getInteger("lastProvider"))) {
					provider = prov.getString("provider");
					redirectUrl = _component->onTry(_request, provider, data::Value(), state, getExternalRedirectUrl());
				}
			}
		}

		if (redirectUrl.empty()) {
			auto &args = _request.getParsedQueryArgs();
			provider = args.getString("provider");

			redirectUrl = _component->onTry(_request, provider, data::Value(), state, getExternalRedirectUrl());
		}

		auto successUrl =  toString(_originPath, "/success");
		if (StringView(redirectUrl).starts_with(StringView(successUrl))) {
			StringView r(redirectUrl); r += successUrl.size() + "?user="_len;
			if (auto userId = r.readInteger().get()) {
				if (_component->pushAuthUser(_request, userId)) {
					return _request.redirectTo(move(getSuccessUrl(userId)));
				}
			}
			return _request.redirectTo(move(getErrorUrl("auth_invalid_provider")));
		}

		auto &args = _request.getParsedQueryArgs();
		auto t = args.getString("target");
		if (!t.empty()) {
			UrlView v(t);
			if (v.host == _request.getHostname() || v.host.empty()) {
				t = v.get();
			} else {
				t.clear();
			}
		}

		if (!redirectUrl.empty()) {
			initStateToken(state, provider, true, t);
			return _request.redirectTo(move(redirectUrl));
		} else {
			return _request.redirectTo(move(getErrorUrl("auth_invalid_provider")));
		}
	}
};


class AuthExternalHandlerCallback : public AuthHandlerInterface {
public:
	AuthExternalHandlerCallback() { }

	virtual int onRequest() override {
		auto &args = _request.getParsedQueryArgs();
		String provider;
		String type;
		data::Value stateData;
		auto &state = args.getString("state");
		if (args.isString("provider")) {
			provider = args.getString("provider");
		} else {
			if (state.empty()) {
				return _request.redirectTo(move(getErrorUrl("auth_invalid_state")));
			}

			auto dataKey = toString("auth-", state);
			stateData = _transaction.getAdapter().get(dataKey);
			if (!stateData.isDictionary()) {
				return _request.redirectTo(move(getErrorUrl("auth_invalid_state")));
			}

			provider = stateData.getString("provider");
			type = stateData.getString("type");
			if (type.empty()) {
				_transaction.getAdapter().clear(dataKey);
			}
		}

		if (provider.empty()) {
			return _request.redirectTo(move(getErrorUrl("auth_invalid_state")));
		}

		if (type.empty()) {
			auto target = stateData.getString("target");
			if (!target.empty()) {
				UrlView v(target);
				if (v.host == _request.getHostname() || v.host.empty()) {
					String query;
					if (auto user = processResult(provider, String())) {
						if (v.query.empty()) {
							query = toString("user=", user);
						} else {
							query = toString(v.query, "&user=", user);
						}
					} else {
						if (v.query.empty()) {
							query = "error=auth_failed";
						} else {
							query = toString(v.query, "&error=", "auth_failed");
						}
					}
					v.query = query;
					return _request.redirectTo(v.get());
				} else {
					return HTTP_FORBIDDEN;
				}
			} else {
				if (auto user = processResult(provider, String())) {
					return _request.redirectTo(move(getSuccessUrl(user)));
				} else {
					return _request.redirectTo(move(getErrorUrl("auth_failed")));
				}
			}
		} else {
			auto success = stateData.getString("success");
			auto failure = stateData.getString("failure");

			if (auto user = processResult(provider, toString("auth-", state))) {
				return _request.redirectTo(success.empty() ? move(getSuccessUrl(user)) : move(success));
			} else {
				return _request.redirectTo(failure.empty() ? move(getErrorUrl("auth_failed")) : move(failure));
			}
		}
	}

	uint64_t processResult(const StringView &provider, const StringView &storage) {
		if (auto user = _component->onResult(_request, provider, getExternalRedirectUrl())) {
			if (storage.empty()) {
				if (_component->pushAuthUser(_request, user)) {
					return user;
				}
			} else {
				if (auto d = _transaction.getAdapter().get(storage)) {
					d.setInteger(user, "user");
					_transaction.getAdapter().set(storage, d, 300_sec);
					return user;
				}
			}
		}
		return 0;
	}
};

class AuthExternalHandlerCancel : public AuthHandlerInterface {
public:
	AuthExternalHandlerCancel() { }

	virtual int onRequest() override {
		if (auto longSession = LongSession::acquire(_request, _component->getKeyPair())) {
			longSession->cancel();
		}

		if (auto session = ExternalSession::get(_request, _component->getKeyPair())) {
			session->cancel();
		}

		auto &target = _request.getParsedQueryArgs().getValue("target");
		if (target.isString()) {
			if (StringView(target.getString()).starts_with(StringView(_request.getFullHostname()))) {
				return _request.redirectTo(String(target.getString()));
			}
		}
		return HTTP_NOT_IMPLEMENTED;
	}
};

class AuthExternalHandlerExit : public AuthHandlerInterface {
public:
	AuthExternalHandlerExit() { }

	virtual int onRequest() override {
		if (auto session = ExternalSession::get(_request, _component->getKeyPair())) {
			session->cancel();
		}

		auto &target = _request.getParsedQueryArgs().getValue("target");
		if (target.isString()) {
			if (StringView(target.getString()).starts_with(StringView(_request.getFullHostname()))) {
				return _request.redirectTo(String(target.getString()));
			}
		}
		return HTTP_NOT_IMPLEMENTED;
	}
};

class AuthExternalHandlerUser : public AuthHandlerInterface {
public:
	AuthExternalHandlerUser() { }

	virtual bool perform(data::Value &result) const override {
		if (auto session = ExternalSession::get(_request, _component->getKeyPair())) {
			if (auto user = session->getUser()) {
				if (auto u = db::Worker(_component->getExternalUserScheme(), _transaction).asSystem().get(user)) {
					result.setValue(move(u), "user");
					return true;
				}
			}
		}
		return false;
	}
};

class AuthExternalHandlerProviders : public AuthHandlerInterface {
public:
	AuthExternalHandlerProviders() { }

	virtual bool perform(data::Value &result) const override {
		if (auto session = ExternalSession::get(_request, _component->getKeyPair())) {
			if (auto user = session->getUser()) {
				if (auto data = db::Worker(_component->getExternalUserScheme(), _transaction).asSystem().getField(user, "providers")) {
					result.setValue(move(data), "providers");
					return true;
				}
			}
		}
		return false;
	}
};

class AuthExternalHandlerLastProvider : public AuthHandlerInterface {
public:
	AuthExternalHandlerLastProvider() { }

	virtual bool perform(data::Value &result) const override {
		if (auto session = ExternalSession::get(_request, _component->getKeyPair())) {
			if (auto user = session->getUser()) {
				if (auto data = db::Worker(_component->getExternalUserScheme(), _transaction).asSystem().getField(user, "lastProvider")) {
					result.setValue(move(data), "provider");
					return true;
				}
			}
		}
		return false;
	}
};

class AuthExternalHandlerOnConnection : public AuthHandlerInterface {
public:
	AuthExternalHandlerOnConnection() { }

	virtual int onRequest() override {
		auto &queryData = _request.getParsedQueryArgs();
		auto &userStorage = queryData.getString("storage");

		if (auto d = _transaction.getAdapter().get(userStorage)) {
			auto userId = d.getInteger("user");
			if (auto u = db::Worker(_component->getExternalUserScheme(), _transaction).asSystem().get(userId)) {
				auto hash = string::Sha512::perform(_request.getUseragentIp(),
						_request.getRequestHeaders().at("user-agent"), Time::now().toHttp(), valid::makeRandomBytes(16));

				d.setBytes(Bytes(hash.data(), hash.data() + hash.size()), "secret");
				_transaction.getAdapter().set(userStorage, d, 300_sec);
				return _request.redirectTo(toString(_component->getBindAuth(), "connected?secret=", base64url::encode(hash), "&storage=", userStorage));
			}
		}
		return HTTP_NOT_FOUND;
	}
};

class AuthExternalHandlerTouch : public AuthHandlerInterface {
public:
	AuthExternalHandlerTouch() { }

	virtual int onRequest() override {
		if (auto s = ExternalSession::get(_request)) {
			auto tok = s->exportToken();
			_request.setCookie(ExternalSession::SessionCookie, tok);
			if (_request.getParsedQueryArgs().getBool("token")) {
				return output::writeResourceData(_request, data::Value(true), data::Value({
					pair("token", data::Value(s->exportToken())),
					pair("user", data::Value(s->getUser()))
				}));
			} else {
				return output::writeResourceData(_request, data::Value(true), data::Value({
					pair("user", data::Value(s->getUser()))
				}));
			}
			return DONE;
		}
		return HTTP_NOT_FOUND;
	}
};

AuthExternalHandlerMap::AuthExternalHandlerMap() {
	addHandler("Login", Request::Method::Get, "/login", SA_HANDLER(AuthExternalHandlerLogin));
	addHandler("Try", Request::Method::Get, "/try", SA_HANDLER(AuthExternalHandlerTry));
	addHandler("Callback", Request::Method::Get, "/callback", SA_HANDLER(AuthExternalHandlerCallback));
	addHandler("Cancel", Request::Method::Get, "/cancel", SA_HANDLER(AuthExternalHandlerCancel));
	addHandler("Exit", Request::Method::Get, "/exit", SA_HANDLER(AuthExternalHandlerExit));
	addHandler("User", Request::Method::Get, "/user", SA_HANDLER(AuthExternalHandlerUser));
	addHandler("Providers", Request::Method::Get, "/providers", SA_HANDLER(AuthExternalHandlerProviders));
	addHandler("LastProvider", Request::Method::Get, "/last_provider", SA_HANDLER(AuthExternalHandlerLastProvider));
	addHandler("OnConnection", Request::Method::Get, "/on_connection", SA_HANDLER(AuthExternalHandlerOnConnection));
	addHandler("Touch", Request::Method::Get, "/touch", SA_HANDLER(AuthExternalHandlerTouch));
	//addHandler("Success", Request::Method::Get, "/success", SA_HANDLER(AuthExternalHandlerSuccess));
}

NS_SA_END
