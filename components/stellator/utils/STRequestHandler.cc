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

#include "RequestHandler.h"
#include "InputFilter.h"

NS_SA_ST_BEGIN

int RequestHandler::onRequestRecieved(Request & rctx, mem::String &&originPath, mem::String &&path, const mem::Value &data) {
	_request = rctx;
	_originPath = std::move(originPath);
	_subPath = std::move(path);
	_options = data;
	_subPathVec = stappler::UrlView::parsePath(_subPath);
	return OK;
}

int DataHandler::writeResult(mem::Value &data) {
	auto status = _request.getStatus();
	if (status >= 400) {
		return status;
	}

	data.setInteger(mem::Time::now().toMicros(), "date");
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
		mem::Value data;

		mem::Value input;
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
	mem::Value data;
	Request rctx(filter->getRequest());
	_filter = filter;

	mem::Value input(filter->getData());
	for (auto &it : filter->getFiles()) {
		input.setInteger(it.negativeId(), it.name);
	}

	result = processDataHandler(rctx, data, input);

	data.setBool(result, "OK");
	writeResult(data);
}

FilesystemHandler::FilesystemHandler(const mem::String &path, size_t cacheTime) : _path(path), _cacheTime(cacheTime) { }
FilesystemHandler::FilesystemHandler(const mem::String &path, const mem::String &ct, size_t cacheTime)
: _path(path), _contentType(ct), _cacheTime(cacheTime) { }

