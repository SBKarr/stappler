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
	addComponent(l);

	_source.setCallback(std::bind(&Button::updateFromSource, this));

	_listener = l;
	_tapCallback = tapCallback;
	_longTapCallback = longTapCallback;

	auto el = Rc<EventListener>::create();
	addComponent(el);
	_formEventListener = el;

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
			bg->setColor(Color::White);
		} else {
			bg->setColor(Color::Black);
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
void Button::setAnimationOpacity(uint8_t value) {
	_animationOpacity = value;
}

bool Button::onPressBegin(const Vec2 &location) {
	if (!_enabled) {
		return false;
	}
	if (!_touchFilter || _touchFilter(location)) {
		if (stappler::node::isTouched(this, location, 8)) {
			_touchPoint = location;
			animateSelection();
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
	animateDeselection();
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
	animateDeselection();
	dropSpawn();
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
	if (_selected != value) {
		_selected = value;
		if (_selected) {
			if (!instant) {
				stopActionByTag(2);
				auto a = cocos2d::EaseQuadraticActionInOut::create(Rc<ProgressAction>::create(
						progress(0.4f, 0.0f, _animationProgress), 1.0f,
						[this] (ProgressAction *a, float progress) {
					updateSelection(progress);
				}, [this] (ProgressAction *a) {
					a->setSourceProgress(getSelectionProgress());
					beginSelection();
				}, [this] (ProgressAction *a) {
					endSelection();
				}));
				a->setTag(2);
				runAction(a);
			} else {
				beginSelection();
				updateSelection(1.0f);
				endSelection();
			}
		} else {
			animateDeselection();
		}
	}
}
bool Button::isSelected() const {
	return _selected;
}

void Button::setSpawnDelay(float value) {
	_spawnDelay = value;
}
float Button::getSpawnDelay() const {
	return _spawnDelay;
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

void Button::animateSelection() {
	if (!_selected) {
		stopActionByTag(2);
		auto a = cocos2d::EaseQuadraticActionInOut::create(Rc<ProgressAction>::create(
				progress(0.4f, 0.0f, _animationProgress), 1.0f,
				[this] (ProgressAction *a, float progress) {
			updateSelection(progress);
		}, [this] (ProgressAction *a) {
			a->setSourceProgress(getSelectionProgress());
			beginSelection();
		}, [this] (ProgressAction *a) {
			endSelection();
		}));
		a->setTag(2);
		runAction(a);
	}

	draw::Image::PathRef *path = new draw::Image::PathRef();
	Size *size = new Size();
	Vec2 *point = new Vec2();

	auto a = Rc<ProgressAction>::create(0.4, 1.0,
			[this, path, size, point] (ProgressAction *a, float progress) {
		updateSpawn(progress, (*path), (*size), (*point));
	}, [this, path, size, point] (ProgressAction *a) {
		(*path) = beginSpawn();
		if (_animationNode) {
			(*size) = _animationNode->getContentSize();
			(*point) = _animationNode->convertToNodeSpace(_touchPoint);
		}
	}, [this, path, size, point] (ProgressAction *a) {
		if (_animationNode) {
			_animationNode->removePath((*path));
		}
		endSpawn();
		delete path;
		delete size;
		delete point;
	});
	a->setForceStopCallback(true);

	auto b = action::sequence(_spawnDelay, a);
	runAction(b, 3);
}

void Button::animateDeselection() {
	if (!_selected) {
		stopActionByTag(2);
		auto a = cocos2d::EaseQuadraticActionInOut::create(Rc<ProgressAction>::create(
				progress(0.35f, 0.0f, 1.0f - _animationProgress), 0.0f,
				[this] (ProgressAction *a, float progress) {
			updateSelection(progress);
		}, [this] (ProgressAction *a) {
			a->setSourceProgress(getSelectionProgress());
			beginSelection();
		}, [this] (ProgressAction *a) {
			endSelection();
		}));
		a->setTag(2);
		runAction(a);
	}
}


void Button::beginSelection() {
	if (_animationProgress == 0.0f && _style == Style::Raised) {
		setLocalZOrder(getLocalZOrder() + 1);
	}
}

void Button::endSelection() {
	if (_animationProgress == 0.0f && _style == Style::Raised) {
		setLocalZOrder(getLocalZOrder() - 1);
	}
}

void Button::updateSelection(float progress) {
	_animationProgress = progress;
	updateSelectionProgress(progress);
}

float Button::getSelectionProgress() const {
	return _animationProgress;
}

void Button::updateSelectionProgress(float pr) {
	if (_style == Raised) {
		setShadowZIndex(progress(_raisedDefaultZIndex, _raisedActiveZIndex, pr));
	} else {
		auto c = getBackground()->getColor();
		if (c == Color3B::WHITE || c == Color3B::BLACK) {
			if (pr > 0.5) {
				uint8_t op = (uint8_t)progress(0.0f, 20.0f, (pr - 0.5) * 2.0);
				getBackground()->setOpacity(op);
			} else {
				getBackground()->setOpacity((uint8_t)0);
			}
		}
	}
}

draw::Image::PathRef Button::beginSpawn() {
	float downscale = 4.0f;
	uint32_t width = _contentSize.width / downscale;
	uint32_t height = _contentSize.height / downscale;

	if (_animationNode && _animationNode->getBaseWidth() == width && _animationNode->getBaseHeight() == height) {
		// do nothing, our node is perfect
	} else {
		if (_animationNode) {
			_animationNode->removeFromParent();
			_animationNode = nullptr;
		}
		auto animationNode = Rc<draw::PathNode>::create(width, height);
		animationNode->setContentSize(Size(_contentSize.width / downscale, _contentSize.height / downscale));
		animationNode->setPosition(Vec2(0, 0));
		animationNode->setAnchorPoint(Vec2(0, 0));
		animationNode->setScale(downscale);
		animationNode->setColor((_style == FlatWhite)?(Color::White):(Color::Black));
		animationNode->setOpacity(_animationOpacity);
		animationNode->setAntialiased(true);
		_animationNode = addChildNode(animationNode, 1);
	}

	auto path = _animationNode->addPath();
	_animationCount ++;

	return path;
}

void Button::updateSpawn(float pr, draw::Image::PathRef &path, const Size &size, const Vec2 &point) {
	if (_animationNode) {
		float threshold = 0.6f;
		float rad = 0.0f;

		if (pr > threshold) {
			rad = std::max(size.width, size.height);
			path.setFillOpacity(uint8_t(progress(255.0f, 0.0f, (pr - threshold) / (1.0 - threshold))));
		} else {
			if (pr < 0.2) {
				path.setFillOpacity(uint8_t(progress(0, 255, pr * 5.0f)));
			} else {
				path.setFillOpacity(255);
			}

			float a = point.length();
			float b = Vec2(point.x, size.height - point.y).length();
			float c = Vec2(size.width - point.x, size.height - point.y).length();
			float d = Vec2(size.width - point.x, point.y).length();

			float minRad = std::min(size.width, size.height) / 8.0f;
			float maxRad = std::max(std::max(a, b), std::max(c, d));

			rad = progress(minRad, maxRad, pr / threshold);
		}

		path.clear().addCircle(point.x, size.height - point.y, rad);
	}
}

void Button::endSpawn() {
	if (_animationCount > 0) {
		_animationCount --;
	}
	if (_animationCount == 0 && _animationNode) {
		_animationNode->removeFromParent();
		_animationNode = nullptr;
	}
}

void Button::dropSpawn() {
	if (_animationNode) {
		_animationCount = 0;
		_animationNode->runAction(cocos2d::Sequence::createWithTwoActions(
				cocos2d::FadeTo::create(0.15f, 0), cocos2d::RemoveSelf::create()));
		_animationNode = nullptr;
	} else {
		stopActionByTag(3);
	}
}

void Button::onContentSizeDirty() {
	MaterialNode::onContentSizeDirty();
	if (_animationNode) {
		float downscale = 4.0f;
		auto size = getContentSizeWithPadding();
		auto pos = getAnchorPositionWithPadding();

		_animationNode->setPosition(pos);
		_animationNode->setContentSize(Size(size.width / downscale, size.height / downscale));
	}
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
