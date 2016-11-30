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

#include "SPDefine.h"
#include "SPDataSource.h"

NS_SP_EXT_BEGIN(data)

Source::Id Source::Self(Source::Id::max());

struct Source::Slice {
	Id::Type idx;
	size_t len;
	Source *cat;
	size_t offset;
	bool recieved;

	Slice(Id::Type idx, size_t len, Source *cat)
	: idx(idx), len(len), cat(cat), offset(0), recieved(false) { }
};

struct Source::SliceRequest : public Ref {
	std::vector<Slice> vec;
	BatchCallback cb;
	size_t ready = 0;
	size_t requests = 0;
	std::map<Id, data::Value> data;

	SliceRequest(const BatchCallback &cb) : cb(cb) { }

	size_t request(size_t off) {
		size_t ret = 0;

		requests = vec.size();
		for (auto &it : vec) {
			it.offset = off;
			off += it.len;
			auto ptr = &it;
			auto cat = it.cat;
			
			retain();
			cat->retain();
			cat->onSliceRequest([this, ptr, cat] (std::map<Id, data::Value> &val) {
				onSliceData(ptr, val);
				cat->release();
				release();
			}, it.idx, it.len);
			ret += it.len;
		}
		// this will destroy (*this), if direct data access available
		release(); // free or decrement ref-count
		return ret;
	}

	bool onSliceData(Slice *ptr, std::map<Id, data::Value> &val) {
		ptr->recieved = true;

		auto front = val.begin()->first;
		for(auto &it : val) {
			if (it.first != Self) {
				data.emplace(it.first + Id(ptr->offset) - front, std::move(it.second));
			} else {
				data.emplace(Id(ptr->offset), std::move(it.second));
			}
		}

		ready ++;
		requests --;

		if (ready >= vec.size() && requests == 0) {
			for (auto &it : vec) {
				if (!it.recieved) {
					return false;
				}
			}

			cb(data);
			return true;
		}

		return false;
	}
};

struct Source::BatchRequest {
	std::vector<Id> vec;
	BatchCallback cb;
	size_t requests = 0;
	Rc<Source> cat;
	std::map<Id, data::Value> map;

	static void request(const BatchCallback &cb, Id::Type first, size_t size, Source *cat, const DataSourceCallback &scb) {
		new BatchRequest(cb, first, size, cat, scb);
	}

	BatchRequest(const BatchCallback &cb, Id::Type first, size_t size, Source *cat, const DataSourceCallback &scb)
	: cb(cb), cat(cat) {
		for (auto i = first; i < first + size; i++) {
			vec.emplace_back(i);
		}

		requests += vec.size();
		for (auto &it : vec) {
			scb([this, it] (data::Value &val) {
				if (val.isArray()) {
					onData(it, val.getValue(0));
				} else {
					onData(it, val);
				}
			}, it);
		}
	}

	void onData(Id id, data::Value &val) {
		map.insert(std::make_pair(id, std::move(val)));
		requests --;

		if (requests == 0) {
			cb(map);
			delete this;
		}
	}
};

void Source::clear() {
	_subCats.clear();
	_count = _orphanCount;
	setDirty();
}

void Source::addSubcategry(Source *cat) {
	_subCats.pushBack(cat);
	_count += cat->getGlobalCount();
	setDirty();
}

Source::~Source() { }

Source * Source::getCategory(size_t n) {
	if (n < getSubcatCount()) {
		return _subCats.at(n);
	}
	return nullptr;
}

size_t Source::getCount(uint32_t l, bool subcats) const {
	auto c = _orphanCount + ((subcats)?_subCats.size():0);
	if (l > 0) {
		for (auto cat : _subCats) {
			c += cat->getCount(l - 1, subcats);
		}
	}
	return c;
}

size_t Source::getSubcatCount() const {
	return _subCats.size();
}
size_t Source::getItemsCount() const {
	return _orphanCount;
}
size_t Source::getGlobalCount() const {
	return _count;
}

Source::Id Source::getId() const {
	return _categoryId;
}

void Source::setSubCategories(cocos2d::Vector<Source *> &&vec) {
	_subCats = std::move(vec);
	setDirty();
}
void Source::setSubCategories(const cocos2d::Vector<Source *> &vec) {
	_subCats = vec;
	setDirty();
}
const cocos2d::Vector<Source *> &Source::getSubCategories() const {
	return _subCats;
}

void Source::setChildsCount(size_t count) {
	_count -= _orphanCount;
	_orphanCount = count;
	_count += _orphanCount;
	setDirty();
}

size_t Source::getChildsCount() const {
	return _orphanCount;
}

void Source::setData(const data::Value &val) {
	_data = val;
}

void Source::setData(data::Value &&val) {
	_data = std::move(val);
}

const data::Value &Source::getData() const {
	return _data;
}

void Source::setDirty() {
	Subscription::setDirty();
}

