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

#include "Define.h"
#include "Request.h"
#include "Root.h"
#include "UrlEncodeParser.h"
#include "SPFilesystem.h"

#include "User.h"
#include "Session.h"

#include "Output.h"
#include "TemplateCache.h"

APLOG_USE_MODULE(serenity);

NS_SA_BEGIN

struct Request::Config : public AllocPool {
	static Config *get(request_rec *r) {
		if (!r) { return nullptr; }
		auto cfg = (Config *)ap_get_module_config(r->request_config, &serenity_module);
		if (cfg) { return cfg; }

		return apr::AllocStack::perform([&] () -> Config * {
			return new Config(r);
		}, r);
	}

	Config(request_rec *r) : _begin(apr_time_now()), _request(r) {
		ap_set_module_config(r->request_config, &serenity_module, this);

		if (r->args) {
			_data = Url::parseDataArgs(String::make_weak(r->args), 1_KiB);
		}

		_path = Url::parsePath(apr::string::make_weak(r->uri));
		registerCleanupDestructor(this, r->pool);
	}

	~Config() {
		if (_session) {
			delete _session;
		}
	}

	const apr::vector<apr::string> &getPath() {
		return _path;
	}

	const data::Value &getData() {
		return _data;
	}

	database::Handle *acquireDatabase(request_rec *r) {
		if (!_database) {
			auto handle = Root::getInstance()->dbdRequestAcquire(r);
			if (handle) {
				_database = new database::Handle(r->pool, handle);
			}
		}
		return _database;
	}

	void finalize() { }

	apr_time_t _begin;

	Vector<String> _path;
	data::Value _data;

	Vector<data::Value> _debug;
	Vector<data::Value> _errors;
	InputConfig _config;

	database::Handle *_database = nullptr;
	RequestHandler *_handler = nullptr;
	Session *_session = nullptr;
	User *_user = nullptr;
	request_rec *_request = nullptr;
};

Request::Request() : _buffer(nullptr), _request(nullptr), _config(nullptr) { }
Request::Request(request_rec *r) : _buffer(r), _request(r), _config(Config::get(r)) {
	this->init(&_buffer);
}
Request & Request::operator =(request_rec *r) {
	_buffer = Buffer(r);
	_request = r;
	_config = Config::get(r);
	this->init(&_buffer);
	return *this;
}

Request::Request(Request &&other) : _buffer(other._request), _request(other._request), _config(other._config) {
	this->init(&_buffer);
}
Request & Request::operator =(Request &&other) {
	_buffer = Buffer(other._request);
	_request = other._request;
	_config = other._config;
	this->init(&_buffer);
	return *this;
}

Request::Request(const Request &other) :_buffer(other._request), _request(other._request), _config(other._config) {
	this->init(&_buffer);
}
Request & Request::operator =(const Request &other) {
	_buffer = Buffer(other._request);
	_request = other._request;
	_config = other._config;
	this->init(&_buffer);
	return *this;
}

Request::Buffer::Buffer(request_rec *r) : _request(r) { }
Request::Buffer::Buffer(Buffer&&other) : _request(other._request) { }
Request::Buffer& Request::Buffer::operator=(Buffer&&other) { _request = other._request; return *this; }

Request::Buffer::Buffer(const Buffer&other) : _request(other._request) { }
Request::Buffer& Request::Buffer::operator=(const Buffer&other) { _request = other._request; return *this; }

Request::Buffer::int_type Request::Buffer::overflow(int_type c) {
	ap_rputc(c, _request);
	return c;
}

Request::Buffer::pos_type Request::Buffer::seekoff(off_type off, ios_base::seekdir way, ios_base::openmode) { return _request->bytes_sent; }
Request::Buffer::pos_type Request::Buffer::seekpos(pos_type pos, ios_base::openmode mode) { return _request->bytes_sent; }

int Request::Buffer::sync() {
	ap_rflush(_request);
	return 0;
}

Request::Buffer::streamsize Request::Buffer::xsputn(const char_type* s, streamsize n) {
	return ap_rwrite((const void *)s, n, _request);
}

apr::weak_string Request::getRequestLine() const {
	return apr::string::make_weak(_request->the_request, _request);
}
bool Request::isSimpleRequest() const {
	return _request->assbackwards;
}
bool Request::isHeaderRequest() const {
	return _request->header_only;
}

void Request::setRequestHandler(RequestHandler *h) {
	_config->_handler = h;
}
RequestHandler *Request::getRequestHandler() const {
	return _config->_handler;
}

