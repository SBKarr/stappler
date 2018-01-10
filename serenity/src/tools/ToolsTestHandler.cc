/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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
#include "StorageScheme.h"
#include "User.h"

#include "SPBitmap.h"

NS_SA_EXT_BEGIN(tools)

TestHandler::TestHandler() : DataHandler() {
	_maxRequestSize = 1_MiB;
	_maxFileSize = 1_MiB;
}
bool TestHandler::isRequestPermitted(Request &rctx) {
	if (_subPath == "/image") {
		_required |= InputConfig::Require::Files;
	}
	return true;
}
bool TestHandler::processEmailTest(Request &rctx, data::Value &ret, const data::Value &input) {
	if (!input.hasValue("email")) {
		data::Value emails;

		emails.addString("prettyandsimple@example.com");
		emails.addString("йакреведко@упячка.рф");
		emails.addString("very.common@example.com");
		emails.addString("disposable.style.email.with+symbol@example.com");
		emails.addString("other.email-with-dash@example.com");
		emails.addString("\"much.more unusual\"@example.com");
		emails.addString("\"very.unusual.@.unusual.com\"@example.com");
		emails.addString("\"very.(),:;<>[]\\\".VERY.\\\"very@\\\\ \\\"very\\\".unusual\"@strange.example.com");
		emails.addString("admin@mailserver1");
		emails.addString("#!$%&'*+-/=?^_`{}|~@example.org");
		emails.addString("\"()<>[]:,;@\\\"!#$%&'*+-/=?^_`{}| ~.a\"@example.org");
		emails.addString("\" \"@example.org");
		emails.addString("example@localhost");
		emails.addString("example@s.solutions");
		emails.addString("user@com");
		emails.addString("user@localserver");
		emails.addString("user@[IPv6:2001:db8::1]");
		emails.addString(" (test comment) user(test comment)@(test comment)localserver(test comment) ");

		// should fail
		emails.addString("Abc.failed.com");
		emails.addString("A@b@c@failed.com");
		emails.addString("a\"b(c)d,e:f;g<h>i[j\\k]l@failed.com"); // (none of the special characters in this local part are allowed outside quotation marks)
		emails.addString("just\"not\"right@failed.com"); // (quoted strings must be dot separated or the only element making up the local-part)
		emails.addString("this is\"not\allowed@failed.com"); // (spaces, quotes, and backslashes may only exist when within quoted strings and preceded by a backslash)
		emails.addString("this\\ still\\\"not\\allowed@failed.com"); // (even if escaped (preceded by a backslash), spaces, quotes, and backslashes must still be contained by quotes)
		emails.addString("john..doe@failed.com"); // (double dot before @)
		emails.addString("john.doe@failed..com");

		ret.setValue(emails, "emails_original");

		for (auto &it : emails.asArray()) {
			if (!valid::validateEmail(it.getString())) {
				it.setBool(false);
			}
		}

		ret.setValue(std::move(emails), "emails_vaildated");
	} else {
		auto original = input.getString("email");
		data::Value email;
		email.setString(original, "source");

		email.setBool(valid::validateEmail(original), "valid");
		email.setString(original, "target");
		ret.setValue(std::move(email), "email");
	}
	return true;
}

bool TestHandler::processUrlTest(Request &rctx, data::Value &ret, const data::Value &input) {
	if (!input.hasValue("url")) {

		String testString("https://йакреведко.рф/test/.././..////?query[креведко][treas][ds][]=qwert#аяклешня");
		Url testUrl(testString);
		ret.setValue(testUrl.encode(), "urls");
		ret.setString(testUrl.get(), "urls_get");

		data::Value urls;

		urls.addString("localhost");
		urls.addString("localhost:8080");
		urls.addString("localhost/test1/test2");
		urls.addString("/usr/local/bin/bljad");
		urls.addString("https://localhost/test/.././..////?query[test][treas][ds][]=qwert#rest");
		urls.addString("http://localhost");
		urls.addString("йакреведко.рф");
		urls.addString("https://йакреведко.рф/test/.././..////?query[креведко][treas][ds][]=qwert#аяклешня");
		urls.addString("ssh://git@atom0.stappler.org:21002/StapplerApi.git?qwr#test");

		ret.setValue(urls, "urls_original");

		for (auto &it : urls.asArray()) {
			if (!valid::validateUrl(it.getString())) {
				it.setBool(false);
			}
		}

		ret.setValue(std::move(urls), "urls_vaildated");
	} else {
		auto original = input.getString("url");
		data::Value url;
		url.setString(original, "source");

		url.setBool(valid::validateUrl(original), "valid");
		url.setString(original, "target");
		ret.setValue(std::move(url), "url");
	}
	return true;
}

bool TestHandler::processUserTest(Request &rctx, data::Value &ret, const data::Value &input) {
	auto adapter = rctx.storage();
	auto &d = rctx.getParsedQueryArgs();

	if (d.getBool("check")) {
		auto user = User::get(adapter, input.getString("name"), input.getString("password"));
		if (user) {
			auto uval = ret.newDict("user");
			for (auto &it : (*user)) {
				uval.setValue(it.second, it.first);
			}
		} else {
			ret.setBool(false, "user");
		}
	} else if (d.getBool("all")) {
		auto s = rctx.server().getUserScheme();
		auto data = s->select(adapter, storage::Query::all());

		ret.setValue(std::move(data), "users");
	} else {
		auto passwd = input.getString("password");
		data::Value user(input);
		auto s = rctx.server().getUserScheme();
		user = s->create(adapter, user);

		auto pswd = base64::decode(
			"AAEFBp6epLB1iBwFWarx6ZKwghFXNaFVQsZ8wblJNvgCxjRe2z1JvigC1IliAFIkHT5NEbNJSUFcHl1r7CroDBIch9KM/087Qp3+ACZOGmg=");
		auto val = valid::validatePassord(passwd, user.getBytes("password"), config::getDefaultPasswordSalt());
		ret.setBool(val, "res");

		ret.setValue(std::move(user), "user");
	}
	return true;
}

bool TestHandler::processImageTest(Request &rctx, data::Value &ret, const data::Value &input, InputFile & f) {
	ret.setString(f.type, "type");
	ret.setString(f.name, "name");
	ret.setString(f.original, "original");
	ret.setString(f.path, "path");
	if (f.type == "image/gif"
		|| f.type == "image/jpeg"
		|| f.type == "image/pjpeg"
		|| f.type == "image/png"
		|| f.type == "image/tiff"
		|| f.type == "image/webp") {
		size_t width = 0, height = 0;
		if (Bitmap::getImageSize(f.file, width, height)) {
			ret.setValue(data::Value{
				std::make_pair("width", data::Value(width)),
				std::make_pair("height", data::Value(height))
			}, "size");
			return true;
		}
	}
	return false;
}

bool TestHandler::processDataHandler(Request &rctx, data::Value &ret, data::Value &input) {
	ret.setString(_subPath, "sub_path");
	ret.setValue(input, "input");

	if (_subPath == "/email") {
		return processEmailTest(rctx, ret, input);
	} else if (_subPath == "/url") {
		return processUrlTest(rctx, ret, input);
	} else if (_subPath == "/user") {
		return processUserTest(rctx, ret, input);
	} else if (_subPath == "/image") {
		if (_filter) {
			auto &files = _filter->getFiles();
			if (files.size() == 1) {
				return processImageTest(rctx, ret, input, files.at(0));
			}
		}
	}

	return false;
}

NS_SA_EXT_END(tools)
