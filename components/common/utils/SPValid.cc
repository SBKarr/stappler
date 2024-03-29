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

#include "SPValid.h"
#include "SPUrl.h"
#include "SPCrypto.h"

#ifdef MSYS
#include "Windows.h"
#include "winnls.h"
#else
#include "idn2.h"
#endif

#if SPAPR
#include "apr_general.h"
#include "apr_base64.h"
#include "httpd.h"
#endif

NS_SP_EXT_BEGIN(valid)

inline auto Config_getInternalPasswordKey() { return "Serenity Password Salt"_weak; }

using HostUnicodeChars = chars::Compose<char, chars::CharGroup<char, CharGroupId::Alphanumeric>,
		chars::Chars<char, '.', '-'>, chars::Range<char, char(128), char(255)>>;

using HostAsciiChars = chars::Compose<char, chars::CharGroup<char, CharGroupId::Alphanumeric>,
		chars::Chars<char, '.', '-'>>;

String idnToAscii(const StringView &source, bool validate) {
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

#ifdef MSYS
	auto in_w = string::toUtf16(source);
	wchar_t punycode[256] = { 0 };
	int chars = IdnToAscii(0, (const wchar_t *)in_w.data(), -1, punycode, 255);
	if (chars) {
		return string::toUtf8<memory::PoolInterface>((char16_t *)punycode);
	}
#else
	char *out = nullptr;
	auto err = idn2_to_ascii_8z(source.data(), &out, IDN2_NFC_INPUT|IDN2_NONTRANSITIONAL);
	if (err == IDN2_OK) {
		String ret(out);
		free(out);
		return ret;
	}
#endif
	return String();
}

String idnToUnicode(const StringView &source, bool validate) {
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

#ifdef MSYS
	auto in_w = string::toUtf16(source);
	wchar_t unicode[256] = { 0 };
	int chars = IdnToUnicode(0, (const wchar_t *)in_w.data(), int(in_w.size()), unicode, 255);
	if (chars) {
		return string::toUtf8<memory::PoolInterface>((char16_t *)unicode);
	}
#else
	char *out = nullptr;
	auto err = idn2_to_unicode_8z8z(source.data(), &out, 0);
	if (err == IDN2_OK) {
		String ret(out);
		free(out);
		return ret;
	}
#endif
	return String();
}

