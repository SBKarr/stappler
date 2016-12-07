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

#include "MaterialRecyclerScroll.h"
#include "MaterialIconSprite.h"
#include "MaterialButtonLabel.h"

#include "SPGestureListener.h"
#include "SPRoundedSprite.h"
#include "SPClippingNode.h"
#include "SPLayer.h"
#include "SPActions.h"
#include "SPScrollController.h"
#include "SPDataSource.h"

NS_MD_BEGIN

bool RecyclerHolder::init(const Function<void()> &cb) {
	if (!Node::init()) {
		return false;
	}

	_layer = construct<Layer>(Color::Grey_400);
	addChild(_layer, 1);

	_icon = construct<IconSprite>(IconName::Content_clear);
	_icon->setAnchorPoint(Vec2(0.5f, 0.5f));
	_icon->setColor(Color::Black);
	addChild(_icon, 2);

	_button = construct<ButtonLabel>(cb);
	_button->setString("SystemRestore"_locale);
	_button->setAnchorPoint(Vec2(1.0f, 0.0f));
	_button->setVisible(false);
	addChild(_button, 3);

	_label = construct<Label>(FontType::Body_1);
	_label->setLocaleEnabled(true);
	_label->setString("SystemRemoved"_locale);
	_label->setAnchorPoint(Vec2(0.0f, 0.5f));
	_label->setVisible(false);
	addChild(_label, 4);

	return true;
}

void RecyclerHolder::onContentSizeDirty() {
	Node::onContentSizeDirty();

	_layer->setContentSize(_contentSize);

	if (_enabled) {
		_label->setPosition(Vec2(24.0f, _contentSize.height / 2.0f));
		_button->setContentSize(Size(_button->getlabel()->getContentSize().width + 32.0f, _contentSize.height));
		_button->setPosition(Vec2(_contentSize.width, 0.0f));
	} else {
		_icon->setPosition(Vec2(_inverted?24.0f:(_contentSize.width - 24.0f), _contentSize.height / 2.0f));
	}
	updateProgress();
}

void RecyclerHolder::setEnabled(bool value) {
	if (_enabled != value) {
		_enabled = value;
		if (_enabled) {
			_icon->setVisible(false);
			_label->setVisible(true);
			_button->setVisible(true);
			showEnabled();
		} else {
			_icon->setVisible(true);
			_label->setVisible(false);
			_button->setVisible(false);
			showDisabled();
		}
	}
}

void RecyclerHolder::setInverted(bool value) {
	if (value != _inverted) {
		_inverted = value;
		_contentSizeDirty = true;
	}
}

void RecyclerHolder::updateProgress() {
	if (_progress < 0.5f) {
		_icon->setOpacity(255 * (1.0f - _progress * 2.0f));
		_label->setOpacity(0);
		_button->setOpacity(0);
	} else {
		_icon->setOpacity(0);
		_label->setOpacity(255 * ((_progress - 0.5f) * 2.0f));
		_button->setOpacity(255 * ((_progress - 0.5f) * 2.0f));
	}
}

void RecyclerHolder::showEnabled() {
	stopAllActionsByTag("HolderAction"_tag);
	runAction(construct<ProgressAction>(0.25f, _progress, 1.0f, [this] (ProgressAction *, float p) {
		_progress = p;
		updateProgress();
	}), "HolderAction"_tag);
}

void RecyclerHolder::showDisabled() {
	stopAllActionsByTag("HolderAction"_tag);
	runAction(construct<ProgressAction>(0.25f, _progress, 0.0f, [this] (ProgressAction *, float p) {
		_progress = p;
		updateProgress();
	}), "HolderAction"_tag);
}

void RecyclerHolder::setPlaceholderText(const String &value) {
	_label->setString(value);
}
void RecyclerHolder::setPlaceholderButtonText(const String &value) {
	_button->setString(value);
}
void RecyclerHolder::setPlaceholderTextColor(const Color &color) {
	_label->setColor(color);
	_button->setLabelColor(color);
}
void RecyclerHolder::setPlaceholderBackgroundColor(const Color &color) {
	_layer->setColor(color);
}


