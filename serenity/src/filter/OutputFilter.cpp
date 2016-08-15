/*
   Copyright 2013 Roman "SBKarr" Katuntsev, LLC St.Appler

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "Define.h"
#include "OutputFilter.h"
#include "SPData.h"
#include "Output.h"
#include "Tools.h"

NS_SA_BEGIN

static apr_status_t filterFunc(ap_filter_t *f, apr_bucket_brigade *bb);
static int filterInit(ap_filter_t *f);

void OutputFilter::filterRegister() {
	ap_register_output_filter_protocol("Serenity::OutputFilter", &(filterFunc),
			&(filterInit), (ap_filter_type)(AP_FTYPE_PROTOCOL),
			AP_FILTER_PROTO_CHANGE | AP_FILTER_PROTO_CHANGE_LENGTH);
}

void OutputFilter::insert(const Request &r) {
	AllocStack::perform([&] () {
		auto f = new (r.request()->pool) OutputFilter(r);
		ap_add_output_filter("Serenity::OutputFilter", (void *)f, r, r.connection());
	}, r.request());
}

apr_status_t filterFunc(ap_filter_t *f, apr_bucket_brigade *bb) {
	return AllocStack::perform([&] () -> apr_status_t {
		if (APR_BRIGADE_EMPTY(bb)) {
			return APR_SUCCESS;
		}
		if (f->ctx) {
			OutputFilter *filter = (OutputFilter *) f->ctx;
			return filter->func(f, bb);
		} else {
			return ap_pass_brigade(f->next,bb);
		}
	}, f->r);
}

int filterInit(ap_filter_t *f) {
	return AllocStack::perform([&] () -> apr_status_t {
		if (f->ctx) {
			OutputFilter *filter = (OutputFilter *) f->ctx;
			return filter->init(f);
		} else {
			return OK;
		}
	}, f->r);
}

OutputFilter::OutputFilter(const Request &rctx)
: _nameBuffer(64), _buffer(255) {
	_tmpBB = NULL;
	_seenEOS = false;
	_request = rctx;
	_char = 0;
	_buf = 0;
	_isWhiteSpace = true;
}

int OutputFilter::init(ap_filter_t* f) {
	_seenEOS = false;
	_skipFilter = false;
	return OK;
}

apr_status_t OutputFilter::func(ap_filter_t *f, apr_bucket_brigade *bb) {
	if (_seenEOS) {
		return APR_SUCCESS;
	}
	if (_skipFilter || _state == State::Body) {
		return ap_pass_brigade(f->next, bb);
	}
    conn_rec *c =  f->c;
	apr_bucket *e;
	const char *data = NULL;
	size_t len = 0;
	apr_status_t rv;

	if (!_tmpBB) {
		_tmpBB = apr_brigade_create(c->pool, c->bucket_alloc);
	}

	apr_read_type_e mode = APR_NONBLOCK_READ;

	while ((e = APR_BRIGADE_FIRST(bb)) != APR_BRIGADE_SENTINEL(bb)) {

        if (APR_BUCKET_IS_EOS(e)) {
			_seenEOS = true;
			APR_BUCKET_REMOVE(e);
            APR_BRIGADE_INSERT_TAIL(_tmpBB,e);
            break;
        }

        if (APR_BUCKET_IS_METADATA(e)) {
			APR_BUCKET_REMOVE(e);
            APR_BRIGADE_INSERT_TAIL(_tmpBB, e);
			continue;
		}
		if (_responseCode < 400 && _state == State::Body) {
			_skipFilter = true;
			APR_BUCKET_REMOVE(e);
			APR_BRIGADE_INSERT_TAIL(_tmpBB, e);
			break;
		}
		rv = apr_bucket_read(e, &data, &len, mode);
		if (rv == APR_EAGAIN && mode == APR_NONBLOCK_READ) {
			/* Pass down a brigade containing a flush bucket: */
			APR_BRIGADE_INSERT_TAIL(_tmpBB, apr_bucket_flush_create(_tmpBB->bucket_alloc));
			rv = ap_pass_brigade(f->next, _tmpBB);
			apr_brigade_cleanup(_tmpBB);
			if (rv != APR_SUCCESS) return rv;

			/* Retry, using a blocking read. */
			mode = APR_BLOCK_READ;
			continue;
		} else if (rv != APR_SUCCESS) {
			return rv;
		}

		if (rv == APR_SUCCESS) {
			if (len > 0 && _state != State::Body) {
				Reader reader(data, len);
				if (_state == State::FirstLine) {
					if (readRequestLine(reader)) {
						_responseLine = _buffer.str();
						_buffer.clear();
						_nameBuffer.clear();
					}
				}
				if (_state == State::Headers) {
					if (readHeaders(reader)) {
						apr::ostringstream servVersion;
						servVersion << "Serenity/2 Stappler/1 (" << tools::getCompileDate() << ")";
						_headers.emplace("Server", servVersion.str());
						_buffer.clear();
						if (_responseCode < 400) {
							_skipFilter = true;
						} else {
							output::writeData(_request, _buffer, [&] (const String &ct) {
								_headers.emplace("Content-Type", ct);
							}, resultForRequest(), true);
							_headers.emplace("Content-Length", apr_psprintf(c->pool, "%lu", _buffer.size()));
							// provide additional info for 416 if we can
							if (_responseCode == 416) {
								// we need to determine requested file size
								if (const char *filename = f->r->filename) {
									apr_finfo_t info;
									memset(&info, 0, sizeof(info));
									if (apr_stat(&info, filename, APR_FINFO_SIZE, c->pool) == APR_SUCCESS) {
										_headers.emplace("X-Range", apr_psprintf(c->pool, "%ld", info.size));
									} else {
										_headers.emplace("X-Range", "0");
									}
								} else {
									_headers.emplace("X-Range", "0");
								}
							} else if (_responseCode == 401) {
								if (_headers.at("WWW-Authenticate").empty()) {
									_headers.emplace("WWW-Authenticate", toString("Basic realm=\"", _request.getHostname(), "\""));
								}
							}
						}

						auto len = _responseLine.size() + 2;
						for (auto &it : _headers) {
							len += strlen(it.key) + strlen(it.val) + 4;
						}

						// create ostream with preallocated storage
						char dataBuf[len+1];
						apr::ostringstream output(dataBuf, len);

						output << _responseLine;
						for (auto &it : _headers) {
							output << it.key << ": " << it.val << "\r\n";
						}
						output << "\r\n";

						apr_bucket *b = apr_bucket_transient_create(output.data(), output.size(),
								_tmpBB->bucket_alloc);

						APR_BRIGADE_INSERT_TAIL(_tmpBB, b);

						if (!_buffer.empty()) {
							APR_BUCKET_REMOVE(e);
							APR_BRIGADE_INSERT_TAIL(_tmpBB, apr_bucket_transient_create(
									_buffer.data(), _buffer.size(), _tmpBB->bucket_alloc));
							APR_BRIGADE_INSERT_TAIL(_tmpBB, apr_bucket_flush_create(_tmpBB->bucket_alloc));
							APR_BRIGADE_INSERT_TAIL(_tmpBB, apr_bucket_eos_create(
									_tmpBB->bucket_alloc));
							_seenEOS = true;

							rv = ap_pass_brigade(f->next, _tmpBB);
							apr_brigade_cleanup(_tmpBB);
							return rv;
						} else {
							rv = ap_pass_brigade(f->next, _tmpBB);
							apr_brigade_cleanup(_tmpBB);
							if (rv != APR_SUCCESS) return rv;
						}
					}
				}
				if (_state == State::Body) {
					if (!reader.empty()) {
						APR_BRIGADE_INSERT_TAIL(_tmpBB, apr_bucket_transient_create(
								reader.data(), reader.size(), _tmpBB->bucket_alloc));
					}
				}
			}
		};
		/* Remove bucket e from bb. */
		APR_BUCKET_REMOVE(e);
		/* Pass brigade downstream. */
		rv = ap_pass_brigade(f->next, _tmpBB);
		apr_brigade_cleanup(_tmpBB);
		if (rv != APR_SUCCESS) return rv;

		mode = APR_NONBLOCK_READ;

		if (_state == State::Body) {
			break;
		}
	}

	if (!APR_BRIGADE_EMPTY(_tmpBB)) {
		rv = ap_pass_brigade(f->next, _tmpBB);
		apr_brigade_cleanup(_tmpBB);
		if (rv != APR_SUCCESS) return rv;
	}

	if (!APR_BRIGADE_EMPTY(bb)) {
		rv = ap_pass_brigade(f->next, bb);
		if (rv != APR_SUCCESS) return rv;
	}

	return APR_SUCCESS;
}

