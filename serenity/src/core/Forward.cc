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

#include "Forward.h"
#include "Request.h"
#include "SPString.h"
#include "Root.h"
#include "StorageAdapter.h"

#include <idna.h>
#include "SPUrl.h"

NS_SA_EXT_BEGIN(idn)

using HostUnicodeChars = chars::Compose<char, chars::CharGroup<char, CharGroupId::Alphanumeric>,
		chars::Chars<char, '.', '-'>, chars::Range<char, char(128), char(255)>>;

using HostAsciiChars = chars::Compose<char, chars::CharGroup<char, CharGroupId::Alphanumeric>,
		chars::Chars<char, '.', '-'>>;

String toAscii(const String &source, bool validate) {
	if (validate) {
		StringView r(source);
		r.skipChars<HostUnicodeChars>();
		if (!r.empty()) {
			return String();
		}
	}

	if (source.empty()) {
		return String();
	}

	char *out = nullptr;
	auto err = idna_to_ascii_8z(source.data(), &out, 0);
	if (err == IDNA_SUCCESS) {
		String ret(out);
		free(out);
		return ret;
	}
	return String();
}

String toUnicode(const String &source, bool validate) {
	if (validate) {
		StringView r(source);
		r.skipChars<HostAsciiChars>();
		if (!r.empty()) {
			return String();
		}
	}

	if (source.empty()) {
		return String();
	}

	char *out = nullptr;
	auto err = idna_to_unicode_8z8z(source.data(), &out, 0);
	if (err == IDNA_SUCCESS) {
		String ret(out);
		free(out);
		return ret;
	}
	return String();
}

NS_SA_EXT_END(idn)


NS_SA_EXT_BEGIN(valid)

/** Identifier starts with [a-zA-Z_] and can contain [a-zA-Z0-9_\-.@] */
bool validateIdentifier(const String &str) {
	if (str.empty()) {
		return false;
	}

	StringView r(str);
	if (!r.is<chars::Compose<char, chars::CharGroup<char, CharGroupId::Alphanumeric>, chars::Chars<char, '_'>>>()) {
		return false;
	}

	r.skipChars<chars::CharGroup<char, CharGroupId::Alphanumeric>, chars::Chars<char, '_', '-', '.', '@'>>();
	if (!r.empty()) {
		return false;
	}

	return true;
}

/** Text can contain all characters above 0x1F and \t, \r, \n, \b, \f */
bool validateText(const String &str) {
	if (str.empty()) {
		return false;
	}

	// 8 9 10 12 13 - allowed
	StringView r(str);
	r.skipUntil<chars::Range<char, 0, 7>, chars::Range<char, 14, 31>, chars::Chars<char, 11>>();
	if (!r.empty()) {
		return false;
	}

	return true;
}

static bool validateEmailQuotation(String &ret, StringView &r) {
	using namespace chars;

	++ r;
	ret.push_back('"');
	while (!r.empty() && !r.is('"')) {
		auto pos = r.readUntil<Chars<char, '"', '\\'>>();
		if (!pos.empty()) {
			ret.append(pos.data(), pos.size());
		}

		if (r.is('\\')) {
			ret.push_back(r[0]);
			++ r;
			if (!r.empty()) {
				ret.push_back(r[0]);
				++ r;
			}
		}
	}
	if (r.empty()) {
		return false;
	} else {
		ret.push_back('"');
		++ r;
		return true;
	}
}

