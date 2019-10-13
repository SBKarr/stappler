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

NS_SP_EXT_BEGIN(pug)

Rc<FileRef> FileRef::read(const FilePath &path, Template::Options opts, const Function<void(const StringView &)> &cb) {
	auto pool = memory::pool::create(memory::pool::acquire());
	memory::pool::push(pool, memory::pool::Template);

	auto ret = Rc<FileRef>::alloc(pool, path, opts, cb);

	memory::pool::pop();

	return ret->getMtime() > 0 ? ret : nullptr;
}

Rc<FileRef> FileRef::read(String && content, bool isTemplate, Template::Options opts, const Function<void(const StringView &)> &cb) {
	auto pool = memory::pool::create(memory::pool::acquire());
	memory::pool::push(pool, memory::pool::Template);

	auto ret = Rc<FileRef>::alloc(pool, move(content), isTemplate, opts, cb);

	memory::pool::pop();

	return ret;
}

FileRef::FileRef(memory::pool_t *pool, const FilePath &path, Template::Options opts, const Function<void(const StringView &)> &cb)
: _pool(pool) {
	auto fpath = path.get();
	_mtime = filesystem::mtime(fpath);

	StringStream readStream;
	readStream.reserve(filesystem::size(fpath));
	filesystem::readFile(readStream, fpath);

	_content = readStream.str();
	if (fpath.ends_with(".pug") || fpath.ends_with(".stl") || fpath.ends_with(".spug")) {
		_template = Template::read(_pool, _content, opts, cb);
		if (!_template) {
			_mtime = 0;
		}
	}
}

FileRef::FileRef(memory::pool_t *pool, String &&src, bool isTemplate, Template::Options opts, const Function<void(const StringView &)> &cb)
: _pool(pool), _content(move(src)) {
	if (isTemplate) {
		_template = Template::read(_pool, _content, opts, cb);
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

time_t FileRef::getMtime() const {
	return _mtime;
}


Cache::Cache(Template::Options opts, const Function<void(const StringView &)> &err)
: _pool(memory::pool::acquire()), _opts(opts), _errorCallback(err) { }

template <typename Callback>
static inline auto perform(const Callback &cb) {
	struct Context {
		Context() {
			pool = memory::pool::create(memory::pool::acquire());
		}
		~Context() {
			memory::pool::destroy(pool);
		}

		memory::pool_t *pool = nullptr;
	} holder;
	return cb();
}

void Cache::update(memory::pool_t *) {
	_mutex.lock();
	for (auto &it : _templates) {
		if (it.second->getMtime() != 0) {
			auto mtime = filesystem::mtime(it.first);
			if (mtime != it.second->getMtime()) {
				if (auto tpl = openTemplate(it.first)) {
					it.second = tpl;
				}
			}
		}
	}
	_mutex.unlock();
}

bool Cache::runTemplate(const StringView &ipath, const RunCallback &cb, std::ostream &out) {
	Rc<FileRef> tpl = acquireTemplate(ipath, true);
	if (!tpl) {
		tpl = acquireTemplate(filesystem::writablePath(ipath), false);
	}

	if (tpl) {
		if (auto t = tpl->getTemplate()) {
			Context exec;
			exec.loadDefaults();
			exec.setIncludeCallback([this] (const StringView &path, Context &exec, std::ostream &out, const Template *) -> bool {
				Rc<FileRef> tpl = acquireTemplate(path, true);
				if (!tpl) {
					tpl = acquireTemplate(filesystem::writablePath(path), false);
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

void Cache::addFile(const StringView &path) {
	if (auto tpl = openTemplate(path)) {
		memory::pool::push(_pool);
		_templates.emplace(String(path.data(), path.size()), tpl);
		memory::pool::pop();
	}
}

void Cache::addContent(const StringView &key, String &&data) {
	memory::pool::push(_pool);
	auto tpl = FileRef::read(move(data), false, _opts);
	_templates.emplace(String(key.data(), key.size()), tpl);
	memory::pool::pop();
}

void Cache::addTemplate(const StringView &key, String &&data) {
	memory::pool::push(_pool);
	auto tpl = FileRef::read(move(data), true, _opts);
	_templates.emplace(String(key.data(), key.size()), tpl);
	memory::pool::pop();
}

Rc<FileRef> Cache::acquireTemplate(const StringView &path, bool readOnly) {
	Rc<FileRef> tpl;

	_mutex.lock();
	auto it = _templates.find(path);
	if (it != _templates.end()) {
		tpl = it->second;
	} else if (!readOnly) {
		tpl = openTemplate(path);
		if (tpl) {
			memory::pool::push(_pool);
			_templates.emplace(String(path.data(), path.size()), tpl);
			memory::pool::pop();
		}
	}
	_mutex.unlock();

	return tpl;
}

Rc<FileRef> Cache::openTemplate(const StringView &path) {
	memory::pool::push(_pool);
	auto ret = FileRef::read(FilePath(path), _opts, [&] (const StringView &err) {
		std::cout << path << ":\n";
		std::cout << err << "\n";
	});
	memory::pool::pop();
	if (!ret) {
		onError(toString("File not found: ", path));
	}
	return ret;
}

void Cache::onError(const StringView &str) {
	if (_errorCallback) {
		_errorCallback(str);
	} else {
		std::cout << str;
	}
}

NS_SP_EXT_END(pug)
