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
#include "MaterialFormField.h"

#include "SPLayer.h"
#include "SPStrictNode.h"
#include "SPActions.h"
#include "SPGestureListener.h"
#include "SPScrollView.h"

NS_MD_BEGIN

bool FormField::init(bool dense) {
	return init(dense);
}

bool FormField::init(FormController *c, const String &name, bool dense = false) {
	if (!InputField::init(dense?FontType::InputDense:FontType::Input)) {
		return false;
	}

	_formController = c;
	_formName = name;

	_dense = dense;
	_padding = _dense?Padding(12.0f, 12.0f):Padding(12.0f, 16.0f);
	_label->setAnchorPoint(Vec2(0.0f, 1.0f));

	_counter = construct<Label>(FontType::Caption);
	_counter->setAnchorPoint(Vec2(1.0f, 1.0f));
	_counter->setVisible(false);
	addChild(_counter, 1);

	_error = construct<Label>(FontType::Caption);
	_error->setAnchorPoint(Vec2(0.0f, 1.0f));
	_error->setVisible(false);
	addChild(_error, 1);

	_underlineLayer = construct<Layer>(material::Color::Grey_500);
	_underlineLayer->setOpacity(64);
	_underlineLayer->setAnchorPoint(Vec2(0.0f, 0.0f));
	addChild(_underlineLayer, 1);

	_labelHeight = nan();
	updateLabelHeight(1.0f + _padding.horizontal());

	if (_formController) {
		auto el = Rc<EventListener>::create();
		el->onEventWithObject(FormController::onForceCollect, _formController, [this] (const Event *) {
			pushFormData();
		});
		el->onEventWithObject(FormController::onForceUpdate, _formController, [this] (const Event *) {
			updateFormData();
		});
	}

	return true;
}

void FormField::onContentSizeDirty() {
	InputField::onContentSizeDirty();

	float extraLine = 0.0f;
	if (_error->isVisible() || _counter->isVisible()) {
		extraLine = _counter->getFontHeight() / _counter->getDensity() + 4.0f;
	}

	_underlineLayer->setContentSize(Size(_contentSize.width - _padding.horizontal(), 2.0f));
	_underlineLayer->setPosition(_padding.left, _padding.bottom - 4.0f + extraLine);

	_node->setContentSize(Size(_contentSize.width - _padding.horizontal(), _labelHeight));
	_node->setPosition(_padding.left, _padding.bottom + extraLine);

	_label->setPosition(0, _node->getContentSize().height);
	_label->tryUpdateLabel();

	_error->setPosition(_padding.left, 4.0f);
	_counter->setPosition(_contentSize.width - _padding.right, 4.0f);

	_placeholder->stopAllActions();
	if (_label->empty()) {
		_placeholder->setPosition(_node->getPosition());
		_placeholder->setFontSize(_label->getFontSize());
	} else {
		_placeholder->setPosition(_node->getPosition() + Vec2(0.0f, _node->getContentSize().height + (_dense?4.0f:8.0f)));
		_placeholder->setFontSize(12);
	}

	setMenuPosition(Vec2(_contentSize.width / 2.0f, _node->getPositionY() + _node->getContentSize().height + 6.0f));

	onInput();
}

void FormField::setContentSize(const Size &size) {
	if (size.width != _contentSize.width) {
		_labelHeight = nan();
		updateLabelHeight(size.width);
	}
}

void FormField::updateLabelHeight(float width) {
	if (isnan(width)) {
		width = _contentSize.width;
	}
	auto h = _label->getContentSize().height;
	if (h != _labelHeight) {
		_labelHeight = h;
		InputField::setContentSize(getSizeForLabelWidth(width, h));
		if (_sizeCallback) {
			_sizeCallback(_contentSize);
		}
	}
}

Size FormField::getSizeForLabelWidth(float width, float labelHeight) {
	return Size(width, labelHeight + _padding.vertical() + _counter->getFontHeight() / _counter->getDensity() + (_dense?4.0f:8.0f)
			+ ((_counter->isVisible() || _error->isVisible())?(_counter->getFontHeight() / _counter->getDensity() + 4.0f):0.0f));
}

void FormField::setMaxChars(size_t max) {
	InputField::setMaxChars(max);
}

void FormField::setSizeCallback(const SizeCallback &cb) {
	_sizeCallback = cb;
}
const FormField::SizeCallback &FormField::getSizeCallback() const {
	return _sizeCallback;
}

