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

#include "SPDrawCache.h"

NS_SP_EXT_BEGIN(draw)

static Cache s_cache;

Cache *Cache::getInstance() {
	return &s_cache;
}

Rc<layout::Image> Cache::addImage(const StringView &path, bool replace) {
	if (!replace) {
		if (auto ret = getImage(path)) {
			return ret;
		}
	}
	if (auto image = Rc<layout::Image>::create(FilePath(path))) {
		return addImage(path, image.get());
	}
	return nullptr;
}

Rc<layout::Image> Cache::addImage(const StringView &key, const Bytes &data, bool replace) {
	if (!replace) {
		if (auto ret = getImage(key)) {
			return ret;
		}
	}
	if (auto image = Rc<layout::Image>::create(data)) {
		return addImage(key, image.get());
	}
	return nullptr;
}

Rc<layout::Image> Cache::addImage(const StringView &key, layout::Image *img) {
	Rc<layout::Image> ret = nullptr;
	_mutex.lock();

	auto it = _images.find(key);
	if (it == _images.end()) {
		it = _images.emplace(key.str(), img).first;
	} else {
		it->second = img;
	}

	ret = it->second;

	_mutex.unlock();
	return ret;
}

Rc<layout::Image> Cache::getImage(const StringView &key) {
	Rc<layout::Image> ret = nullptr;
	_mutex.lock();

	auto it = _images.find(key);
	if (it != _images.end()) {
		ret = it->second;
	}

	_mutex.unlock();
	return ret;
}

bool Cache::hasImage(const StringView &key) {
	bool ret = false;
	_mutex.lock();

	auto it = _images.find(key);
	if (it != _images.end()) {
		ret = true;
	}

	_mutex.unlock();
	return ret;
}

NS_SP_EXT_END(draw)
