/*
 * MaterialButton.cpp
 *
 *  Created on: 10 дек. 2014 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialButton.h"

#include "MaterialColors.h"
#include "MaterialLabel.h"
#include "MaterialMenuSource.h"
#include "MaterialFloatingMenu.h"
#include "MaterialResourceManager.h"

#include "SPDrawPathNode.h"
#include "SPString.h"
#include "SPRoundedSprite.h"
#include "SPProgressAction.h"
#include "SPGestureListener.h"
#include "SPDataListener.h"

#include "2d/CCActionInterval.h"
#include "2d/CCActionInstant.h"
#include "2d/CCActionEase.h"

NS_MD_BEGIN

bool Button::init(const TapCallback &tapCallback, const TapCallback &longTapCallback) {
	if (!MaterialNode::init()) {
		return false;
	}

	auto l = construct<gesture::Listener>();
	l->setGestureFilter([this] (const cocos2d::Vec2 &vec,  stappler::gesture::Type t, const  stappler::gesture::Listener::DefaultGestureFilter &def) -> bool {
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
	if ((_longTapCallback || _tapCallback || _floatingMenuSource) && _enabled) {
		_listener->setEnabled(true);
	} else {
		_listener->setEnabled(false);
	}
}

Button::Style Button::getStyle() const {
	return _style;
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

bool Button::onPressBegin(const cocos2d::Vec2 &location) {
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
	construct<FloatingMenu>(_floatingMenuSource, _touchPoint);
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
				auto a = cocos2d::EaseQuadraticActionInOut::create(construct<ProgressAction>(
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

const cocos2d::Vec2 &Button::getTouchPoint() const {
	return _touchPoint;
}

void Button::animateSelection() {
	if (!_selected) {
		stopActionByTag(2);
		auto a = cocos2d::EaseQuadraticActionInOut::create(construct<ProgressAction>(
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

	stappler::draw::Path **path = new (stappler::draw::Path *);
	cocos2d::Size *size = new cocos2d::Size();
	cocos2d::Vec2 *point = new cocos2d::Point();
	auto b = cocos2d::Sequence::create(cocos2d::DelayTime::create(_spawnDelay), construct<ProgressAction>(0.4, 1.0,
			[this, path, size, point] (ProgressAction *a, float progress) {
		updateSpawn(progress, (*path), (*size), (*point));
	}, [this, path, size, point] (ProgressAction *a) {
		(*path) = beginSpawn();
		(*path)->retain();
		if (_animationNode) {
			(*size) = _animationNode->getContentSize();
			(*point) = _animationNode->convertToNodeSpace(_touchPoint);
		}
	}, [this, path, size, point] (ProgressAction *a) {
		(*path)->remove();
		(*path)->release();
		(*path) = nullptr;
		endSpawn();
		delete path;
		delete size;
		delete point;
	}), nullptr);
	b->setTag(3);
	runAction(b);
}

void Button::animateDeselection() {
	if (!_selected) {
		stopActionByTag(2);
		auto a = cocos2d::EaseQuadraticActionInOut::create(construct<ProgressAction>(
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
		if (pr > 0.5) {
			uint8_t op = (uint8_t)progress(0.0f, 20.0f, (pr - 0.5) * 2.0);
			getBackground()->setOpacity(op);
		} else {
			getBackground()->setOpacity((uint8_t)0);
		}
	}
}

stappler::draw::Path * Button::beginSpawn() {
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
		_animationNode = construct<draw::PathNode>(width, height);
		_animationNode->setContentSize(cocos2d::Size(_contentSize.width / downscale, _contentSize.height / downscale));
		_animationNode->setPosition(cocos2d::Vec2(0, 0));
		_animationNode->setAnchorPoint(cocos2d::Vec2(0, 0));
		_animationNode->setScale(downscale);
		_animationNode->setColor((_style == FlatWhite)?(Color::White):(Color::Black));
		_animationNode->setOpacity(_animationOpacity);
		_animationNode->setAntialiased(true);
		_animationNode->acquireCache();
		addChild(_animationNode, 1);
	}

	auto path = construct<draw::Path>();
	_animationNode->addPath(path);
	_animationCount ++;

	return path;
}

void Button::updateSpawn(float pr, stappler::draw::Path *path, const cocos2d::Size &size, const cocos2d::Point &point) {

	float threshold = 0.6f;
	float rad = 0.0f;

	if (pr > threshold) {
		rad = MAX(size.width, size.height);
		path->setFillOpacity(progress(255.0f, 0.0f, (pr - threshold) / (1.0 - threshold)));
	} else {
		if (pr < 0.2) {
			path->setFillOpacity(progress(0, 255, pr * 10.0f));
		} else {
			path->setFillOpacity(255);
		}

		float a = point.length();
		float b = cocos2d::Vec2(point.x, size.height - point.y).length();
		float c = cocos2d::Vec2(size.width - point.x, size.height - point.y).length();
		float d = cocos2d::Vec2(size.width - point.x, point.y).length();

		float minRad = MIN(size.width, size.height) / 8.0f;
		float maxRad = MAX(MAX(a, b), MAX(c, d));

		rad = progress(minRad, maxRad, pr / threshold);
	}

	path->clear();
	path->addCircle(point.x, size.height - point.y, rad);
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
		_animationNode->setContentSize(cocos2d::Size(_contentSize.width / downscale, _contentSize.height / downscale));
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
		case rich_text::style::LightLevel::Dim:
			setStyle(FlatWhite);
			break;
		case rich_text::style::LightLevel::Normal:
			setStyle(FlatBlack);
			break;
		case rich_text::style::LightLevel::Washed:
			setStyle(FlatBlack);
			break;
		};
	}
}

NS_MD_END