void FormField::setCounterEnabled(bool value) {
	if (_counterEnabled != value) {
		_counterEnabled = value;
		_labelHeight = 0.0f;
		updateLabelHeight();
	}
}

bool FormField::isCounterEnabled() const {
	return _counterEnabled;
}

void FormField::onError(Error err) {
	InputField::onError(err);
	_underlineLayer->setColor(_errorColor);
	_placeholder->setColor(_errorColor);
}

void FormField::onInput() {
	InputField::onInput();
	if (_label->isActive()) {
		_underlineLayer->setOpacity(168);
		_underlineLayer->setColor(_normalColor);
		_placeholder->setColor(_normalColor);
	}
}

void FormField::onActivated(bool active) {
	InputField::onActivated(active);
	if (_underlineLayer) {
		if (active) {
			_underlineLayer->setOpacity(168);
			_underlineLayer->setColor(_normalColor);
			_placeholder->setColor(_normalColor);
			_placeholder->stopAllActions();

			auto origPos = _placeholder->getPosition();
			auto size = _placeholder->getFontSize();
			_placeholder->runAction(cocos2d::EaseQuadraticActionInOut::create(construct<ProgressAction>(0.15f, [this, origPos, size] (ProgressAction *, float p) {
				_placeholder->setPosition(progress(origPos, _node->getPosition() + Vec2(0.0f, _node->getContentSize().height + (_dense?4.0f:8.0f)), p));
				_placeholder->setFontSize(progress(size, uint8_t(12), p));
			})));
		} else {
			_underlineLayer->setOpacity(64);
			_underlineLayer->setColor(Color::Grey_500);
			_placeholder->setColor(Color::Grey_500);
			_placeholder->stopAllActions();

			if (_label->empty()) {
				_placeholder->runAction(cocos2d::EaseQuadraticActionInOut::create(construct<ProgressAction>(0.15f, [this] (ProgressAction *, float p) {
					_placeholder->setPosition(_node->getPosition() + progress(Vec2(0.0f, _node->getContentSize().height + (_dense?4.0f:8.0f)), Vec2(0.0f, 0.0f), p));
					_placeholder->setFontSize(progress(uint8_t(12), _label->getFontSize(), p));
				})));
			} else {
				_placeholder->setPosition(_node->getPosition() + Vec2(0.0f, _node->getContentSize().height + (_dense?4.0f:8.0f)));
				_placeholder->setFontSize(12);
			}
		}
	}
	if (!active) {
		pushFormData();
	}
}

void FormField::onEnter() {
	InputField::onEnter();
	updateFormData();
	updateAutoAdjust();

}
void FormField::onExit() {
	if (_label->isActive()) {
		pushFormData();
	}
	InputField::onExit();
	updateAutoAdjust();
}

void FormField::onCursor(const Cursor &c) {
	InputField::onCursor(c);

	if (!_gestureListener->hasGesture(gesture::Type::Swipe) && _label->isActive()) {
		auto pos = _label->getCursorMarkPosition();

		if (_adjustScroll) {
			auto root = _adjustScroll->getRoot();
			auto tmp = node::chainNodeToParent(root, _label, false);
			Vec3 vec3(pos.x, pos.y, 0);
			Vec3 ret;
			tmp.transformPoint(vec3, &ret);

			auto scrollPos = _adjustScroll->getNodeScrollPosition(Vec2(ret.x, -ret.y));
			_adjustScroll->runAdjust(scrollPos);
		}
	}
}

void FormField::setAutoAdjust(bool value) {
	if (_autoAdjust != value) {
		_autoAdjust = value;
		updateAutoAdjust();
	}
}
bool FormField::isAutoAdjust() const {
	return _autoAdjust;
}

void FormField::updateAutoAdjust() {
	_adjustScroll = nullptr;
	if (isRunning() && _autoAdjust) {
		for (auto p = getParent(); p != nullptr; p = p->getParent()) {
			if (auto s = dynamic_cast<ScrollView *>(p)) {
				if (s->isVertical()) {
					_adjustScroll = s;
				}
				break;
			}
		}
	}
}

void FormField::pushFormData() {
	if (_formController) {
		_formController->setValue(_formName, data::Value(string::toUtf8(_label->getString())));
	}
}
void FormField::updateFormData() {
	auto value = _formController->getValue(_formName);
	setString(value.asString());
}

NS_MD_END
