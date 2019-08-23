// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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
#include "MaterialResourceManager.h"

#include "RTListenerView.h"
#include "RTRenderer.h"

#include "SPEventListener.h"
#include "SPLayer.h"
#include "SPDevice.h"

NS_RT_BEGIN

SP_DECLARE_EVENT_CLASS(ListenerView, onSelection);
SP_DECLARE_EVENT_CLASS(ListenerView, onExternalLink);

bool ListenerView::Selection::init(ListenerView *view) {
	if (!DynamicBatchNode::init()) {
		return false;
	}

	auto markerStart = Rc<draw::PathNode>::create(48, 48);
	markerStart->addPath(draw::Path()
		.moveTo(48, 0)
		.lineTo(24, 0)
		.arcTo(24, 24, 0, true, false, 48, 24)
		.closePath());
	markerStart->setContentSize(Size(24.0f, 24.0f));
	markerStart->setAnchorPoint(Vec2(1.0f, 1.0f));
	markerStart->setColor(material::Color::Blue_500);
	markerStart->setOpacity(192);
	markerStart->setVisible(false);
	_markerStart = addChildNode(markerStart, 1);

	auto markerEnd = Rc<draw::PathNode>::create(48, 48);
	markerEnd->addPath(draw::Path()
		.moveTo(0, 0)
		.lineTo(0, 24)
		.arcTo(24, 24, 0, true, false, 24, 0)
		.closePath());
	markerEnd->setContentSize(Size(24.0f, 24.0f));
	markerEnd->setAnchorPoint(Vec2(0.0f, 1.0f));
	markerEnd->setColor(material::Color::Blue_500);
	markerEnd->setOpacity(192);
	markerEnd->setVisible(false);
	_markerEnd = addChildNode(markerEnd, 2);

	_view = view;

	setColor(material::Color::Blue_400);
	setOpacity(48);

	_blendFunc = cocos2d::BlendFunc::ALPHA_NON_PREMULTIPLIED;
	setOpacityModifyRGB(false);
	setCascadeOpacityEnabled(false);

	_selectionBounds.first = SelectionPosition{maxOf<size_t>(), 0};
	_selectionBounds.second = SelectionPosition{maxOf<size_t>(), 0};

	return true;
}

void ListenerView::Selection::clearSelection() {
	_index = 0;
	_object = nullptr;
	_quads->clear();

	_markerStart->setVisible(false);
	_markerEnd->setVisible(false);

	_selectionBounds.first = SelectionPosition{maxOf<size_t>(), 0};
	_selectionBounds.second = SelectionPosition{maxOf<size_t>(), 0};

	setEnabled(false);
}
void ListenerView::Selection::selectLabel(const rich_text::Object *obj, const Vec2 &loc) {
	if (auto label = obj->asLabel()) {
		_index = obj->index;
		_object = obj;

		if (_mode != SelectMode::Full) {
			_selectionBounds.first = SelectionPosition{_object->index, 0};
			_selectionBounds.second = SelectionPosition{_object->index, uint32_t(label->format.chars.size() - 1)};
		} else {
			const float d = screen::density();
			Vec2 locInLabel((loc - obj->bbox.origin));

			uint32_t charNumber = label->format.selectChar(int32_t(roundf(locInLabel.x * d)), int32_t(roundf(locInLabel.y * d)));
			if (charNumber == maxOf<uint32_t>()) {
				clearSelection();
				return;
			}

			auto b = label->format.selectWord(charNumber);
			_selectionBounds = pair(SelectionPosition{obj->index, b.first}, SelectionPosition{obj->index, b.second});

		}
		updateRects();
		setEnabled(true);
	}
}

void ListenerView::Selection::selectWholeLabel() {
	if (_object) {
		if (auto label = _object->asLabel()) {
			_selectionBounds.first = SelectionPosition{_object->index, 0};
			_selectionBounds.second = SelectionPosition{_object->index, uint32_t(label->format.chars.size() - 1)};

			updateRects();
		}
	}
}

