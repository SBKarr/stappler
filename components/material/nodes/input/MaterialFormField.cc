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
#include "MaterialFormController.h"

#include "SPLayer.h"
#include "SPStrictNode.h"
#include "SPActions.h"
#include "SPGestureListener.h"
#include "SPEventListener.h"
#include "SPScrollView.h"
#include "SPScrollItemHandle.h"

NS_MD_BEGIN

bool FormField::init(bool dense) {
	return init(nullptr, "", dense);
}

bool FormField::init(FormController *c, const String &name, bool dense) {
	if (!InputField::init(dense?FontType::InputDense:FontType::Input)) {
		return false;
	}

	setName(name);

	_dense = dense;
	_padding = _dense?Padding(12.0f, 12.0f):Padding(12.0f, 16.0f);
	_label->setAnchorPoint(Vec2(0.0f, 1.0f));

	auto counter = Rc<Label>::create(FontType::Caption);
	counter->setAnchorPoint(Vec2(1.0f, 1.0f));
	counter->setVisible(false);
	_counter = addChildNode(counter, 1);

	auto error = Rc<Label>::create(FontType::Caption);
	error->setAnchorPoint(Vec2(0.0f, 0.0f));
	error->setVisible(false);
	error->setLocaleEnabled(true);
	error->setColor(_errorColor);
	_error = addChildNode(error, 1);

	auto underlineLayer = Rc<Layer>::create(material::Color::Grey_500);
	underlineLayer->setOpacity(64);
	underlineLayer->setAnchorPoint(Vec2(0.0f, 0.0f));
	_underlineLayer = addChildNode(underlineLayer, 1);

	_labelHeight = nan();
	updateLabelHeight(1.0f + _padding.horizontal());

	auto el = Rc<EventListener>::create();
	_formEventListener = addComponentItem(el);

	auto sh = Rc<ScrollItemHandle>::create();
	_scrollHandle = addComponentItem(sh);

	setFormController(c);

	return true;
}

