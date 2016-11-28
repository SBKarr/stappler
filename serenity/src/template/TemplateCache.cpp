/*
 * TemplateCache.cpp
 *
 *  Created on: 28 нояб. 2016 г.
 *      Author: sbkarr
 */

#include "TemplateCache.h"
#include "SPFilesystem.h"

NS_SA_EXT_BEGIN(tpl)

FileRef::FileRef(apr::MemPool &&pool, const String &file) : _pool(std::move(pool)), _file(file) {
	_refCount.store(1);
}

void FileRef::retain(Request &req) {
	++ _refCount;
	req.addCleanup([this] {
		release();
	});
}

void FileRef::release() {
	if (_refCount.fetch_sub(1) == 0) {
		_pool.free();
	}
}

const File &FileRef::getFile() const {
	return _file;
}
time_t FileRef::getMtime() const {
	return _file.mtime();
}


Cache::Cache() : _pool(getCurrentPool()) { }

void Cache::update() {
	AllocStack::perform([&] {
		_mutex.lock();
		for (auto &it : _templates) {
			auto mtime = filesystem::mtime(it.first);
			if (mtime != it.second->getMtime()) {
				it.second->release();
				it.second = openTemplate(it.first);
			}
		}
		_mutex.unlock();
	});
}

void Cache::runTemplate(const String &ipath, Request &req, const RunCallback &cb) {
	auto path = filesystem::writablePath(ipath);
	if (auto tpl = acquireTemplate(path, req)) {
		auto h = req.getResponseHeaders();
		h.emplace("Cache-Control", "no-cache, no-store, must-revalidate");
		h.emplace("Pragma", "no-cache");
		h.emplace("Expires", "0");

		Exec exec;
		exec.setTemplateCallback([this] (const String &ipath, Exec &exec, Request &req) {
			auto path = filesystem::writablePath(ipath);
			if (auto tpl = acquireTemplate(path, req)) {
				tpl->getFile().run(exec, req);
			}
		});
		exec.setIncludeCallback([this] (const String &ipath, Exec &, Request &req) {
			auto path = filesystem::writablePath(ipath);
			if (filesystem::exists(path)) {
				auto f = filesystem::openForReading(path);
				io::read(f, io::Consumer(req));
			}
		});
		if (cb) {
			cb(exec, req);
		}
		tpl->getFile().run(exec, req);

	}
}

FileRef *Cache::acquireTemplate(const String &path, Request &req) {
	FileRef *tpl = nullptr;

	_mutex.lock();

	auto it = _templates.find(path);
	if (it != _templates.end()) {
		tpl = it->second;
	} else {
		tpl = openTemplate(path);
		AllocStack::perform([&] {
			_templates.emplace(String(path), tpl);
		}, _pool);
	}

	if (tpl) {
		tpl->retain(req);
	}
	_mutex.unlock();

	return tpl;
}

FileRef *Cache::openTemplate(const String &path) {
	apr::MemPool pool(_pool);
	return AllocStack::perform([&] {
		return new FileRef(std::move(pool), path);
	}, pool.pool());
}

NS_SA_EXT_END(tpl)