void ListenerView::Selection::emplaceRect(const Rect &rect, size_t idx, size_t count) {
	Vec2 origin;
	if (_view->isVertical()) {
		origin = Vec2(rect.origin.x, - rect.origin.y - _view->getObjectsOffset() - rect.size.height);
	} else {
		auto res = _view->getResult();
		if (res) {
			size_t page = size_t(floorf((rect.origin.y + rect.size.height) / res->getMedia().surfaceSize.height));
			auto pageData = res->getPageData(page, 0);

			origin = Vec2(rect.origin.x + pageData.viewRect.origin.x + pageData.margin.left,
					pageData.margin.bottom + (pageData.texRect.size.height - rect.origin.y - rect.size.height + pageData.texRect.origin.y));
		} else {
			return;
		}
	}

	auto id = _quads->emplace();
	_quads->setGeometry(id, origin, rect.size, 0.0f);

	if (_mode == SelectMode::Full) {
		if (idx == 0) {
			_markerStart->setPosition(origin);
			_markerStart->setVisible(true);
		}
		if (idx + 1 == count) {
			_markerEnd->setPosition(origin + Vec2(rect.size.width, 0));
			_markerEnd->setVisible(true);
		}
	}
}

void ListenerView::Selection::updateRects() {
	auto res = _view->getResult();
	if (res) {
		_quads->clear();
		Vector<Rect> rects;
		if (_selectionBounds.first.object == _selectionBounds.second.object) {
			auto obj = res->getObject(_selectionBounds.first.object);
			if (obj && obj->isLabel()) {
				obj->asLabel()->getLabelRects(rects, _selectionBounds.first.position, _selectionBounds.second.position, screen::density(), obj->bbox.origin, Padding());
			}
		} else {
			auto firstObj = res->getObject(_selectionBounds.first.object);
			auto secondObj = res->getObject(_selectionBounds.second.object);

			if (firstObj && firstObj->isLabel()) {
				auto label = firstObj->asLabel();
				label->getLabelRects(rects, _selectionBounds.first.position, uint32_t(label->format.chars.size() - 1), screen::density(), firstObj->bbox.origin, Padding());
			}
			for (size_t i = _selectionBounds.first.object + 1; i < _selectionBounds.second.object; ++i) {
				auto obj = res->getObject(i);
				if (obj && obj->isLabel()) {
					auto label = obj->asLabel();
					label->getLabelRects(rects, 0, uint32_t(label->format.chars.size() - 1), screen::density(), obj->bbox.origin, Padding());
				}
			}
			if (secondObj && secondObj->isLabel()) {
				auto label = secondObj->asLabel();
				label->getLabelRects(rects, 0, _selectionBounds.second.position, screen::density(), secondObj->bbox.origin, Padding());
			}
		}

		_quads->setCapacity(rects.size());
		size_t rectIdx = 0;
		for (auto &it : rects) {
			emplaceRect(it, rectIdx, rects.size());
			++ rectIdx;
		}
		updateColor();
		updateBlendFunc(nullptr);
	}
}

bool ListenerView::Selection::onTap(int, const Vec2 &vec) {
	return true;
}

