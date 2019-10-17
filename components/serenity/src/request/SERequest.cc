// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SERequest.h"

#include "SPFilesystem.h"
#include "SPugCache.h"
#include "SPugContext.h"
#include "SPugVariable.h"
#include "Root.h"
#include "UrlEncodeParser.h"
#include "STPqHandle.h"

#include "Session.h"

#include "SPUrl.h"
#include "Output.h"
#include "TemplateCache.h"

NS_SA_BEGIN

struct Request::Config : public AllocPool {
	static Config *get(request_rec *r) {
		if (!r) { return nullptr; }
		auto cfg = (Config *)ap_get_module_config(r->request_config, &serenity_module);
		if (cfg) { return cfg; }

		return apr::pool::perform([&] () -> Config * {
			return new Config(r);
		}, r);
	}

	Config(request_rec *r) : _begin(apr_time_now()), _request(r) {
		ap_set_module_config(r->request_config, &serenity_module, this);

		if (r->args) {
			StringView str(String::make_weak(r->args));
			if (str.is('(')) {
				_data = data::serenity::read<mem::Interface>(str);
			}
			if (!str.empty()) {
				if (str.is('?') || str.is('&')) {
					++ str;
				}
				auto d = Url::parseDataArgs(str, 1_KiB);
				if (_data.empty()) {
					_data = std::move(d);
				} else {
					for (auto &it : d.asDict()) {
						if (!_data.hasValue(it.first)) {
							_data.setValue(std::move(it.second), it.first);
						}
					}
				}
			}
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

	db::pq::Handle *acquireDatabase(request_rec *r) {
		if (!_database) {
			auto handle = Root::getInstance()->dbdRequestAcquire(r);
			if (handle) {
				_database = new db::pq::Handle(db::pq::Driver::open(), db::pq::Driver::Handle(handle));
				if (_database) {
					auto conf = (Server::Config *)Request(r).server().getConfig();
					_database->setStorageTypeMap(&conf->storageTypes);
					_database->setCustomTypeMap(&conf->customTypes);
				}
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

	db::pq::Handle *_database = nullptr;
	RequestHandler *_handler = nullptr;
	Session *_session = nullptr;
	User *_user = nullptr;
	request_rec *_request = nullptr;
	InputFilter *_filter = nullptr;

	bool _hookErrors = true;

	Map<String, CookieStorage> _cookies;
	int64_t _altUserid = 0;
	storage::AccessRoleId _accessRole = storage::AccessRoleId::Nobody;
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
	return apr::string::make_weak(_request->the_request, _request->pool);
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

void Request::setHookErrors(bool value) {
	_config->_hookErrors = value;
}
bool Request::isHookErrors() const {
	return _config->_hookErrors;
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
	return apr::string::make_weak(_request->protocol, _request->pool);
}
apr::weak_string Request::getHostname() const {
	return apr::string::make_weak(_request->hostname, _request->pool);
}

apr_time_t Request::getRequestTime() const {
	return _request->request_time;
}

apr::weak_string Request::getStatusLine() const {
	return apr::string::make_weak(_request->status_line, _request->pool);
}
int Request::getStatus() const {
	return _request->status;
}

Request::Method Request::getMethod() const {
	return Method(_request->method_number);
}
apr::weak_string Request::getMethodName() const {
	return apr::string::make_weak(_request->method, _request->pool);
}

apr::weak_string Request::getRangeLine() const {
	return apr::string::make_weak(_request->range, _request->pool);
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
	return apr::string::make_weak(ap_context_document_root(_request), _request->pool);
}
apr::weak_string Request::getContentType() const {
	return apr::string::make_weak(_request->content_type, _request->pool);
}
apr::weak_string Request::getHandler() const {
	return apr::string::make_weak(_request->handler, _request->pool);
}
apr::weak_string Request::getContentEncoding() const {
	return apr::string::make_weak(_request->content_encoding, _request->pool);
}

apr::weak_string Request::getRequestUser() const {
	return apr::string::make_weak(_request->user, _request->pool);
}
apr::weak_string Request::getAuthType() const {
	return apr::string::make_weak(_request->ap_auth_type, _request->pool);
}

apr::weak_string Request::getUnparsedUri() const {
	return apr::string::make_weak(_request->unparsed_uri, _request->pool);
}
apr::weak_string Request::getUri() const {
	return apr::string::make_weak(_request->uri, _request->pool);
}
apr::weak_string Request::getFilename() const {
	return apr::string::make_weak(_request->filename, _request->pool);
}
apr::weak_string Request::getCanonicalFilename() const {
	return apr::string::make_weak(_request->canonical_filename, _request->pool);
}
apr::weak_string Request::getPathInfo() const {
	return apr::string::make_weak(_request->path_info, _request->pool);
}
apr::weak_string Request::getQueryArgs() const {
	return apr::string::make_weak(_request->args, _request->pool);
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
	ap_set_document_root(_request, str.extract());
}
void Request::setContentType(apr::string &&str) {
	ap_set_content_type(_request, str.extract());
}
void Request::setHandler(apr::string &&str) {
	_request->handler = str.extract();
}

void Request::setCookie(const String &name, const String &value, TimeInterval maxAge, CookieFlags flags) {
	_config->_cookies.emplace(name, CookieStorage{value, flags, maxAge});
}

void Request::removeCookie(const String &name, CookieFlags flags) {
	_config->_cookies.emplace(name, CookieStorage{String(), flags, TimeInterval::seconds(0)});
}

const Map<String, Request::CookieStorage> Request::getResponseCookies() const {
	return _config->_cookies;
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
				ret.resize(strlen(ret.data()));
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

void Request::setAltUserId(int64_t id) {
	_config->_altUserid = id;
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

int64_t Request::getUserId() const {
	if (_config->_user) {
		return _config->_user->getObjectId();
	} else {
		return _config->_altUserid;
	}
}

void Request::setContentEncoding(apr::string &&str) {
	_request->content_encoding = str.extract();
}
void Request::setFilename(apr::string &&str) {
	_request->filename = str.extract();
	_request->canonical_filename = _request->filename;
	apr_stat(&_request->finfo, _request->filename, APR_FINFO_NORM, pool());
}

void Request::setStatus(int status, apr::string && str) {
	_request->status = status;
	if (!str.empty()) {
		_request->status_line = str.extract();
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

void Request::storeObject(void *ptr, const StringView &key, Function<void()> &&cb) const {
	apr::pool::store(_request->pool, ptr, key, std::move(cb));
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

storage::Adapter Request::storage() const {
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
		memory::pool::push(ref->get_allocator());
		(*ref)();
		memory::pool::pop();
	}
	return APR_SUCCESS;
}

void Request::addCleanup(const Function<void()> &cb) {
	auto ref = new (pool()) Function<void()>(cb);
	apr_pool_cleanup_register(pool(), ref, &sa_request_custom_cleanup, &apr_pool_cleanup_null);
}

bool Request::isAdministrative() {
	return getAccessRole() == storage::AccessRoleId::Admin;
}

storage::AccessRoleId Request::getAccessRole() const {
	if (_config->_accessRole == storage::AccessRoleId::Nobody) {
		if (_config->_user) {
			if (_config->_user->isAdmin()) {
				_config->_accessRole = storage::AccessRoleId::Admin;
			} else {
				_config->_accessRole = storage::AccessRoleId::Authorized;
			}
		} else {
			Session s(*this, true);
			if (s.isValid()) {
				auto u = s.getUser();
				if (u) {
					if (u->isAdmin()) {
						_config->_accessRole = storage::AccessRoleId::Admin;
					} else {
						_config->_accessRole = storage::AccessRoleId::Authorized;
					}
				}
			}
#ifdef DEBUG
			auto userIp = getUseragentIp();
			if ((strncmp(userIp.data(), "127.", 4) == 0 || userIp == "::1") && _config->_data.getBool("admin")) {
				_config->_accessRole = storage::AccessRoleId::Admin;
			}
#endif
		}
	}
	return _config->_accessRole;
}

void Request::setAccessRole(storage::AccessRoleId role) const {
	_config->_accessRole = role;
	if (auto t = storage::Transaction::acquireIfExists(pool())) {
		t.setRole(role);
	}
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

apr::string Request::getFullHostname(int port) const {
	if (port == -1) {
		port = getParsedURI().port();
	}

	bool isSecure = isSecureConnection();
	apr::ostringstream ret;
	ret << (isSecure?"https":"http") << "://" << getHostname();
	if (port && ((isSecure && port != 443) || (!isSecure && port != 80))) {
		ret << ':' << port;
	}

	return ret.str();
}

bool Request::checkCacheHeaders(Time t, const StringView &etag) {
	return output::checkCacheHeaders(*this, t, etag);
}

bool Request::checkCacheHeaders(Time t, uint32_t idHash) {
	return output::checkCacheHeaders(*this, t, idHash);
}

void Request::runTemplate(String && path, const Function<void(tpl::Exec &, Request &)> &cb) {
	auto cache = server().getTemplateCache();
	cache->runTemplate(path, *this, cb);
}

int Request::runPug(const StringView & path, const Function<bool(pug::Context &, const pug::Template &)> &cb) {
	auto cache = server().getPugCache();
	if (cache->runTemplate(path, [&] (pug::Context &ctx, const pug::Template &tpl) -> bool {
		initScriptContext(ctx);

		if (cb(ctx, tpl)) {
			auto h = getResponseHeaders();
			auto lm = h.at("Last-Modified");
			auto etag = h.at("ETag");
			h.emplace("Content-Type", "text/html; charset=UTF-8");
			if (lm.empty() && etag.empty()) {
				h.emplace("Cache-Control", "no-cache, no-store, must-revalidate");
				h.emplace("Pragma", "no-cache");
				h.emplace("Expires", Time::seconds(0).toHttp());
			}
			return true;
		}
		return false;
	}, *this)) {
		return DONE;
	}
	return HTTP_INTERNAL_SERVER_ERROR;
}

void Request::setInputFilter(InputFilter *f) {
	_config->_filter = f;
}
InputFilter *Request::getInputFilter() const {
	return _config->_filter;
}

NS_SA_END
