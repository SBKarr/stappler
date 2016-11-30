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
#include "Couchbase.h"
#include "Request.h"
#include "Connection.h"

#ifndef NOCB

NS_SA_EXT_BEGIN(couchbase)

Config::Config() { }

void Config::init(data::Value && data) {
	if (data.isDictionary()) {
		for (auto &it : data.asDict()) {
			if (it.first == "host") {
				host = std::move(it.second.getString());
			} else if (it.first == "name") {
				name = std::move(it.second.getString());
			} else if (it.first == "password") {
				password = std::move(it.second.getString());
			} else if (it.first == "bucket") {
				bucket = std::move(it.second.getString());
			} else if (it.first == "min") {
				min = (int)it.second.getInteger();
			} else if (it.first == "softmax") {
				softMax = (int)it.second.getInteger();
			} else if (it.first == "hardmax") {
				hardMax = (int)it.second.getInteger();
			} else if (it.first == "ttl") {
				ttl = (int)it.second.getInteger();
			}
		}
	}
}

Bytes Key(const Bytes &bin) {
	Bytes ret; ret.resize(bin.size() + 1);
	ret[0] = 'B';
	memcpy(ret.data() + 1, bin.data(), bin.size());
	return ret;
}

Bytes Key(const String &txt, bool weak) {
	if (!weak) {
		Bytes ret; ret.resize(txt.size());
		memcpy(ret.data(), txt.data(), txt.size());
		return ret;
	} else {
		Bytes ret;
		ret.assign_weak((const uint8_t *)txt.data(), txt.size());
		return ret;
	}
}
String Text(const Bytes &bin, bool weak) {
	if (!weak) {
		String ret; ret.resize(bin.size());
		memcpy(ret.data(), bin.data(), bin.size());
		return ret;
	} else {
		String ret;
		ret.assign_weak((const char *)bin.data(), bin.size());
		return ret;
	}
}
Bytes Uuid(const apr::uuid &u) {
	Bytes ret; ret.resize(16 + 1);
	ret[0] = 'U';
	memcpy(ret.data() + 1, u.data(), 16);
	return ret;
}

Bytes Oid(uint64_t oid) {
	Bytes ret; ret.resize(sizeof (uint64_t) + 1);
	ret[0] = 'O';
	memcpy(ret.data() + 1, &oid, sizeof (uint64_t));
	return ret;
}
Bytes Oid(uint64_t oid, Bytes &&reuse) {
	if (reuse.size() != sizeof (uint64_t) + 1) {
		reuse.resize(sizeof (uint64_t) + 1);
		reuse[0] = 'O';
	}
	memcpy(reuse.data() + 1, &oid, sizeof (uint64_t));
	return std::move(reuse);
}
Bytes Sid(uint64_t sid) {
	Bytes ret; ret.resize(sizeof (uint64_t) + 1);
	ret[0] = 'S';
	memcpy(ret.data() + 1, &sid, sizeof (uint64_t));
	return ret;
}
Bytes Sid(uint64_t sid, Bytes &&reuse) {
	if (reuse.size() != sizeof (uint64_t) + 1) {
		reuse.resize(sizeof (uint64_t) + 1);
		reuse[0] = 'S';
	}
	memcpy(reuse.data() + 1, &sid, sizeof (uint64_t));
	return std::move(reuse);
}

static lcb_t couchbase_connect(const Config &cfg) {
	lcb_t instance = nullptr;
	struct lcb_create_st create_options;
	lcb_error_t err = LCB_SUCCESS;

	memset(&create_options, 0, sizeof (create_options));
	create_options.v.v0.host = cfg.host.c_str();
	create_options.v.v0.user = cfg.name.c_str();
	create_options.v.v0.passwd = cfg.password.c_str();
	create_options.v.v0.bucket = cfg.bucket.c_str();;

	err = lcb_create(&instance, &create_options);
	if (err != LCB_SUCCESS) {
		log("CouchBase: Failed to create libcouchbase instance: %s", lcb_strerror(NULL, err));
		return nullptr;
	}

	/*
	 * Initiate the connect sequence in libcouchbase
	 */
	if ((err = lcb_connect(instance)) != LCB_SUCCESS) {
		log("CouchBase: Failed to initiate connect: %s", lcb_strerror(instance, err));
		lcb_destroy(instance);
		return nullptr;
	}

	/* Run the event loop and wait until we've connected */
	lcb_wait(instance);
	if ((err = lcb_get_bootstrap_status(instance)) != LCB_SUCCESS) {
		log("CouchBase: Failed to connect with couchbase server: %s", lcb_strerror(instance, err));
		lcb_destroy(instance);
		return nullptr;
	}

	return instance;
}

