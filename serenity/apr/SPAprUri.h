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

#ifndef COMMON_APR_SPAPRURI_H_
#define COMMON_APR_SPAPRURI_H_

#include "SPMemString.h"

#if SPAPR

NS_APR_BEGIN

class uri : public AllocPool {
public:
	uri(apr_pool_t *p = getCurrentPool()) : _pool(p) { memset(&_uri, 0, sizeof(apr_uri_t)); }
	uri(apr_uri_t uri, apr_pool_t *p = getCurrentPool()) : _pool(p), _uri(uri) { }
	uri(const uri &other, apr_pool_t *p = getCurrentPool()) : _pool(p), _uri(other.clone(p)._uri) { }
	uri(uri &&other, apr_pool_t *p = getCurrentPool()) : _pool(p), _uri(other._uri) { }
	uri(const apr::string &str, apr_pool_t *p = getCurrentPool()) : _pool(p) { parse(str); }

	apr_pool_t *get_allocator() const { return _pool; }

	uri &operator=(apr_uri_t u) { _uri = u; return *this; }
	uri &operator=(const uri & u) { _uri = u.clone(_pool)._uri; return *this; }
	uri &operator=(uri && u) { if (u.get_allocator() == _pool) { _uri = u._uri; } else { _uri = u.clone(_pool)._uri; } return *this; }
	uri &operator=(const apr::string &str) { return parse(str); }

	uri &parse(const apr::string &str) {
		apr_uri_parse(_pool, str.c_str(), &_uri);
		return *this;
	}

	uri clone(apr_pool_t *p = getCurrentPool()) const {
		apr_uri_t ret;
		memset(&ret, 0, sizeof(apr_uri_t));
		ret.scheme = apr_pstrdup(p, _uri.scheme);
		ret.hostinfo = apr_pstrdup(p, _uri.hostinfo);
		ret.user = apr_pstrdup(p, _uri.user);
		ret.password = apr_pstrdup(p, _uri.password);
		ret.hostname = apr_pstrdup(p, _uri.hostname);
		ret.port_str = apr_pstrdup(p, _uri.port_str);
		ret.path = apr_pstrdup(p, _uri.path);
		ret.query = apr_pstrdup(p, _uri.query);
		ret.fragment = apr_pstrdup(p, _uri.fragment);
		ret.port = _uri.port;
		ret.is_initialized = _uri.is_initialized;
		return uri(ret, p);
	}

	enum OmitFlags {
		OmitSize = APR_URI_UNP_OMITSITEPART,
		/** Just omit user */
		OmitUser = APR_URI_UNP_OMITUSER,
		/** Just omit password */
		OmitPassword = APR_URI_UNP_OMITPASSWORD,
		/** omit "user:password\@" part */
		OmitUserInfo = APR_URI_UNP_OMITUSERINFO,
		/** Show plain text password (default: show XXXXXXXX) */
		RevealPassword = APR_URI_UNP_REVEALPASSWORD,
		/** Show "scheme://user\@site:port" only */
		OmitPathInfo = APR_URI_UNP_OMITPATHINFO,
		/** Omit the "?queryarg" from the path */
		OmitQuery = APR_URI_UNP_OMITQUERY,
	};

	apr::string str(apr_pool_t *p = getCurrentPool()) const {
		return apr::string::wrap(apr_uri_unparse(p, &_uri, 0), p);
	}

	apr::string str(OmitFlags flags, apr_pool_t *p = getCurrentPool()) const {
		return apr::string::wrap(apr_uri_unparse(p, &_uri, std::underlying_type<OmitFlags>::type(flags)), p);
	}

	bool empty() const { return _uri.is_initialized == 0; }
	operator bool () const { return _uri.is_initialized == 1; }
	operator apr::string () const { return str(); }

	apr::weak_string scheme() const { return apr::string::make_weak(_uri.scheme); }
	apr::weak_string hostinfo() const { return apr::string::make_weak(_uri.hostinfo); }
	apr::weak_string user() const { return apr::string::make_weak(_uri.user); }
	apr::weak_string password() const { return apr::string::make_weak(_uri.password); }
	apr::weak_string hostname() const { return apr::string::make_weak(_uri.hostname); }
	apr::weak_string port_str() const { return apr::string::make_weak(_uri.port_str); }
	apr::weak_string path() const { return apr::string::make_weak(_uri.path); }
	apr::weak_string query() const { return apr::string::make_weak(_uri.query); }
	apr::weak_string fragment() const { return apr::string::make_weak(_uri.fragment); }
	apr_port_t port() const { return _uri.port; }

protected:
	apr_pool_t *_pool;
	apr_uri_t _uri;
};

inline constexpr uri::OmitFlags operator |(uri::OmitFlags r, uri::OmitFlags l) {
	return uri::OmitFlags(std::underlying_type<uri::OmitFlags>::type(r) | std::underlying_type<uri::OmitFlags>::type(l));
}

NS_APR_END

#endif

#endif /* COMMON_APR_SPAPRURI_H_ */
