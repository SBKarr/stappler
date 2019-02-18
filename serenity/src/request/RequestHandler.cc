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
#include "RequestHandler.h"

#include "SPFilesystem.h"
#include "InputFilter.h"
#include "User.h"

NS_SA_BEGIN

int RequestHandler::onRequestRecieved(Request & rctx, String &&originPath, String &&path, const data::Value &data) {
	_request = rctx;
	_originPath = std::move(originPath);
	_subPath = std::move(path);
	_options = data;
	_subPathVec = Url::parsePath(_subPath);

	auto auth = rctx.getRequestHeaders().at("Authorization");
	if (!auth.empty()) {
		StringView r(auth);
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		auto method = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>().str();
		string::tolower(method);
		auto userIp = rctx.getUseragentIp();
		if (method == "basic" && (rctx.isSecureConnection() || strncmp(userIp.c_str(), "127.", 4) == 0 || userIp == "::1")) {
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			auto str = base64::decode(r);
			StringView source((const char *)str.data(), str.size());
			StringView user = source.readUntil<StringView::Chars<':'>>();
			++ source;

			if (!user.empty() && !source.empty()) {
				auto storage = rctx.storage();
				auto u = User::get(storage, String::make_weak(user.data(), user.size()),
						String::make_weak(source.data(), source.size()));
				if (u) {
					rctx.setUser(u);
				}
			}
		}
	}

	auto &origin = rctx.getRequestHeaders().at("Origin");
	if (origin.empty()) {
		return OK;
	}

	if (rctx.getMethod() != Request::Options) {
		// non-preflightted request
		if (isCorsPermitted(rctx, origin)) {
			rctx.getResponseHeaders().emplace("Access-Control-Allow-Origin", origin);
			rctx.getResponseHeaders().emplace("Access-Control-Allow-Credentials", "true");

			rctx.getErrorHeaders().emplace("Access-Control-Allow-Origin", origin);
			rctx.getErrorHeaders().emplace("Access-Control-Allow-Credentials", "true");
			return OK;
		} else {
			return HTTP_METHOD_NOT_ALLOWED;
		}
	} else {
		auto &method = rctx.getRequestHeaders().at("Access-Control-Request-Method");
		auto &headers = rctx.getRequestHeaders().at("Access-Control-Request-Headers");

		if (isCorsPermitted(rctx, origin, true, method, headers)) {
			rctx.getResponseHeaders().emplace("Access-Control-Allow-Origin", origin);
			rctx.getResponseHeaders().emplace("Access-Control-Allow-Credentials", "true");

			auto c_methods = getCorsAllowMethods(rctx);
			if (!c_methods.empty()) {
				rctx.getResponseHeaders().emplace("Access-Control-Allow-Methods", c_methods);
			} else if (!method.empty()) {
				rctx.getResponseHeaders().emplace("Access-Control-Allow-Methods", method);
			}

			auto c_headers = getCorsAllowHeaders(rctx);
			if (!c_headers.empty()) {
				rctx.getResponseHeaders().emplace("Access-Control-Allow-Headers", c_headers);
			} else if (!headers.empty()) {
				rctx.getResponseHeaders().emplace("Access-Control-Allow-Headers", headers);
			}

			auto c_maxAge = getCorsMaxAge(rctx);
			if (!c_maxAge.empty()) {
				rctx.getResponseHeaders().emplace("Access-Control-Max-Age", c_maxAge);
			}

			return DONE;
		} else {
			return HTTP_METHOD_NOT_ALLOWED;
		}
	}
}

int DataHandler::writeResult(data::Value &data) {
	auto status = _request.getStatus();
	if (status >= 400) {
		return status;
	}

	data.setInteger(apr_time_now(), "date");
#if DEBUG
	auto &debug = _request.getDebugMessages();
	if (!debug.empty()) {
		data.setArray(debug, "debug");
	}
#endif
	auto &error = _request.getErrorMessages();
	if (!error.empty()) {
		data.setArray(error, "errors");
	}

	_request.writeData(data, allowJsonP());
	return DONE;
}

static bool isMethodAllowed(Request::Method r, DataHandler::AllowMethod a) {
	if ((r == Request::Get && (a & DataHandler::AllowMethod::Get) != 0)
			|| (r == Request::Delete && (a & DataHandler::AllowMethod::Delete) != 0)
			|| (r == Request::Put && (a & DataHandler::AllowMethod::Put) != 0)
			|| (r == Request::Post && (a & DataHandler::AllowMethod::Post) != 0)) {
		return true;
	}

	return false;
}

