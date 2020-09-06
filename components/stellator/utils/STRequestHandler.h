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

#ifndef STELLATOR_UTILS_STREQUESTHANDLER_H_
#define STELLATOR_UTILS_STREQUESTHANDLER_H_

#include "Request.h"

NS_SA_ST_BEGIN

#define SA_HANDLER(Type) ([] () -> Type * { return new Type(); })
#define SA_DROP_HANDLER ([] () -> RequestHandler * { return new DefaultHandler(); })

class RequestHandler : public mem::AllocBase {
public:
	virtual ~RequestHandler() { }

	virtual bool isRequestPermitted(Request &) { return false; }

	/**
	 * https://developer.mozilla.org/en-US/docs/HTTP/Access_control_CORS
	 *
     * @param rctx - new request, avoid to modify it
     * @param origin - actual value of Origin header, it's sender's domain name
     * @param isPreflight - we recieve preflight request (so, method and headers should be filled)
     * @param method - method for preflighted request
     * @param headers - extra (X-*) headers for preflighted request
     * @return for forbidden CORS requests server will return "405 Method Not Allowed"
     */
	virtual bool isCorsPermitted(Request &, const mem::StringView &origin, bool isPreflight = false,
		const mem::StringView &method = "", const mem::StringView &headers = "") { return true; }

	/**
	 * Available method for CORS preflighted requests
     */
	virtual mem::StringView getCorsAllowMethods(Request &) {
		return "GET, HEAD, POST, PUT, DELETE, OPTIONS";
	}

	/**
	 * Available extra headers for CORS preflighted requests
     */
	virtual mem::StringView getCorsAllowHeaders(Request &) {
		return mem::StringView();
	}

	/**
	 * Caching time for preflight response
     */
	virtual mem::StringView getCorsMaxAge(Request &) {
		return "1728000"; // 20 days
	}

	/** Be sure to call supermethod when overload this method! */
	virtual int onRequestRecieved(Request &, mem::String && origin, mem::String && path, const mem::Value &);

public:
	virtual int onPostReadRequest(Request &) { return DECLINED; }
	virtual int onTranslateName(Request &) { return DECLINED; }
	virtual int onQuickHandler(Request &, int v) { return DECLINED; }
	virtual void onInsertFilter(Request &) { }
	virtual int onHandler(Request &) { return DECLINED; }

	virtual void onFilterInit(InputFilter *f) { }
	virtual void onFilterUpdate(InputFilter *f) { }
	virtual void onFilterComplete(InputFilter *f) { }

	virtual const mem::Value &getOptions() const { return _options; }

	void setAccessRole(db::AccessRoleId role) { _accessRole = role; }
	db::AccessRoleId getAccessRole() const { return _accessRole; }

protected:
	Request _request;
	mem::String _originPath;
	mem::String _subPath;
	mem::Vector<mem::String> _subPathVec;
	mem::Value _options;
	db::AccessRoleId _accessRole = db::AccessRoleId::Nobody;
};

class DefaultHandler : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request &) override { return true; }
};

class DataHandler : public RequestHandler {
public:
	enum class AllowMethod : uint8_t {
		None = 0,
		Get = 1 << 0,
		Post = 1 << 1,
		Put = 1 << 2,
		Delete = 1 << 3,
		All = 0xFF,
	};

	virtual ~DataHandler() { }

	// overload point
	virtual bool processDataHandler(Request &req, mem::Value &result, mem::Value &input) { return false; }

	virtual int onTranslateName(Request &) override;
	virtual void onInsertFilter(Request &) override;
	virtual int onHandler(Request &) override;

	virtual void onFilterComplete(InputFilter *f) override;

protected:
	virtual bool allowJsonP() { return true; }

	int writeResult(mem::Value &);

	AllowMethod _allow = AllowMethod::All;
	db::InputConfig::Require _required = db::InputConfig::Require::Data | db::InputConfig::Require::FilesAsData;
	size_t _maxRequestSize = 0;
	size_t _maxVarSize = 256;
	size_t _maxFileSize = 0;
	InputFilter *_filter;
};

class FilesystemHandler : public RequestHandler {
public:
	FilesystemHandler(const mem::String &path, size_t cacheTimeInSeconds = stappler::maxOf<size_t>());
	FilesystemHandler(const mem::String &path, const mem::String &ct, size_t cacheTimeInSeconds = stappler::maxOf<size_t>());

	virtual bool isRequestPermitted(Request &) override;
	virtual int onTranslateName(Request &) override;

protected:
	mem::String _path;
	mem::String _contentType;
	size_t _cacheTime;
};

class DataMapHandler : public DataHandler {
public:
	using ProcessFunction = mem::Function<bool(Request &req, mem::Value &result, const mem::Value &input)>;