void Request::writeData(const data::Value &data, bool allowJsonP) {
	output::writeData(*this, data, allowJsonP);
}

void Request::clearFilters() {
    ap_filter_t *f;
    for (f = _request->input_filters; f != NULL; f = f->next) {
        if ((f->frec != NULL) && (f->frec->name != NULL) && !strcasecmp(f->frec->name, "http_in")) {
            ap_remove_input_filter(f);
            break;
        }
    }
}

int Request::getProtocolNumber() const {
	return _request->proto_num;
}
apr::weak_string Request::getProtocol() const {
	return apr::string::make_weak(_request->protocol, _request);
}
apr::weak_string Request::getHostname() const {
	return apr::string::make_weak(_request->hostname, _request);
}

apr_time_t Request::getRequestTime() const {
	return _request->request_time;
}

apr::weak_string Request::getStatusLine() const {
	return apr::string::make_weak(_request->status_line, _request);
}
int Request::getStatus() const {
	return _request->status;
}

Request::Method Request::getMethod() const {
	return Method(_request->method_number);
}
apr::weak_string Request::getMethodName() const {
	return apr::string::make_weak(_request->method, _request);
}

apr::weak_string Request::getRangeLine() const {
	return apr::string::make_weak(_request->range, _request);
}
apr_off_t Request::getContentLength() const {
	return _request->clength;
}
bool Request::isChunked() const {
	return _request->chunked;
}

apr_off_t Request::getRemainingLength() const {
	return _request->remaining;
}
apr_off_t Request::getReadLength() const {
	return _request->read_length;
}

apr::table Request::getRequestHeaders() const {
	return apr::table::wrap(_request->headers_in);
}
apr::table Request::getResponseHeaders() const {
	return apr::table::wrap(_request->headers_out);
}
apr::table Request::getErrorHeaders() const {
	return apr::table::wrap(_request->err_headers_out);
}
apr::table Request::getSubprocessEnvironment() const {
	return apr::table::wrap(_request->subprocess_env);
}
apr::table Request::getNotes() const {
	return apr::table::wrap(_request->notes);
}
apr::uri Request::getParsedURI() const {
	return apr::uri(_request->parsed_uri, _request->pool);
}

apr::weak_string Request::getDocumentRoot() const {
	return apr::string::make_weak(ap_context_document_root(_request), _request);
}
apr::weak_string Request::getContentType() const {
	return apr::string::make_weak(_request->content_type, _request);
}
apr::weak_string Request::getHandler() const {
	return apr::string::make_weak(_request->handler, _request);
}
apr::weak_string Request::getContentEncoding() const {
	return apr::string::make_weak(_request->content_encoding, _request);
}

apr::weak_string Request::getRequestUser() const {
	return apr::string::make_weak(_request->user, _request);
}
apr::weak_string Request::getAuthType() const {
	return apr::string::make_weak(_request->ap_auth_type, _request);
}

apr::weak_string Request::getUnparsedUri() const {
	return apr::string::make_weak(_request->unparsed_uri, _request);
}
apr::weak_string Request::getUri() const {
	return apr::string::make_weak(_request->uri, _request);
}
apr::weak_string Request::getFilename() const {
	return apr::string::make_weak(_request->filename, _request);
}
apr::weak_string Request::getCanonicalFilename() const {
	return apr::string::make_weak(_request->canonical_filename, _request);
}
apr::weak_string Request::getPathInfo() const {
	return apr::string::make_weak(_request->path_info, _request);
}
apr::weak_string Request::getQueryArgs() const {
	return apr::string::make_weak(_request->args, _request);
}

bool Request::isEosSent() const {
	return _request->eos_sent;
}
bool Request::hasCache() const {
	return !_request->no_cache;
}
bool Request::hasLocalCopy() const {
	return !_request->no_local_copy;
}

bool Request::isSecureConnection() const {
	return Root::getInstance()->isSecureConnection(_request->connection);
}

apr::weak_string Request::getUseragentIp() const {
	return apr::string::make_weak(_request->useragent_ip);
}

/* request params setters */
void Request::setDocumentRoot(apr::string &&str) {
	ap_set_document_root(_request, str.c_str());
	str.force_clear();
}
void Request::setContentType(apr::string &&str) {
	ap_set_content_type(_request, str.c_str());
	str.force_clear();
}
void Request::setHandler(apr::string &&str) {
	_request->handler = str.c_str();
	str.force_clear();
}

