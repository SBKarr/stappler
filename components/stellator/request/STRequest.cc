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

#include "Define.h"
#include "Request.h"

#include "SPFilesystem.h"
#include "SPugCache.h"
#include "SPugContext.h"
#include "SPugVariable.h"
#include "STRoot.h"
#include "STPqHandle.h"

#include "STSession.h"

namespace stellator {

struct Request::Config : public AllocPool {
	Config(mem::pool_t *p, Server::Config *s) : pool(p), server(s), _begin(mem::Time::now()) {
		/*if (r->args) {
			if (*r->args == '(') {
				_data = data::read(String::make_weak(r->args));
			} else {
				_data = Url::parseDataArgs(String::make_weak(r->args), 1_KiB);
			}
		}

		_path = Url::parsePath(apr::string::make_weak(r->uri));*/
		registerCleanupDestructor(this, p);
	}

	~Config() {
		if (_session) {
			delete _session;
		}
	}

	const mem::Vector<mem::String> &getPath() {
		return _path;
	}

	const mem::Value &getData() {
		return _data;
	}

	/*db::pq::Handle *acquireDatabase(request_rec *r) {
		if (!_database) {
			auto handle = Root::getInstance()->dbdRequestAcquire(r);
			if (handle) {
				_database = new db::pq::Handle(r->pool, db::pq::Driver::open(), db::pq::Driver::Handle(handle));
			}
		}
		return _database;
	}*/

	void finalize() { }

	mem::pool_t *pool = nullptr;
	Server::Config *server = nullptr;
	mem::Time _begin;

	mem::Vector<mem::String> _path;
	mem::Value _data;

	mem::Vector<mem::Value> _debug;
	mem::Vector<mem::Value> _errors;
	db::InputConfig _config;

	db::pq::Handle *_database = nullptr;
	RequestHandler *_handler = nullptr;
	Session *_session = nullptr;
	db::User *_user = nullptr;
	InputFilter *_filter = nullptr;

	bool _hookErrors = true;