void Source::setCategoryBounds(Id & first, size_t & count, uint32_t l, bool subcats) {
	// first should be 0 or bound value, that <= first
	if (l == 0 || _subCats.size() == 0) {
		first = Id(0);
		count = getCount(l, subcats);
		return;
	}

	size_t lowerBound = 0, subcat = 0, offset = 0;
	do {
		lowerBound += offset;
		offset = _subCats.at(subcat)->getCount(l - 1, subcats);
		subcat ++;
	} while(subcat < (size_t)_subCats.size() && lowerBound + offset <= (size_t)first.get());

	// check if we should skip last subcategory
	if (lowerBound + offset <= first.get()) {
		lowerBound += offset;
	}

	offset = size_t(first.get()) - lowerBound;
	first = Id(lowerBound);
	count += offset; // increment size to match new bound

	size_t upperBound = getCount(l, subcats);
	if (upperBound - _orphanCount >= lowerBound + count) {
		upperBound -= _orphanCount;
	}

	offset = 0;
	subcat = _subCats.size();
	while (subcat > 0 && upperBound - offset >= lowerBound + count) {
		upperBound -= offset;
		offset = _subCats.at(subcat - 1)->getCount(l - 1, subcats);
		subcat --;
	}

	count = upperBound - lowerBound;
}

bool Source::getItemData(const DataCallback &cb, Id index) {
	if (index.get() >= _orphanCount && index != Self) {
		return false;
	}

	if (index == Self && _data) {
		cb(_data);
	}

	_sourceCallback(cb, index);
	return true;
}

bool Source::getItemData(const DataCallback &cb, Id n, uint32_t l, bool subcats) {
	if (l > 0) {
		for (auto &cat : _subCats) {
			if (subcats) {
				if (n.empty()) {
					return getItemData(cb, Self);
				} else {
					n --;
				}
			}
			auto c = Id(cat->getCount(l - 1, subcats));
			if (n < c) {
				return cat->getItemData(cb, n, l - 1, subcats);
			} else {
				n -= c;
			}
		}
	}

	if (!subcats) {
		return getItemData(cb, Id(n));
	} else {
		if (!_subCats.empty() && n < Id(_subCats.size())) {
			return _subCats.at(size_t(n.get()));
		}

		return getItemData(cb, n - Id(_subCats.size()));
	}
}

size_t Source::getSliceData(const BatchCallback &cb, Id first, size_t count, uint32_t l, bool subcats) {
	SliceRequest *req = new SliceRequest(cb);

	size_t f = size_t(first.get());
	onSlice(req->vec, f, count, l, subcats);

	if (!req->vec.empty()) {
		return req->request(size_t(first.get()));
	} else {
		delete req;
		return 0;
	}
}

std::pair<Source *, bool> Source::getItemCategory(Id n, uint32_t l, bool subcats) {
	if (l > 0) {
		for (auto &cat : _subCats) {
			if (subcats) {
				if (n.empty()) {
					return std::make_pair(cat, true);
				} else {
					n --;
				}
			}
			auto c = cat->getCount(l - 1, subcats);
			if (n.get() < c) {
				return cat->getItemCategory(n, l - 1, subcats);
			} else {
				n -= Id(c);
			}
		}
	}

	if (!subcats) {
		return std::make_pair(this, false);
	} else {
		if (!_subCats.empty() && n.get() < (size_t)_subCats.size()) {
			return std::make_pair(_subCats.at(size_t(n.get())), true);
		}

		return std::make_pair(this, false);
	}
}

void Source::onSlice(std::vector<Slice> &vec, size_t &first, size_t &count, uint32_t l, bool subcats) {
	if (l > 0) {
		for (auto it = _subCats.begin(); it != _subCats.end(); it ++) {
			if (first > 0) {
				if (subcats) {
					first --;
				}

				auto sCount = (*it)->getCount(l - 1, subcats);
				if (sCount <= first) {
					first -= sCount;
				} else {
					(*it)->onSlice(vec, first, count, l - 1, subcats);
				}
			} else if (count > 0) {
				if (subcats) {
					vec.push_back(Slice(Self.get(), 1, *it));
					count -= 1;
				}

				if (count > 0) {
					(*it)->onSlice(vec, first, count, l - 1, subcats);
				}
			}
		}
	}

	if (count > 0 && first < _orphanCount) {
		auto c = MIN(count, _orphanCount - first);
		vec.push_back(Slice(first, c, this));

		first = 0;
		count -= c;
	} else if (first >= _orphanCount) {
		first -= _orphanCount;
	}
}

void Source::onSliceRequest(const BatchCallback &cb, Id::Type first, size_t size) {
	if (first == Self.get()) {
		if (!_data) {
			_sourceCallback([this, cb] (data::Value &val) {
				std::map<Id, data::Value> map;
				if (val.isArray()) {
					map.insert(std::make_pair(Self, std::move(val.getValue(0))));
				} else {
					map.insert(std::make_pair(Self, std::move(val)));
				}
				cb(map);
			}, Self);
		} else {
			std::map<Id, data::Value> map;
			map.insert(std::make_pair(Self, _data));
			cb(map);
		}
	} else {
		if (!_batchCallback) {
			BatchRequest::request(cb, first, size, this, _sourceCallback);
		} else {
			_batchCallback(cb, first, size);
		}
	}
}

bool Source::init() {
	return true;
}
bool Source::initValue() {
	return true;
}
bool Source::initValue(const DataSourceCallback &cb) {
	_sourceCallback = cb;
	return true;
}
bool Source::initValue(const BatchSourceCallback &cb) {
	_batchCallback = cb;
	return true;
}
bool Source::initValue(const Id &id) {
	_categoryId = id;
	return true;
}
bool Source::initValue(const ChildsCount &count) {
	_orphanCount = count.get();
	return true;
}
bool Source::initValue(const data::Value &val) {
	_data = val;
	return true;
}
bool Source::initValue(data::Value &&val) {
	_data = std::move(val);
	return true;
}

NS_SP_EXT_END(data)