bool ListenerView::Selection::onPressBegin(const Vec2 &) {
	return true;
}
bool ListenerView::Selection::onLongPress(const Vec2 &vec, const TimeInterval &, int count) {
	if (!_markerTarget) {
		auto res = _view->getResult();
		if (res && count == 1) {
			auto loc = _view->convertToObjectSpace(vec);
			if (auto obj = getSelectedObject(res, loc)) {
				selectLabel(obj, loc);
			} else {
				clearSelection();
			}
		}
		if (count > 2 && _mode == SelectMode::Full) {
			selectWholeLabel();
			return false;
		}
	}
	return true;
}
bool ListenerView::Selection::onPressEnd(const Vec2 &vec, const TimeInterval &time) {
	if (isEnabled() && time < TimeInterval::milliseconds(425)) {
		auto loc = _view->convertToObjectSpace(vec);
		auto obj = getSelectedObject(_view->getResult(), loc);
		const float d = screen::density();
		if (!obj) {
			clearSelection();
			return true;
		}

		if (_mode != SelectMode::Full) {
			if (obj->index != _selectionBounds.first.object) {
				clearSelection();
			}
			return true;
		}

		auto label = obj->asLabel();
		Vec2 locInLabel((loc - obj->bbox.origin));
		uint32_t charNumber = label->format.selectChar(int32_t(roundf(locInLabel.x * d)), int32_t(roundf(locInLabel.y * d)));

		if (charNumber == maxOf<uint32_t>()) {
			clearSelection();
			return true;
		}

		if (_selectionBounds.first.object == _selectionBounds.second.object && obj->index == _selectionBounds.first.object) {
			if (charNumber < _selectionBounds.first.position) {
				_selectionBounds.first.position = charNumber;
				updateRects();
			} else if (charNumber > _selectionBounds.second.position) {
				_selectionBounds.second.position = charNumber;
				updateRects();
			} else if (charNumber != _selectionBounds.first.position && charNumber != _selectionBounds.second.position) {
				if (charNumber - _selectionBounds.first.position < _selectionBounds.second.position - charNumber) {
					_selectionBounds.first.position = charNumber;
					updateRects();
				} else {
					_selectionBounds.second.position = charNumber;
					updateRects();
				}
			}
		} else if (_selectionBounds.first.object >= obj->index && (_selectionBounds.first.object - obj->index) < 10) {
			_selectionBounds.first.object = obj->index;
			_selectionBounds.first.position = charNumber;
			updateRects();
		} else if (_selectionBounds.second.object <= obj->index && (obj->index - _selectionBounds.second.object) < 10) {
			_selectionBounds.second.object = obj->index;
			_selectionBounds.second.position = charNumber;
			updateRects();
		} else if (_selectionBounds.first.object < obj->index && obj->index < _selectionBounds.second.object) {
			if (obj->index - _selectionBounds.first.object < _selectionBounds.second.object - obj->index) {
				_selectionBounds.first.object = obj->index;
				_selectionBounds.first.position = charNumber;
				updateRects();
			} else {
				_selectionBounds.second.object = obj->index;
				_selectionBounds.second.position = charNumber;
				updateRects();
			}
		} else {
			clearSelection();
		}
	}
	return true;
}
bool ListenerView::Selection::onPressCancel(const Vec2 &) {
	return true;
}

bool ListenerView::Selection::onSwipeBegin(const Vec2 &loc) {
	if (_enabled) {
		if (node::isTouched(_markerStart, loc, 8.0f)) {
			_markerTarget = _markerStart;
			return true;
		} else if (node::isTouched(_markerEnd, loc, 8.0f)) {
			_markerTarget = _markerEnd;
			return true;
		}
	}
	return false;
}
bool ListenerView::Selection::onSwipe(const Vec2 &vec, const Vec2 &d) {
	if (!_markerTarget) {
		return false;
	}

	const float density = screen::density();
	auto size = _markerTarget->getContentSize();
	auto anchor = _markerTarget->getAnchorPoint();
	auto offset = Vec2(anchor.x * size.width - size.width / 2.0f, anchor.y * size.height * 1.35f);
	auto loc = _view->convertToObjectSpace(vec + offset * density);

	if (!_object || _object->type != layout::Object::Type::Label || !_object->bbox.containsPoint(loc)) {
		auto obj = getSelectedObject(_view->getResult(), loc,
				(_markerTarget == _markerStart?_selectionBounds.second.object:_selectionBounds.first.object),
				int32_t((_markerTarget == _markerStart?-1:1) * (_selectionBounds.second.object - _selectionBounds.first.object + 12)));
		if (!obj) {
			return true;
		} else {
			if (obj->type == layout::Object::Type::Label) {
				_object = obj;
			} else {
				return true;
			}
		}
	}

	if (_object && _object->type == layout::Object::Type::Label) {
		auto label = _object->asLabel();
		Vec2 locInLabel((loc - _object->bbox.origin));

		if (_markerTarget == _markerStart) {
			uint32_t charNumber = label->format.selectChar(int32_t(roundf(locInLabel.x * density)), int32_t(roundf(locInLabel.y * density)), font::FormatSpec::Prefix);
			if (charNumber != maxOf<uint32_t>()) {
				if (_object->index != _selectionBounds.second.object ||
						(charNumber != _selectionBounds.first.position && charNumber <= _selectionBounds.second.position)) {
					_selectionBounds.first.object = _object->index;
					_selectionBounds.first.position = charNumber;
					updateRects();
				}
			}
		} else {
			uint32_t charNumber = label->format.selectChar(int32_t(roundf(locInLabel.x * density)), int32_t(roundf(locInLabel.y * density)), font::FormatSpec::Suffix);
			if (charNumber != maxOf<uint32_t>()) {
				if (_object->index != _selectionBounds.first.object ||
						(charNumber != _selectionBounds.second.position && charNumber >= _selectionBounds.first.position)) {
					_selectionBounds.second.object = _object->index;
					_selectionBounds.second.position = charNumber;
					updateRects();
				}
			}
		}
	}

	return true;
}
bool ListenerView::Selection::onSwipeEnd(const Vec2 &v) {
	if (!_markerTarget) {
		return false;
	}

	_markerTarget = nullptr;
	return true;
}

