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

#ifndef SERENITY_SRC_REQUEST_REQUEST_H_
#define SERENITY_SRC_REQUEST_REQUEST_H_

#include "SPUrl.h"
#include "Connection.h"
#include "StorageAdapter.h"

NS_SA_BEGIN

enum class CookieFlags {
	Secure = 1 << 0,
	HttpOnly = 1 << 1,
	SetOnError = 1 << 2,
	SetOnSuccess = 1 << 3,

	Default = 1 | 2 | 8 // Secure | HttpOnly | SetOnRequest
};

SP_DEFINE_ENUM_AS_MASK(CookieFlags)

class Request : public std::basic_ostream<char, std::char_traits<char>>, public AllocPool {
public:
	using char_type = char;
	using traits_type = std::char_traits<char>;
	using ostream_type = std::basic_ostream<char_type, traits_type>;
	using Require = InputConfig::Require;

	enum Method : int {
		Get =				M_GET,
		Put =				M_PUT,
		Post =				M_POST,
		Delete =			M_DELETE,
		Connect =			M_CONNECT,
		Options =			M_OPTIONS,
		Trace =				M_TRACE,
		Patch =				M_PATCH,
		Propfind =			M_PROPFIND,
		Proppatch =			M_PROPPATCH,
		MkCol =				M_MKCOL,
		Copy =				M_COPY,
		Move =				M_MOVE,
		Lock =				M_LOCK,
		Unlock =			M_UNLOCK,
		VersionControl =	M_VERSION_CONTROL,
		Checkout =			M_CHECKOUT,
		Uncheckout =		M_UNCHECKOUT,
		Checkin =			M_CHECKIN,
		Update =			M_UPDATE,
		Label =				M_LABEL,
		Report =			M_REPORT,
		MkWorkspace =		M_MKWORKSPACE,
		MkActivity =		M_MKACTIVITY,
		BaselineControl =	M_BASELINE_CONTROL,
		Merge =				M_MERGE,
		Invalid =			M_INVALID,
	};

	struct CookieStorage {
		String data;
		CookieFlags flags;
		TimeInterval maxAge;
	};

	Request();
	Request(request_rec *);
	Request & operator =(request_rec *);

	Request(Request &&);
	Request & operator =(Request &&);

	Request(const Request &);
	Request & operator =(const Request &);

	request_rec *request() const { return _request; }
	operator request_rec *() const { return _request; }
	operator bool () const { return _request != nullptr; }

	void setRequestHandler(RequestHandler *);
	RequestHandler *getRequestHandler() const;

	void writeData(const data::Value &, bool allowJsonP = false);

	void clearFilters();

public: /* request data */
	apr::weak_string getRequestLine() const;
	bool isSimpleRequest() const;
	bool isHeaderRequest() const;

	int getProtocolNumber() const;
	apr::weak_string getProtocol() const;
	apr::weak_string getHostname() const;

	apr_time_t getRequestTime() const;

	apr::weak_string getStatusLine() const;
	int getStatus() const;

	Method getMethod() const;
	apr::weak_string getMethodName() const;

	apr::weak_string getRangeLine() const;
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

	apr::weak_string getDocumentRoot() const;
	apr::weak_string getContentType() const;
	apr::weak_string getHandler() const;
	apr::weak_string getContentEncoding() const;

	apr::weak_string getRequestUser() const;
	apr::weak_string getAuthType() const;

	apr::weak_string getUnparsedUri() const;
	apr::weak_string getUri() const;
	apr::weak_string getFilename() const;
	apr::weak_string getCanonicalFilename() const;
	apr::weak_string getPathInfo() const;
	apr::weak_string getQueryArgs() const;

	bool isEosSent() const;
	bool hasCache() const;
	bool hasLocalCopy() const;
	bool isSecureConnection() const;

	apr::weak_string getUseragentIp() const;

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
	void storeObject(void *ptr, const String &key, Function<void()> && = nullptr) const;

	template <typename T = void>
	T *getObject(const String &key) const {
		return apr::pool::get<T>(_request->pool, key);
	}

	const apr::vector<apr::string> & getParsedQueryPath() const;
	const data::Value &getParsedQueryArgs() const;

	Server server() const;
	Connection connection() const;
	apr_pool_t *pool() const;

	storage::Adapter * storage() const;

	// authorize user and create session for 'maxAge' seconds
	// previous session (if existed) will be canceled
	Session *authorizeUser(User *, TimeInterval maxAge);

	// explicitly set user authority to request
	void setUser(User *);

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

	const Vector<data::Value> & getDebugMessages() const;
	const Vector<data::Value> & getErrorMessages() const;

	void addErrorMessage(data::Value &&);
	void addDebugMessage(data::Value &&);

	void addCleanup(const Function<void()> &);

	void setInputFilter(InputFilter *);
	InputFilter *getInputFilter() const;

protected:
	struct Config;

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
	request_rec *_request = nullptr;
	Config *_config = nullptr;
};

NS_SA_END

#endif /* SERENITY_SRC_REQUEST_REQUEST_H_ */
