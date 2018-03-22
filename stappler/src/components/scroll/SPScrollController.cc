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
#include "SPScrollController.h"
#include "SPScrollViewBase.h"
#include "2d/CCNode.h"
#include "SPScrollItemHandle.h"

NS_SP_BEGIN

ScrollController::Item::Item(const NodeFunction &f, const Vec2 &pos, const Size &size, int z, const String &name)
: nodeFunction(f), size(size), pos(pos), zIndex(z), name(name) { }

ScrollController::~ScrollController() { }

void ScrollController::onAdded() {
	Component::onAdded();
	_scroll = dynamic_cast<ScrollViewBase *>(getOwner());
	if (_scroll) {
		_root = _scroll->getRoot();
		_scroll->setScrollDirty(true);
	}
}
void ScrollController::onRemoved() {
	clear();
	Component::onRemoved();
	_scroll = nullptr;
	_root = nullptr;
}

void ScrollController::onContentSizeDirty() {
	Component::onContentSizeDirty();
	if (!_scroll) {
		return;
	}

	if (!rebuildObjects()) {
		for (auto &it : _nodes) {
			updateScrollNode(it);
		}
	}
}

void ScrollController::onScrollPosition(bool force) {
	if (!_scroll || !_root) {
		return;
	}

	bool isVertical = _scroll->isVertical();
	auto csize = _scroll->getContentSize();
	if ((isVertical && csize.width == 0) || (!isVertical && csize.height == 0)) {
		return;
	}

	do {
		if (_infoDirty || force) {
			float start = nan();
			float end = nan();
			float size = 0;
			float pos = 0;

			for (auto &it : _nodes) {
				pos = _scroll->getNodeScrollPosition(it.pos);
				size = _scroll->getNodeScrollSize(it.size);

				if (isnan(start) || start > pos) {
					start = pos;
				}

				if (isnan(end) || end < pos + size) {
					end = pos + size;
				}
			}

			if (isnan(start)) {
			    setScrollableAreaOffset(0.0f);
			} else {
			    setScrollableAreaOffset(start);
			}
		    setScrollableAreaSize(end - start);
		    _scroll->updateScrollBounds();
		    _infoDirty = false;
		    force = false;
		}

		float pos = _scroll->getScrollPosition();
		float size = _scroll->getScrollSize();

		if (_currentSize == 0) {
			reset(pos, size);
		} else {
			update(pos, size);
		}
	} while (_infoDirty || force);
}

void ScrollController::onScroll(float delta, bool eneded) {

}

void ScrollController::onOverscroll(float delta) {

}

cocos2d::Node *ScrollController::getRoot() const {
	return _root;
}
ScrollViewBase *ScrollController::getScroll() const {
	return _scroll;
}

float ScrollController::getScrollMin() {
	return _currentMin;
}
float ScrollController::getScrollMax() {
	return _currentMax;
}

void ScrollController::clear() {
	for (auto &it : _nodes) {
		if (it.node) {
			it.node->removeFromParent();
		}
	}

	_nodes.clear();
	_currentSize = 0.0f;
	_currentPosition = 0.0f;

	_currentMin = 0.0f;
	_currentMax = 0.0f;
}

void ScrollController::update(float position, float size) {
	reset(position, size);
}

void ScrollController::reset(float origPosition, float origSize) {
	float windowBegin = nan();
	float windowEnd = nan();

	float position = origPosition - 8.0f;
	float size = origSize + 16.0f;

	if (_animationPadding > 0.0f) {
		size += _animationPadding;
	} else if (_animationPadding < 0.0f) {
		position += _animationPadding;
		size -= _animationPadding;
	}

	for (auto &it : _nodes) {
		auto nodePos = _scroll->getNodeScrollPosition(it.pos);
		auto nodeSize = _scroll->getNodeScrollSize(it.size);
		if (nodePos + nodeSize > position && nodePos < position + size) {
			if (it.node) {
				if (isnan(windowBegin) || windowBegin > nodePos) {
					windowBegin = nodePos;
				}
				if (isnan(windowEnd) || windowEnd < nodePos + nodeSize) {
					windowEnd = nodePos + nodeSize;
				}
			}
		}
	}

	_windowBegin = windowBegin;
	_windowEnd = windowEnd;

	for (auto &it : _nodes) {
		auto nodePos = _scroll->getNodeScrollPosition(it.pos);
		auto nodeSize = _scroll->getNodeScrollSize(it.size);
		if (nodePos + nodeSize <= position || nodePos >= position + size) {
			if (it.node && (!_keepNodes || it.node->isVisible())) {
				removeScrollNode(it);
			}
		} else {
			onNextObject(it, nodePos, nodeSize);
		}
	}

	_currentPosition = origPosition;
	_currentSize = origSize;
}