bool RecyclerNode::init(RecyclerScroll *scroll, MaterialNode *node, Scroll::Item *item, data::Source::Id id) {
	if (!MaterialNode::init()) {
		return false;
	}

	_scroll = scroll;
	_shadowClipper->setVisible(false);
	_background->setVisible(false);

	_layer = construct<Layer>(Color::Grey_500);
	_layer->setAnchorPoint(Vec2(0.0f, 0.0f));
	addChild(_layer, 0);

	_holder = construct<RecyclerHolder>([this] {
		onRestore();
	});
	_holder->setAnchorPoint(Vec2(0.0f, 0.0f));
	addChild(_holder, 1);

	auto l = Rc<gesture::Listener>::create();
	l->setSwipeCallback([this] (gesture::Event ev, const gesture::Swipe &s) -> bool {
		auto density = screen::density();
		if (ev == gesture::Event::Began) {
			if (fabsf(s.delta.x * 2.0f) <= fabsf(s.delta.y)) {
				return false;
			}

			onSwipeBegin();
			return onSwipe(- s.delta.x / density, - s.velocity.x / density);
		} else if (ev == gesture::Event::Activated) {
			return onSwipe(- s.delta.x / density, - s.velocity.x / density);
		} else {
			return onSwipeEnd(- s.velocity.x / density);
		}
	});
	addComponent(l);
	_listener = l;

	_item = item;
	_node = node;
	addChild(node, 2);

	setContentSize(item->getContentSize());
	setPosition(item->getPosition());

	_node->setPosition(_contentSize.width * _enabledProgress, 0.0f);
	_node->setAnchorPoint(Vec2(0.0f, 0.0f));
	_node->setContentSize(_contentSize);

	return true;
}

void RecyclerNode::onContentSizeDirty() {
	MaterialNode::onContentSizeDirty();

	updateProgress();
}

bool RecyclerNode::isRemoved() const {
	return _state != Enabled;
}

Scroll::Item *RecyclerNode::getItem() const {
	return _item;
}


void RecyclerNode::setPlaceholderText(const String &value) {
	_holder->setPlaceholderText(value);
}
void RecyclerNode::setPlaceholderButtonText(const String &value) {
	_holder->setPlaceholderButtonText(value);
}
void RecyclerNode::setPlaceholderTextColor(const Color &color) {
	_holder->setPlaceholderTextColor(color);
}
void RecyclerNode::setPlaceholderBackgroundColor(const Color &color) {
	_holder->setPlaceholderBackgroundColor(color);
	_layer->setColor(color.darker(1));
}

void RecyclerNode::updateProgress() {
	if (_state == Enabled) {
		_layer->setVisible(false);
		_holder->setEnabled(false);

		_node->setPosition(_contentSize.width * _enabledProgress, 0.0f);
		_node->setAnchorPoint(Vec2(0.0f, 0.0f));
		_node->setContentSize(_contentSize);

		if (fabs(_enabledProgress) < std::numeric_limits<float>::epsilon()) {
			_holder->setVisible(false);
		} else if (_enabledProgress > 0.0f) {
			_holder->setVisible(true);
			_holder->setPosition(0.0f, 0.0f);
			_holder->setContentSize(Size(_contentSize.width * fabs(_enabledProgress), _contentSize.height));
			_holder->setInverted(true);
		} else {
			_holder->setVisible(true);
			_holder->setPosition(_contentSize.width * (1.0f - fabs(_enabledProgress)), 0.0f);
			_holder->setContentSize(Size(_contentSize.width * fabs(_enabledProgress), _contentSize.height));
			_holder->setInverted(false);
		}
	} else if (_state == Prepared) {
		//_node->setVisible(false);

		_holder->setEnabled(true);
		_holder->setPosition(_contentSize.width * _preparedProgress, 0.0f);
		_holder->setAnchorPoint(Vec2(0.0f, 0.0f));
		_holder->setContentSize(_contentSize);

		if (fabs(_preparedProgress) < std::numeric_limits<float>::epsilon()) {
			_layer->setVisible(false);
		} else if (_preparedProgress > 0.0f) {
			_layer->setVisible(true);
			_layer->setPosition(0.0f, 0.0f);
			_layer->setContentSize(Size(_contentSize.width * fabs(_preparedProgress), _contentSize.height));
		} else {
			_layer->setVisible(true);
			_layer->setPosition(_contentSize.width * (1.0f - fabs(_preparedProgress)), 0.0f);
			_layer->setContentSize(Size(_contentSize.width * fabs(_preparedProgress), _contentSize.height));
		}
	} else {
		//_node->setVisible(false);
		_holder->setVisible(false);
		_layer->setVisible(true);
		_layer->setPosition(0.0f, 0.0f);
		_layer->setContentSize(_contentSize);
	}
}