Connection *Connection::create(apr_pool_t *pool, const Config &cfg) {
	apr_pool_t *newPool;
	apr_status_t status = apr_pool_create(&newPool, pool);
	if (status != APR_SUCCESS) {
		return nullptr;
	}

	return AllocStack::perform([&] () -> Connection * {
		auto instance = couchbase_connect(cfg);
		if (instance) {
			auto ret = new (newPool) Connection(pool, cfg, instance);
			if (ret->shouldBeInvalidated()) {
				delete ret;
				return nullptr;
			}
			return ret;
		}
		return nullptr;
	}, newPool);
}

Connection::~Connection() {
	if (_pool) {
		apr_pool_destroy(_pool);
		_pool = nullptr;
	}
}

void sa_cb_error_callback(lcb_t instance, lcb_error_t error) {
	auto cb = (Connection *) lcb_get_cookie(instance);
	cb->onError(error, "generic error"_weak);
}

void sa_cb_get_callback(lcb_t instance, const void *cookie, lcb_error_t error, const lcb_get_resp_t *resp) {
	if (cookie) {
		auto cb = (Connection *) lcb_get_cookie(instance);
		auto cmd = (CallbackRec<GetCmd, GetSig> *) cookie;
		if (error != LCB_SUCCESS) {
			cb->onError(error, "GET"_weak);
		}
		cmd->cb(*cmd->data, error, Bytes::make_weak((const uint8_t *)resp->v.v0.bytes, resp->v.v0.nbytes),
			resp->v.v0.cas, resp->v.v0.flags);
	}
}

void sa_cb_version_callback(lcb_t instance, const void *cookie, lcb_error_t error, const lcb_server_version_resp_t *resp) {
	if (cookie && resp->v.v0.nvstring > 0) {
		auto cb = (Connection *) lcb_get_cookie(instance);
		auto cmd = (CallbackRec<VersionCmd, VersionSig> *) cookie;
		if (error != LCB_SUCCESS) {
			cb->onError(error, "VERSION"_weak);
		}
		cmd->cb(*cmd->data, error, String::make_weak(resp->v.v0.vstring, resp->v.v0.nvstring));
	}
}

void sa_cb_store_callback(lcb_t instance, const void *cookie, lcb_storage_t operation, lcb_error_t error, const lcb_store_resp_t *resp) {
	if (cookie) {
		auto cb = (Connection *) lcb_get_cookie(instance);
		auto cmd = (CallbackRec<StoreCmd, StoreSig> *) cookie;
		if (error != LCB_SUCCESS) {
			cb->onError(error, "STORE"_weak);
		}
		cmd->cb(*cmd->data, error, operation, resp->v.v0.cas);
	}
}

void sa_cb_touch_callback(lcb_t instance, const void *cookie, lcb_error_t error, const lcb_touch_resp_t *resp) {
	if (cookie) {
		auto cb = (Connection *) lcb_get_cookie(instance);
		auto cmd = (CallbackRec<TouchCmd, TouchSig> *) cookie;
		if (error != LCB_SUCCESS) {
			cb->onError(error, "TOUCH"_weak);
		}
		cmd->cb(*cmd->data, error, resp->v.v0.cas);
	}
}

void sa_cb_unlock_callback(lcb_t instance, const void *cookie, lcb_error_t error, const lcb_unlock_resp_t *resp) {
	if (cookie) {
		auto cb = (Connection *) lcb_get_cookie(instance);
		auto cmd = (CallbackRec<UnlockCmd, UnlockSig> *) cookie;
		if (error != LCB_SUCCESS) {
			cb->onError(error, "UNLOCK"_weak);
		}
		cmd->cb(*cmd->data, error);
	}
}

void sa_cb_arithmetic_callback(lcb_t instance, const void *cookie, lcb_error_t error, const lcb_arithmetic_resp_t *resp) {
	if (cookie) {
		auto cb = (Connection *) lcb_get_cookie(instance);
		auto cmd = (CallbackRec<MathCmd, MathSig> *) cookie;
		if (error != LCB_SUCCESS) {
			cb->onError(error, "MATH"_weak);
		}
		cmd->cb(*cmd->data, error, resp->v.v0.value, resp->v.v0.cas);
	}
}

