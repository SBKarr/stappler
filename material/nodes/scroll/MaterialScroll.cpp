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

#include "Material.h"
#include "MaterialIconSprite.h"
#include "MaterialScroll.h"
#include "MaterialResourceManager.h"
#include "SPScrollController.h"
#include "SPDataListener.h"
#include "SPThread.h"
#include "SPRoundedSprite.h"

#include <unistd.h>

NS_MD_BEGIN

bool Scroll::Loader::init(const std::function<void()> &cb, const Color &c) {
	if (!cocos2d::Node::init()) {
		return false;
	}

	_callback = cb;

	setCascadeColorEnabled(true);
	setCascadeOpacityEnabled(true);

	setColor(c);

	_icon = construct<IconSprite>(IconName::Dynamic_Loader);
	_icon->setContentSize(cocos2d::Size(36.0f, 36.0f));
	_icon->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
	addChild(_icon);

	return true;
}

void Scroll::Loader::onContentSizeDirty() {
	cocos2d::Node::onContentSizeDirty();
	_icon->setPosition(cocos2d::Vec2(_contentSize.width / 2, _contentSize.height / 2));
}

void Scroll::Loader::onEnter() {
	cocos2d::Node::onEnter();
	_icon->animate();
	if (_callback) {
		_callback();
	}
}
void Scroll::Loader::onExit() {
	stopAllActions();
	cocos2d::Node::onExit();
	_icon->stopAllActions();
}


bool Scroll::Item::init(data::Value &&val, const Vec2 &pos, const Size &size) {
	_data = std::move(val);
	_position = pos;
	_size = size;
	return true;
}

const data::Value &Scroll::Item::getData() const {
	return _data;
}

const Size &Scroll::Item::getContentSize() const {
	return _size;
}

const Vec2 &Scroll::Item::getPosition() const {
	return _position;
}

void Scroll::Item::setPosition(const Vec2 &pos) {
	_position = pos;
}
void Scroll::Item::setContentSize(const Size &size) {
	_size = size;
}

void Scroll::Item::setId(uint64_t id) {
	_id = id;
}
uint64_t Scroll::Item::getId() const {
	return _id;
}

void Scroll::Item::setControllerId(size_t value) {
	_controllerId = value;
}
size_t Scroll::Item::getControllerId() const {
	return _controllerId;
}

bool Scroll::Handler::init(Scroll *s) {
	_size = s->getContentSize();
	_layout = s->getLayout();
	_scroll = s;

	return true;
}

const Size &Scroll::Handler::getContentSize() const {
	return _size;
}

Scroll * Scroll::Handler::getScroll() const {
	return _scroll;
}

bool Scroll::init(Source *source, stappler::ScrollViewBase::Layout l) {
	if (!ScrollView::init(l)) {
		return false;
	}

	setScrollMaxVelocity(5000.0f);

	_listener.setCallback(std::bind(&Scroll::onSourceDirty, this));
	_listener = source;

	auto c = construct<ScrollController>();
	setController(c);

	return true;
}

void Scroll::onContentSizeDirty() {
	stappler::ScrollView::onContentSizeDirty();
	if ((isVertical() && _savedSize != _contentSize.width) || (!isVertical() && _savedSize != _contentSize.height)) {
		_savedSize = isVertical()?_contentSize.width:_contentSize.height;
		onSourceDirty();
	}
}

void Scroll::reset() {
	_controller->clear();

	auto min = getScrollMinPosition();
	if (!isnan(min)) {
		setScrollPosition(min);
	} else {
		setScrollPosition(0.0f - (isVertical()?_paddingGlobal.top:_paddingGlobal.left));
	}
}

data::Value Scroll::save() const {
	data::Value ret;
	ret.setDouble(getScrollRelativePosition(), "value");
	ret.setInteger(_currentSliceStart.get(), "start");
	ret.setInteger(_currentSliceLen, "len");
	return ret;
}

void Scroll::load(const data::Value &d) {
	if (d.isDictionary()) {
		_savedRelativePosition = d.getDouble("value");
		_currentSliceStart = Source::Id(d.getInteger("start"));
		_currentSliceLen = (size_t)d.getInteger("len");
		updateSlice();
	}
}

