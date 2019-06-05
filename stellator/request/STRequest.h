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

#ifndef STELLATOR_REQUEST_STREQUEST_H_
#define STELLATOR_REQUEST_STREQUEST_H_

#include "SPUrl.h"
#include "STConnection.h"

namespace stellator {

enum class CookieFlags {
	Secure = 1 << 0,
	HttpOnly = 1 << 1,
	SetOnError = 1 << 2,
	SetOnSuccess = 1 << 3,

	Default = 1 | 2 | 8 // Secure | HttpOnly | SetOnRequest
};

SP_DEFINE_ENUM_AS_MASK(CookieFlags)

class Request : public std::basic_ostream<char, std::char_traits<char>>, public mem::AllocBase {
public:
	using char_type = char;
	using traits_type = std::char_traits<char>;
	using ostream_type = std::basic_ostream<char_type, traits_type>;
	using Require = InputConfig::Require;

	struct Config;

	enum Method {
		None = 0,
		Get,
		Put,
		Post,
		Delete,
		Connect,
		Options,
		Trace,
		Patch,
		Invalid,
	};

	struct CookieStorage {
		mem::String data;
		CookieFlags flags;
		mem::TimeInterval maxAge;
	};

	Request();
	Request(Config *);
	Request & operator =(Config *);

	Request(Request &&);
	Request & operator =(Request &&);

	Request(const Request &);
	Request & operator =(const Request &);

	void setRequestHandler(RequestHandler *);
	RequestHandler *getRequestHandler() const;

	void setHookErrors(bool);
	bool isHookErrors() const;

	void writeData(const mem::Value &, bool allowJsonP = false);

	void clearFilters();

public: /* request data */
	mem::StringView getRequestLine() const;
	bool isSimpleRequest() const;
	bool isHeaderRequest() const;

	int getProtocolNumber() const;
	mem::StringView getProtocol() const;
	mem::StringView getHostname() const;

	mem::Time getRequestTime() const;

	mem::StringView getStatusLine() const;
	int getStatus() const;

	Method getMethod() const;
	mem::StringView getMethodName() const;

	mem::StringView getRangeLine() const;
	apr_off_t getContentLength() const;
	bool isChunked() const;

	apr_off_t getRemainingLength() const;
	apr_off_t getReadLength() const;

	apr::table getRequestHeaders() const;
	apr::table getResponseHeaders() const;
	apr::table getErrorHeaders() const;
	apr::table getSubprocessEnvironment() const;
	apr::table getNotes() const;
	apr::uri getParsedURI() const;

	mem::StringView getDocumentRoot() const;
	mem::StringView getContentType() const;
	mem::StringView getHandler() const;
	mem::StringView getContentEncoding() const;

	mem::StringView getRequestUser() const;
	mem::StringView getAuthType() const;

	mem::StringView getUnparsedUri() const;
	mem::StringView getUri() const;
	mem::StringView getFilename() const;
	mem::StringView getCanonicalFilename() const;
	mem::StringView getPathInfo() const;
	mem::StringView getQueryArgs() const;

	bool isEosSent() const;
	bool hasCache() const;
	bool hasLocalCopy() const;
	bool isSecureConnection() const;

	mem::StringView getUseragentIp() const;

public: /* request params setters */
	void setDocumentRoot(apr::string &&str);
	void setContentType(apr::string &&str);
	void setHandler(apr::string &&str);
	void setContentEncoding(apr::string &&str);

	/* set path for file, that should be returned in response via sendfile */
	void setFilename(apr::string &&str);

	/* set HTTP status code and response status line ("404 NOT FOUND", etc.)
	 * if no string provided, default status line for code will be used */
	void setStatus(int status, apr::string &&str = apr::string());

	void setCookie(const String &name, const String &value, TimeInterval maxAge = TimeInterval(), CookieFlags flags = CookieFlags::Default);
	void removeCookie(const String &name, CookieFlags flags = CookieFlags::Default);

	// cookies, that will be sent in server response
	const Map<String, CookieStorage> getResponseCookies() const;

	apr::weak_string getCookie(const String &name, bool removeFromHeadersTable = true) const;

	int redirectTo(String && location);
	int sendFile(String && path, size_t cacheTimeInSeconds = maxOf<size_t>());
	int sendFile(String && path, String && contentType, size_t cacheTimeInSeconds = maxOf<size_t>());

