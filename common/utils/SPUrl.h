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

		QueryValue(const StringView &k) : key(k.str()), hasSubKey(false) { }
		QueryValue(const StringView &k, const StringView &v, bool subkey = false)
		: key(k.str()), value(v.str()), hasSubKey(subkey) { }
		QueryValue(const StringView &k, const StringView &s, const StringView &v)
		: key(k.str()), subkey(s.str()), value(v.str()), hasSubKey(true) { }
	};

	using QueryVec = Vector<QueryValue>;

	static Vector<String> parsePath(const StringView &);
	static QueryVec parseArgs(const StringView &);
	static data::Value parseDataArgs(const StringView &, size_t maxVarSize = maxOf<size_t>());

	static bool validateEmail(String &);

	Url() : _port(0) { }
	Url(const Url &u) = default;
	Url(Url &&u) = default;
	Url & operator = (const Url &u) = default;
	Url & operator = (Url &&u) = default;

	inline Url(const StringView &str) { set(str); }
	inline Url & operator = (const StringView &str) { return set(str); }
	inline operator String () const { return get(); }

	Url copy() const;
	Url &clear();

	bool parse(const StringView &);
	Url &set(const StringView &);
	Url &setScheme(const StringView &);
	Url &setUser(const StringView &);
	Url &setPassword(const StringView &);
	Url &setHost(const StringView &);
	Url &setPort(uint32_t);

	Url &setPath(const StringView &);
	Url &setPath(const Vector<String> &);
	Url &setPath(Vector<String> &&);
	Url &addPath(const StringView &);
	Url &addPath(const Vector<String> &);
	Url &addPath(Vector<String> &&);
	Url &addPathComponent(const StringView &);
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

	Url &addQuery(const StringView &, const data::Value &);

	Url &setFragment(const StringView &);

	String get() const;
	data::Value encode() const;

	inline StringView getScheme() const { return _scheme; }
	inline StringView getUser() const { return _user; }
	inline StringView getPassword() const { return _password; }
	inline StringView getHost() const { return _host; }
	inline uint32_t getPort() const { return _port; }
    inline const Vector<String> &getPath() const { return _path; }
    inline const QueryVec &getQuery() const { return _query; }
    inline StringView getOriginalQuery() const { return _originalQuery; }
	inline StringView getFragment() const { return _fragment; }

protected:
	bool parseInternal(const StringView &);

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