bool RecyclerNode::onSwipeBegin() {
	stopAllActionsByTag("RecyclerAction"_tag);
	_scroll->unscheduleCleanup();
	return true;
}

bool RecyclerNode::onSwipe(float d, float v) {
	const float pos = (_state == Enabled)?_node->getPositionX():_holder->getPositionX();
	auto newPos = pos - d;
	if (_state == Enabled) {
		_enabledProgress = newPos / _contentSize.width;
	} else {
		_preparedProgress = newPos / _contentSize.width;
	}
	updateProgress();
	return true;
}

bool RecyclerNode::onSwipeEnd(float velocity) {
	const float pos = (_state == Enabled)?_node->getPositionX():_holder->getPositionX();
	const float acceleration = (velocity > 0)?-5000.0f:5000.0f;
	const float duration = fabsf(velocity / acceleration);
	const float path = velocity * duration + acceleration * duration * duration * 0.5f;

	const float newPos = pos - path;
	const float newProgress = newPos / _contentSize.width;
	float target = 0.0f;
	float targetProgress = 0.0f;

	if (fabs(newProgress) < 0.5f) {
		target = 0.0f;
		targetProgress = 0.0f;
	} else if (newProgress > 0.0f) {
		target = _contentSize.width;
		targetProgress = 1.0f;
	} else {
		target = -_contentSize.width;
		targetProgress = -1.0f;
	}

	auto a = Accelerated::createBounce(fabsf(acceleration), (_state == Enabled)?_node->getPosition():_holder->getPosition(), Vec2(target, 0.0f), fabs(velocity),
			50000.0f, [this] (cocos2d::Node *node) {
		if (_state == Enabled) {
			_enabledProgress = _node->getPositionX() / _contentSize.width;
		} else {
			_preparedProgress = _holder->getPositionX() / _contentSize.width;
		}
		updateProgress();
	});

	auto callback = action::callback(a, [this, targetProgress] {
		if (_state == Enabled) {
			if (fabs(fabs(targetProgress) - 1.0f) < std::numeric_limits<float>::epsilon()) {
				onPrepared();
			}
		} else if (_state == Prepared) {
			if (fabs(fabs(targetProgress) - 1.0f) < std::numeric_limits<float>::epsilon()) {
				onRemoved();
			}
		}
	});

	if (_state == Enabled) {
		_node->runAction(callback, "RecyclerAction"_tag);
	} else {
		_holder->runAction(callback, "RecyclerAction"_tag);
	}

	_scroll->scheduleCleanup();
	return true;
}

void RecyclerNode::onPrepared() {
	_state = Prepared;
	_contentSizeDirty = true;
	_holder->setEnabled(true);
	_scroll->scheduleCleanup();
}

void RecyclerNode::onRemoved() {
	_node->removeFromParent();
	_state = Removed;
	_contentSizeDirty = true;
	_scroll->removeRecyclerNode(_item, this);
}

void RecyclerNode::onRestore() {
	if (_state == Prepared) {
		_state = Enabled;
		_contentSizeDirty = true;
		_scroll->unscheduleCleanup();
		_node->runAction(cocos2d::EaseQuadraticActionOut::create(construct<ProgressAction>(0.35f, _enabledProgress, 0.0f,
				[this] (ProgressAction *, float p) {
			_enabledProgress = p;
			_contentSizeDirty = true;
			_holder->setEnabled(false);
			_scroll->scheduleCleanup();
		})), "RecyclerAction"_tag);
	}
}


bool RecyclerScroll::init(Source *dataCategory) {
	if (!Scroll::init(dataCategory)) {
		return false;
	}

	return true;
}

MaterialNode *RecyclerScroll::onItemRequest(const ScrollController::Item &item, Source::Id id) {
	auto node = Scroll::onItemRequest(item, id);
	if (node) {
		auto it = _items.find(id);
		if (it != _items.end()) {
			return construct<RecyclerNode>(this, node, it->second, id);
		}
	}
	return nullptr;
}