void FormField::onContentSizeDirty() {
	InputField::onContentSizeDirty();

	float extraLine = 0.0f;
	if (_fullHeight || _error->isVisible() || _counter->isVisible()) {
		extraLine = _counter->getFontHeight() / _counter->getDensity();
	}

	_underlineLayer->setContentSize(Size(_contentSize.width - _padding.horizontal(), 2.0f));
	_underlineLayer->setPosition(_padding.left, _padding.bottom - 4.0f + extraLine);

	_node->setContentSize(Size(_contentSize.width - _padding.horizontal(), _labelHeight));
	_node->setPosition(_padding.left, _padding.bottom + extraLine);

	_label->setPosition(0, _node->getContentSize().height);
	_label->tryUpdateLabel();

	_error->setPosition(_padding.left, 6.0f);
	_counter->setPosition(_contentSize.width - _padding.right, 6.0f);

	_placeholder->stopAllActions();
	if (_label->empty() && !isInputActive()) {
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

void FormField::updateLabelHeight(float width, bool force) {
	if (isnan(width)) {
		width = _contentSize.width;
	}
	auto h = _label->getContentSize().height;
	if (force || h != _labelHeight) {
		_labelHeight = h;
		auto size = getSizeForLabelWidth(width, h);
		InputField::setContentSize(size);
		if (_scrollHandle && _scrollHandle->isConnected()) {
			_scrollHandle->forceResize(size.height);
		}
		if (_sizeCallback) {
			_sizeCallback(_contentSize);
		}
	}
}

Size FormField::getSizeForLabelWidth(float width, float labelHeight) {
	return Size(width, labelHeight + _padding.vertical() + _counter->getFontHeight() / _counter->getDensity() + (_dense?4.0f:8.0f)
			+ ((_fullHeight || _counter->isVisible() || _error->isVisible())?(_counter->getFontHeight() / _counter->getDensity()):0.0f));
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

	switch (err) {
	case InputError::OverflowChars:
		setError("SystemErrorOverflowChars"_locale.to_string());
		break;
	case InputError::InvalidChar:
		setError("SystemErrorInvalidChar"_locale.to_string());
		break;
	}
}

void FormField::onInput() {
	InputField::onInput();
	if (_label->isActive()) {
		_underlineLayer->setOpacity(168);
		_underlineLayer->setColor(_normalColor);
		_placeholder->setColor(_normalColor);
		if (_formController) {
			_formController->clearError(_name);
		} else {
			_error->setString("");
			_error->setVisible(false);
		}
	}
}

void FormField::onActivated(bool active) {
	InputField::onActivated(active);
	if (_underlineLayer) {
		if (active) {
			_underlineLayer->setOpacity(168);
			if (!_error->empty()) {
				_underlineLayer->setColor(_errorColor);
				_placeholder->setColor(_errorColor);
			} else {
				_underlineLayer->setColor(_normalColor);
				_placeholder->setColor(_normalColor);
			}
			_placeholder->stopAllActions();

			auto origPos = _placeholder->getPosition();
			auto size = _placeholder->getFontSize();
			_placeholder->runAction(cocos2d::EaseQuadraticActionInOut::create(
					Rc<ProgressAction>::create(0.15f, [this, origPos, size] (ProgressAction *, float p) {
				_placeholder->setPosition(progress(origPos, _node->getPosition() + Vec2(0.0f, _node->getContentSize().height + (_dense?4.0f:8.0f)), p));
				_placeholder->setFontSize(progress(size, uint16_t(12), p));
			})));
		} else {
			_underlineLayer->setOpacity(64);
			_underlineLayer->setColor(Color::Grey_500);
			_placeholder->setColor(_error->empty()?Color::Grey_500:_errorColor);
			_placeholder->stopAllActions();

			if (_label->empty()) {
				_placeholder->runAction(cocos2d::EaseQuadraticActionInOut::create(
						Rc<ProgressAction>::create(0.15f, [this] (ProgressAction *, float p) {
					_placeholder->setPosition(_node->getPosition() + progress(Vec2(0.0f, _node->getContentSize().height + (_dense?4.0f:8.0f)), Vec2(0.0f, 0.0f), p));
					_placeholder->setFontSize(progress(uint16_t(12), _label->getFontSize(), p));
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
	updateFormData();
	InputField::onEnter();
	updateAutoAdjust();

}
void FormField::onExit() {
	pushFormData();
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

void FormField::setFullHeight(bool value) {
	if (value != _fullHeight) {
		_fullHeight = value;
		_labelHeight = 0.0f;
		updateLabelHeight();
	}
}
bool FormField::isFullHeight() const {
	return _fullHeight;
}

void FormField::setError(const StringView &str) {
	if (str != _error->getString8()) {
		_error->setString(str);
		if (str.empty()) {
			_error->setVisible(false);
			if (_label->isActive()) {
				_underlineLayer->setColor(_normalColor);
				_placeholder->setColor(_normalColor);
			} else {
				_underlineLayer->setColor(Color::Grey_500);
				_placeholder->setColor(Color::Grey_500);
			}
		} else {
			_error->setVisible(true);
			_underlineLayer->setColor(_errorColor);
			_placeholder->setColor(_errorColor);
		}
		updateLabelHeight(_contentSize.width, true);
	}
}
StringView FormField::getError() const {
	return _error->getString8();
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

void FormField::setFormController(FormController *controller) {
	if (_formController != controller) {
		_formEventListener->clear();
		_formController = controller;
		if (_formController) {
			_formEventListener->onEventWithObject(FormController::onEnabled, _formController, [this] (const Event &ev) {
				setEnabled(ev.getBoolValue());
			});
			_formEventListener->onEventWithObject(FormController::onForceCollect, _formController, [this] (const Event &) {
				pushFormData();
			});
			_formEventListener->onEventWithObject(FormController::onForceUpdate, _formController, [this] (const Event &) {
				updateFormData();
			});
			_formEventListener->onEventWithObject(FormController::onErrors, _formController, [this] (const Event &) {
				setError(_formController->getError(_name));
			});
			setEnabled(_formController->isEnabled());
			if (isEnabled()) {
				updateFormData();
			}
			setError(_formController->getError(_name));
		} else {
			setEnabled(true);
		}
	}
}
FormController *FormField::getFormController() const {
	return _formController;
}

void FormField::pushFormData() {
	if (_formController) {
		_formController->setValue(_name, data::Value(string::toUtf8(_label->getString())));
	}
}
void FormField::updateFormData() {
	if (_formController) {
		auto value = _formController->getValue(_name);
		setString(value.asString());
	}
}

NS_MD_END
