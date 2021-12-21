/**
Copyright (c) 2020-2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPCommon.h"
#include "SPString.h"
#include "SPNetworkHandle.h"

#include <curl/curl.h>

namespace stappler {

bool NetworkHandle::perform(const Callback<bool(CURL *)> &onBeforePerform, const Callback<bool(CURL *)> &onAfterPerform) {
	_isRequestPerformed = false;
	_errorCode = CURLE_OK;
	_responseCode = -1;

	Context ctx;
	ctx.curl = Network::getHandle(_reuse);

	if (!ctx.curl) {
		return false;
	}

	if (!prepare(&ctx, onBeforePerform)) {
		return false;
	}

	ctx.code = curl_easy_perform(ctx.curl);
	auto ret = finalize(&ctx, onAfterPerform);
	Network::releaseHandle(ctx.curl, _reuse, ctx.code == CURLE_OK);
	return ret;
}

bool NetworkHandle::prepare(Context *ctx, const Callback<bool(CURL *)> &onBeforePerform) {
	if (_debug) {
		_debugData = StringStream();
	}

	_recievedHeaders.clear();
	_parsedHeaders.clear();

	ctx->handle = this;

	bool check = true;
	if (ctx->share) {
		SetOpt(check, ctx->curl, CURLOPT_SHARE, _sharedHandle);
	} else if (_shared) {
		if (!_sharedHandle) {
			_sharedHandle = curl_share_init();
			curl_share_setopt((CURLSH *)_sharedHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
			curl_share_setopt((CURLSH *)_sharedHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
			curl_share_setopt((CURLSH *)_sharedHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
			curl_share_setopt((CURLSH *)_sharedHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
		}
		SetOpt(check, ctx->curl, CURLOPT_COOKIEFILE, "/undefined");
		SetOpt(check, ctx->curl, CURLOPT_SHARE, _sharedHandle);
	} else {
		SetOpt(check, ctx->curl, CURLOPT_SHARE, nullptr);
	}

	check = (check) ? setupCurl(ctx->curl, ctx->error.data()) : false;
	check = (check) ? setupDebug(ctx->curl, _debug) : false;
	check = (check) ? setupRootCert(ctx->curl, _rootCertFile) : false;
	check = (check) ? setupHeaders(ctx->curl, _sendedHeaders, &ctx->headers, _user.empty() ? _keySign : StringView()) : false;
	check = (check) ? setupUserAgent(ctx->curl, _userAgent) : false;
	check = (check) ? setupUser(ctx->curl, _user, _password, _authMethod) : false;
	check = (check) ? setupFrom(ctx->curl, _from) : false;
	check = (check) ? setupRecv(ctx->curl, _recv, &ctx->mailTo) : false;
	check = (check) ? setupProgress(ctx->curl, _method != Method::Head && (_uploadProgress || _downloadProgress)) : false;
	check = (check) ? setupCookies(ctx->curl, _cookieFile) : false;
	check = (check) ? setupProxy(ctx->curl, _proxyAddress, _proxyAuth) : false;
	check = (check) ? setupReceive(ctx->curl, ctx->inputFile, ctx->inputPos) : false;

    switch (_method) {
        case Method::Get:
        	check = (check) ? setupMethodGet(ctx->curl) : false;
            break;
        case Method::Head:
        	check = (check) ? setupMethodHead(ctx->curl) : false;
            break;
        case Method::Post:
        	check = (check) ? setupMethodPost(ctx->curl, ctx->outputFile) : false;
            break;
        case Method::Put:
        	check = (check) ? setupMethodPut(ctx->curl, ctx->outputFile) : false;
            break;
        case Method::Delete:
        	check = (check) ? setupMethodDelete(ctx->curl) : false;
            break;
        case Method::Smtp:
        	check = (check) ? setupMethodSmpt(ctx->curl, ctx->outputFile) : false;
        	break;
        default:
            break;
    }

    if (!check) {
    	if (!_silent) {
        	log::format("CURL", "Fail to setup %s", _url.c_str());
    	}
        return false;
    }

	if (onBeforePerform) {
		if (!onBeforePerform(ctx->curl)) {
	    	if (!_silent) {
	        	log::text("CURL", "onBeforePerform failed");
	    	}
	        return false;
		}
	}

	_debugData.clear();
	_parsedHeaders.clear();
	_recievedHeaders.clear();
	return true;
}

bool NetworkHandle::finalize(Context *ctx, const Callback<bool(CURL *)> &onAfterPerform) {
	_errorCode = ctx->code;
	if (ctx->headers) {
		curl_slist_free_all(ctx->headers);
		ctx->headers = nullptr;
	}

	if (ctx->mailTo) {
		curl_slist_free_all(ctx->mailTo);
		ctx->mailTo = nullptr;
	}

	if (CURLE_RANGE_ERROR == _errorCode && _method == Method::Get) {
		size_t allowedRange = size_t(getReceivedHeaderInt("X-Range"));
		if (allowedRange == ctx->inputPos) {
			if (!_silent) {
				log::text("CURL", "Get 0-range is not an error, fixed error code to CURLE_OK");
			}
			ctx->success = true;
			_errorCode = CURLE_OK;
		}
    }

	if (CURLE_OK == _errorCode) {
        _isRequestPerformed = true;
        if (_method != Method::Smtp) {
            const char *ct = NULL;
            long code = 200;

        	curl_easy_getinfo(ctx->curl, CURLINFO_RESPONSE_CODE, &code);
        	curl_easy_getinfo(ctx->curl, CURLINFO_CONTENT_TYPE, &ct);
			if (ct) {
				_contentType = ct;
#if LINUX
				if (ctx->inputFile && !_contentType.empty()) {
					fflush(ctx->inputFile);
					NetworkHandle_setUserAttributes(ctx->inputFile, _contentType,
							Time::microseconds(getReceivedHeaderInt("X-FileModificationTime")));
					fclose(ctx->inputFile);
					ctx->inputFile = nullptr;
				}
#endif
			}

            _responseCode = (long)code;

        	SP_NETWORK_LOG("performed: %d %s %ld", (int)_method, _url.c_str(), _responseCode);

	        if (_responseCode == 416) {
	        	size_t allowedRange = size_t(getReceivedHeaderInt("X-Range"));
				if (allowedRange == ctx->inputPos) {
					_responseCode = 200;
					if (!_silent) {
						log::text("CURL", toString(_url, ": Get 0-range is not an error, fixed response code to 200"));
					}
				}
	        }

			if (_responseCode >= 200 && _responseCode < 400) {
				ctx->success = true;
			} else {
				ctx->success = false;
			}
        } else {
        	ctx->success = true;
        }
	} else {
		if (!_silent) {
			log::format("CURL", "fail to perform %s: (%ld) %s",  _url.c_str(), _errorCode, ctx->error.data());
		}
		_error = ctx->error.data();
		ctx->success = false;
    }

	if (onAfterPerform) {
		if (!onAfterPerform(ctx->curl)) {
			ctx->success = false;
		}
	}

	if (ctx->inputFile) {
		fflush(ctx->inputFile);
		fclose(ctx->inputFile);
		ctx->inputFile = nullptr;
	}
	if (ctx->outputFile) {
		fclose(ctx->outputFile);
		ctx->outputFile = nullptr;
	}
	return ctx->success;
}

void NetworkMultiHandle::addHandle(NetworkHandle *h, void *ptr) {
	pending.emplace_back(h, ptr);
}

bool NetworkMultiHandle::perform(const Callback<bool(NetworkHandle *, void *)> &cb) {
	auto m = curl_multi_init();
	Map<CURL *, NetworkHandle::Context> handles;

	auto initPending = [&] {
		for (auto &it : pending) {
			++ s_activeHandles;
			auto h = curl_easy_init();
			auto i = handles.emplace(h, NetworkHandle::Context()).first;
			i->second.userdata = it.second;
			i->second.curl = h;
			it.first->prepare(&i->second, nullptr);

			curl_multi_add_handle(m, h);
		}
		auto s = pending.size();
		pending.clear();
		return s;
	};

	auto cancel = [&] {
		for (auto &it : handles) {
			curl_multi_remove_handle(m, it.first);
			it.second.code = CURLE_FAILED_INIT;
			it.second.handle->finalize(&it.second, nullptr);

			-- s_activeHandles;
			curl_easy_cleanup(it.first);
		}
		curl_multi_cleanup(m);
	};

	//handles.reserve(pending.size());

	int running = initPending();
	do {
		auto err = curl_multi_perform(m, &running);
		if (err != CURLM_OK) {
			log::text("CURL", toString("Fail to perform multi: ", err));
			return false;
		}

		if (running > 0) {
			err = curl_multi_poll(m, NULL, 0, 1000, nullptr);
			if (err != CURLM_OK) {
				log::text("CURL", toString("Fail to poll multi: ", err));
				return false;
			}
		}

		struct CURLMsg *msg = nullptr;
		do {
			int msgq = 0;
			msg = curl_multi_info_read(m, &msgq);
			if (msg && (msg->msg == CURLMSG_DONE)) {
				CURL *e = msg->easy_handle;
				curl_multi_remove_handle(m, e);

				auto it = handles.find(e);
				if (it != handles.end()) {
					it->second.code = msg->data.result;
					it->second.handle->finalize(&it->second, nullptr);
					if (cb) {
						if (!cb(it->second.handle, it->second.userdata)) {
							handles.erase(it);
							cancel();
							return false;
						}
					}
					handles.erase(it);
				}

				-- s_activeHandles;
				curl_easy_cleanup(e);
			}
		} while (msg);

		running += initPending();
	} while (running > 0);

	curl_multi_cleanup(m);
 	return true;
}

}