	struct MapResult {
		MapResult(int);
		MapResult(ProcessFunction &&);

		int status = 0;
		ProcessFunction function = nullptr;
	};

	using MapFunction = mem::Function<MapResult(Request &req, Request::Method, const mem::StringView &subpath)>;

	virtual bool isRequestPermitted(Request &) override;
	virtual bool processDataHandler(Request &req, mem::Value &result, mem::Value &input) override;

protected:
	MapFunction _mapFunction = nullptr;
	ProcessFunction _selectedProcessFunction = nullptr;
};

SP_DEFINE_ENUM_AS_MASK(DataHandler::AllowMethod)


class HandlerMap : public mem::AllocBase {
public:
	struct HandlerInfo;

	class Handler : public RequestHandler {
	public: // simplified interface
		virtual bool isPermitted() { return false; }
		virtual int onRequest() { return DECLINED; }
		virtual mem::Value onData() { return mem::Value(); }

	public:
		Handler() { }
		virtual ~Handler() { }

		virtual void onParams(const HandlerInfo *, mem::Value &&);
		virtual bool isRequestPermitted(Request &) override { return isPermitted(); }
		virtual int onTranslateName(Request &) override;
		virtual void onInsertFilter(Request &) override;
		virtual int onHandler(Request &) override;

		virtual void onFilterComplete(InputFilter *f);

		const Request &getRequest() const { return _request; }
		const mem::Value &getParams() const { return _params; }
		const mem::Value &getQueryFields() const { return _queryFields; }
		const mem::Value &getInputFields() const { return _inputFields; }

	protected:
		virtual bool allowJsonP() { return true; }

		bool processQueryFields(mem::Value &&);
		bool processInputFields(InputFilter *);

		int writeResult(mem::Value &);

		db::InputFile *getInputFile(const mem::StringView &);

		const HandlerInfo *_info = nullptr;
		InputFilter *_filter = nullptr;

		mem::Value _params; // query path params
		mem::Value _queryFields;
		mem::Value _inputFields;
	};

	class HandlerInfo : public mem::AllocBase {
	public:
		HandlerInfo(const mem::StringView &name, Request::Method, const mem::StringView &pattern,
				mem::Function<Handler *()> &&, mem::Value && = mem::Value());

		HandlerInfo &addQueryFields(std::initializer_list<db::Field> il);
		HandlerInfo &addQueryFields(mem::Vector<db::Field> &&il);

		HandlerInfo &addInputFields(std::initializer_list<db::Field> il);
		HandlerInfo &addInputFields(mem::Vector<db::Field> &&il);
		HandlerInfo &setInputConfig(db::InputConfig);

		mem::Value match(const mem::StringView &path, size_t &score) const;

		Handler *onHandler(mem::Value &&) const;

		Request::Method getMethod() const;
		const db::InputConfig &getInputConfig() const;

		mem::StringView getName() const;
		mem::StringView getPattern() const;
		const data::Value &getOptions() const;

		const db::Scheme &getQueryScheme() const;
		const db::Scheme &getInputScheme() const;

	protected:
		struct Fragment {
			enum Type : uint16_t {
				Text,
				Pattern,
			};

			Fragment(Type t, mem::StringView s) : type(t), string(s.str<mem::Interface>()) { }

			Type type;
			mem::String string;
		};

		mem::String name;
		Request::Method method = Request::Method::Get;
		mem::String pattern;
		mem::Function<Handler *()> handler;
		mem::Value options;

		db::Scheme queryFields;
		db::Scheme inputFields;
		mem::Vector<Fragment> fragments;
	};

	HandlerMap();
	virtual ~HandlerMap();

	HandlerMap(HandlerMap &&) = default;
	HandlerMap &operator=(HandlerMap &&) = default;

	HandlerMap(const HandlerMap &) = delete;
	HandlerMap &operator=(const HandlerMap &) = delete;

	Handler *onRequest(Request &req, const mem::StringView &path) const;

	HandlerInfo &addHandler(const mem::StringView &name, Request::Method, const mem::StringView &pattern,
			mem::Function<Handler *()> &&, mem::Value && = mem::Value());

	HandlerInfo &addHandler(const mem::StringView &name, Request::Method, const mem::StringView &pattern,
			mem::Function<bool(Handler &)> &&, mem::Function<data::Value(Handler &)> &&, mem::Value && = mem::Value());

	const mem::Vector<HandlerInfo> &getHandlers() const;

protected:
	mem::Vector<HandlerInfo> _handlers;
};

NS_SA_ST_END

#endif /* STELLATOR_REQUEST_STREQUESTHANDLER_H_ */