bool FilesystemHandler::isRequestPermitted(Request &) {
	return true;
}
int FilesystemHandler::onTranslateName(Request &rctx) {
	if (rctx.getUri() == "/") {
		return rctx.sendFile(stappler::filesystem::writablePath(_path), std::move(_contentType), _cacheTime);
	} else {
		auto npath = stappler::filesystem::writablePath(rctx.getUri(), true);
		if (stappler::filesystem::exists(npath) && _subPath != "/") {
			return DECLINED;
		}
		return rctx.sendFile(stappler::filesystem::writablePath(_path), std::move(_contentType), _cacheTime);
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

bool DataMapHandler::processDataHandler(Request &req, mem::Value &result, mem::Value &input) {
	if (!_selectedProcessFunction) {
		return false;
	} else {
		return _selectedProcessFunction(req, result, input);
	}
}


void HandlerMap::Handler::onParams(const HandlerInfo *info, mem::Value &&val) {
	_info = info;
	_params = std::move(val);
}

int HandlerMap::Handler::onTranslateName(Request &rctx) {
	if (rctx.getMethod() != _info->getMethod()) {
		return HTTP_METHOD_NOT_ALLOWED;
	}

	switch (rctx.getMethod()) {
	case Request::Method::Post:
	case Request::Method::Put:
	case Request::Method::Patch:
		return OK;
		break;
	default: {
		if (!processQueryFields(mem::Value(rctx.getParsedQueryArgs()))) {
			return HTTP_BAD_REQUEST;
		}
		auto ret = onRequest();
		if (ret == DECLINED) {
			if (auto data = onData()) {
				data.setBool(true, "OK");
				return writeResult(data);
			} else {
				mem::Value ret({ stappler::pair("OK", mem::Value(false)) });
				return writeResult(ret);
			}
		}
		return ret;
		break;
	}
	}
	return HTTP_BAD_REQUEST;
}

void HandlerMap::Handler::onInsertFilter(Request &rctx) {
	switch (rctx.getMethod()) {
	case Request::Method::Post:
	case Request::Method::Put:
	case Request::Method::Patch: {
		rctx.setRequiredData(db::InputConfig::Require::Data | db::InputConfig::Require::Files);
		rctx.setMaxRequestSize(_info->getInputConfig().maxRequestSize);
		rctx.setMaxVarSize(_info->getInputConfig().maxVarSize);
		rctx.setMaxFileSize(_info->getInputConfig().maxFileSize);

		auto ex = InputFilter::insert(rctx);
		if (ex != InputFilter::Exception::None) {
			if (ex == InputFilter::Exception::TooLarge) {
				rctx.setStatus(HTTP_REQUEST_ENTITY_TOO_LARGE);
			} else if (ex == InputFilter::Exception::Unrecognized) {
				rctx.setStatus(HTTP_UNSUPPORTED_MEDIA_TYPE);
			}
		}
		break;
	}
	default:
		break;
	}
}

int HandlerMap::Handler::onHandler(Request &) {
	return OK;
}

void HandlerMap::Handler::onFilterComplete(InputFilter *filter) {
	_filter = filter;

	if (!processQueryFields(mem::Value(_request.getParsedQueryArgs()))) {
		_request.setStatus(HTTP_BAD_REQUEST);
		return;
	}
	if (!processInputFields(_filter)) {
		_request.setStatus(HTTP_BAD_REQUEST);
		return;
	}
	auto ret = onRequest();
	if (ret == DECLINED) {
		if (auto data = onData()) {
			data.setBool(true, "OK");
			writeResult(data);
		} else {
			mem::Value ret({ stappler::pair("OK", mem::Value(false)) });
			writeResult(ret);
		}
	}
	_request.setStatus(ret);
}

bool HandlerMap::Handler::processQueryFields(mem::Value &&args) {
	_queryFields = std::move(args);
	if (_info->getQueryScheme().getFields().empty()) {
		return true;
	}

	_info->getQueryScheme().transform(_queryFields, db::Scheme::TransformAction::ProtectedCreate);

	bool success = true;
	for (auto &it : _info->getQueryScheme().getFields()) {
		auto &val = _queryFields.getValue(it.first);
		if (val.isNull() && it.second.hasFlag(db::Flags::Required)) {
			messages::error("HandlerMap", "No value for required field",
					mem::Value({ std::make_pair("field", mem::Value(it.first)) }));
			success = false;
		}
	}
	return success;
}

bool HandlerMap::Handler::processInputFields(InputFilter *f) {
	_inputFields = std::move(f->getData());

	if (_info->getInputScheme().getFields().empty()) {
		return true;
	}

	_info->getInputScheme().transform(_inputFields, db::Scheme::TransformAction::ProtectedCreate);

	for (auto &it : f->getFiles()) {
		if (auto f = _info->getInputScheme().getField(it.name)) {
			if (db::File::validateFileField(*f, it)) {
				_inputFields.setInteger(it.negativeId(), it.name);
			}
		}
	}

	bool success = true;
	for (auto &it : _info->getQueryScheme().getFields()) {
		auto &val = _inputFields.getValue(it.first);
		if (val.isNull() && it.second.hasFlag(db::Flags::Required)) {
			messages::error("HandlerMap", "No value for required field",
					mem::Value({ std::make_pair("field", mem::Value(it.first)) }));
			success = false;
		}
	}
	return success;
}

int HandlerMap::Handler::writeResult(mem::Value &data) {
	auto status = _request.getStatus();
	if (status >= 400) {
		return status;
	}

	data.setInteger(mem::Time::now().toMicros(), "date");
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

db::InputFile *HandlerMap::Handler::getInputFile(const mem::StringView &name) {
	if (!_filter) {
		return nullptr;
	}

	for (auto &it : _filter->getFiles()) {
		if (it.name == name) {
			return &it;
		}
	}

	return nullptr;
}


HandlerMap::HandlerInfo::HandlerInfo(const mem::StringView &name, Request::Method m, const mem::StringView &pt,
		mem::Function<Handler *()> &&cb, mem::Value &&opts)
: name(name.str<mem::Interface>()), method(m), pattern(pt.str<mem::Interface>()), handler(std::move(cb)), options(std::move(opts))
, queryFields(name), inputFields(name) {
	mem::StringView p(pattern);
	while (!p.empty()) {
		auto tmp = p.readUntil<mem::StringView::Chars<':'>>();
		if (!tmp.empty()) {
			fragments.emplace_back(Fragment::Text, tmp);
		}
		if (p.is(':')) {
			auto ptrn = p.readUntil<mem::StringView::Chars<'/'>>();
			if (!ptrn.empty()) {
				fragments.emplace_back(Fragment::Pattern, ptrn);
			}
		}
	}
}

HandlerMap::HandlerInfo &HandlerMap::HandlerInfo::addQueryFields(std::initializer_list<db::Field> il) {
	queryFields.define(il);
	return *this;
}
HandlerMap::HandlerInfo &HandlerMap::HandlerInfo::addQueryFields(mem::Vector<db::Field> &&il) {
	queryFields.define(std::move(il));
	return *this;
}

HandlerMap::HandlerInfo &HandlerMap::HandlerInfo::addInputFields(std::initializer_list<db::Field> il) {
	inputFields.define(il);
	return *this;
}
HandlerMap::HandlerInfo &HandlerMap::HandlerInfo::addInputFields(mem::Vector<db::Field> &&il) {
	inputFields.define(std::move(il));
	return *this;
}

mem::Value HandlerMap::HandlerInfo::match(const mem::StringView &path, size_t &match) const {
	size_t nmatch = 0;
	mem::Value ret({ stappler::pair("path", mem::Value(path)) });
	auto it = fragments.begin();
	mem::StringView r(path);
	while (!r.empty() && it != fragments.end()) {
		switch (it->type) {
		case Fragment::Text:
			if (r.starts_with(mem::StringView(it->string))) {
				r += it->string.size();
				nmatch += it->string.size();
				++ it;
			} else {
				return mem::Value();
			}
			break;
		case Fragment::Pattern:
			if (mem::StringView(it->string).is(':')) {
				mem::StringView name(it->string.data() + 1, it->string.size() - 1);
				if (name.empty()) {
					return mem::Value();
				}

				++ it;
				if (it != fragments.end()) {
					auto tmp = r.readUntilString(it->string);
					if (tmp.empty()) {
						return mem::Value();
					}
					ret.setString(tmp, name.str<mem::Interface>());
				} else {
					ret.setString(r, name.str<mem::Interface>());
					r += r.size();
				}
			}
			break;
		}
	}

	if (!r.empty() || it != fragments.end()) {
		return mem::Value();
	}

	match = nmatch;
	return ret;
}

HandlerMap::Handler *HandlerMap::HandlerInfo::onHandler(mem::Value &&p) const {
	if (auto h = handler()) {
		h->onParams(this, std::move(p));
		return h;
	}
	return nullptr;
}

Request::Method HandlerMap::HandlerInfo::getMethod() const {
	return method;
}

const db::InputConfig &HandlerMap::HandlerInfo::getInputConfig() const {
	return inputFields.getConfig();
}

mem::StringView HandlerMap::HandlerInfo::getName() const {
	return name;
}
mem::StringView HandlerMap::HandlerInfo::getPattern() const {
	return pattern;
}

const db::Scheme &HandlerMap::HandlerInfo::getQueryScheme() const {
	return queryFields;
}
const db::Scheme &HandlerMap::HandlerInfo::getInputScheme() const {
	return inputFields;
}

HandlerMap::HandlerMap() { }

HandlerMap::~HandlerMap() { }

HandlerMap::Handler *HandlerMap::onRequest(Request &req, const mem::StringView &ipath) const {
	mem::StringView path(ipath.empty() ? mem::StringView("/") : ipath);
	const HandlerInfo *info = nullptr;
	mem::Value params;
	size_t score = 0;
	for (auto &it : _handlers) {
		size_t pscore = 0;
		if (auto val = it.match(path, pscore)) {
			if (pscore > score) {
				params = std::move(val);
				if (it.getMethod() == req.getMethod() || !info || info->getMethod() != it.getMethod()) {
					info = &it;
					score = pscore;
				}
			}
		}
	}

	if (info) {
		return info->onHandler(std::move(params));
	}

	return nullptr;
}

const mem::Vector<HandlerMap::HandlerInfo> &HandlerMap::getHandlers() const {
	return _handlers;
}

HandlerMap::HandlerInfo &HandlerMap::addHandler(const mem::StringView &name, Request::Method m, const mem::StringView &pattern,
		mem::Function<Handler *()> &&cb, mem::Value &&opts) {
	_handlers.emplace_back(name, m, pattern, std::move(cb), std::move(opts));
	return _handlers.back();
}

NS_SA_ST_END
