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

NS_SA_BEGIN

class LocalProvider : public Provider {
public:
	LocalProvider(const StringView &str, const data::Value &data) : Provider(str, data) { }

	virtual String onLogin(const Request &rctx, const StringView &state, const StringView &redirectUrl) const override {
		return String();
	}

	virtual String onTry(const Request &rctx, const data::Value &data, const StringView &state, const StringView &redirectUrl) const override {
		return String();
	}

	virtual uint64_t onResult(const Request &rctx, const StringView &redirectUrl) const override {
		auto &args = rctx.getParsedQueryArgs();
		if (args.isString("id_token")) {
			auto h = rctx.server().getComponent<UsersComponent>();

			auto iss = rctx.getFullHostname();

			JsonWebToken token(args.getString("id_token"));
			if (token.validate(JsonWebToken::RS512, h->getPublicKey()) && token.validatePayload(iss, iss)) {
				auto sub = token.payload.getString("sub");
				auto patch = makeUserPatch(token);
				patch.setString(_name, "provider");

				if (auto user = authUser(rctx, sub, move(patch))) {
					return uint64_t(user.getInteger("__oid"));
				} else {
					return 0;
				}
			}
		}
		return 0;
	}

	virtual uint64_t onToken(const Request &rctx, const StringView &tokenStr) const override {
		return 0;
	}

	virtual data::Value makeProviderPatch(data::Value &&val) const override {
		data::Value ret(Provider::makeProviderPatch(move(val)));
		ret.setBool(ret.getBool("isActivated"), "emailValidated");
		return ret;
	}

	virtual String getUserId(const data::Value &val) const override {
		return toString("local-", val.getInteger("__oid"));
	}
};

NS_SA_END