/** Identifier starts with [a-zA-Z_] and can contain [a-zA-Z0-9_\-.@] */
bool validateIdentifier(const StringView &str) {
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
bool validateText(const StringView &str) {
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

static bool valid_validateEmailQuotation(StringView &r, String *target) {
	++ r;
	if (target) { target->push_back('"'); }
	while (!r.empty() && !r.is('"')) {
		auto pos = r.readUntil<StringView::Chars<'"', '\\'>>();
		if (!pos.empty()) {
			if (target) { target->append(pos.data(), pos.size()); }
		}

		if (r.is('\\')) {
			if (target) { target->push_back(r[0]); }
			++ r;
			if (!r.empty()) {
				if (target) { target->push_back(r[0]); }
				++ r;
			}
		}
	}
	if (r.empty()) {
		return false;
	} else {
		if (target) { target->push_back('"'); }
		++ r;
		return true;
	}
}

static bool doValidateEmail(StringView r, String *target) {
	using namespace chars;
	using LocalChars = StringView::Compose<StringView::CharGroup<CharGroupId::Alphanumeric>,
			StringView::Chars<'_', '-', '+', '#', '!', '$', '%', '&', '\'', '*', '/', '=', '?', '^', '`', '{', '}', '|', '~' >,
			StringView::Range<char(128), char(255)>>;

	using Whitespace =  CharGroup<char, CharGroupId::WhiteSpace>;

	r.trimChars<Whitespace>();

	if (r.is('(')) {
		r.skipUntil<StringView::Chars<')'>>();
		if (!r.is(')')) {
			return false;
		}
		r ++;
		r.skipChars<Whitespace>();
	}
	if (r.is('"')) {
		if (!valid_validateEmailQuotation(r, target)) {
			return false;
		}
	}

	while (!r.empty() && !r.is('@')) {
		auto pos = r.readChars<LocalChars>();
		if (!pos.empty()) {
			if (target) { target->append(pos.data(), pos.size()); }
		}

		if (r.is('.')) {
			if (target) { target->push_back('.'); }
			++ r;
			if (r.is('"')) {
				if (!valid_validateEmailQuotation(r, target)) {
					return false;
				}
				if (!r.is('.') && !r.is('@')) {
					return false;
				} else if (r.is('.')) {
					if (target) { target->push_back('.'); }
					++ r;
				}
			} else if (!r.is<LocalChars>()) {
				return false;
			}
		}
		if (r.is('(')) {
			r.skipUntil<StringView::Chars<')'>>();
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

	if (target) { target->push_back('@'); }
	++ r;
	if (r.is('(')) {
		r.skipUntil<StringView::Chars<')'>>();
		if (!r.is(')')) {
			return false;
		}
		r ++;
		r.skipChars<Whitespace>();
	}

	if (r.is('[')) {
		if (target) { target->push_back('['); }
		auto pos = r.readUntil<StringView::Chars<']'>>();
		if (r.is(']')) {
			r ++;
			if (r.empty()) {
				if (target) { target->append(pos.data(), pos.size()); }
				if (target) { target->push_back(']'); }
			}
		}
	} else {
		auto tmp = r;
		while (!tmp.empty()) {
			auto t = tmp.readUntil<StringView::Chars<'.'>>();
			if (tmp.is('.')) {
				if (t.empty()) {
					return false;
				}
				++ tmp;
			}
		}

		String host = idnToAscii(r);
		if (host.empty()) {
			return false;
		}

		if (target) { target->append(host); }
	}

	return true;
}

bool validateEmailWithoutNormalization(const StringView &str) {
	return doValidateEmail(str, nullptr);
}

bool validateEmail(String &str) {
	string::trim<String, memory::PoolInterface>(str);
	if (str.empty()) {
		return false;
	}

	if (str.back() == ')') {
		auto pos = str.rfind('(');
		if (pos != String::npos) {
			str.erase(str.begin() + pos, str.end());
		} else {
			return false;
		}
	}
	String ret; ret.reserve(str.size());

	if (doValidateEmail(str, &ret)) {
		str = std::move(ret);
		return true;
	}

	return false;
}

bool validateEmail(memory::StandartInterface::StringType &str) {
	String istr = StringView(str).str<memory::PoolInterface>();
	if (validateEmail(istr)) {
		str = StringView(istr).str<memory::StandartInterface>();
		return true;
	}
	return false;
}

bool validateUrl(String &str) {
	UrlView url;
	if (!url.parse(str)) {
		return false;
	}

	auto oldHost = url.host;
	if (url.host.empty() && url.path.size() < 2) {
		return false;
	}

	if (!oldHost.empty()) {
		auto str = oldHost.str();
		auto newHost = idnToAscii(str);
		if (newHost.empty()) {
			return false;
		}
		url.host = newHost;
	}

	auto newUrl = url.get<memory::PoolInterface>();
	str = String(newUrl.data(), newUrl.size());
	return true;
}

bool validateUrl(memory::StandartInterface::StringType &str) {
	String istr = StringView(str).str<memory::PoolInterface>();
	if (validateUrl(istr)) {
		str = StringView(istr).str<memory::StandartInterface>();
		return true;
	}
	return false;
}

bool validateNumber(const StringView &str) {
	if (str.empty()) {
		return false;
	}

	StringView r(str);
	if (r.is('-')) { ++ r; }
	r.skipChars<chars::Range<char, '0', '9'>>();
	if (!r.empty()) {
		return false;
	}

	return true;
}

bool validateHexadecimial(const StringView &str) {
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

bool validateBase64(const StringView &str) {
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
#if SPAPR
	if (apr_generate_random_bytes(buf, count) != APR_SUCCESS) {
		ap_random_insecure_bytes(buf, count);
	}
#else
	auto bytesInRand = 0;
	size_t tmp = RAND_MAX;
	while (tmp > 255) {
		++ tmp;
		tmp /= 256;
	}

	size_t b = bytesInRand;
	size_t r = rand();
	for (size_t i = 0; i < count; i++) {
		buf[i] = (uint8_t)(r % 256);
		r /= 256;
		-- b;
		if (b == 0) {
			b = bytesInRand;
			r = rand();
		}
	}
#endif
}

void makeRandomBytes(uint8_t *buf, size_t count) {
	makeRandomBytes_buf(buf, count);
}

template <>
auto makeRandomBytes<memory::PoolInterface>(size_t count) -> memory::PoolInterface::BytesType  {
	memory::PoolInterface::BytesType ret; ret.resize(count);
	makeRandomBytes_buf(ret.data(), count);
	return ret;
}

template <>
auto makeRandomBytes<memory::StandartInterface>(size_t count) -> memory::StandartInterface::BytesType  {
	memory::StandartInterface::BytesType ret; ret.resize(count);
	makeRandomBytes_buf(ret.data(), count);
	return ret;
}

static void makePassword_buf(uint8_t *passwdKey, const StringView &str, const StringView &key) {
	string::Sha512::Buf source = string::Sha512::make(str, Config_getInternalPasswordKey());

	passwdKey[0] = 0; passwdKey[1] = 1; // version code
	makeRandomBytes_buf(passwdKey + 2, 14);

	string::Sha512 hash_ctx;
	hash_ctx.update(passwdKey, 16);
	if (!key.empty()) {
		hash_ctx.update(key);
	}
	hash_ctx.update(source);
	hash_ctx.final(passwdKey + 16);
}

template <>
auto makePassword<memory::PoolInterface>(const StringView &str, const StringView &key) -> memory::PoolInterface::BytesType {
	if (str.empty() || key.empty()) {
		return memory::PoolInterface::BytesType();
	}

	memory::PoolInterface::BytesType passwdKey; passwdKey.resize(16 + string::Sha512::Length);
	makePassword_buf(passwdKey.data(), str, key);
	return passwdKey;
}

template <>
auto makePassword<memory::StandartInterface>(const StringView &str, const StringView &key) -> memory::StandartInterface::BytesType {
	if (str.empty() || key.empty()) {
		return memory::StandartInterface::BytesType();
	}

	memory::StandartInterface::BytesType passwdKey; passwdKey.resize(16 + string::Sha512::Length);
	makePassword_buf(passwdKey.data(), str, key);
	return passwdKey;
}

bool validatePassord(const StringView &str, const BytesView &passwd, const StringView &key) {
	if (passwd.size() < 8 + string::Sha256::Length) {
		return false; // not a password
	}

#if SPAPR
	if (passwd[0] != 0 || passwd[1] != 1) {
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
#endif

	// Serenity/2 protocol
	if (passwd.size() != 16 + string::Sha512::Length) {
		return false;
	}

	string::Sha512::Buf source = string::Sha512::make(str, Config_getInternalPasswordKey());
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
}

#define PSWD_NUMBERS "12345679"
#define PSWD_LOWER "abcdefghijkmnopqrstuvwxyz"
#define PSWD_UPPER "ABCDEFGHJKLMNPQRSTUVWXYZ"

static const char * const pswd_numbers = PSWD_NUMBERS;
static const char * const pswd_lower = PSWD_LOWER;
static const char * const pswd_upper = PSWD_UPPER;
static const char * const pswd_all = PSWD_NUMBERS PSWD_LOWER PSWD_UPPER;

static uint8_t pswd_numbersCount = uint8_t(strlen(PSWD_NUMBERS));
static uint8_t pswd_lowerCount = uint8_t(strlen(PSWD_LOWER));
static uint8_t pswd_upperCount = uint8_t(strlen(PSWD_UPPER));
static uint8_t pswd_allCount = uint8_t(strlen(PSWD_NUMBERS PSWD_LOWER PSWD_UPPER));


template <typename Callback>
static void generatePassword_buf(size_t len, const uint8_t *bytes, const Callback &cb) {
	uint16_t meta = 0;
	memcpy(&meta, bytes, sizeof(uint16_t));

	bool extraChars[3] = { false, false, false };
	for (size_t i = 0; i < len - 3; ++ i) {
		cb(pswd_all[bytes[i + 5] % pswd_allCount]);
		if (!extraChars[0] && i == bytes[2] % (len - 3)) {
			cb(pswd_numbers[meta % pswd_numbersCount]);
			meta /= pswd_numbersCount;
			extraChars[0] = true;
		}
		if (!extraChars[1] && i == bytes[3] % (len - 3)) {
			cb(pswd_lower[meta % pswd_lowerCount]);
			meta /= pswd_lowerCount;
			extraChars[1] = true;
		}
		if (!extraChars[2] && i == bytes[4] % (len - 3)) {
			cb(pswd_upper[meta % pswd_upperCount]);
			meta /= pswd_upperCount;
			extraChars[2] = true;
		}
	}
}

template <>
auto generatePassword<memory::PoolInterface>(size_t len) -> memory::PoolInterface::StringType {
	if (len < 6) {
		return memory::PoolInterface::StringType();
	}

	auto bytes = makeRandomBytes<memory::PoolInterface>(len + 2);
	memory::PoolInterface::StringType ret; ret.reserve(len);
	generatePassword_buf(len, bytes.data(), [&] (char c) {
		ret.push_back(c);
	});
	return ret;
}

template <>
auto generatePassword<memory::StandartInterface>(size_t len) -> memory::StandartInterface::StringType {
	if (len < 6) {
		return memory::StandartInterface::StringType();
	}

	auto bytes = makeRandomBytes<memory::StandartInterface>(len + 2);
	memory::StandartInterface::StringType ret; ret.reserve(len);
	generatePassword_buf(len, bytes.data(), [&] (char c) {
		ret.push_back(c);
	});
	return ret;
}


template <typename Interface>
static auto doConvertOpenSSHKey(StringView r, uint8_t * outBuf, size_t outSize) -> typename Interface::BytesType {
	// uint8_t out[4_KiB];
	// uint8_t *buf = out;

	auto keyType = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	auto dataBlock = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	if (validateBase64(dataBlock)) {
		auto data = base64::decode<Interface>(dataBlock);
		BytesViewNetwork dataView(data);
		auto len = dataView.readUnsigned32();
		keyType = dataView.readString(len);

		if (keyType != "ssh-rsa") {
			return typename Interface::BytesType();
		}

		auto mlen = dataView.readUnsigned32();
		auto modulus = dataView.readBytes(mlen);

		auto elen = dataView.readUnsigned32();
		auto exp = dataView.readBytes(elen);

		crypto::PublicKey pk;
		pk.import(modulus, exp);

		return pk.exportDer<Interface>();
	}
	return typename Interface::BytesType();
}

template <>
auto convertOpenSSHKey<memory::PoolInterface>(const StringView &key) -> memory::PoolInterface::BytesType {
	uint8_t out[2_KiB];
	return doConvertOpenSSHKey<memory::PoolInterface>(key, out, sizeof(out));
}

template <>
auto convertOpenSSHKey<memory::StandartInterface>(const StringView &key) -> memory::StandartInterface::BytesType {
	uint8_t out[2_KiB];
	return doConvertOpenSSHKey<memory::StandartInterface>(key, out, sizeof(out));
}

uint32_t readIp(StringView r) {
	bool err = false;
	return readIp(r, err);
}

uint32_t readIp(StringView r, bool &err) {
	uint32_t octets = 0;
	uint32_t ret = 0;
	while (!r.empty() && octets < 4) {
		auto n = r.readChars<StringView::CharGroup<CharGroupId::Numbers>>();
		if (!n.empty()) {
			auto num = n.readInteger(10).get(256);
			if (num < 256) {
				ret = (ret << 8) | uint32_t(num);
			} else {
				err = true;
				return 0;
			}
		}
		if (r.is('.') && octets < 3) {
			++ r;
			++ octets;
		} else if (octets == 3 && r.empty()) {
			return ret;
		} else {
			err = true;
			return 0;
		}
	}
	err = true;
	return 0;
}

Pair<uint32_t, uint32_t> readIpRange(StringView r) {
	uint32_t start = 0;
	uint32_t end = 0;
	uint32_t mask = 0;

	auto fnReadIp = [] (StringView &r, bool &err) -> uint32_t {
		uint32_t octets = 0;
		uint32_t ret = 0;
		while (!r.empty() && octets < 4) {
			auto n = r.readChars<StringView::CharGroup<CharGroupId::Numbers>>();
			if (!n.empty()) {
				auto num = n.readInteger(10).get(256);
				if (num < 256) {
					ret = (ret << 8) | uint32_t(num);
				} else {
					err = true;
					return 0;
				}
			}
			if (r.is('.') && octets < 3) {
				++ r;
				++ octets;
			} else if (octets == 3) {
				if (r.empty()) {
					return ret;
				} else if (r.is('/') || r.is('-')) {
					return ret;
				}
			} else {
				err = true;
				return 0;
			}
		}
		err = true;
		return 0;
	};

	bool err = false;
	start = fnReadIp(r, err);
	if (err) {
		return pair(0, 0);
	}
	if (r.empty()) {
		return pair(start, start);
	} else if (r.is('-')) {
		++ r;
		end = fnReadIp(r, err);
		if (err || !r.empty()) {
			return pair(0, 0);
		} else {
			return pair(start, end);
		}
	} else if (r.is('/')) {
		++ r;
		auto tmp = r;
		auto n = tmp.readChars<StringView::CharGroup<CharGroupId::Numbers>>();
		auto num = n.readInteger(10).get(256);
		if (tmp.is('.') && num < 256) {
			mask = fnReadIp(r, err);
			if (err || !r.empty()) {
				return pair(0, 0);
			}

			uint32_t i = 0;
			while ((mask & (1 << i)) == 0) {
				++ i;
			}

			while ((mask & (1 << i)) != 0 && i < 32) {
				++ i;
			}

			if (i != 32) {
				return pair(0, 0);
			}
		} else if (num < 32 && tmp.empty()) {
			mask = maxOf<uint32_t>() << (32 - num);
			r = tmp;
		} else {
			return pair(0, 0);
		}

		if (!r.empty()) {
			return pair(0, 0);
		}

		uint32_t netstart = (start & mask); // first ip in subnet
		uint32_t netend = (netstart | ~mask); // last ip in subnet

		return pair(netstart, netend);
	}
	return pair(0, 0);
}

NS_SP_EXT_END(valid)