bool validateEmail(String &str) {
	string::trim(str);
	if (str.back() == ')') {
		auto pos = str.rfind('(');
		if (pos != String::npos) {
			str.erase(str.begin() + pos, str.end());
		} else {
			return false;
		}
	}
	String ret; ret.reserve(str.size());

	using namespace chars;
	using LocalChars = Compose<char, CharGroup<char, CharGroupId::Alphanumeric>,
			Chars<char, '_', '-', '+', '#', '!', '$', '%', '&', '\'', '*', '/', '=', '?', '^', '`', '{', '}', '|', '~' >,
			Range<char, char(128), char(255)>>;

	using Whitespace =  CharGroup<char, CharGroupId::WhiteSpace>;

	StringView r(str);
	r.skipChars<Whitespace>();

	if (r.is('(')) {
		r.skipUntil<Chars<char, ')'>>();
		if (!r.is(')')) {
			return false;
		}
		r ++;
		r.skipChars<Whitespace>();
	}
	if (r.is('"')) {
		if (!validateEmailQuotation(ret, r)) {
			return false;
		}
	}

	while (!r.empty() && !r.is('@')) {
		auto pos = r.readChars<LocalChars>();
		if (!pos.empty()) {
			ret.append(pos.data(), pos.size());
		}

		if (r.is('.')) {
			ret.push_back('.');
			++ r;
			if (r.is('"')) {
				if (!validateEmailQuotation(ret, r)) {
					return false;
				}
				if (!r.is('.') && !r.is('@')) {
					return false;
				} else if (r.is('.')) {
					ret.push_back('.');
					++ r;
				}
			} else if (!r.is<LocalChars>()) {
				return false;
			}
		}
		if (r.is('(')) {
			r.skipUntil<Chars<char, ')'>>();
			if (!r.is(')')) {
				return false;
			}
			r ++;
			r.skipChars<Whitespace>();
			break;
		}
		if (!r.is('@') && !r.is<LocalChars>()) {
			return false;
		}
	}

	if (r.empty() || !r.is('@')) {
		return false;
	}

	ret.push_back('@');
	++ r;
	if (r.is('(')) {
		r.skipUntil<Chars<char, ')'>>();
		if (!r.is(')')) {
			return false;
		}
		r ++;
		r.skipChars<Whitespace>();
	}

	if (r.is('[')) {
		ret.push_back('[');
		auto pos = r.readUntil<Chars<char, ']'>>();
		if (r.is(']')) {
			r ++;
			if (r.empty()) {
				ret.append(pos.data(), pos.size());
				ret.push_back(']');
			}
		}
	} else {
		String host = idn::toAscii(String::make_weak(r.data(), r.size()));
		if (host.empty()) {
			return false;
		}

		ret.append(host);
	}

	str = std::move(ret);
	return true;
}

bool validateUrl(String &str) {
	Url url;
	if (!url.parse(str)) {
		return false;
	}

	auto oldHost = url.getHost();
	if (url.getHost().empty() && url.getPath().size() < 2) {
		return false;
	}

	if (!oldHost.empty()) {
		auto newHost = idn::toAscii(oldHost);
		if (newHost.empty()) {
			return false;
		}
		url.setHost(newHost);
	}

	auto newUrl = url.get();
	str = std::move(newUrl);
	return true;
}

bool validateNumber(const String &str) {
	if (str.empty()) {
		return false;
	}

	StringView r(str);
	r.skipChars<chars::Range<char, '0', '9'>>();
	if (!r.empty()) {
		return false;
	}

	return true;
}

bool validateHexadecimial(const String &str) {
	if (str.empty()) {
		return false;
	}

	StringView r(str);
	r.skipChars<chars::CharGroup<char, CharGroupId::Hexadecimial>>();
	if (!r.empty()) {
		return false;
	}

	return true;
}

bool validateBase64(const String &str) {
	if (str.empty()) {
		return false;
	}

	StringView r(str);
	r.skipChars<chars::CharGroup<char, CharGroupId::Base64>>();
	if (!r.empty()) {
		return false;
	}

	return true;
}

void makeRandomBytes_buf(uint8_t * buf, size_t count) {
	if (apr_generate_random_bytes(buf, count) != APR_SUCCESS) {
		ap_random_insecure_bytes(buf, count);
	}
}

void makeRandomBytes(uint8_t *buf, size_t count) {
	makeRandomBytes_buf(buf, count);
}

Bytes makeRandomBytes(size_t count) {
	Bytes ret; ret.resize(count);
	makeRandomBytes_buf(ret.data(), count);
	return ret;
}

Bytes makePassword(const String &str, const String &key) {
	if (str.empty() || key.empty()) {
		return Bytes();
	}
	string::Sha512::Buf source = string::Sha512::make(str, config::getInternalPasswordKey());

	Bytes passwdKey; passwdKey.resize(16 + string::Sha512::Length);
	passwdKey[0] = 0; passwdKey[1] = 1; // version code
	makeRandomBytes_buf(passwdKey.data() + 2, 14);

	string::Sha512 hash_ctx;
	hash_ctx.update(passwdKey.data(), 16);
	if (!key.empty()) {
		hash_ctx.update(key);
	}
	hash_ctx.update(source);
	hash_ctx.final(passwdKey.data() + 16);

	return passwdKey;
}

