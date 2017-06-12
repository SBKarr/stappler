// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "Define.h"
#include "Tools.h"
#include "User.h"
#include "Session.h"

NS_SA_EXT_BEGIN(tools)

inline TimeInterval getMaxAuthTimeInSeconds() { return config::getMaxAuthTime(); }

bool AuthHandler::isRequestPermitted(Request &rctx) {
	if (_subPath != "/login" && _subPath != "/update" && _subPath != "/cancel" && _subPath != "/setup" && _subPath != "/basic") {
		rctx.setStatus(HTTP_NOT_FOUND);
		return false;
	}
	if (_subPath != "/login" && _subPath != "/setup" && _subPath != "/basic" && rctx.getUser() == nullptr) {
		messages::error("Auth", "You are not logged in");
		return false;
	}
	_allow = AllowMethod::Get;
	return rctx.getMethod() == Request::Get;
}

bool AuthHandler::processDataHandler(Request &rctx, data::Value &result, data::Value &input) {
	auto &queryData = rctx.getParsedQueryArgs();

	if (_subPath == "/login") {
		auto &name = queryData.getString("name");
		auto &passwd = queryData.getString("passwd");
		if (name.empty() || passwd.empty()) {
			messages::error("Auth", "Name or password is not specified", data::Value{
				std::make_pair("Doc", data::Value("You should specify 'name' and 'passwd' variables in request"))
			});
			return false;
		}

		TimeInterval maxAge = TimeInterval::seconds(queryData.getInteger("maxAge"));
		if (!maxAge || maxAge > getMaxAuthTimeInSeconds()) {
			maxAge = getMaxAuthTimeInSeconds();
		}

		auto user = User::get(rctx.storage(), name, passwd);
		if (!user) {
			messages::error("Auth", "Invalid username or password");
			return false;
		}

		auto &opts = getOptions();
		bool isAuthorized = false;
		if (!opts.getBool("AdminOnly") || user->isAdmin()) {
			isAuthorized = true;
		}

		if (isAuthorized) {
			Session *session = _request.authorizeUser(user, maxAge);
			if (session) {
				auto &token = session->getSessionToken();
				result.setString(base64::encode(CoderSource(token.data(), token.size())), "token");
				result.setInteger(maxAge.toSeconds(), "maxAge");
				result.setInteger(user->getObjectId(), "userId");
				result.setString(user->getName(), "userName");
				return true;
			}
		}

		messages::error("Auth", "Fail to create session");

		return false;
	} else if (_subPath == "/update") {
		if (auto session = rctx.getSession()) {
			User *user = session->getUser();
			if (!user) {
				return false;
			}
			TimeInterval maxAge = TimeInterval::seconds(rctx.getParsedQueryArgs().getInteger("maxAge"));
			if (!maxAge || maxAge > getMaxAuthTimeInSeconds()) {
				maxAge = getMaxAuthTimeInSeconds();
			}

			if (session->touch(maxAge)) {
				auto &token = session->getSessionToken();
				result.setString(base64::encode(CoderSource(token.data(), token.size())), "token");
				result.setInteger(maxAge.toSeconds(), "maxAge");
				result.setInteger(user->getObjectId(), "userId");
				result.setString(user->getName(), "userName");
				return true;
			}
		}

		return false;
	} else if (_subPath == "/cancel") {
		if (auto session = rctx.getSession()) {
			return session->cancel();
		}
		return false;
	} else if (_subPath == "/setup") {
		auto &name = queryData.getString("name");
		auto &passwd = queryData.getString("passwd");

		if (name.empty() || passwd.empty()) {
			messages::error("Auth", "Name or password is not specified", data::Value{
				std::make_pair("Doc", data::Value("You should specify 'name' and 'passwd' variables in request"))
			});
			return false;
		}

		auto user = User::setup(rctx.storage(), name, passwd);
		if (user) {
			data::Value &u = result.emplace("user");
			for (auto &it : *user) {
				u.setValue(it.second, it.first);
			}
			return true;
		} else {
			messages::error("Auth", "Setup failed");
			return false;
		}
	}
	return false;
}

NS_SA_EXT_END(tools)
