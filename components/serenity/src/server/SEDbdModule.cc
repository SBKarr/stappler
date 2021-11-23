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

	db::sql::Driver::Handle rec;
	mem::perform([&] {
		rec = group->getDriver()->connect(*group->getParams());
	}, pool);

    *data_ptr = rec.get();
    return APR_SUCCESS;
}

static apr_status_t DbdModule_destruct(void *data, void *params, mem::pool_t *pool) {
	DbdModule *group = (DbdModule *)params;

	if (!group->isDestroyed()) {
		group->getDriver()->finish(db::sql::Driver::Handle(data));
	}

	return APR_SUCCESS;
}

DbdModule *DbdModule::create(mem::pool_t *root, Map<StringView, StringView> *params) {
	auto pool = mem::pool::create(root);
	DbdModule *m = nullptr;

	mem::perform([&] {
		db::sql::Driver *driver = nullptr;

		StringView driverName;
		Config cfg;
		for (auto &it : *params) {
			if (it.first == "nmin") {
				if (int v = StringView(it.second).readInteger(10).get(0)) {
					cfg.nmin = stappler::math::clamp(v, 1, config::getMaxDatabaseConnections());
				} else {
					std::cout << "[DbdModule] invalid value for nmin: " << it.second << "\n";
				}
			} else if (it.first == "nkeep") {
				if (int v = StringView(it.second).readInteger(10).get(0)) {
					cfg.nkeep = stappler::math::clamp(v, 1, config::getMaxDatabaseConnections());
				} else {
					std::cout << "[DbdModule] invalid value for nkeep: " << it.second << "\n";
				}
			} else if (it.first == "nmax") {
				if (int v = StringView(it.second).readInteger(10).get(0)) {
					cfg.nmax = stappler::math::clamp(v, 1, config::getMaxDatabaseConnections());
				} else {
					std::cout << "[DbdModule] invalid value for nmax: " << it.second << "\n";
				}
			} else if (it.first == "exptime") {
				if (auto v = StringView(it.second).readInteger(10).get(0)) {
					cfg.exptime = v;
				} else {
					std::cout << "[DbdModule] invalid value for exptime: " << it.second << "\n";
				}
			} else if (it.first == "persistent") {
				if (it.second == "1" || it.second == "yes") {
					cfg.persistent = true;
				} else if (it.second == "0" || it.second == "no") {
					cfg.persistent = false;
				} else {
					std::cout << "[DbdModule] invalid value for persistent: " << it.second << "\n";
				}
			} else if (it.first == "driver") {
				driverName = it.second;
				driver = Root::getInstance()->getDbDriver(it.second);
			}
		}

		if (driver) {
			m = new (pool) DbdModule(pool, cfg, driver, params);
		} else {
			std::cout << "[DbdModule] driver not found: " << driverName << "\n";
		}
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

db::sql::Driver::Handle DbdModule::openConnection(apr_pool_t *pool) {
	db::sql::Driver::Handle rec;

	if (!_config.persistent) {
		DbdModule_construct((void**) &rec, this, pool);
		return rec;
	}

	auto rv = apr_reslist_acquire(_reslist, (void**) &rec);
	if (rv != APR_SUCCESS) {
		return db::sql::Driver::Handle(nullptr);
	}

	if (!_dbDriver->isValid(rec)) {
		apr_reslist_invalidate(_reslist, rec.get());
		return db::sql::Driver::Handle(nullptr);
	}

	return rec;
}

void DbdModule::closeConnection(db::sql::Driver::Handle rec) {
	if (!_config.persistent) {
		_dbDriver->finish(rec);
	} else {
		apr_reslist_release(_reslist, rec.get());
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

DbdModule::DbdModule(mem::pool_t *pool, const Config &cfg, db::sql::Driver *driver, Map<StringView, StringView> *params)
: _pool(pool), _config(cfg), _dbParams(params), _dbDriver(driver) {
	auto handle = _dbDriver->connect(*_dbParams);
	if (handle.get()) {
		_dbDriver->init(handle, mem::Vector<mem::StringView>());
		_dbDriver->finish(handle);
	} else {
		std::cout << "[DbdModule] fail to initialize connection with driver " << _dbDriver->getDriverName() << "\n";
	}

	if (_config.persistent) {
		auto rv = apr_reslist_create(&_reslist, _config.nmin, _config.nkeep, _config.nmax,
				apr_time_from_sec(cfg.exptime), DbdModule_construct, DbdModule_destruct, this, _pool);
		if (rv == APR_SUCCESS) {
			_destroyed = false;
		}
	}

	mem::pool::cleanup_register(_pool, [this] {
		_destroyed = true;
	});
}

NS_SA_END