void ListenerView::Selection::setEnabled(bool value) {
	if (value != _enabled) {
		_enabled = value;
		onSelection(_view, value);
	}
}

bool ListenerView::Selection::isEnabled() const {
	return _enabled;
}

void ListenerView::Selection::setMode(SelectMode m) {
	_mode = m;
}
ListenerView::SelectMode ListenerView::Selection::getMode() const {
	return _mode;
}

bool ListenerView::Selection::hasSelection() const {
	return _selectionBounds.first.object != maxOf<size_t>() && _selectionBounds.first.object != maxOf<size_t>();
}

static void pushString(StringStream &stream, const WideString &str) {
	WideStringView r(str);
	r.skipChars<chars::CharGroup<char16_t, CharGroupId::OpticalAlignmentBullet>>();
	if (r.empty()) {
		stream << string::toUtf8(str) << " ";
	} else {
		stream << string::toUtf8(str) << "\r\n";
	}
}

String ListenerView::Selection::getSelectedString(size_t maxWords) const {
	if (hasSelection()) {
		auto res = _view->getResult();
		if (res) {
			if (_selectionBounds.first.object == _selectionBounds.second.object) {
				auto obj = res->getObject(_selectionBounds.first.object);
				if (obj && obj->isLabel()) {
					return string::toUtf8(obj->asLabel()->format.str(_selectionBounds.first.position, _selectionBounds.second.position, maxWords, true));
				}
			} else if (maxWords != maxOf<size_t>()) {
				auto obj = res->getObject(_selectionBounds.first.object);
				if (obj && obj->isLabel()) {
					auto l = obj->asLabel();
					return string::toUtf8(l->format.str(_selectionBounds.first.position, uint32_t(l->format.chars.size() - 1), maxWords, true));
				}
			} else {
				StringStream ret;
				auto firstObj = res->getObject(_selectionBounds.first.object);
				auto lastObj = res->getObject(_selectionBounds.second.object);

				String str;
				if (firstObj && firstObj->isLabel()) {
					auto l = firstObj->asLabel();
					pushString(ret, l->format.str(_selectionBounds.first.position, uint32_t(l->format.chars.size() - 1), maxWords, true));
				}

				for (size_t i = _selectionBounds.first.object + 1; i < _selectionBounds.second.object; ++i) {
					auto obj = res->getObject(i);
					if (obj && obj->isLabel()) {
						auto l = obj->asLabel();
						pushString(ret, l->format.str(0, uint32_t(l->format.chars.size() - 1), maxWords, true));
					}
				}

				if (lastObj && lastObj->isLabel()) {
					auto l = lastObj->asLabel();
					pushString(ret, l->format.str(0, _selectionBounds.second.position, maxWords, true));
				}

				return ret.str();
			}
		}
	}
	return String();
}

Pair<ListenerView::SelectionPosition, ListenerView::SelectionPosition> ListenerView::Selection::getSelectionPosition() const {
	return _selectionBounds;
}


StringView ListenerView::Selection::getSelectedHash() const {
	if (auto res = _view->getResult()) {
		if (_mode != SelectMode::Full) {
			auto obj = res->getObject(_selectionBounds.first.object);
			if (obj->isLabel()) {
				return obj->asLabel()->hash;
			}
		}
	}
	return StringView();
}

