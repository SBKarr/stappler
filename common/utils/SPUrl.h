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

#ifndef COMMON_UTILS_SPURL_H_
#define COMMON_UTILS_SPURL_H_

#include "SPData.h"

NS_SP_BEGIN

/* common url class, should be valid for:
 * http(s), sip(s), mailto, file, ftp(s)
 */

class Url : public AllocBase {
public:
	struct QueryValue {
		String key;
		String subkey;
		String value;
		bool hasSubKey;

		QueryValue(const String &k) : key(k), hasSubKey(false) { }
		QueryValue(const String &k, const String &v, bool subkey = false)
		: key(k), value(v), hasSubKey(subkey) { }
		QueryValue(const String &k, const String &s, const String &v)
		: key(k), subkey(s), value(v), hasSubKey(true) { }
	};

	using QueryVec = Vector<QueryValue>;

	static Vector<String> parsePath(const String &);
	static QueryVec parseArgs(const String &);
	static data::Value parseDataArgs(const String &, size_t maxVarSize = maxOf<size_t>());
	static data::Value parseDataArgs(const StringView &, size_t maxVarSize = maxOf<size_t>());

	Url() : _port(0) { }
	Url(const Url &u) = default;
	Url(Url &&u) = default;
	Url & operator = (const Url &u) = default;
	Url & operator = (Url &&u) = default;

	inline Url(const String &str) { set(str); }
	inline Url & operator = (const String &str) { return set(str); }
	inline operator String () const { return get(); }

	Url copy() const;
	Url &clear();

	bool parse(const String &);
	Url &set(const String &);
	Url &setScheme(const String &);
	Url &setUser(const String &);
	Url &setPassword(const String &);
	Url &setHost(const String &);
	Url &setPort(uint32_t);

	Url &setPath(const String &);
	Url &setPath(const Vector<String> &);
	Url &setPath(Vector<String> &&);
	Url &addPath(const String &);
	Url &addPath(const Vector<String> &);
	Url &addPath(Vector<String> &&);
	Url &addPathComponent(const String &);
	Url &addPathComponent(String &&);

	Url &setQuery(const QueryVec &);
	Url &setQuery(QueryVec &&);
	Url &setQuery(const data::Value &);
	Url &addQuery(const QueryVec &);
	Url &addQuery(QueryVec &&);
	Url &addQuery(const data::Value &);
	Url &addQuery(QueryValue &&);
	Url &addQuery(const QueryValue &);

	template <class ...Args>
	Url &addQuery(Args && ...args) {
		return addQuery(QueryValue(std::forward<Args>(args)...));
	}

	Url &addQuery(const String &, const data::Value &);

	Url &setFragment(const String &);

	String get() const;
	data::Value encode() const;

	inline const String &getScheme() const { return _scheme; }
	inline const String &getUser() const { return _user; }
	inline const String &getPassword() const { return _password; }
	inline const String &getHost() const { return _host; }
	inline uint32_t getPort() const { return _port; }
    inline const Vector<String> &getPath() const { return _path; }
    inline const QueryVec &getQuery() const { return _query; }
    inline const String &getOriginalQuery() const { return _originalQuery; }
	inline const String &getFragment() const { return _fragment; }

protected:
	bool parseInternal(const String &);

	String _scheme;
	String _user;
	String _password;
	String _host;
	uint32_t _port;
	Vector<String> _path;
	String _originalQuery;
	QueryVec _query;
	String _fragment;
};

NS_SP_END

#endif /* COMMON_UTILS_SPURL_H_ */