void RecyclerScroll::removeRecyclerNode(Item *item, cocos2d::Node *node) {
	auto &items = _controller->getItems();

	struct ItemRects {
		float startPos;
		float startSize;
		float targetPos;
		float targetSize;
		ScrollController::Item *item;
	};

	Vector<ItemRects> vec;

	float offset = 0.0f;
	for (auto &it : items) {
		if (it.node == node) {
			offset += it.size.height;
			vec.emplace_back(ItemRects{it.pos.y, it.size.height, it.pos.y - offset, 0, &it});
		} else {
			vec.emplace_back(ItemRects{it.pos.y, it.size.height, it.pos.y - offset, it.size.height, &it});
		}
	}

	offset = 0.0f;
	for (auto &it : _items) {
		if (it.second == item) {
			auto size = it.second->getContentSize();
			offset += size.height;
			it.second->setContentSize(Size(size.width, 0.0f));
			it.second->setPosition(it.second->getPosition() - Vec2(0.0f, offset));
		} else {
			it.second->setPosition(it.second->getPosition() - Vec2(0.0f, offset));
		}
	}

	runAction(construct<ProgressAction>(0.20f, 0.0f, 1.0f, [vec, this] (ProgressAction *, float p) {
		for (auto &it : vec) {
			it.item->pos.y = progress(it.startPos, it.targetPos, p);
			it.item->size.height = progress(it.startSize, it.targetSize, p);
			if (it.item->node) {
				updateScrollNode(it.item->node, it.item->pos, it.item->size, it.item->zIndex);
			}
		}
		_controller->onScrollPosition(true);
	}, [] (ProgressAction *) {

	}, [this, vec] (ProgressAction *) {
		for (auto &it : vec) {
			if (it.item->node && it.item->size.height == 0.0f) {
				it.item->node->removeFromParent();
				it.item->node = nullptr;
			}
		}
		afterCleanup();
	}));

	scheduleCleanup();
}

void RecyclerScroll::scheduleCleanup() {
	stopAllActionsByTag("CleanupDelay"_tag);
	runAction(action::callback(5.0f, [this] {
		performCleanup();
	}), "CleanupDelay"_tag);
}
void RecyclerScroll::unscheduleCleanup() {
	stopAllActionsByTag("CleanupDelay"_tag);
}
void RecyclerScroll::performCleanup() {
	auto &items = _controller->getItems();

	struct ItemRects {
		float startPos;
		float startSize;
		float targetPos;
		float targetSize;
		ScrollController::Item *item;
	};

	Vector<ItemRects> vec;
	Vector<Item *> removedItems;

	float offset = 0.0f;
	for (auto &it : items) {
		if (it.node) {
			RecyclerNode *n = dynamic_cast<RecyclerNode *>(it.node);
			if (n && n->isRemoved()) {
				offset += it.size.height;
				vec.emplace_back(ItemRects{it.pos.y, it.size.height, it.pos.y - offset, 0, &it});
				removedItems.push_back(n->getItem());
				continue;
			}
		}
		vec.emplace_back(ItemRects{it.pos.y, it.size.height, it.pos.y - offset, it.size.height, &it});
	}

	offset = 0.0f;
	for (auto &it : _items) {
		if (std::find(removedItems.begin(), removedItems.end(), it.second.get()) != removedItems.end()) {
			auto size = it.second->getContentSize();
			offset += size.height;
			it.second->setContentSize(Size(size.width, 0.0f));
			it.second->setPosition(it.second->getPosition() - Vec2(0.0f, offset));
		} else {
			it.second->setPosition(it.second->getPosition() - Vec2(0.0f, offset));
		}
	}

	if (!removedItems.empty()) {
		runAction(construct<ProgressAction>(0.20f, 0.0f, 1.0f, [vec, this] (ProgressAction *, float p) {
			for (auto &it : vec) {
				it.item->pos.y = progress(it.startPos, it.targetPos, p);
				it.item->size.height = progress(it.startSize, it.targetSize, p);
				if (it.item->node) {
					updateScrollNode(it.item->node, it.item->pos, it.item->size, it.item->zIndex);
				}
			}
			_controller->onScrollPosition(true);
		}, [] (ProgressAction *) {

		}, [this, vec] (ProgressAction *) {
			for (auto &it : vec) {
				if (it.item->node && it.item->size.height == 0.0f) {
					it.item->node->removeFromParent();
					it.item->node = nullptr;
				}
			}
			afterCleanup();
		}));
	}
}

