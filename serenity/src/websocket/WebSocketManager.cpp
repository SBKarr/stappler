// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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
#include "WebSocket.h"

NS_SA_EXT_BEGIN(websocket)

constexpr auto WEBSOCKET_FILTER = "Serenity::WebsocketFilter";
constexpr auto WEBSOCKET_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
constexpr auto WEBSOCKET_GUID_LEN = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"_len;

static String makeAcceptKey(const String &key) {
	apr_byte_t digest[APR_SHA1_DIGESTSIZE];
	apr_sha1_ctx_t context;

	apr_sha1_init(&context);
	apr_sha1_update(&context, key.c_str(), key.size());
	apr_sha1_update(&context, (const char *)WEBSOCKET_GUID, (unsigned int)WEBSOCKET_GUID_LEN);
	apr_sha1_final(digest, &context);

	return base64::encode(CoderSource(digest, APR_SHA1_DIGESTSIZE));
}

apr_status_t Manager::filterFunc(ap_filter_t *f, apr_bucket_brigade *bb) {
	if (APR_BRIGADE_EMPTY(bb)) {
		return APR_SUCCESS;
	}

	auto h = (websocket::Handler *)f->ctx;
	if (h->isEnabled()) {
		if (h->writeBrigade(bb)) {
			return APR_SUCCESS;
		} else {
			return APR_EGENERAL;
		}
	} else {
		return ap_pass_brigade(f->next, bb);
	}
}

int Manager::filterInit(ap_filter_t *f) {
	return apr::pool::perform([&] () -> apr_status_t {
		return OK;
	}, f->c);
}

void Manager::filterRegister() {
	ap_register_output_filter(WEBSOCKET_FILTER, &(filterFunc),
			&(filterInit), ap_filter_type(AP_FTYPE_NETWORK - 1));
}

Manager::Manager() : _pool(getCurrentPool()), _mutex(_pool) { }
Manager::~Manager() { }

Handler * Manager::onAccept(const Request &req) {
	return nullptr;
}
bool Manager::onBroadcast(const data::Value & val) {
	return false;
}

size_t Manager::size() const {
	return _count.load();
}

void Manager::receiveBroadcast(const data::Value &val) {
	if (onBroadcast(val)) {
		_mutex.lock();
		for (auto &it : _handlers) {
			it->receiveBroadcast(val);
		}
		_mutex.unlock();
	}
}

int Manager::accept(Request &req) {
	auto h = req.getRequestHeaders();
	auto &version = h.at("sec-websocket-version");
	auto &key = h.at("sec-websocket-key");
	auto decKey = base64::decode(key);
	if (decKey.size() != 16 || version != "13") {
		req.getErrorHeaders().emplace("Sec-WebSocket-Version", "13");
		return HTTP_BAD_REQUEST;
	}

	auto handler = onAccept(req);
	if (handler) {
		auto hout = req.getResponseHeaders();

		hout.clear();
		hout.emplace("Upgrade", "websocket");
		hout.emplace("Connection", "Upgrade");
		hout.emplace("Sec-WebSocket-Accept", makeAcceptKey(key));

		apr_socket_timeout_set((apr_socket_t *)ap_get_module_config (req.request()->connection->conn_config, &core_module), -1);
		req.setStatus(HTTP_SWITCHING_PROTOCOLS);
		ap_send_interim_response(req.request(), 1);

		ap_filter_t *input_filter = req.request()->input_filters;
		while (input_filter != NULL) {
			if ((input_filter->frec != NULL) && (input_filter->frec->name != NULL)) {
				if (!strcasecmp(input_filter->frec->name, "http_in")
						|| !strcasecmp(input_filter->frec->name, "reqtimeout")) {
					auto next = input_filter->next;
					ap_remove_input_filter(input_filter);
					input_filter = next;
					continue;
				}
			}
			input_filter = input_filter->next;
		}

		auto r = req.request();

	    ap_add_output_filter(WEBSOCKET_FILTER, (void *)handler, r, r->connection);

		addHandler(handler);
		handler->run();
		removeHandler(handler);

	    ap_lingering_close(req.request()->connection);

	    return DONE;
	}
	if (req.getStatus() == HTTP_OK) {
		return HTTP_BAD_REQUEST;
	}
	return req.getStatus();
}

void Manager::addHandler(Handler * h) {
	_mutex.lock();
	_handlers.emplace_back(h);
	++ _count;
	_mutex.unlock();
}

void Manager::removeHandler(Handler * h) {
	_mutex.lock();
	auto it = _handlers.begin();
	while (it != _handlers.end() && *it != h) {
		++ it;
	}
	if (it != _handlers.end()) {
		_handlers.erase(it);
	}
	-- _count;
	_mutex.unlock();
}

NS_SA_EXT_END(websocket)