size_t ListenerView::Selection::getSelectedSourceIndex() const {
	if (auto res = _view->getResult()) {
		if (_mode != SelectMode::Full) {
			auto obj = res->getObject(_selectionBounds.first.object);
			if (obj->isLabel()) {
				return obj->asLabel()->sourceIndex;
			}
		}
	}
	return 0;
}

const rich_text::Object *ListenerView::Selection::getSelectedObject(rich_text::Result *res, const Vec2 &loc) const {
	auto &objs = res->getObjects();
	for (auto &it : objs) {
		if (it->isLabel()) {
			if (_mode != SelectMode::Indexed || (!it->asLabel()->hash.empty() && it->asLabel()->sourceIndex != 0)) {
				if (loc.x >= it->bbox.getMinX() - 8.0f && loc.x <= it->bbox.getMaxX() + 8.0f
						&& loc.y >= it->bbox.getMinY() - 8.0f && loc.y <= it->bbox.getMaxY() + 8.0f) {
					return it;
				}
			}
		}
	}
	return nullptr;
}

const rich_text::Object *ListenerView::Selection::getSelectedObject(rich_text::Result *res, const Vec2 &loc, size_t pos, int32_t offset) const {
	auto &objs = res->getObjects();
	if (offset < 0 && pos > 0) {
		if (-offset > int32_t(pos)) {
			offset = int32_t(-pos);
		}

		auto end = objs.begin() + pos + 1;
		auto start = objs.begin() + (pos + offset);
		for (auto it = start; it != end; ++ it) {
			if ((*it)->isLabel()) {
				if (loc.x >= (*it)->bbox.getMinX() - 8.0f && loc.x <= (*it)->bbox.getMaxX() + 8.0f
						&& loc.y >= (*it)->bbox.getMinY() - 8.0f && loc.y <= (*it)->bbox.getMaxY() + 8.0f) {
					return (*it);
				}
			}
		}

	} else if (offset > 0 && pos < objs.size()) {
		if (pos + offset > objs.size()) {
			offset = int32_t(objs.size() - pos);
		}

		auto start = objs.begin() + pos;
		auto end = objs.begin() + (pos + 1 + offset);
		for (auto it = start; it != end; ++ it) {
			if ((*it)->isLabel()) {
				if (loc.x >= (*it)->bbox.getMinX() - 8.0f && loc.x <= (*it)->bbox.getMaxX() + 8.0f
						&& loc.y >= (*it)->bbox.getMinY() - 8.0f && loc.y <= (*it)->bbox.getMaxY() + 8.0f) {
					return (*it);
				}
			}
		}
	}
	return nullptr;
}

ListenerView::~ListenerView() { }

bool ListenerView::init(Layout l, CommonSource *source, const Vector<String> &ids) {
	auto eventListener = Rc<EventListener>::create();
	_eventListener = eventListener;

	if (!CommonView::init(l, source, ids)) {
		return false;
	}

	eventListener->onEvent(material::ResourceManager::onLightLevel, std::bind(&ListenerView::onLightLevelChanged, this));
	_eventListener = addComponentItem(eventListener);

	auto selection = Rc<Selection>::create(this);
	_selection = addChildNode(selection, 10);

	onLightLevelChanged();

	return true;
}

void ListenerView::setUseSelection(bool value) {
	_useSelection = value;
}

void ListenerView::onExit() {
	if (isSelectionEnabled()) {
		disableSelection();
	}
	CommonView::onExit();
}

void ListenerView::setLayout(Layout l) {
	_selection->clearSelection();
	CommonView::setLayout(l);
}

void ListenerView::onLightLevelChanged() {
	auto level = material::ResourceManager::getInstance()->getLightLevel();
	_renderer->setLightLevelValue(level);

	switch(level) {
	case material::ResourceManager::LightLevel::Washed:
		_background->setColor(material::Color::White);
		break;
	case material::ResourceManager::LightLevel::Normal:
		_background->setColor(material::Color::Grey_200);
		break;
	case material::ResourceManager::LightLevel::Dim:
		_background->setColor(material::Color::Grey_800);
		break;
	}
}