	void runTemplate(String && path, const Function<void(tpl::Exec &, Request &)> &);
	int runPug(const StringView & path, const Function<bool(pug::Context &, const pug::Template &)> & = nullptr);

	apr::string getFullHostname(int port = -1) const;

	// true if successful cache test
	bool checkCacheHeaders(Time, const StringView &etag);
	bool checkCacheHeaders(Time, uint32_t idHash);

public: /* input config */
	InputConfig & getInputConfig();
	const InputConfig & getInputConfig() const;

	/* Sets required data flags for input filter on POST and PUT queries
	 * if no data is requested, onInsertFilter and onFilterComplete phases will be ignored */
	void setRequiredData(InputConfig::Require);

	/* Sets max size for input query content length, checked from Content-Length header */
	void setMaxRequestSize(size_t);

	/* Sets max size for a single input variable (in urlencoded and multipart types)
	 * Has no effect on JSON or CBOR input */
	void setMaxVarSize(size_t);

	/* Sets max size for input file (for multipart or unrecognized Content-Type)
	 * File size will be checked before writing next chunk of data,
	 * if new size will exceed max size, filter will be aborted */
	void setMaxFileSize(size_t);

public: /* engine and errors */
	void storeObject(void *ptr, const StringView &key, Function<void()> && = nullptr) const;

	template <typename T = void>
	T *getObject(const StringView &key) const {
		return apr::pool::get<T>(_request->pool, key);
	}

	const apr::vector<apr::string> & getParsedQueryPath() const;
	const data::Value &getParsedQueryArgs() const;

	Server server() const;
	Connection connection() const;
	apr_pool_t *pool() const;

	storage::Adapter storage() const;

	// authorize user and create session for 'maxAge' seconds
	// previous session (if existed) will be canceled
	Session *authorizeUser(User *, TimeInterval maxAge);

	// explicitly set user authority to request
	void setUser(User *);

	void setAltUserId(int64_t);

	// try to access to session data
	// if session object already created - returns it
	// if all authorization data is valid - session object will be created
	// otherwise - returns nullptr
	Session *getSession();

	// try to get user data from session (by 'getSession' call)
	// if session is not existed - returns nullptr
	// if session is anonymous - returns nullptr
	User *getUser();
	User *getAuthorizedUser() const;

	int64_t getUserId() const;

	// check if request is sent by server/handler administrator
	// uses 'User::isAdmin' or tries to authorize admin by cross-server protocol
	bool isAdministrative();

	storage::AccessRoleId getAccessRole() const;
	void setAccessRole(storage::AccessRoleId) const;

	const Vector<data::Value> & getDebugMessages() const;
	const Vector<data::Value> & getErrorMessages() const;

	void addErrorMessage(data::Value &&);
	void addDebugMessage(data::Value &&);

	void addCleanup(const Function<void()> &);

	void setInputFilter(InputFilter *);
	InputFilter *getInputFilter() const;

protected:
	struct Config;

	void initScriptContext(pug::Context &ctx);

	/* Buffer class used as basic_streambuf to allow stream writing to request
	 * like 'request << "String you want to send"; */
	class Buffer : public std::basic_streambuf<char, std::char_traits<char>> {
	public:
		using int_type = typename traits_type::int_type;
		using pos_type = typename traits_type::pos_type;
		using off_type = typename traits_type::off_type;

		using streamsize = std::streamsize;
		using streamoff = std::streamoff;

		using ios_base = std::ios_base;

		Buffer(request_rec *r);
		Buffer(Buffer&&);
		Buffer& operator=(Buffer&&);

		Buffer(const Buffer&);
		Buffer& operator=(const Buffer&);

	protected:
		virtual int_type overflow(int_type c = traits_type::eof()) override;

		virtual pos_type seekoff(off_type off, ios_base::seekdir way, ios_base::openmode) override;
		virtual pos_type seekpos(pos_type pos, ios_base::openmode mode) override;

		virtual int sync() override;

		virtual streamsize xsputn(const char_type* s, streamsize n) override;

		request_rec *_request;
	};

	Buffer _buffer;
	Config *_config = nullptr;
};

}

#endif /* STELLATOR_REQUEST_STREQUEST_H_ */
