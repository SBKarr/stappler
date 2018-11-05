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

#include "SPCommon.h"
#include "SPString.h"
#include "SPUrl.h"
#include "SPStringView.h"
#include "SPUrlencodeParser.h"

NS_SP_BEGIN

template <char ...Args>
using Chars = StringView::Chars<Args...>;

template <char A, char B>
using Range = StringView::Range<A, B>;

Vector<String> Url::parsePath(const StringView &str) {
	Vector<String> ret;
	StringView s(str);
	do {
		if (s.is('/')) {
			s ++;
		}
		auto path = s.readUntil<Chars<'/', '?', ';', '&', '#'>>();
		if (path == "..") {
			if (!ret.empty()) {
				ret.pop_back();
			}
		} else if (path == ".") {
			// skip this component
		} else {
			if (!path.empty()) {
				ret.push_back(path.str());
			}
		}
	} while (!s.empty() && s.is('/'));
	return ret;
}
Url::QueryVec Url::parseArgs(const StringView &str) {
	QueryVec vec;
	StringView s(str);
	do {
		if (s.is(';') || s.is('?') || s.is('&')) {
			s ++;
		}
		auto pair = s.readUntil<Chars<';', '&', '#'>>();
		if (!pair.empty()) {
			auto key = pair.readUntil<Chars<'='>>();
			if (pair.is('=')) {
				pair ++;
				auto name = key.readUntil<Chars<'['>>();
				if (key.is('[') && !name.empty()) {
					key ++;
					key = key.readUntil<Chars<']'>>();
					if (!key.empty()) {
						vec.emplace_back(name.str(), key.str(), pair.str());
					} else {
						vec.emplace_back(name.str(), pair.str(), true);
					}
				} else {
					vec.emplace_back(name.str(), pair.str(), false);
				}
			} else {
				vec.emplace_back(key.str());
			}
		}
	} while (!s.empty() && (s.is('&') || s.is(';')));
	return vec;
}

data::Value Url::parseDataArgs(const StringView &str, size_t maxVarSize) {
	if (str.empty()) {
		return data::Value();
	}
	data::Value ret;
	StringView r(str);
	if (r.front() == '?' || r.front() == '&' || r.front() == ';') {
		++ r;
	}
	UrlencodeParser parser(ret, r.size(), maxVarSize);
	parser.read((const uint8_t *)r.data(), r.size());
	return ret;
}

