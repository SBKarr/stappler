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
#include "MaterialTextField.h"
#include "SPStrictNode.h"
#include "SPScrollView.h"

NS_MD_BEGIN

bool TextField::init(bool dense) {
	if (!FormField::init(dense)) {
		return false;
	}

	auto clearButton = Rc<material::ButtonIcon>::create(material::IconName::Navigation_close, [this] {
		setString(String());
	});
	clearButton->setSizeHint(IconSprite::SizeHint::Small);
	clearButton->setIconColor(material::Color::Grey_500);
	clearButton->setContentSize(Size(32.0f, 32.0f));
	clearButton->setAnchorPoint(Anchor::TopRight);
	clearButton->setVisible(false);
	_clearIcon = addChildNode(clearButton, 1);

	return true;
}

bool TextField::init(FormController *c, const String &name, bool dense) {
	if (!FormField::init(c, name, dense)) {
		return false;
	}

	auto clearButton = Rc<material::ButtonIcon>::create(material::IconName::Navigation_close, [this] {
		setString(String());
	});
	clearButton->setSizeHint(IconSprite::SizeHint::Small);
	clearButton->setIconColor(material::Color::Grey_500);
	clearButton->setContentSize(Size(32.0f, 32.0f));
	clearButton->setAnchorPoint(Anchor::TopRight);
	clearButton->setVisible(false);
	_clearIcon = addChildNode(clearButton, 1);

	return true;
}

void TextField::setContentSize(const Size &size) {
	auto w = size.width - _padding.horizontal();
	if (w > 0) {
		_label->setWidth(w);
		_label->tryUpdateLabel();
	}
	FormField::setContentSize(size);
}

void TextField::onContentSizeDirty() {
	FormField::onContentSizeDirty();

	_clearIcon->setPosition(_contentSize.width - 2.0f, _contentSize.height - 2.0f);
}

bool TextField::onSwipe(const Vec2 &loc, const Vec2 &delta) {
	if (FormField::onSwipe(loc, delta)) {
		if (_adjustScroll) {
			auto pos = convertToNodeSpace(loc);
			auto root = _adjustScroll->getRoot();
			auto tmp = node::chainNodeToParent(root, _label);
			Vec3 vec3(pos.x, pos.y, 0);
			Vec3 ret;
			tmp.transformPoint(vec3, &ret);
			pos = Vec2(ret.x, ret.y);

			if (pos.y < 32.0f) {
				_adjustScroll->scheduleAdjust(ScrollView::Adjust::Front, 32.0f - pos.y);
			} else if (pos.y > _adjustScroll->getContentSize().height - 32.0f) {
				_adjustScroll->scheduleAdjust(ScrollView::Adjust::Back, pos.y - _adjustScroll->getContentSize().height + 32.0f);
			} else {
				_adjustScroll->scheduleAdjust(ScrollView::Adjust::None, 0.0f);
			}
		}
		return true;
	}
	return false;
}

bool TextField::onSwipeEnd(const Vec2 &loc) {
	auto ret = FormField::onSwipeEnd(loc);
	if (_adjustScroll) {
		_adjustScroll->scheduleAdjust(ScrollView::Adjust::None, 0.0f);
	}
	/*if (_positionCallback) {
		_positionCallback(Vec2(0, 0), true);
	}*/
	return ret;
}

void TextField::onInput() {
	FormField::onInput();
	updateLabelHeight();
	_clearIcon->setVisible(!_label->empty());
}

void TextField::onMenuVisible() {
	auto pos = _label->getCursorMarkPosition();
	auto &tmp = _node->getNodeToParentTransform();
	Vec3 vec3(pos.x + _label->getPositionX(), pos.y, 0);
	Vec3 ret;
	tmp.transformPoint(vec3, &ret);

	setMenuPosition(Vec2(ret.x, ret.y + _label->getFontHeight() / _label->getDensity() * 1.5f));
}

NS_MD_END
