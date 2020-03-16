/**
Copyright (c) 2016-2020 Roman Katuntsev <sbkarr@stappler.org>

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
#include "MaterialButton.h"

#include "MaterialLabel.h"
#include "MaterialMenuSource.h"
#include "MaterialFloatingMenu.h"
#include "MaterialResourceManager.h"
#include "MaterialFormController.h"

#include "SPDrawPathNode.h"
#include "SPString.h"
#include "SPRoundedSprite.h"
#include "SPProgressAction.h"
#include "SPGestureListener.h"
#include "SPDataListener.h"
#include "SPActions.h"
#include "SPEventListener.h"
#include "SPLayer.h"

#include "MaterialButtonHighlight.cc"

NS_MD_BEGIN

bool Button::init(const TapCallback &tapCallback, const TapCallback &longTapCallback) {
	if (!MaterialNode::init()) {
		return false;
	}

	auto l = Rc<gesture::Listener>::create();
	l->setGestureFilter([this] (const Vec2 &vec,  gesture::Type t, const gesture::Listener::DefaultGestureFilter &def) -> bool {
		if (!_touchFilter) {
			return def(vec, t);
		} else {
			return _touchFilter(vec);
		}
	});
	l->setPressCallback([this] (stappler::gesture::Event ev, const stappler::gesture::Press &g) -> bool {
		if (ev == stappler::gesture::Event::Began) {
			return onPressBegin(g.location());
		} else if (ev == stappler::gesture::Event::Activated) {
			onLongPress();
			return true;
		} else if (ev == stappler::gesture::Event::Ended) {
			return onPressEnd();
		} else {
			onPressCancel();
			return true;
		}
	});
	l->setSwallowTouches(true);
	_listener = addComponentItem(l);

	_source.setCallback(std::bind(&Button::updateFromSource, this));

	_tapCallback = tapCallback;
	_longTapCallback = longTapCallback;

	auto el = Rc<EventListener>::create();
	_formEventListener = addComponentItem(el);

	auto highlight = Rc<ButtonHighLight>::create(_backgroundClipper, [this] {
		if (_style == Style::Raised) {
			 setLocalZOrder(getLocalZOrder() + 1);
		}
	}, [this] {
		if (_style == Style::Raised) {
			 setLocalZOrder(getLocalZOrder() - 1);
		}
	}, [this] (float pr) {
		if (_style == Style::Raised) {
			setShadowZIndex(progress(_raisedDefaultZIndex, _raisedActiveZIndex, pr));
		}
	});
	_highlight = addChildNode(highlight, 2);

	updateStyle();
	updateEnabled();

	return true;
}

void Button::setStyle(Style style) {
	if (_style != style) {
		_style = style;
		updateStyle();
	}
}

void Button::updateStyle() {
	auto bg = getBackground();
	if (_style == Raised) {
		setShadowZIndex(_raisedDefaultZIndex);
		bg->setOpacity(255);
		bg->setColor(Color::White);
	} else {
		setShadowZIndex(0);
		if (_style == FlatWhite) {
			_highlight->setAnimationColor(Color::White);
		} else {
			_highlight->setAnimationColor(Color::Black);
		}
		bg->setOpacity(0);
	}
}

void Button::updateEnabled() {
	if ((_longTapCallback || _tapCallback || _floatingMenuSource) && _enabled && (!_formController || _formController->isEnabled())) {
		_listener->setEnabled(true);
	} else {
		_listener->setEnabled(false);
	}
}

Button::Style Button::getStyle() const {
	return _style;
}

void Button::setPriority(int32_t value) {
	_listener->setPriority(value);
}
int32_t Button::getPriority() const {
	return _listener->getPriority();
}

void Button::setTouchPadding(float p) {
	_listener->setTouchPadding(p);
}
float Button::getTouchPadding() const {
	return _listener->getTouchPadding();
}

void Button::setTapCallback(const TapCallback & tapCallback) {
	_tapCallback = tapCallback;
	updateEnabled();
}

void Button::setLongTapCallback(const TapCallback & longTapCallback) {
	_longTapCallback = longTapCallback;
	updateEnabled();
}

void Button::setTouchFilter(const TouchFilter & touchFilter) {
	_touchFilter = touchFilter;
}

void Button::setSwallowTouches(bool value) {
	static_cast<stappler::gesture::Listener *>(_listener)->setSwallowTouches(value);
}
void Button::setGesturePriority(int32_t value) {
	static_cast<stappler::gesture::Listener *>(_listener)->setPriority(value);
}
void Button::setAnimationOpacity(uint8_t op) {
	_highlight->setAnimationOpacity(op);
}
void Button::setSpawnDelay(float s) {
	_highlight->setSpawnDelay(s);
}

bool Button::onPressBegin(const Vec2 &location) {
	if (!_enabled) {
		return false;
	}
	if (!_touchFilter || _touchFilter(location)) {
		if (stappler::node::isTouched(this, location, 8)) {
			_touchPoint = location;
			_highlight->animateSelection(location);
			return true;
		}
	}
	return false;
}
void Button::onLongPress() {
	if (_longTapCallback) {
		_longTapCallback();
	}
}
bool Button::onPressEnd() {
	_highlight->animateDeselection();
	if (_enabled) {
		if (_floatingMenuSource) {
			onOpenMenuSource();
		}
		if (_tapCallback) {
			_tapCallback();
		}
	}
	return true;
}
void Button::onPressCancel() {
	_highlight->animateDeselection();
	_highlight->dropSpawn();
}

void Button::onOpenMenuSource() {
	FloatingMenu::push(_floatingMenuSource, _touchPoint);
}

void Button::setEnabled(bool value) {
	_enabled = value;
	updateEnabled();
}

bool Button::isEnabled() const {
	return _enabled;
}

void Button::setSelected(bool value, bool instant) {
	_highlight->setSelected(value, instant);
}

bool Button::isSelected() const {
	return _highlight->isSelected();
}

void Button::setRaisedDefaultZIndex(float value) {
	_raisedDefaultZIndex = value;
	if (_style == Button::Style::Raised) {
		setShadowZIndex(value);
	}
}

void Button::setRaisedActiveZIndex(float value) {
	_raisedActiveZIndex = value;
}

void Button::setMenuSource(MenuSource *menu) {
	if (_floatingMenuSource != menu) {
		_floatingMenuSource = menu;
		updateEnabled();
	}
}
MenuSource * Button::getMenuSource() const {
	return _floatingMenuSource;
}

const Vec2 &Button::getTouchPoint() const {
	return _touchPoint;
}

void Button::cancel() {
	static_cast<gesture::Listener *>(_listener)->cancel();
}

gesture::Listener * Button::getListener() const {
	return _listener;
}

ButtonHighLight *Button::getHighlight() const {
	return _highlight;
}

void Button::onContentSizeDirty() {
	MaterialNode::onContentSizeDirty();

	_highlight->setPosition(_content->getPosition());
	_highlight->setContentSize(_content->getContentSize());
}

void Button::setMenuSourceButton(MenuSourceButton *source) {
	if (source != _source) {
		if (_source) {
			_source->onNodeDetached(this);
		}
		_source = source;
		updateFromSource();
		if (_source) {
			_source->onNodeAttached(this);
		}
	}
}

MenuSourceButton *Button::getMenuSourceButton() const {
	return _source;
}

void Button::setFormController(FormController *c) {
	if (_formController != c) {
		_formController = c;
		_formEventListener->clear();
		if (_formController) {
			_formEventListener->onEventWithObject(FormController::onEnabled, _formController, [this] (const Event &ev) {
				updateEnabled();
			});
			updateEnabled();
		}
	}
}
FormController *Button::getFormController() const {
	return _formController;
}

void Button::updateFromSource() {
	if (_source) {
		setSelected(_source->isSelected(), true);
		if (_source->getCallback()) {
			setTapCallback([this] {
				_source->getCallback()(this, _source.get());
			});
		} else {
			setTapCallback(nullptr);
		}
		setMenuSource(_source->getNextMenu());
	} else {
		setSelected(false);
		setTapCallback(nullptr);
		setMenuSource(nullptr);
	}
}

void Button::onLightLevel() {
	MaterialNode::onLightLevel();
	if (isAutoLightLevel() && (_style == FlatBlack || _style == FlatWhite)) {
		auto level = material::ResourceManager::getInstance()->getLightLevel();
		switch(level) {
		case layout::style::LightLevel::Dim:
			setStyle(FlatWhite);
			break;
		case layout::style::LightLevel::Normal:
			setStyle(FlatBlack);
			break;
		case layout::style::LightLevel::Washed:
			setStyle(FlatBlack);
			break;
		};
	}
}

NS_MD_END