void ListenerView::onTap(int count, const Vec2 &vec) {
	if (count == 1 && _tapCallback) {
		if (_linksEnabled) {
			if (auto res = getResult()) {
				auto loc = convertToObjectSpace(vec);
				auto &objs = res->getRefs();
				for (auto &it : objs) {
					if (isObjectTapped(loc, *it)) {
						return;
					}
				}
			}
		}

		_tapCallback(count, vec);
	}
}

void ListenerView::onObjectPressEnd(const Vec2 &vec, const rich_text::Object &obj) {
	if (auto ref = obj.asLink()) {
		onLink(ref->target, ref->mode, vec);
	}
}

void ListenerView::onLink(const StringView &ref, const StringView &target, const Vec2 &pos) {
	if (ref.starts_with("http://") || ref.starts_with( "https://")) {
		stappler::Device::getInstance()->goToUrl(ref);
		onExternalLink(this, ref);
	} else if (ref.starts_with("mailto:") == 0) {
		stappler::Device::getInstance()->mailTo(ref);
		onExternalLink(this, ref);
	} else if (ref.starts_with("tel:") == 0) {
		stappler::Device::getInstance()->makePhoneCall(ref);
		onExternalLink(this, ref);
	}
}

bool ListenerView::onSwipeEventBegin(const Vec2 &loc, const Vec2 &d, const Vec2 &v) {
	if (_useSelection && _selection->isEnabled() && _selection->onSwipeBegin(loc)) {
		return _selection->onSwipe(loc, d);
	}
	return CommonView::onSwipeEventBegin(loc, d, v);
}
bool ListenerView::onSwipeEvent(const Vec2 &loc, const Vec2 &d, const Vec2 &v) {
	if (_useSelection && _selection->isEnabled() && _selection->onSwipe(loc, d)) {
		return true;
	}
	return CommonView::onSwipeEvent(loc, d, v);
}
bool ListenerView::onSwipeEventEnd(const Vec2 &loc, const Vec2 &d, const Vec2 &v) {
	if (_useSelection && _selection->isEnabled() && _selection->onSwipeEnd(v)) {
		return true;
	}
	return CommonView::onSwipeEventEnd(loc, d, v);
}

bool ListenerView::onPressBegin(const Vec2 &loc) {
	if (_useSelection && _selection->isEnabled()) {
		return _selection->onPressBegin(loc);
	}
	return CommonView::onPressBegin(loc);
}

bool ListenerView::onLongPress(const Vec2 &vec, const TimeInterval &interval, int count) {
	if (CommonView::onLongPress(vec, interval, count) && _useSelection) {
		return _selection->onLongPress(vec, interval, count);
	}
	return false;
}

bool ListenerView::onPressEnd(const Vec2 &loc, const TimeInterval &time) {
	if (_useSelection && _selection->isEnabled()) {
		return _selection->onPressEnd(loc, time);
	}
	return CommonView::onPressEnd(loc, time);
}
bool ListenerView::onPressCancel(const Vec2 &loc, const TimeInterval &time) {
	if (_useSelection && _selection->isEnabled()) {
		return _selection->onPressCancel(loc);
	}
	return CommonView::onPressCancel(loc, time);
}

void ListenerView::disableSelection() {
	_selection->clearSelection();
}
bool ListenerView::isSelectionEnabled() const {
	return _selection->isEnabled();
}

void ListenerView::setSelectMode(SelectMode m) {
	_selection->setMode(m);
}
ListenerView::SelectMode ListenerView::getSelectMode() const {
	return _selection->getMode();
}

String ListenerView::getSelectedString(size_t maxWords) const {
	return _selection->getSelectedString(maxWords);
}
Pair<ListenerView::SelectionPosition, ListenerView::SelectionPosition> ListenerView::getSelectionPosition() const {
	return _selection->getSelectionPosition();
}

StringView ListenerView::getSelectedHash() const {
	return _selection->getSelectedHash();
}
size_t ListenerView::getSelectedSourceIndex() const {
	return _selection->getSelectedSourceIndex();
}

void ListenerView::onPosition() {
	_selection->setPosition(_root->getPosition());
	_selection->setContentSize(_root->getContentSize());
	CommonView::onPosition();
}

void ListenerView::onRenderer(rich_text::Result *res, bool status) {
	_selection->clearSelection();
	CommonView::onRenderer(res, status);
}

NS_RT_END