void ScrollController::onNextObject(Item &h, float pos, float size) {
	if (!_scroll || !_root) {
		return;
	}

	if (!h.node) {
		auto node = h.nodeFunction(h);
		if (node) {
			bool forward = true;
			if (!isnan(_windowBegin) && !isnan(_windowEnd)) {
				float windowMid = (_windowBegin + _windowEnd) / 2.0f;
				if (pos + size < windowMid) {
					forward = false;
				} else if (pos > windowMid) {
					forward = true;
				}
			}

			if (auto handle = node->getComponentByType<ScrollItemHandle>()) {
				h.handle = handle;
				_scroll->updateScrollNode(node, h.pos, h.size, h.zIndex, h.name);
				handle->onNodeInserted(this, h, size_t(&h - _nodes.data()));

				auto nodeSize = _scroll->getNodeScrollSize(node->getContentSize());
				if (nodeSize > 0.0f && nodeSize != size) {
					resizeItem(&h, nodeSize, forward);
				}
			}
			h.node = node;
			addScrollNode(h);
		}
	} else {
		h.node->setVisible(true);
		h.node->pushForceRendering();
		if (h.handle) {
			h.handle->onNodeUpdated(this, h, size_t(&h - _nodes.data()));
		}
		_scroll->updateScrollNode(h.node, h.pos, h.size, h.zIndex, h.name);
	}
}

size_t ScrollController::addItem(const NodeFunction &fn, const Size &size, const Vec2 &vec, int z, const String &tag) {
	_nodes.emplace_back(fn, vec, size, z, tag);
	_infoDirty = true;
	return _nodes.size() - 1;
}

size_t ScrollController::addItem(const NodeFunction &fn, float size, float pos, int z, const String &tag) {
	if (!_scroll) {
		return std::numeric_limits<size_t>::max();
	}

	_nodes.emplace_back(fn, _scroll->getPositionForNode(pos), _scroll->getContentSizeForNode(size), z, tag);
	_infoDirty = true;
	return _nodes.size() - 1;
}

size_t ScrollController::addItem(const NodeFunction &fn, float size, int zIndex, const String &tag) {
	if (!_scroll) {
		return std::numeric_limits<size_t>::max();
	}

	auto pos = 0.0f;
	if (!_nodes.empty()) {
		pos = _scroll->getNodeScrollPosition(_nodes.back().pos) + _scroll->getNodeScrollSize(_nodes.back().size);
	}

	return addItem(fn, size, pos, zIndex, tag);
}

size_t ScrollController::addPlaceholder(const Size &size, const Vec2 &pos) {
	return addItem([] (const Item &item) -> Rc<cocos2d::Node> {
		return Rc<cocos2d::Node>::create();
	}, size, pos);
}
size_t ScrollController::addPlaceholder(float size, float pos) {
	return addItem([] (const Item &item) -> Rc<cocos2d::Node>  {
		return Rc<cocos2d::Node>::create();
	}, size, pos);
}
size_t ScrollController::addPlaceholder(float size) {
	return addItem([] (const Item &item) -> Rc<cocos2d::Node>  {
		return Rc<cocos2d::Node>::create();
	}, size);
}

float ScrollController::getNextItemPosition() const {
	if (!_nodes.empty()) {
		return _scroll->getNodeScrollPosition(_nodes.back().pos) + _scroll->getNodeScrollSize(_nodes.back().size);
	}
	return 0.0f;
}