bool validatePassord(const String &str, const Bytes &passwd, const String &key) {
	if (passwd.size() < 8 + string::Sha256::Length) {
		return false; // not a password
	}

	if (passwd[0] == 0 && passwd[1] == 1) {
		// Serenity/2 protocol
		if (passwd.size() != 16 + string::Sha512::Length) {
			return false;
		}

		string::Sha512::Buf source = string::Sha512::make(str, config::getInternalPasswordKey());
		uint8_t controlKey [16 + string::Sha512::Length] = { 0 };
		memcpy(controlKey, passwd.data(), 16);

		string::Sha512 hash_ctx;
		hash_ctx.update(passwd.data(), 16);
		if (!key.empty()) {
			hash_ctx.update(key);
		}
		hash_ctx.update(source);
		hash_ctx.final(controlKey + 16);

		if (memcmp(passwd.data() + 16, controlKey + 16, string::Sha512::Length) == 0) {
			return true;
		} else {
			return false;
		}
	} else {
		// Serenity/1 protocol
		uint8_t passwdKey [8 + string::Sha256::Length] = { 0 };
		apr_base64_decode_binary(passwdKey, (const char *)passwd.data());

		uint8_t controlKey [8 + string::Sha256::Length] = { 0 };
		memcpy(controlKey, passwdKey, 8);

		string::Sha256 hash_ctx;
		hash_ctx.update(passwdKey, 8);
		hash_ctx.update(str);
		if (!key.empty()) {
			hash_ctx.update(key);
		}
		hash_ctx.final(controlKey + 8);

		if (memcmp(passwdKey + 8, controlKey + 8, string::Sha256::Length) == 0) {
			return true;
		} else {
			return false;
		}
	}
}

NS_SA_EXT_END(valid)


NS_SA_EXT_BEGIN(messages)

struct ErrorNotificator {
	using Callback = Function<void(data::Value &&)>;

	Callback error;
	Callback debug;
};

bool isDebugEnabled() {
	return Root::getInstance()->isDebugEnabled();
}

void setDebugEnabled(bool v) {
	Root::getInstance()->setDebugEnabled(v);
}

void _addErrorMessage(data::Value &&data) {
	auto root = Root::getInstance();
	if (root->isDebugEnabled()) {
		data::Value bcast{
			std::make_pair("message", data::Value(true)),
			std::make_pair("level", data::Value("error")),
			std::make_pair("data", data::Value(data)),
		};
		auto a = storage::Adapter::FromContext();
		if (a) {
			a->broadcast(bcast);
		}
	}

	if (auto serv = Server(apr::pool::server())) {
		serv.reportError(data);
	}

	Request rctx(apr::pool::request());
	if (rctx) {
		rctx.addErrorMessage(std::move(data));
	} else {
		auto pool = apr::pool::acquire();
		ErrorNotificator *err = nullptr;
		apr_pool_userdata_get((void **)&err, "Serenity.ErrorNotificator", pool);
		if (err && err->error) {
			err->error(std::move(data));
		} else {
			log::text("Error", data::toString(data, false));
		}
	}
}

void _addDebugMessage(data::Value &&data) {
	auto root = Root::getInstance();
	if (root->isDebugEnabled()) {
		data::Value bcast{
			std::make_pair("message", data::Value(true)),
			std::make_pair("level", data::Value("debug")),
			std::make_pair("data", data::Value(data)),
		};
		auto a = storage::Adapter::FromContext();
		if (a) {
			a->broadcast(bcast);
		}
	}

#if DEBUG
	if (auto serv = Server(apr::pool::server())) {
		serv.reportError(data);
	}
#endif

	Request rctx(apr::pool::request());
	if (rctx) {
		rctx.addDebugMessage(std::move(data));
	} else {
		auto pool = apr::pool::acquire();
		ErrorNotificator *err = nullptr;
		apr_pool_userdata_get((void **)&err, (const char *)config::getSerenityErrorNotificatorName(), pool);
		if (err && err->debug) {
			err->debug(std::move(data));
		} else {
			log::text("Debug", data::toString(data, false));
		}
	}
}

void broadcast(const data::Value &val) {
	auto a = storage::Adapter::FromContext();
	if (a) {
		a->broadcast(val);
	}
}
void broadcast(const Bytes &val) {
	auto a = storage::Adapter::FromContext();
	if (a) {
		a->broadcast(val);
	}
}

void setNotifications(apr_pool_t *pool,
		const Function<void(data::Value &&)> &error,
		const Function<void(data::Value &&)> &debug) {
	ErrorNotificator *err = (ErrorNotificator *)apr_pcalloc(pool, sizeof(ErrorNotificator));
	new (err) ErrorNotificator();
	err->error = error;
	err->debug = debug;

	apr_pool_userdata_set(err, (const char *)config::getSerenityErrorNotificatorName(), nullptr, pool);
}

NS_SA_EXT_END(messages)