bool OutputFilter::readRequestLine(Reader &r) {
	while (!r.empty()) {
		if (_isWhiteSpace) {
			_buffer << r.readChars<Reader::CharGroup<chars::CharGroupId::WhiteSpace>>();
			if (!r.empty() && !r.is<chars::CharGroupId::WhiteSpace>()) {
				_isWhiteSpace = false;
			} else {
				continue;
			}
		}
		if (_subState == State::Protocol) {
			_buffer << r.readUntil<Reader::CharGroup<chars::CharGroupId::WhiteSpace>>();
			if (r.is<chars::CharGroupId::WhiteSpace>()) {
				_nameBuffer.clear();
				_isWhiteSpace = true;
				_subState = State::Code;
			}
		} else if (_subState == State::Code) {
			auto b = r.readChars<Reader::Range<'0', '9'>>();
			_nameBuffer << b;
			_buffer << b;
			if (r.is<chars::CharGroupId::WhiteSpace>()) {
				_nameBuffer << '\0';
				_responseCode = apr_strtoi64(_nameBuffer.data(), NULL, 0);
				_isWhiteSpace = true;
				_subState = State::Status;
			}
		} else if (_subState == State::Status) {
			auto statusText = r.readUntil<Reader::Chars<'\n'>>();
			_statusText = statusText.str();
			string::trim(_statusText);
			_buffer << statusText;
			if (r.is('\n')) {
				++ r;
				_buffer << '\n';
				_state = State::Headers;
				_subState = State::HeaderName;
				_isWhiteSpace = true;
				return true;
			}
		}
	}
	return false;
}
bool OutputFilter::readHeaders(Reader &r) {
	while (!r.empty()) {
		if (_subState == State::HeaderName) {
			_nameBuffer << r.readUntil<Reader::Chars<':', '\n'>>();
			if (r.is('\n')) {
				r ++;
				_state = State::Body;
				return true;
			} else if (r.is<':'>()) {
				r ++;
				_isWhiteSpace = true;
				_subState = State::HeaderValue;
			}
		} else if (_subState == State::HeaderValue) {
			_buffer << r.readUntil<Reader::Chars<'\n'>>();
			if (r.is('\n')) {
				r ++;

				auto key = _nameBuffer.str(); string::trim(key);
				auto value = _buffer.str(); string::trim(value);

				_headers.emplace(std::move(key), std::move(value));

				_buffer.clear();
				_nameBuffer.clear();
				_subState = State::HeaderName;
				_isWhiteSpace = true;
			}
		}
	}
	return false;
}

data::Value OutputFilter::resultForRequest() {
	data::Value ret;
	ret.setBool(false, "OK");
	ret.setInteger(apr_time_now(), "date");
	ret.setInteger(_responseCode, "status");
	ret.setString(std::move(_statusText), "message");

#if DEBUG
	if (!_request.getDebugMessages().empty()) {
		ret.setArray(std::move(_request.getDebugMessages()), "debug");
	}
#endif
	if (!_request.getErrorMessages().empty()) {
		ret.setArray(std::move(_request.getErrorMessages()), "errors");
	}

	return ret;
}

NS_SA_END