void ScrollController::setKeepNodes(bool value) {
	if (_keepNodes != value) {
		_keepNodes = value;
	}
}
bool ScrollController::isKeepNodes() const {
	return _keepNodes;
}

const ScrollController::Item *ScrollController::getItem(size_t n) {
	if (n < _nodes.size()) {
		_infoDirty = true;
		return &_nodes[n];
	}
	return nullptr;
}

const ScrollController::Item *ScrollController::getItem(cocos2d::Node *node) {
	if (node) {
		for (auto &it : _nodes) {
			if (node == it.node) {
				_infoDirty = true;
				return &it;
			}
		}
	}
	return nullptr;
}

const ScrollController::Item *ScrollController::getItem(const String &str) {
	if (str.empty()) {
		return nullptr;
	}
	for (auto &it : _nodes) {
		if (it.name == str && it.node) {
			return &it;
		}
	}
	return nullptr;
}

size_t ScrollController::getItemIndex(cocos2d::Node *node) {
	size_t idx = 0;
	for (auto &it : _nodes) {
		if (it.node == node) {
			return idx;
		}
		idx++;
	}
	return std::numeric_limits<size_t>::max();
}

const Vector<ScrollController::Item> &ScrollController::getItems() const {
	return _nodes;
}

Vector<ScrollController::Item> &ScrollController::getItems() {
	return _nodes;
}

size_t ScrollController::size() const {
	return _nodes.size();
}

void ScrollController::setScrollableAreaOffset(float value) {
	if (_scrollAreaOffset != value) {
		_scrollAreaOffset = value;
		_currentMin = _scrollAreaOffset;
		_currentMax = _scrollAreaOffset + _scrollAreaSize;
		if (_scroll) {
			_scroll->setScrollDirty(true);
		}
	}
}

float ScrollController::getScrollableAreaOffset() const {
	return _scrollAreaOffset;
}

void ScrollController::setScrollableAreaSize(float value) {
	if (_scrollAreaSize != value) {
		_scrollAreaSize = value;
		_currentMin = _scrollAreaOffset;
		_currentMax = _scrollAreaOffset + _scrollAreaSize;
		if (_scroll) {
			_scroll->setScrollDirty(true);
		}
	}
}
float ScrollController::getScrollableAreaSize() const {
	return _scrollAreaSize;
}

bool ScrollController::rebuildObjects(bool force) {
	return false;
}

void ScrollController::setScrollRelativeValue(float value) {
	if (!_scroll) {
		return;
	}

	onScrollPosition();

	if (!isnan(value)) {
		if (value < 0.0f) {
			value = 0.0f;
		} else if (value > 1.0f) {
			value = 1.0f;
		}
	} else {
		value = 0.0f;
	}

	float areaSize = _scroll->getScrollableAreaSize();
	float areaOffset = _scroll->getScrollableAreaOffset();
	float size = _scroll->getScrollSize();

	auto &padding = _scroll->getPadding();
	auto paddingFront = (_scroll->isVertical())?padding.top:padding.left;
	auto paddingBack = (_scroll->isVertical())?padding.bottom:padding.right;

	if (!isnan(areaSize) && !isnan(areaOffset)) {
		float liveSize = areaSize - size + paddingFront + paddingBack;
		float pos = (value * liveSize) - paddingFront + areaOffset;
		clear();
		_scroll->setScrollPosition(pos);
	}
}

void ScrollController::addScrollNode(Item &it) {
	if (it.node) {
		_scroll->addScrollNode(it.node, it.pos, it.size, it.zIndex, it.name);
		it.node->pushForceRendering();
	}
}

void ScrollController::updateScrollNode(Item &it) {
	if (it.node) {
		_scroll->updateScrollNode(it.node, it.pos, it.size, it.zIndex, it.name);
	}
}

void ScrollController::removeScrollNode(Item &it) {
	if (it.node) {
		if (!_keepNodes) {
			if (it.handle) {
				it.handle->onNodeRemoved(this, it, size_t(&it - _nodes.data()));
			}
			if (_scroll->removeScrollNode(it.node)) {
				it.node = nullptr;
				it.handle = nullptr;
			}
		} else {
			it.node->setVisible(false);
		}
	}
}

