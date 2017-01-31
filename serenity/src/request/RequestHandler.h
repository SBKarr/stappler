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

#ifndef SERENITY_SRC_REQUEST_REQUESTHANDLER_H_
#define SERENITY_SRC_REQUEST_REQUESTHANDLER_H_

#include "Request.h"

NS_SA_BEGIN

#define SA_HANDLER(Type) ([] () -> Type * { return new Type(); })
#define SA_DROP_HANDLER ([] () -> RequestHandler * { return new DefaultHandler(); })

class RequestHandler : public AllocPool {
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
	virtual bool isCorsPermitted(Request &, const String &origin, bool isPreflight = false,
		const String &method = "", const String &headers = "") { return true; }

	/**
	 * Available method for CORS preflighted requests
     */
	virtual apr::weak_string getCorsAllowMethods(Request &) {
		return "GET, HEAD, POST, PUT, DELETE, OPTIONS"_weak;
	}

	/**
	 * Available extra headers for CORS preflighted requests
     */
	virtual apr::weak_string getCorsAllowHeaders(Request &) {
		return ""_weak;
	}

	/**
	 * Caching time for preflight response
     */
	virtual apr::weak_string getCorsMaxAge(Request &) {
		return "1728000"_weak; // 20 days
	}

	/** Be sure to call supermethod when overload this method! */
	virtual int onRequestRecieved(Request &, String && origin, String && path, const data::Value &);

public:
	virtual int onPostReadRequest(Request &) { return DECLINED; }
	virtual int onTranslateName(Request &) { return DECLINED; }
	virtual int onQuickHandler(Request &, int v) { return DECLINED; }
	virtual void onInsertFilter(Request &) { }
	virtual int onHandler(Request &) { return DECLINED; }

	virtual void onFilterInit(InputFilter *f) { }
	virtual void onFilterUpdate(InputFilter *f) { }
	virtual void onFilterComplete(InputFilter *f) { }

	virtual const data::Value &getOptions() const { return _options; }

protected:
	Request _request;
	String _originPath;
	String _subPath;
	Vector<String> _subPathVec;
	data::Value _options;
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
	virtual bool processDataHandler(Request &req, data::Value &result, data::Value &input) { return false; }

	virtual int onTranslateName(Request &) override;
	virtual void onInsertFilter(Request &) override;
	virtual int onHandler(Request &) override;

	virtual void onFilterComplete(InputFilter *f) override;

protected:
	virtual bool allowJsonP() { return true; }

	int writeResult(data::Value &);

	AllowMethod _allow = AllowMethod::All;
	InputConfig::Require _required = InputConfig::Require::Data | InputConfig::Require::FilesAsData;
	size_t _maxRequestSize = 0;
	size_t _maxVarSize = 256;
	size_t _maxFileSize = 0;
	InputFilter *_filter;
};

class FilesystemHandler : public RequestHandler {
public:
	FilesystemHandler(const String &path, size_t cacheTimeInSeconds = maxOf<size_t>());
	FilesystemHandler(const String &path, const String &ct, size_t cacheTimeInSeconds = maxOf<size_t>());

	virtual bool isRequestPermitted(Request &) override;
	virtual int onTranslateName(Request &) override;

protected:
	String _path;
	String _contentType;
	size_t _cacheTime;
};

SP_DEFINE_ENUM_AS_MASK(DataHandler::AllowMethod)

NS_SA_END

#endif /* SERENITY_SRC_REQUEST_REQUESTHANDLER_H_ */