const Scroll::ItemMap &Scroll::getItems() const {
	return _items;
}

void Scroll::setSource(Source *c) {
	if (c != _listener) {
		_listener = c;
		_categoryDirty = true;

		_invalidateAfter = stappler::Time::now();

		if (!_contentSize.equals(Size::ZERO)) {
			_controller->clear();

			if (isVertical()) {
				_controller->addItem(std::bind(&Scroll::onLoaderRequest, this, Request::Reset), _loaderSize, 0.0f);
			} else {
				auto size = _contentSize.width - _paddingGlobal.left - _loaderSize;
				_controller->addItem(std::bind(&Scroll::onLoaderRequest, this, Request::Reset), MAX(_loaderSize, size), 0.0f);
			}

			setScrollPosition(0.0f);
		}
	}
}

Scroll::Source *Scroll::getSource() const {
	return _listener;
}

void Scroll::setLookupLevel(uint32_t level) {
	_categoryLookupLevel = level;
	_categoryDirty = true;
	_listener.setDirty();
}
uint32_t Scroll::getLookupLevel() const {
	return _categoryLookupLevel;
}
void Scroll::setItemsForSubcats(bool value) {
	_itemsForSubcats = value;
	_categoryDirty = true;
	_listener.setDirty();
}
bool Scroll::isItemsForSubcat() const {
	return _itemsForSubcats;
}

void Scroll::setCategoryBounds(bool value) {
	if (_useCategoryBounds != value) {
		_useCategoryBounds = value;
		_categoryDirty = true;
	}
}

bool Scroll::hasCategoryBounds() const {
	return _useCategoryBounds;
}

void Scroll::setMaxSize(size_t max) {
	_sliceMax = max;
	_categoryDirty = true;
	_listener.setDirty();
}
size_t Scroll::getMaxSize() const {
	return _sliceMax;
}

void Scroll::setOriginId(Source::Id id) {
	_sliceOrigin = id;
}
Scroll::Source::Id Scroll::getOriginId() const {
	return _sliceOrigin;
}

void Scroll::setLoaderSize(float value) {
	_loaderSize = value;
}
float Scroll::getLoaderSize() const {
	return _loaderSize;
}

void Scroll::setLoaderColor(const Color &c) {
	_loaderColor = c;
}
const Color &Scroll::getLoaderColor() const {
	return _loaderColor;
}

void Scroll::setMinLoadTime(TimeInterval time) {
	_minLoadTime = time;
}
TimeInterval Scroll::getMinLoadTime() const {
	return _minLoadTime;
}

void Scroll::setHandlerCallback(const HandlerCallback &cb) {
	_handlerCallback = cb;
}

void Scroll::setItemCallback(const ItemCallback &cb) {
	_itemCallback = cb;
}

void Scroll::setLoaderCallback(const LoaderCallback &cb) {
	_loaderCallback = cb;
}

void Scroll::onSourceDirty() {
	if ((isVertical() && _contentSize.height == 0.0f) || (!isVertical() && _contentSize.width == 0.0f)) {
		return;
	}

	if (!_listener || _items.size() == 0) {
		_controller->clear();
		if (isVertical()) {
			_controller->addItem(std::bind(&Scroll::onLoaderRequest, this, Request::Reset), _loaderSize, 0.0f);
		} else {
			auto size = _contentSize.width - _paddingGlobal.left - _loaderSize;
			_controller->addItem(std::bind(&Scroll::onLoaderRequest, this, Request::Reset), MAX(_loaderSize, size), 0.0f);
		}
	}

	if (!_listener) {
		return;
	}

	bool init = (_itemsCount == 0);
	_itemsCount = _listener->getCount(_categoryLookupLevel, _itemsForSubcats);

	if (_itemsCount == 0) {
		_categoryDirty = true;
		_currentSliceStart = Source::Id(0);
		_currentSliceLen = 0;
		return;
	} else if (_itemsCount <= _sliceMax) {
		_slicesCount = 1;
		_sliceSize = _itemsCount;
	} else {
		_slicesCount = (_itemsCount + _sliceMax - 1) / _sliceMax;
		_sliceSize = _itemsCount / _slicesCount + 1;
	}

	if ((!init && _categoryDirty) || _currentSliceLen == 0) {
		resetSlice();
	} else {
		updateSlice();
	}

	_scrollDirty = true;
	_categoryDirty = false;
}

