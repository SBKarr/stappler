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
#include "MaterialLabel.h"

#include "RTView.h"

#include "SPGestureListener.h"
#include "SPDrawPathNode.h"

NS_RT_BEGIN

bool View::PageWithLabel::init(const PageData &data, float density) {
	if (!Page::init(data, density)) {
		return false;
	}

	auto label = Rc<material::Label>::create(material::FontType::Caption);
	label->setString(toString(data.num + 1));
	label->setPosition(Vec2(4.0f, 0.0f));
	_label = addChildNode(label, 1);

	return true;
}

void View::PageWithLabel::onContentSizeDirty() {
	Page::onContentSizeDirty();
	if (_data.isSplit) {
		if (_data.num % 2 == 0) {
			_label->setAnchorPoint(Vec2(0.0f, 0.0f));
			_label->setPosition(Vec2(_data.margin.left, -2.0f));
		} else {
			_label->setAnchorPoint(Vec2(1.0f, 0.0f));
			_label->setPosition(Vec2(_contentSize.width - _data.margin.right, -2.0f));
		}
	} else {
		_label->setAnchorPoint(Vec2(0.5f, 0.0f));
		_label->setPosition(Vec2(_contentSize.width / 2.0f, -2.0f));
	}
}


bool View::Highlight::init(View *view) {
	if (!DynamicBatchNode::init()) {
		return false;
	}

	_view = view;

	setColor(material::Color::Teal_500);
	setOpacity(48);

	_blendFunc = cocos2d::BlendFunc::ALPHA_NON_PREMULTIPLIED;
	setOpacityModifyRGB(false);
	setCascadeOpacityEnabled(false);

	return true;
}
void View::Highlight::visit(cocos2d::Renderer *r, const Mat4 &t, uint32_t f, ZPath &z) {
	if (!_visible) {
		return;
	}

	if (_dirty) {
		updateRects();
	}

	DynamicBatchNode::visit(r, t, f, z);
}

void View::Highlight::clearSelection() {
	_selectionBounds.clear();
	setDirty();
}
void View::Highlight::addSelection(const Pair<SelectionPosition, SelectionPosition> &p) {
	_selectionBounds.push_back(p);
	setDirty();
}

void View::Highlight::setDirty() {
	_dirty = true;
	_quads->clear();
}

void View::Highlight::emplaceRect(const Rect &rect, size_t idx, size_t count) {
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
}
void View::Highlight::updateRects() {
	auto res = _view->getResult();
	if (res) {
		_quads->clear();
		Vector<Rect> rects;
		for (auto &it : _selectionBounds) {
			if (it.first.object == it.second.object) {
				auto obj = res->getObject(it.first.object);
				if (obj && obj->isLabel()) {
					auto l = obj->asLabel();
					uint32_t size = uint32_t(l->format.chars.size() - 1);
					l->getLabelRects(rects,std::min(it.first.position,size),std::min(it.second.position,size), screen::density(), obj->bbox.origin, Padding());
				}
			} else {
				auto firstObj = res->getObject(it.first.object);
				auto secondObj = res->getObject(it.second.object);
				if (firstObj && firstObj->isLabel()) {
					auto label = firstObj->asLabel();
					uint32_t size = uint32_t(label->format.chars.size() - 1);
					label->getLabelRects(rects, std::min(it.first.position,size), size, screen::density(), firstObj->bbox.origin, Padding());
				}
				for (size_t i = it.first.object + 1; i < it.second.object; ++i) {
					auto obj = res->getObject(i);
					if (obj && obj->isLabel()) {
						auto label = obj->asLabel();
						label->getLabelRects(rects, 0, uint32_t(label->format.chars.size() - 1), screen::density(), obj->bbox.origin, Padding());
					}
				}
				if (secondObj && secondObj->isLabel()) {
					auto label = secondObj->asLabel();
					uint32_t size = uint32_t(label->format.chars.size() - 1);
					label->getLabelRects(rects, 0, std::min(it.second.position,size), screen::density(), secondObj->bbox.origin, Padding());
				}
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

NS_RT_END