void sa_cb_observe_callback(lcb_t instance, const void *cookie, lcb_error_t error, const lcb_observe_resp_t *resp) {
	if (cookie) {
		auto cb = (Connection *) lcb_get_cookie(instance);
		auto &v = resp->v.v0;
		auto cmd = (CallbackRec<ObserveCmd, ObserveSig> *) cookie;
		if (error != LCB_SUCCESS) {
			cb->onError(error, "OBSERVE"_weak);
		}
		cmd->cb(*cmd->data, error, v.cas, v.status, (v.from_master != 0), v.ttp, v.ttr);
	}
}

void sa_cb_remove_callback(lcb_t instance, const void *cookie, lcb_error_t error, const lcb_remove_resp_t *resp) {
	if (cookie) {
		auto cb = (Connection *) lcb_get_cookie(instance);
		auto cmd = (CallbackRec<RemoveCmd, RemoveSig> *) cookie;
		if (error != LCB_SUCCESS) {
			cb->onError(error, "REMOVE"_weak);
		}
		cmd->cb(*cmd->data, error, resp->v.v0.cas);
	}
}

void sa_cb_flush_callback(lcb_t instance, const void *cookie, lcb_error_t error, const lcb_flush_resp_t *resp) {
	if (cookie) {
		auto cb = (Connection *) lcb_get_cookie(instance);
		auto cmd = (CallbackRec<FlushCmd, FlushSig> *) cookie;
		if (error != LCB_SUCCESS) {
			cb->onError(error, "FLUSH"_weak);
		}
		cmd->cb(*cmd->data, error);
	}
}

Connection::Connection(apr_pool_t *pool, const Config &cfg, lcb_t instance)
: _pool(pool), _instance(instance), _host(cfg.host), _bucket(cfg.bucket) {
	lcb_set_cookie(_instance, this);

	lcb_set_bootstrap_callback(_instance, sa_cb_error_callback);
	lcb_set_get_callback(_instance, sa_cb_get_callback);
	lcb_set_version_callback(_instance, sa_cb_version_callback);
	lcb_set_store_callback(_instance, sa_cb_store_callback);
	lcb_set_unlock_callback(_instance, sa_cb_unlock_callback);
	lcb_set_touch_callback(_instance, sa_cb_touch_callback);
	lcb_set_arithmetic_callback(_instance, sa_cb_arithmetic_callback);
	lcb_set_remove_callback(_instance, sa_cb_remove_callback);
	lcb_set_observe_callback(_instance, sa_cb_observe_callback);
	lcb_set_flush_callback(_instance, sa_cb_flush_callback);
}

bool Connection::shouldBeInvalidated() const {
	if (_lastError == LCB_EBADHANDLE) {
		return true;
	} else {
		return false;
	}
}

void Connection::onError(lcb_error_t err, const apr::string &txt) {
	_lastError = err;
	if (_lastError != LCB_SUCCESS) {
		Request::AddError("couchbase", "Error in query", data::Value {
				std::make_pair("query", data::Value(txt)),
				std::make_pair("error", data::Value(lcb_strerror(_instance, _lastError)))
		});
	}
}

