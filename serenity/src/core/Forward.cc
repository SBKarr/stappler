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

#include "Forward.h"
#include "Request.h"
#include "SPString.h"
#include "Root.h"
#include "StorageAdapter.h"

#include <idna.h>
#include "SPUrl.h"

NS_SA_EXT_BEGIN(messages)

struct ErrorNotificator {
	using Callback = Function<void(data::Value &&)>;

	Callback error;
	Callback debug;
};

bool isDebugEnabled() {
	return Root::getInstance()->isDebugEnabled();
}

void setDebugEnabled(bool v) {
	Root::getInstance()->setDebugEnabled(v);
}

void _addErrorMessage(data::Value &&data) {
	auto root = Root::getInstance();
	if (root->isDebugEnabled()) {
		data::Value bcast{
			std::make_pair("message", data::Value(true)),
			std::make_pair("level", data::Value("error")),
			std::make_pair("data", data::Value(data)),
		};
		if (auto a = storage::Adapter::FromContext()) {
			a.broadcast(bcast);
		}
	}

	if (auto serv = Server(apr::pool::server())) {
		serv.reportError(data);
	}

	Request rctx(apr::pool::request());
	if (rctx) {
		rctx.addErrorMessage(std::move(data));
	} else {
		auto pool = apr::pool::acquire();
		ErrorNotificator *err = nullptr;
		apr_pool_userdata_get((void **)&err, "Serenity.ErrorNotificator", pool);
		if (err && err->error) {
			err->error(std::move(data));
		} else {
			log::text("Error", data::toString(data, false));
		}
	}
}

void _addDebugMessage(data::Value &&data) {
	auto root = Root::getInstance();
	if (root->isDebugEnabled()) {
		data::Value bcast{
			std::make_pair("message", data::Value(true)),
			std::make_pair("level", data::Value("debug")),
			std::make_pair("data", data::Value(data)),
		};
		if (auto a = storage::Adapter::FromContext()) {
			a.broadcast(bcast);
		}
	}

	if (auto serv = Server(apr::pool::server())) {
		serv.reportError(data);
	}

	if (isDebugEnabled()) {
		Request rctx(apr::pool::request());
		if (rctx) {
			rctx.addDebugMessage(std::move(data));
		} else {
			auto pool = apr::pool::acquire();
			ErrorNotificator *err = nullptr;
			apr_pool_userdata_get((void **)&err, (const char *)config::getSerenityErrorNotificatorName(), pool);
			if (err && err->debug) {
				err->debug(std::move(data));
			} else {
				log::text("Debug", data::toString(data, false));
			}
		}
	}
}

void broadcast(const data::Value &val) {
	if (auto a = storage::Adapter::FromContext()) {
		a.broadcast(val);
	}
}
void broadcast(const Bytes &val) {
	if (auto a = storage::Adapter::FromContext()) {
		a.broadcast(val);
	}
}

void setNotifications(apr_pool_t *pool,
		const Function<void(data::Value &&)> &error,
		const Function<void(data::Value &&)> &debug) {
	ErrorNotificator *err = (ErrorNotificator *)apr_pcalloc(pool, sizeof(ErrorNotificator));
	new (err) ErrorNotificator();
	err->error = error;
	err->debug = debug;

	apr_pool_userdata_set(err, (const char *)config::getSerenityErrorNotificatorName(), nullptr, pool);
}

NS_SA_EXT_END(messages)
