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

#include "STUsersProvider.h"
#include "STUsersComponent.h"
#include "Networking.h"

NS_SA_BEGIN

class OidcProvider : public Provider {
public:
	OidcProvider(const StringView &str, const data::Value &data) : Provider(str, data) {
		if (data.isString("discovery")) {
			_discoveryUrl = data.getString("discovery");
		}
		if (data.isArray("external_ids")) {
			for (auto &it : data.getArray("external_ids")) {
				_externaIds.emplace_back(it.asString());
			}
		}
	}

	virtual String onLogin(const Request &rctx, const StringView &state, const StringView &redirectUrl) const override {
		String endpoint;
		if (auto doc = _discovery) {
			endpoint = doc->getAuthorizationEndpoint();
		}
		if (endpoint.empty()) {
			messages::error("OauthGoogleHandler", "invalid `authorization_endpoint`");
			return String();
		}

		StringStream url;
		url << endpoint << "?client_id=" << _clientId << "&response_type=code&scope=openid%20email%20profile&"
				"redirect_uri=" << redirectUrl << "&"
				"state=" << state;

		return url.str();
	}

	virtual String onTry(const Request &rctx, const data::Value &data, const StringView &state, const StringView &redirectUrl) const override {
		String endpoint;
		if (auto doc = _discovery) {
			endpoint = doc->getAuthorizationEndpoint();
		}
		if (endpoint.empty()) {
			messages::error("OauthGoogleHandler", "invalid `authorization_endpoint`");
			return String();
		}

		StringStream url;
		url << endpoint << "?client_id=" << _clientId << "&response_type=code&scope=openid%20email%20profile&"
				"redirect_uri=" << redirectUrl << "&"
				"state=" << state << "&prompt=none";

		if (data && data.isString("id")) {
			url << "&login_hint=" << data.getString("id");
		}

		return url.str();
	}

	virtual uint64_t onResult(const Request &rctx, const StringView &redirectUrl) const override {
		auto doc = _discovery;
		if (!doc) {
			messages::error("OauthGoogleHandler", "OpenID discovery document not loaded");
			return 0;
		}

		auto endpoint = doc->getTokenEndpoint();
		if (endpoint.empty()) {
			messages::error("OauthGoogleHandler", "`token_endpoint` not found in OpenID discovery document");
			return 0;
		}

		auto &args = rctx.getParsedQueryArgs();

		StringStream stream;
		stream << "code=" << args.getString("code") << "&"
				"client_id=" << _clientId << "&"
				"client_secret=" << _clientSecret << "&"
				"redirect_uri=" << redirectUrl << "&"
				"grant_type=authorization_code";

		network::Handle h(network::Handle::Method::Post, endpoint);
		h.addHeader("Content-Type: application/x-www-form-urlencoded");
		h.setSendData(stream.weak());

		auto data = h.performDataQuery();

		if (data && data.isString("id_token")) {
			JsonWebToken token(data.getString("id_token"));
			if (auto key = doc->getKey(token.kid)) {
				if (token.validate(*key)) {
					auto sub = token.payload.getString("sub");
					if (sub.empty()) {
						messages::error("UserDatabase", "Empty sub key", data::Value{
							pair("sub", data::Value(sub)),
							pair("url", data::Value(redirectUrl)),
						});
						return 0;
					}

					auto patch = makeUserPatch(token);
					patch.setString(_name, "provider");

					if (auto user = authUser(rctx, sub, move(patch))) {
						return uint64_t(user.getInteger("__oid"));
					} else {
						messages::error("UserDatabase", "Fail to authorize user", data::Value{
							pair("sub", data::Value(sub)),
							pair("patch", data::Value(patch)),
							pair("url", data::Value(redirectUrl)),
						});
						return 0;
					}
				}
			}
		}

		messages::error("UserDatabase", "Fail read auth data", data::Value{
			pair("data", data::Value(data)),
			pair("url", data::Value(redirectUrl)),
		});
		return 0;
	}

	virtual uint64_t onToken(const Request &rctx, const StringView &tokenStr) const override {
		auto doc = _discovery;
		if (!doc) {
			messages::error("OauthGoogleHandler", "OpenID discovery document not loaded");
			return 0;
		}

		if (!tokenStr.empty()) {
			JsonWebToken token(tokenStr);
			if (auto key = doc->getKey(token.kid)) {
				if (token.validate(*key)) {
					bool valid = false;
					if (token.validatePayload(doc->getIssuer(), _clientId)) {
						valid = true;
					} else {
						for (auto &it : _externaIds) {
							if (token.validatePayload(doc->getIssuer(), it)) {
								valid = true;
								break;
							}
						}
					}

					if (!valid) {
						messages::error("OauthGoogleHandler", "Fail to validate token payload");
						return 0;
					}

					auto sub = token.payload.getString("sub");
					if (sub.empty()) {
						return HTTP_INTERNAL_SERVER_ERROR;
					}

					auto patch = makeUserPatch(token);
					if (auto user = authUser(rctx, sub, move(patch))) {
						return uint64_t(user.getInteger("__oid"));
					} else {
						return 0;
					}
				} else {
					messages::error("OauthGoogleHandler", "Fail to validate token signing");
				}
			} else {
				messages::error("OauthGoogleHandler", "No key for kid", data::Value(token.kid));
			}
		} else {
			messages::error("OauthGoogleHandler", "Invalid token");
		}
		return 0;
	}

	virtual void update(Mutex &mutex) override {
		network::Handle h(network::Handle::Method::Get, _discoveryUrl);
		auto discoveryDoc = h.performDataQuery();

		auto ptr = DiscoveryDocument::create<DiscoveryDocument>(Server(mem::server()).getProcessPool(), [&] (DiscoveryDocument &doc) {
			doc.init(discoveryDoc);
		});

		std::unique_lock<Mutex> lock(mutex);
		_discovery = ptr;
	}

protected:
	String _discoveryUrl;
	Vector<String> _externaIds;
	Rc<DiscoveryDocument> _discovery;
};

NS_SA_END