int DataHandler::onTranslateName(Request &rctx) {
	if (!isMethodAllowed(rctx.getMethod(), _allow)) {
		return HTTP_METHOD_NOT_ALLOWED;
	}

	if ((rctx.getMethod() == Request::Get && (_allow & AllowMethod::Get) != AllowMethod::None)
			|| (rctx.getMethod() == Request::Delete && (_allow & AllowMethod::Delete) != AllowMethod::None)) {
		bool result = false;
		data::Value data;

		data::Value input;
		result = processDataHandler(rctx, data, input);
		data.setBool(result, "OK");
		return writeResult(data);
	}

	return DECLINED;
}

void DataHandler::onInsertFilter(Request &rctx) {
	if ((rctx.getMethod() == Request::Method::Post && (_allow & AllowMethod::Post) != AllowMethod::None)
			|| (rctx.getMethod() == Request::Method::Put && (_allow & AllowMethod::Put) != AllowMethod::None)) {
		rctx.setRequiredData(_required);
		rctx.setMaxRequestSize(_maxRequestSize);
		rctx.setMaxVarSize(_maxVarSize);
		rctx.setMaxFileSize(_maxFileSize);
	}

	if (rctx.getMethod() == Request::Put || rctx.getMethod() == Request::Post) {
		auto ex = InputFilter::insert(rctx);
		if (ex != InputFilter::Exception::None) {
			if (ex == InputFilter::Exception::TooLarge) {
				rctx.setStatus(HTTP_REQUEST_ENTITY_TOO_LARGE);
			} else if (ex == InputFilter::Exception::Unrecognized) {
				rctx.setStatus(HTTP_UNSUPPORTED_MEDIA_TYPE);
			}
		}
	}
}

int DataHandler::onHandler(Request &) {
	return OK;
}

void DataHandler::onFilterComplete(InputFilter *filter) {
	bool result = false;
	data::Value data;
	Request rctx(filter->getRequest());
	_filter = filter;

	data::Value input(filter->getData());
	for (auto &it : filter->getFiles()) {
		input.setInteger(it.negativeId(), it.name);
	}

	result = processDataHandler(rctx, data, input);

	data.setBool(result, "OK");
	writeResult(data);
}

FilesystemHandler::FilesystemHandler(const String &path, size_t cacheTime) : _path(path), _cacheTime(cacheTime) { }
FilesystemHandler::FilesystemHandler(const String &path, const String &ct, size_t cacheTime)
: _path(path), _contentType(ct), _cacheTime(cacheTime) { }

bool FilesystemHandler::isRequestPermitted(Request &) {
	return true;
}
int FilesystemHandler::onTranslateName(Request &rctx) {
	if (rctx.getUri() == "/") {
		return rctx.sendFile(filesystem::writablePath(_path), std::move(_contentType), _cacheTime);
	} else {
		auto npath = filesystem::writablePath(rctx.getUri(), true);
		if (filesystem::exists(npath) && _subPath != "/") {
			return DECLINED;
		}
		return rctx.sendFile(filesystem::writablePath(_path), std::move(_contentType), _cacheTime);
	}
}


DataMapHandler::MapResult::MapResult(int s) : status(s) { }
DataMapHandler::MapResult::MapResult(ProcessFunction &&fn) : function(move(fn)) { }

bool DataMapHandler::isRequestPermitted(Request &rctx) {
	if (_mapFunction) {
		auto res = _mapFunction(rctx, rctx.getMethod(), _subPath);
		if (res.function) {
			_selectedProcessFunction = move(res.function);
			return true;
		} else {
			if (res.status) {
				rctx.setStatus(res.status);
			} else {
				rctx.setStatus(HTTP_NOT_FOUND);
			}
			return false;
		}
	}
	rctx.setStatus(HTTP_NOT_FOUND);
	return false;
}

bool DataMapHandler::processDataHandler(Request &req, data::Value &result, data::Value &input) {
	if (!_selectedProcessFunction) {
		return false;
	} else {
		return _selectedProcessFunction(req, result, input);
	}
}

NS_SA_END
