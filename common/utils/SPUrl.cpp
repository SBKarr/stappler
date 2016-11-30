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
#include "SPUrl.h"
#include "SPString.h"
#include "SPCharReader.h"
#include "SPUrlencodeParser.h"

NS_SP_BEGIN

template <char ...Args>
using Chars = CharReaderBase::Chars<Args...>;

template <char A, char B>
using Range = CharReaderBase::Range<A, B>;

Vector<String> Url::parsePath(const String &str) {
	Vector<String> ret;
	CharReaderBase s(str);
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
			ret.push_back(path.str());
		}
	} while (!s.empty() && s.is('/'));
	return ret;
}
Url::QueryVec Url::parseArgs(const String &str) {
	QueryVec vec;
	CharReaderBase s(str);
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
data::Value Url::parseDataArgs(const String &str, size_t maxVarSize) {
	if (str.empty()) {
		return data::Value();
	}
	data::Value ret;
	if (str.front() == '?' || str.front() == '&' || str.front() == ';') {
		UrlencodeParser parser(ret, str.size() - 1, maxVarSize);
		parser.read((const uint8_t *)str.data() + 1, str.size() - 1);
	} else {
		UrlencodeParser parser(ret, str.size(), maxVarSize);
		parser.read((const uint8_t *)str.data(), str.size());
	}
	return ret;
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

using Scheme =  chars::Compose<char, chars::CharGroup<char, chars::CharGroupId::Alphanumeric>, chars::Chars<char, '+', '-', '.'>>;
using Ipv6 =  chars::Compose<char, chars::CharGroup<char, chars::CharGroupId::Hexadecimial>, chars::Chars<char, ':'>>;

using Unreserved = chars::Compose<char, chars::CharGroup<char, chars::CharGroupId::Alphanumeric>, chars::Chars<char, '-', '.', '_', '~', '%'>>;
using SubDelim = chars::Chars<char, '!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '='>;
using GenDelim = chars::Chars<char, ':', '/', '?', '#', '[', ']', '@'>;

using UnreservedUni = chars::Compose<char, Unreserved, chars::UniChar>;

static bool validateScheme(const CharReaderBase &r) {
	auto cpy = r;
	if (cpy.is<chars::CharGroup<char, chars::CharGroupId::Alphanumeric>>()) {
		cpy ++;
		cpy.skipChars<Scheme>();
		if (cpy.empty()) {
			return true;
		}
	}
	return false;
}

static bool validateHost(const CharReaderBase &r) {
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

static bool validateUserOrPassword(const CharReaderBase &r) {
	auto cpy = r;
	cpy.skipChars<Unreserved, SubDelim, chars::UniChar>();
	if (cpy.empty()) {
		return true;
	}
	return false;
}

static bool validatePathComponent(const CharReaderBase &r) {
	auto cpy = r;
	cpy.skipChars<Unreserved, SubDelim, Chars<':', '@'>, chars::UniChar>();
	if (cpy.empty()) {
		return true;
	}
	return false;
}

static bool valudateQueryOrFragmentString(const CharReaderBase &r) {
	auto cpy = r;
	cpy.skipChars<Unreserved, SubDelim, Chars<':', '@', '/', '?', '[', ']'>, chars::UniChar>();
	if (cpy.empty()) {
		return true;
	}
	return false;
}

bool Url::parseInternal(const String &str) {
	CharReaderBase s(str);

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
			return false;
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

bool Url::parse(const String &str) {
	clear();
	if (!parseInternal(str)) {
		clear();
		return false;
	}
	return true;
}

Url &Url::set(const String &str) {
	parse(str);
	return *this;
}

Url &Url::setScheme(const String &str) {
	_scheme = str;
	return *this;
}

Url &Url::setUser(const String &str) {
	_user = str;
	return *this;
}

Url &Url::setPassword(const String &str) {
	_password = str;
	return *this;
}

Url &Url::setHost(const String &str) {
	auto pos = str.find(':');
	if (pos == String::npos) {
		_host = str;
	} else {
		_host = str.substr(0, pos);
		_port = StringToNumber<uint32_t>(str.c_str() + pos + 1, nullptr);
	}
	return *this;
}
Url &Url::setPort(uint32_t p) {
	_port = p;
	return *this;
}


Url &Url::setPath(const String &str) {
	_path.clear();

	CharReaderBase s(str);
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

Url &Url::addPath(const String &str) {
	CharReaderBase s(str);
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

Url &Url::addPathComponent(const String &str) {
	_path.push_back(str);
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
			addQuery(it.first, it.second);
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
Url &Url::addQuery(const String &key, const data::Value &value) {
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

Url &Url::setFragment(const String &str) {
	_fragment = str;
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