Vector<Rc<cocos2d::Node>> ScrollController::getNodes() const {
	Vector<Rc<cocos2d::Node>> ret;
	for (auto &it : _nodes) {
		if (it.node) {
			ret.emplace_back(it.node);
		}
	}
	return ret;
}

cocos2d::Node * ScrollController::getNodeByName(const String &str) const {
	if (str.empty()) {
		return nullptr;
	}
	for (auto &it : _nodes) {
		if (it.name == str && it.node) {
			return it.node;
		}
	}
	return nullptr;
}

cocos2d::Node * ScrollController::getFrontNode() const {
	cocos2d::Node *ret = nullptr;
	float pos = _currentMax;
	if (!_nodes.empty() && _scroll) {
		for (auto &it : _nodes) {
			auto npos = _scroll->getNodeScrollPosition(it.pos);
			if (it.node && npos < pos) {
				pos = npos;
				ret = it.node;
			}
		}
	}
	return ret;
}

cocos2d::Node * ScrollController::getBackNode() const {
	cocos2d::Node *ret = nullptr;
	float pos = _currentMin;
	if (!_nodes.empty() && _scroll) {
		for (auto it = _nodes.rbegin(); it != _nodes.rend(); it++) {
			auto npos = _scroll->getNodeScrollPosition(it->pos);
			auto size = _scroll->getNodeScrollSize(it->size);
			if (it->node && npos + size > pos) {
				pos = npos + size;
				ret = it->node;
			}
		}
	}
	return ret;
}

void ScrollController::resizeItem(const Item *item, float newSize, bool forward) {
	auto &items = getItems();

	if (forward) {
		float offset = 0.0f;
		for (auto &it : items) {
			if (&it == item) {
				offset += (newSize - _scroll->getNodeScrollSize(it.size));
				it.size =  _scroll->isVertical()?Size(it.size.width, newSize):Size(newSize, it.size.height);
				if (it.node) {
					_scroll->updateScrollNode(it.node, it.pos, it.size, it.zIndex, it.name);
				}
			} else if (offset != 0.0f) {
				it.pos = _scroll->isVertical()?Vec2(it.pos.x, it.pos.y + offset):Vec2(it.pos.x + offset, it.pos.y);
				if (it.node) {
					_scroll->updateScrollNode(it.node, it.pos, it.size, it.zIndex, it.name);
				}
			}
		}
	} else {
		float offset = 0.0f;
		for (auto it = items.rbegin(); it != items.rend(); ++ it) {
			if (&(*it) == item) {
				offset += (newSize - _scroll->getNodeScrollSize(it->size));
				it->size = _scroll->isVertical()?Size(it->size.width, newSize):Size(newSize, it->size.height);
				it->pos = _scroll->isVertical()?Vec2(it->pos.x, it->pos.y - offset):Vec2(it->pos.x - offset, it->pos.y);
				if (it->node) {
					_scroll->updateScrollNode(it->node, it->pos, it->size, it->zIndex, it->name);
				}
			} else if (offset != 0.0f) {
				it->pos = _scroll->isVertical()?Vec2(it->pos.x, it->pos.y - offset):Vec2(it->pos.x - offset, it->pos.y);
				if (it->node) {
					_scroll->updateScrollNode(it->node, it->pos, it->size, it->zIndex, it->name);
				}
			}
		}
	}
    _infoDirty = true;
}

void ScrollController::setAnimationPadding(float padding) {
	if (_animationPadding != padding) {
		_animationPadding = padding;
		_infoDirty = true;
	}
}

void ScrollController::dropAnimationPadding() {
	if (_animationPadding != 0.0f) {
		_animationPadding = 0.0f;
		_infoDirty = true;
	}
}

void ScrollController::updateAnimationPadding(float value) {
	if (_animationPadding != 0.0f) {
		auto val = _animationPadding - value;
		if (val * _animationPadding <= 0.0f) {
			_animationPadding = 0.0f;
			_infoDirty = true;
		} else {
			_animationPadding = val;
			_infoDirty = true;
		}
	}
}

NS_SP_END