void Connection::addCmd(GetCmd &c, GetRec &rec) {
	lcb_get_cmd_t cmd{0, { c.key.data(), c.key.size(), c.exptime, c.lock?1:0 }};

	lcb_get_cmd_t * commands[] = {&cmd};
	lcb_error_t error = lcb_get(_instance, &rec, 1, commands);
	if (error != LCB_SUCCESS) {
		Request::AddError("couchbase", "Failed to prepare GET", data::Value(lcb_strerror(_instance, error)));
	}
}
void Connection::addCmd(StoreCmd &c, StoreRec &rec) {
	lcb_store_cmd_t cmd{0, { c.key.data(), c.key.size(), c.data.data(), c.data.size(),
			c.flags, c.cas, 0 /*datatype*/, c.exptime, (lcb_storage_t)c.op }};

	lcb_store_cmd_t * commands[] = {&cmd};
	lcb_error_t error = lcb_store(_instance, &rec, 1, commands);
	if (error != LCB_SUCCESS) {
		Request::AddError("couchbase", "Failed to prepare STORE", data::Value(lcb_strerror(_instance, error)));
	}
}
void Connection::addCmd(MathCmd &c, MathRec &rec) {
	lcb_arithmetic_cmd_t cmd{0, { c.key.data(), c.key.size(), c.exptime, c.create?1:0, c.delta, c.initial }};

	lcb_arithmetic_cmd_t * commands[] = {&cmd};
	lcb_error_t error = lcb_arithmetic(_instance, &rec, 1, commands);
	if (error != LCB_SUCCESS) {
		Request::AddError("couchbase", "Failed to prepare MATH", data::Value(lcb_strerror(_instance, error)));
	}
}
void Connection::addCmd(ObserveCmd &c, ObserveRec &rec) {
	lcb_observe_cmd_t cmd{0, { c.key.data(), c.key.size() } };

	lcb_observe_cmd_t * commands[] = {&cmd};
	lcb_error_t error = lcb_observe(_instance, &rec, 1, commands);
	if (error != LCB_SUCCESS) {
		Request::AddError("couchbase", "Failed to prepare OBSERVE", data::Value(lcb_strerror(_instance, error)));
	}
}
void Connection::addCmd(TouchCmd &c, TouchRec &rec) {
	lcb_touch_cmd_t cmd{0, { c.key.data(), c.key.size(), c.exptime, 0 } };

	lcb_touch_cmd_t * commands[] = {&cmd};
	lcb_error_t error = lcb_touch(_instance, &rec, 1, commands);
	if (error != LCB_SUCCESS) {
		Request::AddError("couchbase", "Failed to prepare TOUCH", data::Value(lcb_strerror(_instance, error)));
	}
}
void Connection::addCmd(UnlockCmd &c, UnlockRec &rec) {
	lcb_unlock_cmd_t cmd{0, { c.key.data(), c.key.size(), c.cas } };

	lcb_unlock_cmd_t * commands[] = {&cmd};
	lcb_error_t error = lcb_unlock(_instance, &rec, 1, commands);
	if (error != LCB_SUCCESS) {
		Request::AddError("couchbase", "Failed to prepare UNLOCK", data::Value(lcb_strerror(_instance, error)));
	}
}
void Connection::addCmd(RemoveCmd &c, RemoveRec &rec) {
	lcb_remove_cmd_t cmd{0, { c.key.data(), c.key.size(), c.cas } };

	lcb_remove_cmd_t * commands[] = {&cmd};
	lcb_error_t error = lcb_remove(_instance, &rec, 1, commands);
	if (error != LCB_SUCCESS) {
		Request::AddError("couchbase", "Failed to prepare REMOVE", data::Value(lcb_strerror(_instance, error)));
	}
}
void Connection::addCmd(VersionCmd &c, VersionRec &rec) {
	lcb_server_version_cmd_t cmd{0};

	lcb_server_version_cmd_t * commands[] = {&cmd};
	lcb_error_t error = lcb_server_versions(_instance, &rec, 1, commands);
	if (error != LCB_SUCCESS) {
		Request::AddError("couchbase", "Failed to prepare VERSION", data::Value(lcb_strerror(_instance, error)));
	}
}
void Connection::addCmd(FlushCmd &c, FlushRec &rec) {
	lcb_flush_cmd_t cmd{0};

	lcb_flush_cmd_t * commands[] = {&cmd};
	lcb_error_t error = lcb_flush(_instance, &rec, 1, commands);
	if (error != LCB_SUCCESS) {
		Request::AddError("couchbase", "Failed to prepare FLUSH", data::Value(lcb_strerror(_instance, error)));
	}
}

void Connection::run() {
	lcb_wait(_instance);
}

Handle::Handle() : connection(nullptr) {
	auto serv = AllocStack::get().server();
	if (serv) {
		acquire(Server(serv));
	}
}
Handle::Handle(const serenity::Server &serv) : connection(nullptr) {
	acquire(serv);
}
Handle::Handle(const serenity::Request &req) : connection(nullptr) {
	acquire(req.server());
}
Handle::Handle(const serenity::Connection &conn) : connection(nullptr) {
	acquire(conn.server());
}

Handle::~Handle() {
	if (connection) {
		release();
	}
}

Handle::Handle(Handle &&h) : serv(std::move(h.serv)), connection(h.connection) { }
Handle& Handle::operator=(Handle &&h) {
	serv = std::move(h.serv);
	connection = h.connection;
	return *this;
}

void Handle::acquire(const Server &s) {
	serv = s;
	connection = serv.acquireCouchbase();
}
void Handle::release() {
	if (connection && serv) {
		serv.releaseCouchbase(connection);
	}
}

NS_SA_EXT_END(couchbase)

#endif
