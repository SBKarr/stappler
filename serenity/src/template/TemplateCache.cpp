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
	apr::pool::perform([&] {
		_mutex.lock();
		for (auto &it : _templates) {
			if (it.second->getMtime() != 0) {
				auto mtime = filesystem::mtime(it.first);
				if (mtime != it.second->getMtime()) {
					it.second->release();
					it.second = openTemplate(it.first);
				}
			}
		}
		_mutex.unlock();
	});
}

void Cache::runTemplate(const String &ipath, Request &req, const RunCallback &cb) {
	String path = StringView(ipath).is("virtual://") ? ipath : filesystem::writablePath(ipath);
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
		apr::pool::perform([&] {
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
	return apr::pool::perform([&] {
		return new FileRef(std::move(pool), path);
	}, pool.pool());
}

NS_SA_EXT_END(tpl)