static bool validateEmailQuotation(String &ret, StringView &r) {
	using namespace chars;

	++ r;
	ret.push_back('"');
	while (!r.empty() && !r.is('"')) {
		auto pos = r.readUntil<chars::Chars<char, '"', '\\'>>();
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

bool Url::validateEmail(String &str) {
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
	using LocalChars = chars::Compose<char, CharGroup<char, CharGroupId::Alphanumeric>,
			chars::Chars<char, '_', '-', '+', '#', '!', '$', '%', '&', '\'', '*', '/', '=', '?', '^', '`', '{', '}', '|', '~' >,
			chars::Range<char, char(128), char(255)>>;

	using Whitespace =  CharGroup<char, CharGroupId::WhiteSpace>;

	StringView r(str);
	r.skipChars<Whitespace>();

	if (r.is('(')) {
		r.skipUntil<chars::Chars<char, ')'>>();
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
			r.skipUntil<chars::Chars<char, ')'>>();
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
		r.skipUntil<chars::Chars<char, ')'>>();
		if (!r.is(')')) {
			return false;
		}
		r ++;
		r.skipChars<Whitespace>();
	}

	if (r.is('[')) {
		ret.push_back('[');
		auto pos = r.readUntil<chars::Chars<char, ']'>>();
		if (r.is(']')) {
			r ++;
			if (r.empty()) {
				ret.append(pos.data(), pos.size());
				ret.push_back(']');
			}
		}
	} else {
		ret.append(r.str());
	}

	str = move(ret);
	return true;
}

Url Url::copy() const {
	return *this;
}

Url &Url::clear() {
	_scheme.clear();
	_user.clear();
	_password.clear();
	_host.clear();
	_port = 0;
	_path.clear();
	_originalQuery.clear();
	_query.clear();
	_fragment.clear();
	return *this;
}

using Scheme =  chars::Compose<char, chars::CharGroup<char, CharGroupId::Alphanumeric>, chars::Chars<char, '+', '-', '.'>>;
using Ipv6 =  chars::Compose<char, chars::CharGroup<char, CharGroupId::Hexadecimial>, chars::Chars<char, ':'>>;

using Unreserved = chars::Compose<char, chars::CharGroup<char, CharGroupId::Alphanumeric>, chars::Chars<char, '-', '.', '_', '~', '%'>>;
using SubDelim = chars::Chars<char, '!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '='>;
using GenDelim = chars::Chars<char, ':', '/', '?', '#', '[', ']', '@'>;

using UnreservedUni = chars::Compose<char, Unreserved, chars::UniChar>;

static bool validateScheme(const StringView &r) {
	auto cpy = r;
	if (cpy.is<chars::CharGroup<char, CharGroupId::Alphanumeric>>()) {
		cpy ++;
		cpy.skipChars<Scheme>();
		if (cpy.empty()) {
			return true;
		}
	}
	return false;
}

static bool validateHost(const StringView &r) {
	auto cpy = r;
	if (cpy.is('[')) {
		++ cpy;
		cpy.skipChars<Ipv6>();
		if (cpy.is(']')) {
			cpy ++;
			if (cpy.empty()) {
				return true;
			}
		}
		// check ipv6
	} else {
		cpy.skipChars<Unreserved, SubDelim, chars::UniChar>();
		if (cpy.empty()) {
			return true;
		}
	}
	return false;
}

static bool validateUserOrPassword(const StringView &r) {
	auto cpy = r;
	cpy.skipChars<Unreserved, SubDelim, chars::UniChar>();
	if (cpy.empty()) {
		return true;
	}
	return false;
}

static bool validatePathComponent(const StringView &r) {
	auto cpy = r;
	cpy.skipChars<Unreserved, SubDelim, Chars<':', '@'>, chars::UniChar>();
	if (cpy.empty()) {
		return true;
	}
	return false;
}

static bool valudateQueryOrFragmentString(const StringView &r) {
	auto cpy = r;
	cpy.skipChars<Unreserved, SubDelim, Chars<':', '@', '/', '?', '[', ']'>, chars::UniChar>();
	if (cpy.empty()) {
		return true;
	}
	return false;
}

static bool isEmptyHostAllowed(const String &scheme) {
	if (scheme == "file") {
		return true;
	}
	return false;
}

bool Url::parseInternal(const StringView &str) {
	StringView s(str);

	enum State {
		Scheme,
		User,
		Password,
		Host,
		Port,
		Path,
		Query,
		Fragment,
	} state = State::Scheme;

	auto tmp = s.readUntil<Chars<':', '/', '?', '&', ';', '#'>>();

	if (s.is(':')) {
		// scheme or host+port
		if (tmp.empty()) {
			return false; // error
		}

		if (s.is("://")) {
			// scheme
			s += 3;
			if (validateScheme(tmp)) {
				_scheme = tmp.str();
				state = State::User;
			} else {
				return false;
			}
		} else {
			// if it's port, next chars will be numbers only
			auto tmpS = s;
			tmpS ++;
			auto port = tmpS.readChars<Range<'0', '9'>>();
			if (tmpS.empty() || tmpS.is<Chars<'/', '?', '&', ';', '#'>>()) {
				// host + port
				if (validateHost(tmp)) {
					_host = tmp.str();
				} else {
					return false;
				}
				_port = StringToNumber<uint32_t>(port.data(), nullptr);
				s = tmpS;
				if (s.is('/')) {
					++ s;
					state = Path;
				} else if (s.is<Chars<'?', '&', ';'>>()) {
					++ s;
					state = Query;
				} else if (s.is('#')) {
					++ s;
					state = Fragment;
				} else if (s.empty()) {
					return true;
				} else {
					return false;
				}
			} else {
				// scheme
				++ s;
				if (validateScheme(tmp)) {
					_scheme = tmp.str();
					state = State::User;
				} else {
					return false;
				}
			}
		}
	} else if (s.is('/')) {
		if (tmp.empty()) {
			// absolute path with no host
			_path.push_back("");
			state = State::Path;
			++ s;
		} else {
			// host + path
			if (validateHost(tmp)) {
				_host = tmp.str();
				++ s;
			} else {
				return false;
			}
			state = State::Path;
		}
	} else if (s.empty()) {
		// assume host-only
		if (!tmp.empty()) {
			if (validateHost(tmp)) {
				_host = tmp.str();
				return true;
			} else {
				return false;
			}
		} else {
			return false;
		}
	}

	// authority path check
	if (state == State::User) {
		auto tmp_s = s;
		auto userInfo = tmp_s.readUntil<Chars<'@', '/'>>();
		if (userInfo.empty()) {
			if (!tmp_s.is('/') || !isEmptyHostAllowed(_scheme)) {
				return false;
			}
		}
		if (tmp_s.is('@')) {
			auto user = userInfo.readUntil<Chars<':'>>();
			if (user.empty()) {
				return false;
			}
			if (validateUserOrPassword(user)) {
				_user = user.str();
			} else {
				return false;
			}
			if (userInfo.is(':')) {
				++ userInfo;
				if (!userInfo.empty()) {
					if (validateUserOrPassword(userInfo)) {
						_password = userInfo.str();
					} else {
						return false;
					}
				}
			}

			++ tmp_s; // skip '@'
			s = tmp_s;
		}
		state = Host;
	}

	if (state == Host) {
		tmp = s.readUntil<Chars<':', '/', '?', '&', ';', '#'>>();
		if (validateHost(tmp)) {
			_host = tmp.str();
		} else {
			return false;
		}
		if (s.is(':')) {
			++ s;
			auto port = s.readChars<Range<'0', '9'>>();
			if (!port.empty()) {
				_port = StringToNumber<uint32_t>(port.data(), nullptr);
			} else {
				return false;
			}
		}
		if (s.is('/')) {
			++ s;
			state = Path;
		} else if (s.is<Chars<'?', '&', ';'>>()) {
			++ s;
			state = Query;
		} else if (s.is('#')) {
			++ s;
			state = Fragment;
		} else if (s.empty()) {
			return true;
		} else {
			return false;
		}
	}

	if (state == Path) {
		if (s.is('/')) {
			_path.push_back("");
			++ s;
		}

		do {
			if (s.is('/')) {
				++ s;
			}

			auto path = s.readUntil<Chars<'/', '?', ';', '&', '#'>>();
			if (path == "..") {
				if (!_path.empty() && (!_path.front().empty() || _path.size() >= 2)) {
					_path.pop_back();
				}
			} else if (path == ".") {
				// skip this component
			} else {
				if (validatePathComponent(path)) {
					_path.push_back(path.str());
				} else {
					return false;
				}
			}
		} while (!s.empty() && s.is('/'));

		if (s.is<Chars<'?', '&', ';'>>()) {
			++ s;
			state = Query;
		} else if (s.is('#')) {
			++ s;
			state = Fragment;
		} else if (s.empty()) {
			return true;
		} else {
			return false;
		}
	}

	if (state == Query) {
		tmp = s.readUntil<Chars<'#'>>();
		if (!valudateQueryOrFragmentString(tmp)) {
			return false;
		} else {
			_originalQuery = tmp.str();
		}

		do {
			if (tmp.is('?') || tmp.is('&') || tmp.is(';')) { ++ tmp; }
			auto pair = tmp.readUntil<Chars<';', '&', '#'>>();
			if (!pair.empty()) {
				auto key = pair.readUntil<Chars<'='>>();
				if (pair.is('=')) {
					pair ++;
					auto name = key.readUntil<Chars<'['>>();
					if (key.is('[') && !name.empty()) {
						key ++;
						key = key.readUntil<Chars<']'>>();
						if (!key.empty()) {
							_query.emplace_back(name.str(), key.str(), pair.str());
						} else {
							_query.emplace_back(name.str(), pair.str(), true);
						}
					} else {
						_query.emplace_back(name.str(), pair.str(), false);
					}
				} else {
					_query.emplace_back(key.str());
				}
			}
		} while (!tmp.empty() && (tmp.is('&') || tmp.is(';')));

		if (s.is('#')) {
			++ s;
			state = Fragment;
		}
	}

	if (state == Fragment) {
		if (s.is('#')) {
			++ s;
		}

		if (valudateQueryOrFragmentString(s)) {
			_fragment = s.str();
		} else {
			return false;
		}
	}

	return true;
}

bool Url::parse(const StringView &str) {
	clear();
	if (!parseInternal(str)) {
		clear();
		return false;
	}
	return true;
}

Url &Url::set(const StringView &str) {
	parse(str);
	return *this;
}

Url &Url::setScheme(const StringView &str) {
	_scheme = str.str();
	return *this;
}

Url &Url::setUser(const StringView &str) {
	_user = str.str();
	return *this;
}

Url &Url::setPassword(const StringView &str) {
	_password = str.str();
	return *this;
}

Url &Url::setHost(const StringView &str) {
	auto pos = str.find(':');
	if (pos == String::npos) {
		_host = str.str();
	} else {
		_host = str.sub(0, pos).str();
		_port = StringToNumber<uint32_t>(str.data() + pos + 1, nullptr);
	}
	return *this;
}
Url &Url::setPort(uint32_t p) {
	_port = p;
	return *this;
}


Url &Url::setPath(const StringView &str) {
	_path.clear();

	StringView s(str);
	do {
		if (s.is('/')) {
			s ++;
		}
		auto path = s.readUntil<Chars<'/', '?', ';', '&', '#'>>();
		_path.push_back(path.str());
	} while (!s.empty() && s.is('/'));
	return *this;
}
Url &Url::setPath(const Vector<String> &vec) {
	_path = vec;
	return *this;
}
Url &Url::setPath(Vector<String> &&vec) {
	_path = std::move(vec);
	return *this;
}

Url &Url::addPath(const StringView &str) {
	StringView s(str);
	do {
		if (s.is('/')) {
			s ++;
		}
		auto path = s.readUntil<Chars<'/', '?', ';', '&', '#'>>();
		_path.push_back(path.str());
	} while (!s.empty() && s.is('/'));
	return *this;
}

Url &Url::addPath(const Vector<String> &vec) {
	_path.reserve(_path.size() + vec.size());
	for (auto &it : vec) {
		_path.push_back(it);
	}
	return *this;
}

Url &Url::addPath(Vector<String> &&vec) {
	_path.reserve(_path.size() + vec.size());
	for (auto &it : vec) {
		_path.push_back(std::move(it));
	}
	return *this;
}

Url &Url::addPathComponent(const StringView &str) {
	_path.push_back(str.str());
	return *this;
}
Url &Url::addPathComponent(String &&str) {
	_path.push_back(std::move(str));
	return *this;
}


Url &Url::setQuery(const QueryVec &q) {
	_originalQuery.clear();
	_query = q;
	return *this;
}
Url &Url::setQuery(QueryVec &&q) {
	_originalQuery.clear();
	_query = std::move(q);
	return *this;
}
Url &Url::setQuery(const data::Value &data) {
	_originalQuery.clear();
	_query.clear();
	return addQuery(data);
}
Url &Url::addQuery(const QueryVec &q) {
	_originalQuery.clear();
	for (auto &it : q) {
		_query.push_back(it);
	}
	return *this;
}
Url &Url::addQuery(QueryVec &&q) {
	_originalQuery.clear();
	for (auto &it : q) {
		_query.push_back(std::move(it));
	}
	return *this;
}
Url &Url::addQuery(const data::Value &data) {
	_originalQuery.clear();
	if (data.isDictionary()) {
		for (auto &it : data.getDict()) {
			addQuery(it.first, it.second.asString());
		}
	} else if (data.isArray()) {
		for (auto &it : data.getArray()) {
			if (it.isBasicType()) {
				addQuery(it.asString());
			}
		}
	}
	return *this;
}
Url &Url::addQuery(QueryValue &&q) {
	_originalQuery.clear();
	_query.push_back(std::move(q));
	return *this;
}
Url &Url::addQuery(const QueryValue &q) {
	_originalQuery.clear();
	_query.push_back(q);
	return *this;
}
Url &Url::addQuery(const StringView &key, const data::Value &value) {
	_originalQuery.clear();
	if (value.isArray()){
		for (auto &it : value.getArray()) {
			if (it.isBasicType()) {
				addQuery(key, it.asString(), true);
			}
		}
	} else if (value.isDictionary()) {
		for (auto &it : value.getDict()) {
			if (it.second.isBasicType()) {
				addQuery(key, it.first, it.second.asString());
			}
		}
	} else if (value && value.isBasicType()) {
		return addQuery(key, value.asString());
	}
	return *this;
}

Url &Url::setFragment(const StringView &str) {
	_fragment = str.str();
	return *this;
}

String Url::get() const {
	StringStream ret;
	if (!_scheme.empty()) {
		ret << _scheme << ':';
	}

	if (!_scheme.empty() && _scheme != "mailto" && _scheme != "tel" && _scheme != "sip" && _scheme != "sips") {
		ret << "//";
	}

	if (!_user.empty()) {
		ret << _user;
		if (!_password.empty()) {
			ret << ':' << _password;
		}
		ret << '@';
	}

	if (!_host.empty()) {
		ret << _host;
		if (_port != 0) {
			ret << ':' << _port;
		}
	}

	if (!_path.empty()) {
		for (auto it = _path.begin(); it != _path.end(); it ++) {
			if (it != _path.begin() || !_host.empty()) {
				ret << "/";
			}
			ret << string::urlencode((*it));
		}
	}

	if (!_originalQuery.empty()) {
		ret << '?' << _originalQuery;
	} else if (!_query.empty()) {
		char sep = '?';
		for (auto &it : _query) {
			ret << sep;
			if (it.value.empty()) {
				ret << it.key;
			} else if (it.subkey.empty()) {
				if (it.hasSubKey) {
					ret << string::urlencode(it.key) << "[]=" << string::urlencode(it.value);
				} else {
					ret << string::urlencode(it.key) << '=' << string::urlencode(it.value);
				}
			} else {
				ret << string::urlencode(it.key) << "[" << string::urlencode(it.subkey) << "]=" << string::urlencode(it.value);
			}
			sep = '&';
		}
	}

	if (!_fragment.empty()) {
		ret << '#' << string::urlencode(_fragment);
	}

	return ret.str();
}

data::Value Url::encode() const {
	data::Value ret;
	ret.setString(_scheme, "scheme");
	ret.setString(_user, "user");
	ret.setString(_password, "password");
	ret.setString(_host, "host");
	ret.setInteger(_port, "port");

	data::Value path;
	for (auto &it : _path) {
		path.addString(it);
	}
	ret.setValue(std::move(path), "path");

	data::Value query;
	for (auto &it : _query) {
		data::Value q;
		q.addString(it.key);
		q.addString(it.value);
		if (!it.subkey.empty()) {
			q.addString(it.subkey);
		} else if (it.hasSubKey) {
			q.addBool(it.hasSubKey);
		}
		query.addValue(std::move(q));
	}
	ret.setValue(query, "query");
	ret.setString(_fragment, "fragment");
	ret.setString(get(), "url");
	return ret;
}

NS_SP_END
