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

#include "STInputFilter.h"

namespace stellator {

static InputFilter::Accept getAcceptedData(const Request &req, InputFilter::Exception &e) {
	Request r = req;
	auto &cfg = r.getInputConfig();
	InputFilter::Accept ret = InputFilter::Accept::None;
	if (r.getMethod() == Request::Post || r.getMethod() == Request::Put || r.getMethod() == Request::Patch) {
		const auto &ct = r.getRequestHeaders().at("Content-Type");
		const auto &b = r.getRequestHeaders().at("Content-Length");

		size_t cl = 0;
		if (!b.empty()) {
			cl = (size_t) mem::StringView(b).readInteger().get(0);
		}

		if (cl > config::getMaxInputPostSize() || cl > cfg.maxRequestSize) {
			messages::error("InputFilter", "Request size is out of limits", mem::Value{
				std::make_pair("length", mem::Value((int64_t)cl)),
				std::make_pair("local", mem::Value((int64_t)cfg.maxRequestSize)),
				std::make_pair("global", mem::Value((int64_t)config::getMaxInputPostSize())),
			});
			e = InputFilter::Exception::TooLarge;
			return ret;
		}

 		if (!ct.empty() && cl != 0 && cl < config::getMaxInputPostSize()) {
			if (ct.compare(0, 16, "application/json") == 0 || ct.compare(0, 16, "application/cbor") == 0) {
				if ((cfg.required & db::InputConfig::Require::Data) == db::InputConfig::Require::None
						&& (cfg.required & db::InputConfig::Require::Body) == db::InputConfig::Require::None) {
					messages::error("InputFilter", "No data to process", mem::Value{
						std::make_pair("content", mem::Value(ct)),
						std::make_pair("available", mem::Value("data, body")),
					});
					e = InputFilter::Exception::Unrecognized;
					return ret;
				}
				ret = InputFilter::Accept::Json;
			} else if (ct.compare(0, 33, "application/x-www-form-urlencoded") == 0) {
				if ((cfg.required & db::InputConfig::Require::Data) == db::InputConfig::Require::None
						&& (cfg.required & db::InputConfig::Require::Body) == db::InputConfig::Require::None) {
					messages::error("InputFilter", "No data to process", mem::Value{
						std::make_pair("content", mem::Value(ct)),
						std::make_pair("available", mem::Value("data, body")),
					});
					e = InputFilter::Exception::Unrecognized;
					return ret;
				}
				ret = InputFilter::Accept::Urlencoded;
			} else if (ct.compare(0, 30, "multipart/form-data; boundary=") == 0) {
				if ((cfg.required & db::InputConfig::Require::Data) == db::InputConfig::Require::None
						&& (cfg.required & db::InputConfig::Require::Body) == db::InputConfig::Require::None
						&& (cfg.required & db::InputConfig::Require::Files) == db::InputConfig::Require::None
						&& (cfg.required & db::InputConfig::Require::FilesAsData) == db::InputConfig::Require::None) {
					messages::error("InputFilter", "No data to process", mem::Value{
						std::make_pair("content", mem::Value(ct)),
						std::make_pair("available", mem::Value("data, body, files")),
					});
					e = InputFilter::Exception::Unrecognized;
					return ret;
				}
				ret = InputFilter::Accept::Multipart;
			} else {
				if ((cfg.required & db::InputConfig::Require::Files) == db::InputConfig::Require::None) {
					messages::error("InputFilter", "No data to process", mem::Value{
						std::make_pair("content", mem::Value(ct)),
						std::make_pair("available", mem::Value("files")),
					});
					e = InputFilter::Exception::Unrecognized;
					return ret;
				}
				ret = InputFilter::Accept::Files;
			}
		} else {
			e = InputFilter::Exception::Unrecognized;
		}
	}
	return ret;
}

db::InputFile *InputFilter::getFileFromContext(int64_t id) {
	/*auto req = apr::pool::request();
	if (req) {
		Request rctx(req);
		auto f = rctx.getInputFilter();
		if (f) {
			return f->getInputFile(id);
		}
	}*/
	return nullptr;

}
InputFilter::Exception InputFilter::insert(const Request &r) {
	return mem::perform([&] () -> InputFilter::Exception {
		Exception e = Exception::None;
		auto accept = getAcceptedData(r, e);
		if (accept == Accept::None) {
			return e;
		}

		auto f = new (r.pool()) InputFilter(r, accept);
		//ap_add_input_filter("Serenity::InputFilter", (void *)f, r, r.connection());
		Request(r).setInputFilter(f);
		return e;
	}, r);
}

/*apr_status_t InputFilter::filterFunc(ap_filter_t *f, apr_bucket_brigade *bb,
		ap_input_mode_t mode, apr_read_type_e block, apr_off_t readbytes) {
	return apr::pool::perform([&] () -> apr_status_t {
		if (f->ctx) {
			InputFilter *filter = (InputFilter *) f->ctx;
			return filter->func(f, bb, mode, block, readbytes);
		} else {
			return ap_get_brigade(f->next, bb, mode, block, readbytes);
		}
	}, f->r);
}

int InputFilter::filterInit(ap_filter_t *f) {
	return apr::pool::perform([&] () -> int {
		if (f->ctx) {
			InputFilter *filter = (InputFilter *) f->ctx;
			return filter->init(f);
		} else {
			return OK;
		}
	}, f->r);
}*/

InputFilter::InputFilter(const Request &r, Accept a) : _body() {
	_accept = a;
	_request = r;

	_isStarted = false;
	_isCompleted = false;

	const auto &b = _request.getRequestHeaders().at("Content-Length");

	if (!b.empty()) {
		_contentLength = mem::StringView(b).readInteger().get(0);
	}
}

/*int InputFilter::init(ap_filter_t *f) {
	_startTime =_time = Time::now();
	_isStarted = true;

	if (_accept == Accept::Multipart) {
		const auto &ct = _request.getRequestHeaders().at("Content-Type");
		auto pos = ct.find("boundary="_weak);
		if (pos != apr::string::npos) {
			_parser = new MultipartParser(_request.getInputConfig(), _contentLength,
					ct.substr(pos + "boundary="_len));
		}
	} else if (_accept == Accept::Urlencoded) {
		_parser = new UrlEncodeParser(_request.getInputConfig(), _contentLength);
	} else if (_accept == Accept::Json) {
		_parser = new DataParser(_request.getInputConfig(), _contentLength);
	} else if (_accept == Accept::Files) {
		const auto &ct = _request.getRequestHeaders().at("Content-Type");
		const auto &name = _request.getRequestHeaders().at("X-File-Name");

		_parser = new FileParser(_request.getInputConfig(), ct, name, _contentLength);
	}

	if (isBodySavingAllowed()) {
		_body.reserve(_contentLength);
	}

	if (_parser) {
		Root::getInstance()->onFilterInit(this);
	}

	return OK;
}

apr_status_t InputFilter::func(ap_filter_t *f, apr_bucket_brigade *bb,
		ap_input_mode_t mode, apr_read_type_e block, apr_off_t readbytes) {

	apr_status_t rv = ap_get_brigade(f->next, bb, mode, block, readbytes);
	if (rv == APR_SUCCESS && _parser && getConfig().required != InputConfig::Require::None && _accept != Accept::None) {
		if (_read + readbytes > _contentLength) {
			_unupdated += _contentLength - _read;
			_read = _contentLength;
		} else {
			_read += readbytes;
			_unupdated += readbytes;
		}

		if (_read > getConfig().maxRequestSize) {
			f->ctx = NULL;
			return rv;
		}

		auto t = Time::now();
		_timer = t - _time;
		_time = t;

		if (_timer > getConfig().updateTime || _unupdated > (_contentLength * getConfig().updateFrequency)) {
			Root::getInstance()->onFilterUpdate(this);
			_timer = TimeInterval();
			_unupdated = 0;
		}

		apr_bucket *b = NULL;
		for (b = APR_BRIGADE_FIRST(bb); b != APR_BRIGADE_SENTINEL(bb); b = APR_BUCKET_NEXT(b)) {
			if (!_eos) {
				step(b, block);
			}
		}
	}
	return rv;
}

void InputFilter::step(apr_bucket* b, apr_read_type_e block) {
	if (getConfig().required == InputConfig::Require::None || _accept == Accept::None) {
		return;
	}
	if (!APR_BUCKET_IS_METADATA(b)) {
		const char *str;
		size_t len;
		apr_bucket_read(b, &str, &len, block);
		Reader reader(str, len);

		if (isBodySavingAllowed()) {
			_body.write(str, len);
		}
		if (_parser && (
				(_accept == Accept::Urlencoded && isDataParsingAllowed()) ||
				(_accept == Accept::Multipart && (isFileUploadAllowed() || isDataParsingAllowed()) ) ||
				(_accept == Accept::Json && isDataParsingAllowed()) ||
				(_accept == Accept::Files && isFileUploadAllowed()))) {
			_parser->run(str, len);
		}
	} else if (APR_BUCKET_IS_EOS(b)) {
		if (_parser && (
				(_accept == Accept::Urlencoded && isDataParsingAllowed()) ||
				(_accept == Accept::Multipart && (isFileUploadAllowed() || isDataParsingAllowed()) ) ||
				(_accept == Accept::Json && isDataParsingAllowed()) ||
				(_accept == Accept::Files && isFileUploadAllowed()))) {
			_parser->finalize();
			auto &files = _parser->getFiles();
			if (!files.empty()) {

			}
		}
		_eos = true;
		_isCompleted = true;
		Root::getInstance()->onFilterComplete(this);
		if (_parser) {
			_parser->cleanup();
		}
	}
}*/

size_t InputFilter::getContentLength() const {
	return _contentLength;
}
size_t InputFilter::getBytesRead() const {
	return _read;
}
size_t InputFilter::getBytesReadSinceUpdate() const {
	return _unupdated;
}

mem::Time InputFilter::getStartTime() const {
	return _startTime;
}
mem::TimeInterval InputFilter::getElapsedTime() const {
	return mem::Time::now() - _startTime;
}
mem::TimeInterval InputFilter::getElapsedTimeSinceUpdate() const {
	return mem::Time::now() - _time;
}

bool InputFilter::isFileUploadAllowed() const {
	return (getConfig().required & db::InputConfig::Require::Files) != db::InputConfig::Require::None;
}
bool InputFilter::isDataParsingAllowed() const {
	return (getConfig().required & db::InputConfig::Require::Data) != db::InputConfig::Require::None;
}
bool InputFilter::isBodySavingAllowed() const {
	return (getConfig().required & db::InputConfig::Require::Body) != db::InputConfig::Require::None;
}

bool InputFilter::isCompleted() const {
	return _isCompleted;
}

const mem::ostringstream & InputFilter::getBody() const {
	return _body;
}
mem::Value & InputFilter::getData() {
	return _data;
}
mem::Vector<db::InputFile> &InputFilter::getFiles() {
	return _files;
}

db::InputFile * InputFilter::getInputFile(int64_t idx) const {
	/*if (idx < 0) {
		idx = -(idx + 1);
	}

	auto &files = _parser->getFiles();
	if (size_t(idx) >= files.size()) {
		return nullptr;
	}
	return &files.at(size_t(idx));*/
	return nullptr;
}

const db::InputConfig & InputFilter::getConfig() const {
	return _request.getInputConfig();
}

Request InputFilter::getRequest() const {
	return _request;
}

}