void Request::setCookie(const String &name, const String &value, TimeInterval maxAge, CookieFlags flags, const String &path) {
	apr::ostringstream attrs;
	if (((flags & CookieFlags::Secure) != 0 && isSecureConnection()) && ((flags & CookieFlags::HttpOnly) != 0)) {
		attrs << "HttpOnly;Secure;Version=1";
	} else if (((flags & CookieFlags::Secure) != 0) && isSecureConnection()) {
		attrs << "Secure;Version=1";
	} else if ((flags & CookieFlags::HttpOnly) != 0) {
		attrs << "HttpOnly;Version=1";
	} else {
		attrs << "Version=1";
	}

	if (!path.empty()) {
		attrs << ";Path=" << path;
	}

	if (((flags & CookieFlags::SetOnError) != 0) && ((flags & CookieFlags::SetOnSuccess) != 0)) {
		ap_cookie_write(_request, name.c_str(), string::urlencode(value).c_str(), attrs.weak().c_str(), maxAge.toSeconds(), _request->err_headers_out, _request->headers_out, NULL);
	} else if ((flags & CookieFlags::SetOnError) != 0) {
		ap_cookie_write(_request, name.c_str(), string::urlencode(value).c_str(), attrs.weak().c_str(), maxAge.toSeconds(), _request->err_headers_out, NULL);
	} else if ((flags & CookieFlags::SetOnSuccess) != 0) {
		ap_cookie_write(_request, name.c_str(), string::urlencode(value).c_str(), attrs.weak().c_str(), maxAge.toSeconds(), _request->headers_out, NULL);
	}
}
void Request::removeCookie(const String &name, CookieFlags flags) {
	if (((flags & CookieFlags::SetOnError) != 0) && ((flags & CookieFlags::SetOnSuccess) != 0)) {
		ap_cookie_remove(_request, name.c_str(), CLEAR_ATTRS, _request->err_headers_out, _request->headers_out, NULL);
	} else if ((flags & CookieFlags::SetOnError) != 0) {
		ap_cookie_remove(_request, name.c_str(), CLEAR_ATTRS, _request->err_headers_out, NULL);
	} else if ((flags & CookieFlags::SetOnSuccess) != 0) {
		ap_cookie_remove(_request, name.c_str(), CLEAR_ATTRS, _request->headers_out, NULL);
	}
}
apr::weak_string Request::getCookie(const String &name, bool removeFromHeadersTable) const {
	const char *val = nullptr;
	if (ap_cookie_read(_request, name.c_str(), &val, (int)removeFromHeadersTable) == APR_SUCCESS) {
		if (val) {
			if (removeFromHeadersTable) {
				ap_unescape_urlencoded((char *)val);
				return apr::string::make_weak(val);
			} else {
				apr::string ret(val);
				ap_unescape_urlencoded(ret.data());
				return ret;
			}
		}
	}
	return apr::weak_string();
}

Session *Request::authorizeUser(User *user, TimeInterval maxAge) {
	if (_config->_session) {
		_config->_session->cancel();
	}
	auto s = new Session(*this, user, maxAge);
	if (s->isValid()) {
		_config->_session = s;
		_config->_user = user;
		return s;
	}
	return nullptr;
}

void Request::setUser(User *u) {
	if (u) {
		_config->_user = u;
	}
}

Session *Request::getSession() {
	if (!_config->_session) {
		_config->_session = new Session(*this);
	}

	if (_config->_session->isValid()) {
		return _config->_session;
	}
	return nullptr;
}

User *Request::getUser() {
	if (!_config->_user) {
		if (auto s = getSession()) {
			_config->_user = s->getUser();
		}
	}
	return _config->_user;
}

User *Request::getAuthorizedUser() const {
	return _config->_user;
}

void Request::setContentEncoding(apr::string &&str) {
	_request->content_encoding = str.c_str();
	str.force_clear();
}
void Request::setFilename(apr::string &&str) {
	_request->filename = str.data();
	_request->canonical_filename = _request->filename;
	apr_stat(&_request->finfo, _request->filename, APR_FINFO_NORM, pool());
	str.force_clear();
}

void Request::setStatus(int status, apr::string && str) {
	_request->status = status;
	if (!str.empty()) {
		_request->status_line = str.c_str();
		str.force_clear();
	} else {
		_request->status_line = ap_get_status_line(status);
	}
}

