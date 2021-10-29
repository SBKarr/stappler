/**
 Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SEDbdModule.h"

NS_SA_BEGIN

static apr_status_t DbdModule_construct(void **data_ptr, void *params, mem::pool_t *pool) {
	DbdModule *group = (DbdModule *)params;
	ap_dbd_t *rec = group->connect(pool);

	mem::pool::cleanup_register(rec->pool, [rec] {
		apr_dbd_close(rec->driver, rec->handle);
	});

    *data_ptr = rec;
    return APR_SUCCESS;
}

static apr_status_t DbdModule_destruct(void *data, void *params, mem::pool_t *pool) {
	DbdModule *group = (DbdModule *)params;

	if (!group->isDestroyed()) {
		ap_dbd_t *rec = (ap_dbd_t *)data;
		apr_pool_destroy(rec->pool);
	}

	return APR_SUCCESS;
}

DbdModule *DbdModule::create(mem::pool_t *root, const Config &cfg, Map<StringView, StringView> *params, StringView dbName) {
	auto pool = mem::pool::create(root);
	DbdModule *m = nullptr;
	mem::perform([&] {
		m = new (pool) DbdModule(pool, cfg, params, dbName.pdup(pool));
	}, pool);
	return m;
}

void DbdModule::destroy(DbdModule *module) {
	auto p = module->getPool();
	mem::perform([&] {
		module->close();
	}, p);
	mem::pool::destroy(p);
}

ap_dbd_t *DbdModule::connect(apr_pool_t *pool) const {
	ap_dbd_t *ret = nullptr;
	mem::perform([&] {
		ret = (ap_dbd_t *)_dbDriver->connect(_keywords.data(), _values.data(), 0).get();
	}, pool);
	return ret;
}

ap_dbd_t *DbdModule::openConnection(apr_pool_t *pool) {
	ap_dbd_t *rec = NULL;

	if (!_config.persistent) {
		DbdModule_construct((void**) &rec, this, pool);
		return rec;
	}

	auto rv = apr_reslist_acquire(_reslist, (void**) &rec);
	if (rv != APR_SUCCESS) {
		return NULL;
	}

	if (!_dbDriver->isValid(db::pq::Driver::Handle(rec))) {
		apr_reslist_invalidate(_reslist, rec);
		return nullptr;
	}

	return rec;
}

void DbdModule::closeConnection(ap_dbd_t *rec) {
	if (!_config.persistent) {
		apr_pool_destroy(rec->pool);
	} else {
		apr_reslist_release(_reslist, rec);
	}
}

void DbdModule::close() {
	if (!_destroyed) {
		if (_reslist) {
			apr_reslist_destroy(_reslist);
			_reslist = nullptr;
		}
		_destroyed = true;
	}
}

DbdModule::DbdModule(mem::pool_t *pool, const Config &cfg, Map<StringView, StringView> *params, StringView dbName)
: _pool(pool), _config(cfg), _dbParams(params), _dbName(dbName) {

	_dbDriver = db::pq::Driver::open(StringView("pgsql"));
	_dbDriver->setDbCtrl([this] (bool complete) {
		if (complete) {
			Root::getInstance()->incrementReleasedQueries();
		} else {
			Root::getInstance()->incrementPerformedQueries();
		}
	});

	_keywords.reserve(params->size());
	_values.reserve(params->size());

	for (auto &it : *_dbParams) {
		if (it.first == "dbname") {
			_keywords.emplace_back(it.first.data());
			_values.emplace_back(_dbName.data());
		} else {
			_keywords.emplace_back(it.first.data());
			_values.emplace_back(it.second.data());
		}
	}
	_keywords.emplace_back(nullptr);
	_keywords.emplace_back(nullptr);

	if (_config.persistent) {
		auto rv = apr_reslist_create(&_reslist, _config.nmin, _config.nkeep, _config.nmax,
				apr_time_from_sec(cfg.exptime), DbdModule_construct, DbdModule_destruct, this, _pool);
		if (rv == APR_SUCCESS) {
			_destroyed = false;
		}
	}
}

NS_SA_END