size_t Scroll::getMaxId() const {
	if (!_listener) {
		return 0;
	}

	auto ret = _listener->getCount(_categoryLookupLevel, _itemsForSubcats);
	if (ret == 0) {
		return 0;
	} else {
		return ret - 1;
	}
}

std::pair<Scroll::Source *, bool> Scroll::getSourceCategory(int64_t id) {
	if (!_listener) {
		return std::make_pair(nullptr, false);
	}

	return _listener->getItemCategory(Source::Id(id), _categoryLookupLevel, _itemsForSubcats);
}

bool Scroll::requestSlice(Source::Id first, size_t count, Request type) {
	if (!_listener) {
		return false;
	}

	if (first.get() >= _itemsCount) {
		return false;
	}

	if (first.get() + count > _itemsCount) {
		count = _itemsCount - (size_t)first.get();
	}

	if (_useCategoryBounds) {
		_listener->setCategoryBounds(first, count, _categoryLookupLevel, _itemsForSubcats);
	}

	_invalidateAfter = stappler::Time::now();
	_listener->getSliceData(
		std::bind(&Scroll::onSliceData, Rc<Scroll>(this), std::placeholders::_1, Time::now(), type),
		first, count, _categoryLookupLevel, _itemsForSubcats);

	return true;
}

bool Scroll::updateSlice() {
	auto size = MAX(_currentSliceLen, _sliceSize);
	auto first = _currentSliceStart;
	if (size > _itemsCount) {
		size = _itemsCount;
	}
	if (first.get() > _itemsCount - size) {
		first = Source::Id(_itemsCount - size);
	}
	return requestSlice(first, size, Request::Update);
}

bool Scroll::resetSlice() {
	if (_listener) {
		int64_t start = (int64_t)_sliceOrigin.get() - (int64_t)_sliceSize / 2;
		if (start + (int64_t)_sliceSize > (int64_t)_itemsCount) {
			start = (int64_t)_itemsCount - (int64_t)_sliceSize;
		}
		if (start < 0) {
			start = 0;
		}
		return requestSlice(Source::Id(start), _sliceSize, Request::Reset);
	}
	return false;
}

bool Scroll::downloadFrontSlice(size_t size) {
	if (size == 0) {
		size = _sliceSize;
	}

	if (_listener && !_currentSliceStart.empty()) {
		Source::Id first(0);
		if (_currentSliceStart.get() > _sliceSize) {
			first = _currentSliceStart - Source::Id(_sliceSize);
		} else {
			size = (size_t)_currentSliceStart.get();
		}

		return requestSlice(first, size, Request::Front);
	}
	return false;
}

bool Scroll::downloadBackSlice(size_t size) {
	if (size == 0) {
		size = _sliceSize;
	}

	if (_listener && _currentSliceStart.get() + _currentSliceLen != _itemsCount) {
		Source::Id first(_currentSliceStart.get() + _currentSliceLen);
		if (first.get() + size > _itemsCount) {
			size = _itemsCount - (size_t)first.get();
		}

		return requestSlice(first, size, Request::Back);
	}
	return false;
}

void Scroll::onSliceData(DataMap &val, Time time, Request type) {
	if (time < _invalidateAfter || !_handlerCallback) {
		return;
	}

	if (_items.empty() && type != Request::Update) {
		type = Request::Reset;
	}

	auto itemPtr = new ItemMap();
	auto dataPtr = new DataMap(std::move(val));
	auto handlerPtr = new Rc<Handler>(onHandler());
	ResourceManager::thread().perform([this, handlerPtr, itemPtr, dataPtr, time, type] (cocos2d::Ref *) -> bool {
		(*itemPtr) = (*handlerPtr)->run(type, std::move(*dataPtr));
		auto interval = Time::now() - time;
		if (interval < _minLoadTime && type != Request::Update) {
			usleep ((useconds_t)(_minLoadTime.toMicroseconds() - interval.toMicroseconds()));
		}

		for (auto &it : (*itemPtr)) {
			it.second->setId(it.first.get());
		}
		return true;
	}, [this, handlerPtr, itemPtr, dataPtr, time, type] (cocos2d::Ref *, bool) {
		onSliceItems(std::move(*itemPtr), time, type);
		delete handlerPtr;
		delete itemPtr;
		delete dataPtr;
	}, this);
}