	mem::Map<mem::String, CookieStorage> _cookies;
	int64_t _altUserid = 0;
	db::AccessRoleId _accessRole = db::AccessRoleId::Nobody;
};

Request::Request() : _buffer(nullptr), _config(nullptr) { }
Request::Request(Config *c) : _buffer(c), _config(c) {
	this->init(&_buffer);
}
Request & Request::operator =(Config *c) {
	_buffer = Buffer(c);
	_config = c;
	this->init(&_buffer);
	return *this;
}

Request::Request(Request &&other) : _buffer(other._config), _config(other._config) {
	this->init(&_buffer);
}
Request & Request::operator =(Request &&other) {
	_buffer = Buffer(other._config);
	_config = other._config;
	this->init(&_buffer);
	return *this;
}

Request::Request(const Request &other) :_buffer(other._config), _config(other._config) {
	this->init(&_buffer);
}
Request & Request::operator =(const Request &other) {
	_buffer = Buffer(other._config);
	_config = other._config;
	this->init(&_buffer);
	return *this;
}

Request::Buffer::Buffer(Config *c) : _request(c) { }
Request::Buffer::Buffer(Buffer&&other) : _request(other._request) { }
Request::Buffer& Request::Buffer::operator=(Buffer&&other) { _request = other._request; return *this; }

Request::Buffer::Buffer(const Buffer&other) : _request(other._request) { }
Request::Buffer& Request::Buffer::operator=(const Buffer&other) { _request = other._request; return *this; }

Request::Buffer::int_type Request::Buffer::overflow(int_type c) {
	//ap_rputc(c, _request);
	return c;
}

Request::Buffer::pos_type Request::Buffer::seekoff(off_type off, ios_base::seekdir way, ios_base::openmode) {
	return 0;
	//return _request->bytes_sent;
}
Request::Buffer::pos_type Request::Buffer::seekpos(pos_type pos, ios_base::openmode mode) {
	return 0;
	//return _request->bytes_sent;
}

int Request::Buffer::sync() {
	//ap_rflush(_request);
	return 0;
}

Request::Buffer::streamsize Request::Buffer::xsputn(const char_type* s, streamsize n) {
	return 0; // ap_rwrite((const void *)s, n, _request);
}

mem::StringView Request::getRequestLine() const {
	return mem::StringView(); // apr::string::make_weak(_request->the_request, _request->pool);
}
bool Request::isSimpleRequest() const {
	return false; //_request->assbackwards;
}
bool Request::isHeaderRequest() const {
	return false; // _request->header_only;
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

void Request::writeData(const mem::Value &data, bool allowJsonP) {
	//output::writeData(*this, data, allowJsonP);
}

void Request::clearFilters() {
	/*ap_filter_t *f;
	for (f = _request->input_filters; f != NULL; f = f->next) {
		if ((f->frec != NULL) && (f->frec->name != NULL) && !strcasecmp(f->frec->name, "http_in")) {
			ap_remove_input_filter(f);
			break;
		}
	}*/
}

const mem::String Request::getProtocol() const {
	return mem::String(); // mem::String::make_weak(_request->protocol, _request->pool);
}
const mem::String Request::getHostname() const {
	return mem::String(); // mem::String::make_weak(_request->hostname, _request->pool);
}

mem::Time Request::getRequestTime() const {
	return _config->_begin;
}

const mem::String Request::getStatusLine() const {
	return mem::String(); // apr::string::make_weak(_request->status_line, _request->pool);
}
int Request::getStatus() const {
	return 0; //_request->status;
}

Request::Method Request::getMethod() const {
	return Request::Method::Get; //Method(_request->method_number);
}

off_t Request::getContentLength() const {
	return 0; // _request->clength;
}

mem::table Request::getRequestHeaders() const {
	return mem::table::wrap(nullptr); // apr::table::wrap(_request->headers_in);
}
mem::table Request::getResponseHeaders() const {
	return mem::table::wrap(nullptr); // apr::table::wrap(_request->headers_out);
}
mem::table Request::getErrorHeaders() const {
	return mem::table::wrap(nullptr); // apr::table::wrap(_request->err_headers_out);
}

mem::StringView Request::getDocumentRoot() const {
	return mem::StringView(); // apr::string::make_weak(ap_context_document_root(_request), _request->pool);
}
mem::StringView Request::getContentType() const {
	return mem::StringView(); // apr::string::make_weak(_request->content_type, _request->pool);
}
mem::StringView Request::getContentEncoding() const {
	return mem::StringView(); // apr::string::make_weak(_request->content_encoding, _request->pool);
}

mem::StringView Request::getUnparsedUri() const {
	return mem::StringView(); // apr::string::make_weak(_request->unparsed_uri, _request->pool);
}
mem::StringView Request::getUri() const {
	return mem::StringView(); // apr::string::make_weak(_request->uri, _request->pool);
}
mem::StringView Request::getFilename() const {
	return mem::StringView(); // apr::string::make_weak(_request->filename, _request->pool);
}
mem::StringView Request::getPathInfo() const {
	return mem::StringView(); // apr::string::make_weak(_request->path_info, _request->pool);
}
mem::StringView Request::getQueryArgs() const {
	return mem::StringView(); // apr::string::make_weak(_request->args, _request->pool);
}

bool Request::isEosSent() const {
	return false; // _request->eos_sent;
}

bool Request::isSecureConnection() const {
	return false; // Root::getInstance()->isSecureConnection(_request->connection);
}

mem::StringView Request::getUseragentIp() const {
	return mem::StringView(); // apr::string::make_weak(_request->useragent_ip);
}

/* request params setters */
void Request::setDocumentRoot(mem::String &&str) {
	//ap_set_document_root(_request, str.extract());
}
void Request::setContentType(mem::String &&str) {
	//ap_set_content_type(_request, str.extract());
}
void Request::setContentEncoding(mem::String &&str) {
	//_request->content_encoding = str.extract();
}

void Request::setCookie(const mem::StringView &name, const mem::String &value, mem::TimeInterval maxAge, CookieFlags flags) {
	_config->_cookies.emplace(name.str<mem::Interface>(), CookieStorage{value, flags, maxAge});
}

void Request::removeCookie(const mem::StringView &name, CookieFlags flags) {
	_config->_cookies.emplace(name.str<mem::Interface>(), CookieStorage{mem::String(), flags, mem::TimeInterval::seconds(0)});
}

const mem::Map<mem::String, Request::CookieStorage> Request::getResponseCookies() const {
	return _config->_cookies;
}

mem::StringView Request::getCookie(const mem::StringView &name, bool removeFromHeadersTable) const {
	/*const char *val = nullptr;
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
	}*/
	return mem::StringView();
}

Session *Request::authorizeUser(db::User *user, mem::TimeInterval maxAge) {
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

void Request::setUser(db::User *u) {
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

db::User *Request::getUser() {
	if (!_config->_user) {
		if (auto s = getSession()) {
			_config->_user = s->getUser();
		}
	}
	return _config->_user;
}

db::User *Request::getAuthorizedUser() const {
	return _config->_user;
}

int64_t Request::getUserId() const {
	if (_config->_user) {
		return _config->_user->getObjectId();
	} else {
		return _config->_altUserid;
	}
}

void Request::setFilename(mem::String && str) {
	/*_request->filename = str.extract();
	_request->canonical_filename = _request->filename;
	apr_stat(&_request->finfo, _request->filename, APR_FINFO_NORM, pool());*/
}

void Request::setStatus(int status, mem::String && str) {
	/*_request->status = status;
	if (!str.empty()) {
		_request->status_line = str.extract();
	} else {
		_request->status_line = ap_get_status_line(status);
	}*/
}

db::InputConfig & Request::getInputConfig() {
	return _config->_config;
}
const db::InputConfig & Request::getInputConfig() const {
	return _config->_config;
}

void Request::setRequiredData(db::InputConfig::Require r) {
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

void Request::storeObject(void *ptr, const mem::StringView &key, mem::Function<void()> &&cb) const {
	mem::pool::store(_config->pool, ptr, key, std::move(cb));
}

const mem::Vector<mem::String> & Request::getParsedQueryPath() const {
	return _config->getPath();
}
const mem::Value & Request::getParsedQueryArgs() const {
	return _config->getData();
}

Server Request::server() const {
	return Server(_config->server);
}

mem::pool_t *Request::pool() const {
	return _config->pool;
}

db::Adapter Request::storage() const {
	return db::Adapter(nullptr); // _config->acquireDatabase(_request);
}

const mem::Vector<mem::Value> & Request::getDebugMessages() const {
	return _config->_debug;
}

const mem::Vector<mem::Value> & Request::getErrorMessages() const {
	return _config->_errors;
}

void Request::addErrorMessage(mem::Value &&val) {
	if (_config) {
		_config->_errors.emplace_back(std::move(val));
	}
}

void Request::addDebugMessage(mem::Value &&val) {
	if (_config) {
		_config->_debug.emplace_back(std::move(val));
	}
}

static int sa_request_custom_cleanup(void *ptr) {
	if (ptr) {
		auto ref = (mem::Function<void()> *)ptr;
		mem::pool::push(ref->get_allocator());
		(*ref)();
		mem::pool::pop();
	}
	return 0;
}

void Request::addCleanup(const mem::Function<void()> &cb) {
	auto ref = new (pool()) mem::Function<void()>(cb);
	mem::pool::cleanup_register(pool(), ref, &sa_request_custom_cleanup);
}

bool Request::isAdministrative() {
	return getAccessRole() == db::AccessRoleId::Admin;
}

db::AccessRoleId Request::getAccessRole() const {
	if (_config->_accessRole == db::AccessRoleId::Nobody) {
		if (_config->_user) {
			if (_config->_user->isAdmin()) {
				_config->_accessRole = db::AccessRoleId::Admin;
			} else {
				_config->_accessRole = db::AccessRoleId::Authorized;
			}
		} else {
			Session s(*this, true);
			if (s.isValid()) {
				auto u = s.getUser();
				if (u) {
					if (u->isAdmin()) {
						_config->_accessRole = db::AccessRoleId::Admin;
					} else {
						_config->_accessRole = db::AccessRoleId::Authorized;
					}
				}
			}
#ifdef DEBUG
			auto userIp = getUseragentIp();
			if ((strncmp(userIp.data(), "127.", 4) == 0 || userIp == "::1") && _config->_data.getBool("admin")) {
				_config->_accessRole = db::AccessRoleId::Admin;
			}
#endif
		}
	}
	return _config->_accessRole;
}

void Request::setAccessRole(db::AccessRoleId role) const {
	_config->_accessRole = role;
	if (auto t = db::Transaction::acquireIfExists(pool())) {
		t.setRole(role);
	}
}

int Request::redirectTo(mem::String && location) {
	getResponseHeaders().emplace("Location", std::move(location));
	return HTTP_SEE_OTHER;
}

int Request::sendFile(mem::String && file, size_t cacheTime) {
	setFilename(stappler::filesystem::writablePath(file));
	if (cacheTime == 0) {
		getResponseHeaders().emplace("Cache-Control", "no-cache, must-revalidate");
	} else if (cacheTime < SIZE_MAX) {
		getResponseHeaders().emplace("Cache-Control", mem::toString("max-age=", cacheTime, ", must-revalidate", cacheTime));
	}
	return OK;
}

int Request::sendFile(mem::String && file, mem::String && contentType, size_t cacheTime) {
	if (!contentType.empty()) {
		setContentType(std::move(contentType));
	}
	return sendFile(std::move(file), cacheTime);
}

mem::String Request::getFullHostname(int port) const {
	if (port == -1) {
		//port = getParsedURI().port();
	}

	bool isSecure = isSecureConnection();
	stappler::memory::ostringstream ret;
	ret << (isSecure?"https":"http") << "://" << getHostname();
	if (port && ((isSecure && port != 443) || (!isSecure && port != 80))) {
		ret << ':' << port;
	}

	return ret.str();
}

bool Request::checkCacheHeaders(mem::Time t, const mem::StringView &etag) {
	return false; //output::checkCacheHeaders(*this, t, etag);
}

bool Request::checkCacheHeaders(mem::Time t, uint32_t idHash) {
	return false; // output::checkCacheHeaders(*this, t, idHash);
}

int Request::runPug(const mem::StringView & path, const mem::Function<bool(pug::Context &, const pug::Template &)> &cb) {
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
				h.emplace("Expires", mem::Time::seconds(0).toHttp());
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

Request::Config *Request::getConfig() const {
	return _config;
}

void Request::initScriptContext(pug::Context &ctx) {
	pug::VarClass serenityClass;
	serenityClass.staticFunctions.emplace("prettify", [] (pug::VarStorage &, pug::Var *var, size_t argc) -> pug::Var {
		if (var && argc == 1) {
			return pug::Var(mem::Value(mem::toString(var->readValue(), true)));
		}
		return pug::Var();
	});
	serenityClass.staticFunctions.emplace("timeToHttp", [] (pug::VarStorage &, pug::Var *var, size_t argc) -> pug::Var {
		if (var && argc == 1 && var->readValue().isInteger()) {
			return pug::Var(mem::Value(mem::Time::microseconds(var->readValue().asInteger()).toHttp()));
		}
		return pug::Var();
	});
	serenityClass.staticFunctions.emplace("uuidToString", [] (pug::VarStorage &, pug::Var *var, size_t argc) -> pug::Var {
		if (var && argc == 1 && var->readValue().isBytes()) {
			return pug::Var(mem::Value(mem::uuid(var->readValue().getBytes()).str()));
		}
		return pug::Var();
	});
	ctx.set("serenity", std::move(serenityClass));
	ctx.set("window", mem::Value{
		std::pair("location", mem::Value({
			std::pair("href", mem::Value(mem::toString(getFullHostname(), getUnparsedUri()))),
			std::pair("hostname", mem::Value(getHostname())),
			std::pair("pathname", mem::Value(getUri())),
			std::pair("protocol", mem::Value(isSecureConnection() ? "https:" : "http:")),
		}))
	});
}

}
