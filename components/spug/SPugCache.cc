/**
 Copyright (c) 2018-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPugCache.h"
#include "SPugContext.h"
#include "SPugTemplate.h"

#include <sys/inotify.h>

NS_SP_EXT_BEGIN(pug)

Rc<FileRef> FileRef::read(memory::pool_t *p, FilePath path, Template::Options opts, const Callback<void(const StringView &)> &cb, int watch, int wId) {
	auto fpath = path.get();
	if (filesystem::exists(fpath)) {
		auto pool = memory::pool::create(p);

		memory::pool::context ctx(pool, memory::pool::Template);
		return Rc<FileRef>::alloc(pool, path, opts, cb, watch, wId);
	}

	return nullptr;
}

Rc<FileRef> FileRef::read(memory::pool_t *p, String && content, bool isTemplate, Template::Options opts, const Callback<void(const StringView &)> &cb) {
	auto pool = memory::pool::create(p);

	memory::pool::context ctx(pool, memory::pool::Template);
	return Rc<FileRef>::alloc(pool, move(content), isTemplate, opts, cb);
}

FileRef::FileRef(memory::pool_t *pool, const FilePath &path, Template::Options opts, const Callback<void(const StringView &)> &cb, int watch, int wId)
: _pool(pool), _opts(opts) {
	auto fpath = path.get();

	_mtime = filesystem::mtime(fpath);
	_content.resize(filesystem::size(fpath));
	filesystem::readIntoBuffer((uint8_t *)_content.data(), fpath);

	if (_content.size() > 0) {
		if (wId < 0 && watch >= 0) {
			_watch = inotify_add_watch(watch, SP_TERMINATED_DATA(fpath), IN_CLOSE_WRITE);
			if (_watch == -1 && errno == ENOSPC) {
				cb("inotify limit is reached: fall back to timed watcher");
			}
		} else {
			_watch = wId;
		}
		_valid = true;
	}
	if (_valid && (fpath.ends_with(".pug") || fpath.ends_with(".stl") || fpath.ends_with(".spug"))) {
		_template = Template::read(_pool, _content, opts, cb);
		if (!_template) {
			_valid = false;
		}
	}
}

FileRef::FileRef(memory::pool_t *pool, String &&src, bool isTemplate, Template::Options opts, const Callback<void(const StringView &)> &cb)
: _pool(pool), _content(move(src)), _opts(opts) {
	if (_content.size() > 0) {
		_valid = true;
	}
	if (isTemplate && _valid) {
		_template = Template::read(_pool, _content, opts, cb);
		if (!_template) {
			_valid = false;
		}
	}
}

FileRef::~FileRef() {
	if (_pool) {
		memory::pool::destroy(_pool);
	}
}

const String &FileRef::getContent() const {
	return _content;
}

const Template *FileRef::getTemplate() const {
	return _template;
}

int FileRef::getWatch() const {
	return _watch;
}

time_t FileRef::getMtime() const {
	return _mtime;
}

bool FileRef::isValid() const {
	return _valid;
}

const Template::Options &FileRef::getOpts() const {
	return _opts;
}

Cache::Cache(Template::Options opts, const Function<void(const StringView &)> &err)
: _pool(memory::pool::acquire()), _opts(opts), _errorCallback(err) {
	_inotify = inotify_init1(IN_NONBLOCK);
	if (_inotify != -1) {
		_inotifyAvailable = true;
	}
}

Cache::~Cache() {
	if (_inotify > 0) {
		for (auto &it : _templates) {
			auto fd = it.second->getWatch();
			if (fd >= 0) {
				inotify_rm_watch(_inotify, fd);
			}
		}

		close(_inotify);
	}
}

void Cache::update(int watch) {
	std::unique_lock<Mutex> lock(_mutex);
	auto it = _watches.find(watch);
	if (it != _watches.end()) {
		auto tIt = _templates.find(it->second);
		if (tIt != _templates.end()) {
			if (auto tpl = openTemplate(it->second, tIt->second->getWatch(), tIt->second->getOpts())) {
				tIt->second = tpl;
			}
		}
	}
}

void Cache::update(memory::pool_t *pool) {
	memory::pool::context ctx(pool);
	for (auto &it : _templates) {
		if (it.second->getMtime() != 0) {
			auto mtime = filesystem::mtime(it.first);
			if (mtime != it.second->getMtime()) {
				if (auto tpl = openTemplate(it.first, -1, it.second->getOpts())) {
					it.second = tpl;
				}
			}
		}
	}
}

int Cache::getNotify() const {
	return _inotify;
}

bool Cache::isNotifyAvailable() {
	std::unique_lock<Mutex> lock(_mutex);
	return _inotifyAvailable;
}

bool Cache::runTemplate(const StringView &ipath, const RunCallback &cb, std::ostream &out) {
	Rc<FileRef> tpl = acquireTemplate(ipath, true, _opts);
	if (!tpl) {
		tpl = acquireTemplate(filesystem::writablePath(ipath), false, _opts);
	}

	return runTemplate(tpl, ipath, cb, out);
}

bool Cache::runTemplate(const StringView &ipath, const RunCallback &cb, std::ostream &out, Template::Options opts) {
	Rc<FileRef> tpl = acquireTemplate(ipath, true, opts);
	if (!tpl) {
		tpl = acquireTemplate(filesystem::writablePath(ipath), false, opts);
	}

	return runTemplate(tpl, ipath, cb, out);
}

bool Cache::addFile(StringView path) {
	std::unique_lock<Mutex> lock(_mutex);
	auto it = _templates.find(path);
	if (it == _templates.end()) {
		memory::pool::context ctx(_pool);
		if (auto tpl = openTemplate(path, -1, _opts)) {
			auto it = _templates.emplace(path.pdup(_templates.get_allocator()), tpl).first;
			if (tpl->getWatch() >= 0) {
				_watches.emplace(tpl->getWatch(), it->first);
			}
			return true;
		}
	} else {
		onError(toString("Already added: '", path, "'"));
	}
	return false;
}

bool Cache::addContent(StringView key, String &&data) {
	std::unique_lock<Mutex> lock(_mutex);
	auto it = _templates.find(key);
	if (it == _templates.end()) {
		auto tpl = FileRef::read(_pool, move(data), false, _opts);
		_templates.emplace(key.pdup(_templates.get_allocator()), tpl);
		return true;
	} else {
		onError(toString("Already added: '", key, "'"));
	}
	return false;
}

bool Cache::addTemplate(StringView key, String &&data) {
	return addTemplate(key, move(data), _opts);
}

bool Cache::addTemplate(StringView key, String &&data, Template::Options opts) {
	std::unique_lock<Mutex> lock(_mutex);
	auto it = _templates.find(key);
	if (it == _templates.end()) {
		auto tpl = FileRef::read(_pool, move(data), true, opts, [&] (const StringView &err) {
			std::cout << key << ":\n";
			std::cout << err << "\n";
		});
		_templates.emplace(key.pdup(_templates.get_allocator()), tpl);
		return true;
	} else {
		onError(toString("Already added: '", key, "'"));
	}
	return false;
}

Rc<FileRef> Cache::acquireTemplate(StringView path, bool readOnly, const Template::Options &opts) {
	std::unique_lock<Mutex> lock(_mutex);
	auto it = _templates.find(path);
	if (it != _templates.end()) {
		return it->second;
	} else if (!readOnly) {
		if (auto tpl = openTemplate(path, -1, opts)) {
			auto it = _templates.emplace(path.pdup(_templates.get_allocator()), tpl).first;
			if (tpl->getWatch() >= 0) {
				_watches.emplace(tpl->getWatch(), it->first);
			}
			return tpl;
		}
	}
	return nullptr;
}

Rc<FileRef> Cache::openTemplate(StringView path, int wId, const Template::Options &opts) {
	auto ret = FileRef::read(_pool, FilePath(path), opts, [&] (const StringView &err) {
		std::cout << path << ":\n";
		std::cout << err << "\n";
	}, _inotify, wId);
	if (!ret) {
		onError(toString("File not found: ", path));
	}  else if (ret->isValid()) {
		return ret;
	}
	return nullptr;
}

bool Cache::runTemplate(Rc<FileRef> tpl, StringView ipath, const RunCallback &cb, std::ostream &out) {
	if (tpl) {
		if (auto t = tpl->getTemplate()) {
			auto iopts = tpl->getOpts();
			Context exec;
			exec.loadDefaults();
			exec.setIncludeCallback([this, iopts] (const StringView &path, Context &exec, std::ostream &out, const Template *) -> bool {
				Rc<FileRef> tpl = acquireTemplate(path, true, iopts);
				if (!tpl) {
					tpl = acquireTemplate(filesystem::writablePath(path), false, iopts);
				}

				if (!tpl) {
					return false;
				}

				bool ret = false;
				if (const Template *t = tpl->getTemplate()) {
					ret = t->run(exec, out);
				} else {
					out << tpl->getContent();
					ret = true;
				}

				return ret;
			});
			if (cb) {
				if (!cb(exec, *t)) {
					return false;
				}
			}
			return t->run(exec, out);
		} else {
			onError(toString("File '", ipath, "' is not executable"));
		}
	} else {
		onError(toString("No template '", ipath, "' found"));
	}
	return false;
}

void Cache::onError(const StringView &str) {
	if (str == "inotify limit is reached: fall back to timed watcher") {
		std::unique_lock<Mutex> lock(_mutex);
		_inotifyAvailable = false;
	}
	if (_errorCallback) {
		_errorCallback(str);
	} else {
		std::cout << str;
	}
}

NS_SP_EXT_END(pug)