void Scroll::onSliceItems(ItemMap &&val, Time time, Request type) {
	if (time < _invalidateAfter) {
		return;
	}

	if (_items.size() > _sliceSize) {
		if (type == Request::Back) {
			auto newId = _items.begin()->first + Source::Id(_items.size() - _sliceSize);
			_items.erase(_items.begin(), _items.lower_bound(newId));
		} else if (type == Request::Front) {
			auto newId = _items.begin()->first + Source::Id(_sliceSize);
			_items.erase(_items.upper_bound(newId), _items.end());
		}
	}

	if (type == Request::Front || type == Request::Back) {
		val.insert(_items.begin(), _items.end()); // merge item maps
	}

	_items = std::move(val);

	_currentSliceStart = Source::Id(_items.begin()->first);
	_currentSliceLen = (size_t)_items.rbegin()->first.get() + 1 - (size_t)_currentSliceStart.get();

	float relPos = getScrollRelativePosition();

	updateItems();

	if (type == Request::Update) {
		setScrollRelativePosition(relPos);
	} else if (type == Request::Reset) {
		if (_sliceOrigin.empty()) {
			setScrollRelativePosition(0.0f);
		} else {
			auto it = _items.find(Source::Id(_sliceOrigin));
			if (it == _items.end()) {
				setScrollRelativePosition(0.0f);
			} else {
				auto start = _items.begin()->second->getPosition();
				auto end = _items.rbegin()->second->getPosition();

				if (isVertical()) {
					setScrollRelativePosition(fabs((it->second->getPosition().y - start.y) / (end.y - start.y)));
				} else {
					setScrollRelativePosition(fabs((it->second->getPosition().x - start.x) / (end.x - start.x)));
				}
			}
		}
	}
}

void Scroll::updateItems() {
	_controller->clear();

	if (!_items.empty()) {
		if (_items.begin()->first.get() > 0) {
			Item * first = _items.begin()->second;
			_controller->addItem(std::bind(&Scroll::onLoaderRequest, this, Request::Front),
					_loaderSize,
					(_layout == Vertical)?(first->getPosition().y - _loaderSize):(first->getPosition().x - _loaderSize));
		}

		for (auto &it : _items) {
			it.second->setControllerId(_controller->addItem(std::bind(&Scroll::onItemRequest, this, std::placeholders::_1, it.first),
					it.second->getContentSize(), it.second->getPosition()));
		}

		if (_items.rbegin()->first.get() < _itemsCount - 1) {
			Item * last = _items.rbegin()->second;
			_controller->addItem(std::bind(&Scroll::onLoaderRequest, this, Request::Back),
					_loaderSize,
					(_layout == Vertical)?(last->getPosition().y + last->getContentSize().height):(last->getPosition().x + last->getContentSize().width));
		}
	} else {
		_controller->addItem(std::bind(&Scroll::onLoaderRequest, this, Request::Reset), _loaderSize, 0.0f);
	}

	auto b = _movement;
	_movement = Movement::None;

	updateScrollBounds();
	onPosition();

	_movement = b;
}

Scroll::Handler *Scroll::onHandler() {
	return _handlerCallback(this);
}

MaterialNode *Scroll::onItemRequest(const ScrollController::Item &item, Source::Id id) {
	if ((isVertical() && item.size.height > 0.0f) || (isHorizontal() && item.size.width > 0)) {
		auto it = _items.find(id);
		if (it != _items.end()) {
			if (_itemCallback) {
				return _itemCallback(it->second);
			}
		}
	}
	return nullptr;
}

