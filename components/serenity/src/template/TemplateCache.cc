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

#if LINUX
#include <sys/inotify.h>
#endif

NS_SA_EXT_BEGIN(tpl)

FileRef::FileRef(mem::pool_t *pool, const String &file, int watch, int wId) : _pool(pool), _file(file) {
	_refCount.store(1);

	if (file.find("://") == String::npos) {
		if (wId < 0 && watch >= 0) {
	#if LINUX
			_watch = inotify_add_watch(watch, file.data(), IN_CLOSE_WRITE);
			if (_watch == -1 && errno == ENOSPC) {
				std::cout << "inotify limit is reached: fall back to timed watcher\n";
			}
	#endif
		} else {
			_watch = wId;
		}
	}
}

void FileRef::retain(Request &req) {
	++ _refCount;
	req.addCleanup([this] {
		release();
	});
}

void FileRef::release() {
	if (_refCount.fetch_sub(1) == 0) {
		mem::pool::destroy(_pool);
	}
}

int FileRef::regenerate(int notify) {
	if (_watch >= 0) {
		inotify_rm_watch(notify, _watch);
		_watch = inotify_add_watch(notify, _file.file().data(), IN_CLOSE_WRITE);
		return _watch;
	}
	return 0;
}

const File &FileRef::getFile() const {
	return _file;
}
time_t FileRef::getMtime() const {
	return _file.mtime();
}


Cache::Cache() : _pool(getCurrentPool()) {
#if LINUX
	_inotify = inotify_init1(IN_NONBLOCK);
#endif
	if (_inotify != -1) {
		_inotifyAvailable = true;
	}
}

Cache::~Cache() {
	if (_inotify > 0) {
		for (auto &it : _templates) {
#if LINUX
			auto fd = it.second->getWatch();
			if (fd >= 0) {
				inotify_rm_watch(_inotify, fd);
			}
#endif
		}

		close(_inotify);
	}
}

void Cache::update(int watch, bool regenerate) {
	std::unique_lock<Mutex> lock(_mutex);
	auto it = _watches.find(watch);
	if (it != _watches.end()) {
		auto tIt = _templates.find(it->second);
		if (tIt != _templates.end()) {
			if (regenerate) {
				_watches.erase(it);
				if (auto tpl = openTemplate(tIt->first, -1)) {
					tIt->second = tpl;
					watch = tIt->second->getWatch();
					if (watch < 0) {
						_inotifyAvailable = false;
					} else {
						_watches.emplace(watch, tIt->first);
					}
				}
			} else {
				if (auto tpl = openTemplate(tIt->first, tIt->second->getWatch())) {
					tIt->second = tpl;
				}
			}
		}
	}
}

void Cache::update(apr_pool_t *) {
	_mutex.lock();
	for (auto &it : _templates) {
		if (it.second->getMtime() != 0) {
			auto mtime = filesystem::mtime(it.first);
			if (mtime != it.second->getMtime()) {
				it.second->release();
				it.second = openTemplate(it.first, -1);
			}
		}
	}
	_mutex.unlock();
}

bool Cache::isNotifyAvailable() {
	std::unique_lock<Mutex> lock(_mutex);
	return _inotifyAvailable;
}

void Cache::runTemplate(const String &ipath, Request &req, const RunCallback &cb) {
	String path = StringView(ipath).is("virtual://") ? ipath : filesystem::writablePath(ipath);
	if (auto tpl = acquireTemplate(path, req)) {
		auto h = req.getResponseHeaders();
		auto lm = h.at("Last-Modified");
		auto etag = h.at("ETag");

		h.emplace("Content-Type", "text/html; charset=UTF-8");
		if (lm.empty() && etag.empty()) {
			h.emplace("Cache-Control", "no-cache, no-store, must-revalidate");
			h.emplace("Pragma", "no-cache");
			h.emplace("Expires", "0");
		}

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
		tpl = openTemplate(path, -1);
		apr::pool::perform([&] {
			_templates.emplace(String(path), tpl);
			if (tpl->getWatch() >= 0) {
				_watches.emplace(tpl->getWatch(), path);
			}
		}, _pool);
	}

	if (tpl) {
		tpl->retain(req);
	}
	_mutex.unlock();

	return tpl;
}

FileRef *Cache::openTemplate(const String &path, int wId) {
	auto pool = mem::pool::create(_pool);
	return apr::pool::perform([&] {
		return new FileRef(std::move(pool), path, _inotify, wId);
	}, pool);
}

NS_SA_EXT_END(tpl)
