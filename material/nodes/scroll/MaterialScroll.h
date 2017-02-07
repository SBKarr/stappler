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

#ifndef LIBS_MATERIAL_NODES_SCROLL_MATERIALSCROLL_H_
#define LIBS_MATERIAL_NODES_SCROLL_MATERIALSCROLL_H_

#include "SPScrollView.h"
#include "MaterialNode.h"
#include "SPDataSource.h"
#include "SPDataListener.h"
#include "SPScrollController.h"

NS_MD_BEGIN

class Scroll : public ScrollView {
public:
	enum class Request {
		Reset,
		Update,
		Front,
		Back,
	};

	class Item;
	class Handler;
	class Loader;

	using Source = data::Source;

	using ItemMap = std::map<Source::Id, Rc<Item>>;
	using DataMap = std::map<Source::Id, data::Value>;

	using HandlerCallback = std::function<Handler * (Scroll *)>;
	using ItemCallback = std::function<MaterialNode *(Item *)>;
	using LoaderCallback = std::function<Loader *(Request, const std::function<void()> &, const Color &)>;

	virtual ~Scroll() { }
	virtual bool init(Source *dataCategory = nullptr, Scroll::Layout = Scroll::Vertical);

    virtual void onContentSizeDirty() override;
	virtual void reset();

	virtual data::Value save() const;
	virtual void load(const data::Value &);

public: // properties

	virtual const ItemMap &getItems() const;

	virtual void setSource(Source *);
	virtual Source *getSource() const;

	virtual void setLookupLevel(uint32_t level);
	virtual uint32_t getLookupLevel() const;

	virtual void setItemsForSubcats(bool value);
	virtual bool isItemsForSubcat() const;

	virtual void setCategoryBounds(bool value);
	virtual bool hasCategoryBounds() const;

	virtual void setLoaderSize(float);
	virtual float getLoaderSize() const;

	virtual void setMinLoadTime(TimeInterval time);
	virtual TimeInterval getMinLoadTime() const;

	virtual void setMaxSize(size_t max);
	virtual size_t getMaxSize() const;

	virtual void setLoaderColor(const Color &);
	virtual const Color &getLoaderColor() const;

	virtual void setOriginId(Source::Id);
	virtual Source::Id getOriginId() const;

public:
	// if you need to share some resources with slice loader thread, use this callback
	// resources will be retained and released by handler in main thread
	virtual void setHandlerCallback(const HandlerCallback &);

	// this callback will be called when scroll tries to load next item in slice
	virtual void setItemCallback(const ItemCallback &);

	// you can use custom loader with this callback
	virtual void setLoaderCallback(const LoaderCallback &);

protected:
	virtual void onSourceDirty();

	virtual size_t getMaxId() const;
	virtual std::pair<Source *, bool> getSourceCategory(int64_t id);

protected:
	virtual bool requestSlice(Source::Id, size_t, Request);

	virtual bool updateSlice();
	virtual bool resetSlice();
	virtual bool downloadFrontSlice(size_t = 0);
	virtual bool downloadBackSlice(size_t = 0);

	virtual void onSliceData(DataMap &, Time, Request);
	//virtual void onSliceThreadData(SliceDataMap &, SliceItemMap &, SliceDataCallback &, const SliceHandler &h);
	virtual void onSliceItems(ItemMap &&, Time, Request);

	virtual void updateItems();
	virtual Handler *onHandler();
	virtual MaterialNode *onItemRequest(const ScrollController::Item &, Source::Id);
	virtual Loader *onLoaderRequest(Request type);

	virtual void onOverscroll(float delta) override;
	virtual void updateIndicatorPosition() override;

	uint32_t _categoryLookupLevel = 0;
	bool _itemsForSubcats = false;
	bool _categoryDirty = true;
	bool _useCategoryBounds = false;

	data::Listener<Source> _listener;

	Rc<cocos2d::Ref> _dataRef;
	HandlerCallback _handlerCallback = nullptr;
	ItemCallback _itemCallback = nullptr;
	LoaderCallback _loaderCallback = nullptr;

	Source::Id _currentSliceStart = Source::Id(0);
	size_t _currentSliceLen = 0;

	Source::Id _sliceOrigin = Source::Id(0);

	size_t _sliceMax = 24;
	size_t _sliceSize = 0;
	size_t _slicesCount = 0;
	size_t _itemsCount = 0;

	ItemMap _items;

	Time _invalidateAfter = 0;
	Color _loaderColor = Color::Grey_500;

	float _savedSize = nan();
	float _loaderSize = 48.0f;
	TimeInterval _minLoadTime = TimeInterval::milliseconds(600);
};

class Scroll::Loader : public cocos2d::Node {
public:
	virtual ~Loader() { }
	virtual bool init(const Function<void()> &, const Color &);
	virtual void onContentSizeDirty() override;

	virtual void onEnter() override;
	virtual void onExit() override;

protected:
	IconSprite *_icon = nullptr;
	Function<void()> _callback = nullptr;
};

class Scroll::Item : public Ref {
public:
	virtual ~Item() { }
	virtual bool init(data::Value &&val, const Vec2 &, const Size &size);

	virtual const data::Value &getData() const;
	virtual const Size &getContentSize() const;
	virtual const Vec2 &getPosition() const;

	virtual void setPosition(const Vec2 &);
	virtual void setContentSize(const Size &);

	virtual void setId(uint64_t);
	virtual uint64_t getId() const;

	virtual void setControllerId(size_t);
	virtual size_t getControllerId() const;

protected:
	uint64_t _id = 0;
	Size _size;
	Vec2 _position;
	data::Value _data;
	size_t _controllerId = 0;
};

class Scroll::Handler : public Ref {
public:
	using Request = Scroll::Request;
	using DataMap = Scroll::DataMap;
	using ItemMap = Scroll::ItemMap;
	using Layout = Scroll::Layout;

	virtual ~Handler() { }
	virtual bool init(Scroll *);

	virtual const Size &getContentSize() const;
	virtual Scroll * getScroll() const;

	virtual ItemMap run(Request t, DataMap &&) = 0; // on worker thread

protected:
	Size _size;
	Layout _layout;
	Rc<Scroll> _scroll;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_SCROLL_MATERIALSCROLL_H_ */