Scroll::Loader *Scroll::onLoaderRequest(Request type) {
	if (type == Request::Back) {
		if (_loaderCallback) {
			return _loaderCallback(type, [this] {
				downloadBackSlice(_sliceSize);
			}, _loaderColor);
		} else {
			return construct<Loader>([this] {
				downloadBackSlice(_sliceSize);
			}, _loaderColor);
		}
	} else if (type == Request::Front) {
		if (_loaderCallback) {
			return _loaderCallback(type, [this] {
				downloadFrontSlice(_sliceSize);
			}, _loaderColor);
		} else {
			return construct<Loader>([this] {
				downloadFrontSlice(_sliceSize);
			}, _loaderColor);
		}
	} else {
		if (_loaderCallback) {
			_loaderCallback(type, nullptr, _loaderColor);
		} else {
			return construct<Loader>(nullptr, _loaderColor);
		}
	}
	return nullptr;
}

void Scroll::updateIndicatorPosition() {
	if (!_indicatorVisible) {
		return;
	}

	float scrollWidth = _contentSize.width;
	float scrollHeight = _contentSize.height;

	float itemSize = getScrollLength() / _currentSliceLen;
	float scrollLength = itemSize * _itemsCount;

	float min = getScrollMinPosition() - _currentSliceStart.get() * itemSize;
	float max = getScrollMaxPosition() + (_itemsCount - _currentSliceStart.get() - _currentSliceLen) * itemSize;

	if (isnan(scrollLength)) {
		_indicator->setVisible(false);
	} else {
		_indicator->setVisible(_indicatorVisible);
	}

	auto paddingLocal = _paddingGlobal;
	if (_indicatorIgnorePadding) {
		if (isVertical()) {
			paddingLocal.top = 0;
			paddingLocal.bottom = 0;
		} else {
			paddingLocal.left = 0;
			paddingLocal.right = 0;
		}
	}

	if (scrollLength > _scrollSize) {
		if (isVertical()) {
			float h = (scrollHeight - 4 - paddingLocal.top - paddingLocal.bottom) * scrollHeight / scrollLength;
			if (h < 20) {
				h = 20;
			}
			float r = scrollHeight - h - 4 - paddingLocal.top - paddingLocal.bottom;
			float value = (_scrollPosition - min) / (max - min);

			_indicator->setContentSize(cocos2d::Size(3, h));
			_indicator->setPosition(cocos2d::Vec2(scrollWidth - 2, paddingLocal.bottom + 2 + r * (1.0f - value)));
			_indicator->setAnchorPoint(cocos2d::Vec2(1, 0));
		} else {
			float h = (scrollWidth - 4 - paddingLocal.left - paddingLocal.right) * scrollWidth / scrollLength;
			if (h < 20) {
				h = 20;
			}
			float r = scrollWidth - h - 4 - paddingLocal.left - paddingLocal.right;
			float value = (_scrollPosition - min) / (max - min);

			_indicator->setContentSize(cocos2d::Size(h, 3));
			_indicator->setPosition(cocos2d::Vec2(paddingLocal.left + 2 + r * (value), 2));
			_indicator->setAnchorPoint(cocos2d::Vec2(0, 0));
		}
		if (_indicator->getOpacity() != 255) {
			cocos2d::Action *a = _indicator->getActionByTag(19);
			if (!a) {
				a = cocos2d::FadeTo::create(progress(0.1f, 0.0f, _indicator->getOpacity() / 255.0f), 255);
				a->setTag(19);
				_indicator->runAction(a);
			}
		}

		_indicator->stopActionByTag(18);
		auto fade = cocos2d::Sequence::create(cocos2d::DelayTime::create(2.0f), cocos2d::FadeOut::create(0.25f), nullptr);
		fade->setTag(18);
		_indicator->runAction(fade);
	} else {
		_indicator->setVisible(false);
	}
}

void Scroll::onOverscroll(float delta) {
	if (delta > 0 && _currentSliceStart.get() + _currentSliceLen == _itemsCount) {
		ScrollView::onOverscroll(delta);
	} else if (delta < 0 && _currentSliceStart.empty()) {
		ScrollView::onOverscroll(delta);
	}
}

NS_MD_END