InputConfig & Request::getInputConfig() {
	return _config->_config;
}
const InputConfig & Request::getInputConfig() const {
	return _config->_config;
}

void Request::setRequiredData(InputConfig::Require r) {
	_config->_config.required = r;
}
void Request::setMaxRequestSize(size_t s) {
	_config->_config.maxRequestSize = s;
}
void Request::setMaxVarSize(size_t s) {
	_config->_config.maxVarSize = s;
}
void Request::setMaxFileSize(size_t s) {
	_config->_config.maxFileSize = s;
}

const apr::vector<apr::string> & Request::getParsedQueryPath() const {
	return _config->getPath();
}
const data::Value & Request::getParsedQueryArgs() const {
	return _config->getData();
}

Server Request::server() const {
	return Server(_request->server);
}

Connection Request::connection() const {
	return Connection(_request->connection);
}
apr_pool_t *Request::pool() const {
	return _request->pool;
}

storage::Adapter *Request::storage() const {
	return _config->acquireDatabase(_request);
}

const Vector<data::Value> & Request::getDebugMessages() const {
	return _config->_debug;
}

const Vector<data::Value> & Request::getErrorMessages() const {
	return _config->_errors;
}

void Request::addErrorMessage(data::Value &&val) {
	if (_config) {
		_config->_errors.emplace_back(std::move(val));
	}
}

void Request::addDebugMessage(data::Value &&val) {
	if (_config) {
		_config->_debug.emplace_back(std::move(val));
	}
}

static apr_status_t sa_request_custom_cleanup(void *ptr) {
	if (ptr) {
		auto ref = (Function<void()> *)ptr;
		(*ref)();
	}
	return APR_SUCCESS;
}

void Request::addCleanup(const Function<void()> &cb) {
	auto ref = new (pool()) Function<void()>(cb);
	apr_pool_cleanup_register(pool(), ref, &sa_request_custom_cleanup, &apr_pool_cleanup_null);
}

bool Request::isAdministrative() {
	bool isAuthorized = (getUser() && getUser()->isAdmin());
	if (isAuthorized) {
		return true;
	}

	auto userIp = getUseragentIp();
	if (isSecureConnection() || strncmp(userIp.data(), "127.", 4) == 0) {
		// try cross-server auth
		auto headers = getRequestHeaders();
		auto ua = headers.at("User-Agent");
		auto auth = headers.at("X-Serenity-Auth");
		auto name = headers.at("X-Serenity-Auth-User");
		auto passwd = headers.at("X-Serenity-Auth-Password");

		if (!ua.empty() && !auth.empty() && !name.empty() && !passwd.empty()) {
			if (strncasecmp(ua.data(), "SerenityServerAuth/1", 20) == 0 && auth == "1") {
				auto user = User::get(storage(), name, passwd);
				if (user && user->isAdmin()) {
					isAuthorized = true;
					messages::debug("Auth", "Authorized on single-request basis");
				}
			}
		}
	}

#ifdef DEBUG
	if (_config->_data.getBool("admin")) {
		isAuthorized = true;
	}
#endif

	return isAuthorized;
}

int Request::redirectTo(String && location) {
	getResponseHeaders().emplace("Location", std::move(location));
	return HTTP_SEE_OTHER;
}

int Request::sendFile(String && file, size_t cacheTime) {
	setFilename(filesystem::writablePath(file));
	if (cacheTime == 0) {
		getResponseHeaders().emplace("Cache-Control", "no-cache, must-revalidate");
	} else if (cacheTime < SIZE_MAX) {
		getResponseHeaders().emplace("Cache-Control", toString("max-age=", cacheTime, ", must-revalidate", cacheTime));
	}
	return OK;
}

int Request::sendFile(String && file, String && contentType, size_t cacheTime) {
	if (!contentType.empty()) {
		setContentType(std::move(contentType));
	}
	return sendFile(std::move(file), cacheTime);
}

apr::string Request::getFullHostname(int port) {
	if (port == -1) {
		port = getParsedURI().port();
	}

	bool isSecure = isSecureConnection();
	apr::ostringstream ret;
	ret << (isSecure?"https":"http");
	ret << getHostname();
	if ((isSecure && port != 443) || (!isSecure && port != 80)) {
		ret << ':' << port;
	}

	return ret.str();
}

void Request::runTemplate(String && path, const Function<void(tpl::Exec &, Request &)> &cb) {
	auto cache = server().getTemplateCache();
	cache->runTemplate(path, *this, cb);
}

NS_SA_END