void RecyclerScroll::onSwipeBegin() {
	Scroll::onSwipeBegin();
	unscheduleCleanup();
	performCleanup();
}

void RecyclerScroll::setPlaceholderText(const String &value) {
	if (_placeholderText != value) {
		_placeholderText = value;
		updateNodes();
	}
}
const String & RecyclerScroll::getPlaceholderText() const {
	return _placeholderText;
}

void RecyclerScroll::setPlaceholderButtonText(const String &value) {
	if (_placeholderButton != value) {
		_placeholderButton = value;
		updateNodes();
	}
}
const String & RecyclerScroll::getPlaceholderButtonText() const {
	return _placeholderButton;
}

void RecyclerScroll::setPlaceholderTextColor(const Color &value) {
	if (_placeholderTextColor != value) {
		_placeholderTextColor = value;
		updateNodes();
	}
}
const Color & RecyclerScroll::getPlaceholderTextColor() const {
	return _placeholderTextColor;
}

void RecyclerScroll::setPlaceholderBackgroundColor(const Color &value) {
	if (_placeholderBackgroundColor != value) {
		_placeholderBackgroundColor = value;
		updateNodes();
	}
}
const Color & RecyclerScroll::getPlaceholderBackgroundColor() const {
	return _placeholderBackgroundColor;
}

void RecyclerScroll::updateNodes() {
	auto &nodes = _root->getChildren();
	for (auto &it : nodes) {
		if (auto n = dynamic_cast<RecyclerNode *>(it)) {
			n->setPlaceholderText(_placeholderText);
			n->setPlaceholderButtonText(_placeholderButton);
			n->setPlaceholderTextColor(_placeholderTextColor);
			n->setPlaceholderBackgroundColor(_placeholderBackgroundColor);
		}
	}
}

void RecyclerScroll::afterCleanup() {
	auto &citems = _controller->getItems();
	uint64_t idOffset(0);

	Vector<Rc<Item>> removedItems;

	Vector<size_t> controllerItemToRemove;
	auto it = _items.begin();
	while (it != _items.end()) {
		if (it->second->getContentSize().height == 0) {
			++ idOffset;
			controllerItemToRemove.push_back(it->second->getControllerId());
			removedItems.emplace_back(it->second);
			it = _items.erase(it);
		} else {
			if (idOffset > 0) {
				auto newId = it->second->getId() - idOffset;
				it->second->setId(newId);
				_items.emplace(data::Source::Id(newId), it->second);
				citems.at(it->second->getControllerId()).nodeFunction = std::bind(&RecyclerScroll::onItemRequest, this, std::placeholders::_1, data::Source::Id(newId));

				it->second->setControllerId(it->second->getControllerId() - idOffset);
				it = _items.erase(it);
			} else {
				++ it;
			}
		}
	}

	for (auto rem_it = controllerItemToRemove.rbegin(); rem_it != controllerItemToRemove.rend(); ++ rem_it) {
		citems.erase(citems.begin() + *rem_it);
	}

	auto rbegin = _items.rbegin();
	while(idOffset > 0) {
		++ rbegin;
		-- idOffset;
	}

	for (auto &r_it : removedItems) {
		onItemRemoved(r_it);
	}

	_items.erase(++ rbegin.base(), _items.end());

	_currentSliceStart = Source::Id(_items.begin()->first);
	_currentSliceLen = (size_t)_items.rbegin()->first.get() + 1 - (size_t)_currentSliceStart.get();

	_itemsCount -= controllerItemToRemove.size();

	_controller->onScrollPosition(true);
	updateIndicatorPosition();
}

void RecyclerScroll::onItemRemoved(const Item *item) {
	if (_listener) {
		_listener->removeItem(Source::Id(item->getId()), _categoryLookupLevel, _itemsForSubcats);
	}
}

NS_MD_END
