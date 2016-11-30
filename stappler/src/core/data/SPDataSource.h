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

#ifndef LIBS_STAPPLER_CORE_DATA_SPDATASOURCE_H_
#define LIBS_STAPPLER_CORE_DATA_SPDATASOURCE_H_

#include "SPData.h"
#include "SPDataSubscription.h"
#include "base/CCVector.h"

NS_SP_EXT_BEGIN(data)

class Source : public Subscription {
public:
	using ChildsCount = ValueWrapper<size_t, class ChildsCountClassFlag>;

	static Id Self;

	using BatchCallback = std::function<void(std::map<Id, data::Value> &)>;
	using BatchSourceCallback = std::function<void(const BatchCallback &, Id::Type first, size_t size)>;

	using DataCallback = std::function<void(data::Value &)>;
	using DataSourceCallback = std::function<void(const DataCallback &, Id)>;

	template <class T, class... Args>
	bool init(const T &t, Args&&... args) {
		auto ret = initValue(t);
		if (ret) {
			return init(args...);
		}
		return false;
	}
	bool init();

	virtual ~Source();

	Source * getCategory(size_t n);

	size_t getCount(uint32_t l = 0, bool subcats = false) const;
	size_t getSubcatCount() const; // number of subcats
	size_t getItemsCount() const; // number of data items (not subcats)
	size_t getGlobalCount() const; // number of all data items in cat and subcats

	void setCategoryBounds(Id & first, size_t & count, uint32_t l = 0, bool subcats = false);

	bool getItemData(const DataCallback &, Id index);
	bool getItemData(const DataCallback &, Id index, uint32_t l, bool subcats = false);
	size_t getSliceData(const BatchCallback &, Id first, size_t count, uint32_t l = 0, bool subcats = false);

	std::pair<Source *, bool> getItemCategory(Id itemId, uint32_t l, bool subcats = false);

	Id getId() const;

	void setSubCategories(const cocos2d::Vector<Source *> &);
	void setSubCategories(cocos2d::Vector<Source *> &&);
	const cocos2d::Vector<Source *> &getSubCategories() const;

	void setChildsCount(size_t count);
	size_t getChildsCount() const;

	void setData(const data::Value &);
	void setData(data::Value &&);
	const data::Value &getData() const;

	void clear();
	void addSubcategry(Source *);

	void setDirty();

protected:
	struct BatchRequest;
	struct SliceRequest;
	struct Slice;

	void onSlice(std::vector<Slice> &, size_t &first, size_t &count, uint32_t l, bool subcats);

	virtual bool initValue();
	virtual bool initValue(const DataSourceCallback &);
	virtual bool initValue(const BatchSourceCallback &);
	virtual bool initValue(const Id &);
	virtual bool initValue(const ChildsCount &);
	virtual bool initValue(const data::Value &);
	virtual bool initValue(data::Value &&);

	virtual void onSliceRequest(const BatchCallback &, Id::Type first, size_t size);

	cocos2d::Vector<Source *> _subCats;

	Id _categoryId;
	size_t _count = 0;
	size_t _orphanCount = 0;
	Value _data;

	DataSourceCallback _sourceCallback = nullptr;
	BatchSourceCallback _batchCallback = nullptr;
};

NS_SP_EXT_END(data)

#endif /* LIBS_STAPPLER_CORE_DATA_SPDATASOURCE_H_ */
