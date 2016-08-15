/*
 * MaterialIcon.cpp
 *
 *  Created on: 19 нояб. 2014 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialIcon.h"

#include "MaterialColors.h"
#include "MaterialResourceManager.h"
#include "2d/CCSprite.h"
#include "2d/CCActionEase.h"
#include "2d/CCActionInstant.h"

#include "base/CCDirector.h"
#include "base/CCEventDispatcher.h"
#include "base/CCEventType.h"
#include "base/CCEventCustom.h"
#include "base/CCEventListenerCustom.h"

#include "renderer/CCRenderer.h"
#include "renderer/CCGLProgram.h"
#include "renderer/CCGLProgramCache.h"
#include "renderer/ccGLStateCache.h"

#include "SPScreen.h"

NS_MD_BEGIN

namespace _icons {

class EmptyIcon : public DynamicIcon::Icon {
public:
	virtual bool init() override { return true; }
	virtual void redraw(float progress, float diff) override { }
};

class NavIcon : public DynamicIcon::Icon {
public:
	virtual bool init() override {
		_top = construct<draw::Path>();
		_center = construct<draw::Path>();
		_bottom = construct<draw::Path>();

		_paths.pushBack(_top);
		_paths.pushBack(_bottom);
		_paths.pushBack(_center);

		return true;
	}
	virtual void redraw(float pr, float diff) override {
		float p = pr;

		_top->clear();
		_top->moveTo( progress(2.0f, 13.0f, p),				progress(5.0f, 3.0f, p) );
		_top->lineTo( progress(2.0f, 13.0f - (float)M_SQRT2, p),	progress(7.0f, 3.0f + (float)M_SQRT2, p) );
		_top->lineTo( progress(22.0f, 22.0f - (float)M_SQRT2, p),	progress(7.0f, 12.0f + (float)M_SQRT2, p) );
		_top->lineTo( progress(22.0f, 22.0f, p),			progress(5.0f, 12.0f, p) );
		_top->setFillColor(cocos2d::Color4B(0, 0, 0, 255));
		_top->closePath();

		_center->clear();
		_center->moveTo( progress(2.0f, 3.0f, p), 11 );
		_center->lineTo( progress(22.0f, 20.0f, p), 11 );
		_center->lineTo( progress(22.0f, 20.0f, p), 13 );
		_center->lineTo( progress(2.0f, 3.0f, p), 13 );
		_center->setFillColor(cocos2d::Color4B(0, 0, 0, 255));
		_center->closePath();

		_bottom->clear();
		_bottom->moveTo( progress(2.0f, 13.0f - (float)M_SQRT2, p),	progress(17.0f, 21.0f - (float)M_SQRT2, p) );
		_bottom->lineTo( progress(22.0f, 22.0f - (float)M_SQRT2, p),	progress(17.0f, 12.0f - (float)M_SQRT2, p) );
		_bottom->lineTo( progress(22.0f, 22.0f, p),				progress(19.0f, 12.0f, p) );
		_bottom->lineTo( progress(2.0f, 13.0f, p),				progress(19.0f, 21.0f, p) );
		_bottom->setFillColor(cocos2d::Color4B(0, 0, 0, 255));
		_bottom->closePath();

		float rotation = _parent->getRotation();

		if (pr == 0.0f) {
			_parent->setRotation(0);
		} else if (pr == 1.0f) {
			_parent->setRotation(180);
		} else if ((diff < 0 && fabsf(rotation) == 180) || rotation < 0) {
			_parent->setRotation(progress(0, -180, pr));
		} else if ((diff > 0 && rotation == 0) || rotation > 0) {
			_parent->setRotation(progress(0, 180, pr));
		}
	}

protected:
	float _progress = -1.0f;
	stappler::draw::Path *_top;
	stappler::draw::Path *_center;
	stappler::draw::Path *_bottom;
};

class CircleLoaderIcon : public DynamicIcon::Icon {
public:
	virtual bool init() override {
		_path = construct<draw::Path>();
		_path->setStyle(stappler::draw::Path::Style::Stroke);
		_path->setStrokeWidth(2.0f);
		_paths.pushBack(_path);

		return true;
	}
	virtual void redraw(float pr, float diff) override {
		float p = pr;

		float arcLen = 20.0_to_rad;
		float arcStart = -100.0_to_rad;
		if (p < 0.5) {
			arcStart = arcStart + progress(0_to_rad, 75_to_rad, p * 2.0f);
			arcLen = progress(20_to_rad, 230_to_rad, p * 2.0f);
		} else {
			arcStart = arcStart + progress(75_to_rad, 360_to_rad, (p - 0.5f) * 2.0f);
			arcLen = progress(230_to_rad, 20_to_rad, (p - 0.5f) * 2.0f);
		}

		_path->clear();
		_path->addArc(cocos2d::Rect(4, 4, 16, 16), arcStart, arcLen);
	}
	virtual void animate() override {
		if (_parent) {
			auto a = cocos2d::EaseBezierAction::create(construct<DynamicIconProgress>(1.2f, 1.0f));
			a->setBezierParamer(0.0, 0.050, 0.950, 1.0);
			auto b = cocos2d::CallFunc::create([this] {
				_parent->setProgress(0.0f);
			});
			auto c = cocos2d::RepeatForever::create(cocos2d::Sequence::create(a, b, nullptr));
			c->setTag(1);
			_parent->stopActionByTag(1);
			_parent->runAction(c);
		}
	}
protected:
	stappler::draw::Path *_path = nullptr;
};

};

bool DynamicIconProgress::init(float duration, float targetProgress) {
	if (!ActionInterval::initWithDuration(duration)) {
		return false;
	}

	_destProgress = targetProgress;
	return true;
}

void DynamicIconProgress::startWithTarget(cocos2d::Node *t) {
	if (auto target = dynamic_cast<DynamicIcon *>(t)) {
    	_sourceProgress = target->getProgress();
    	target->acquireCache();
	}
	ActionInterval::startWithTarget(t);
}

void DynamicIconProgress::update(float time) {
	if (auto target = dynamic_cast<DynamicIcon *>(_target)) {
		target->setProgress(_sourceProgress + (_destProgress - _sourceProgress) * time);
	}
}

void DynamicIconProgress::stop() {
	if (auto target = dynamic_cast<DynamicIcon *>(_target)) {
    	target->releaseCache();
	}
	cocos2d::ActionInterval::stop();
}

StaticIcon *StaticIcon::defaultIcon(IconName iconName) {
	auto icon = ResourceManager::getInstance()->getIcon(iconName);
	if (icon) {
		StaticIcon *ret = new StaticIcon();
		if (ret->init(icon)) {
			ret->autorelease();
			return ret;
		} else {
			delete ret;
		}
	}
	return nullptr;
}

bool StaticIcon::init(stappler::Icon *icon) {
	if (!IconSprite::init(icon)) {
		return false;
	}

	return true;
}

bool StaticIcon::init() {
	if (!IconSprite::init(nullptr)) {
		return false;
	}

	setNormalized(true);
	setColor(Color::Black);

	return true;
}

IconName StaticIcon::getIconName() const {
	return _iconName;
}

void StaticIcon::setIconName(IconName iconName) {
	auto icon = ResourceManager::getInstance()->getIcon(iconName);
	if (icon) {
		setIcon(icon);
	} else {
		setIcon(nullptr);
	}
	_iconName = iconName;
}

bool DynamicIcon::Icon::init() { return true; }
void DynamicIcon::Icon::redraw(float progress, float diff) { }

const cocos2d::Vector<stappler::draw::Path *> &DynamicIcon::Icon::getPaths() const { return _paths; }
void DynamicIcon::Icon::setParent(DynamicIcon *p) { _parent = p; }

void DynamicIcon::Icon::animate() { }

DynamicIcon *DynamicIcon::defaultIcon(Name name) {
	auto icon = construct<DynamicIcon>();
	icon->setIconName(name);
	return icon;
}

DynamicIcon::~DynamicIcon() {
	CC_SAFE_RELEASE(_icon);
}

void DynamicIcon::setProgress(float progress) {
	if (_progress != progress) {
		float diff = progress - _progress;
		_progress = progress;
		if (diff != 0.0f) {
			redraw(progress, diff);
		}
	}
}

float DynamicIcon::getProgress() {
	return _progress;
}

void DynamicIcon::setIconName(Name name) {
	DynamicIcon::Icon *newIcon = nullptr;
	switch (name) {
	case Name::Navigation:
		newIcon = new _icons::NavIcon();
		break;
	case Name::Loader:
		newIcon = new _icons::CircleLoaderIcon();
		break;
	case Name::Empty:
		newIcon = new _icons::EmptyIcon();
		break;
	default:
		break;
	}

	setIcon(newIcon);
	CC_SAFE_RELEASE(newIcon);
	_iconName = name;
}

DynamicIcon::Name DynamicIcon::getIconName() const {
	return _iconName;
}

void DynamicIcon::setIcon(Icon *i) {
	clear();
	stopAllActions();
	setRotation(0);
	setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
	setContentSize(cocos2d::Size(24.0f, 24.0f));
	_progress = 0.0f;

	CC_SAFE_RELEASE(_icon);
	_icon = i;
	CC_SAFE_RETAIN(_icon);

	if (_icon) {
		_icon->setParent(this);
		_icon->init();
		auto &childs = _icon->getPaths();
		for (auto &it : childs) {
			addPath(it);
		}
	}
	_icon->redraw(0.0f, 0.0f);
	setProgress(0.0f);
}

void DynamicIcon::animate() {
	if (_icon) {
		_icon->animate();
	}
}

void DynamicIcon::animate(float targetProgress, float duration) {
	if (_progress != targetProgress) {
		if (duration > 0.0f) {
			auto a = cocos2d::EaseQuadraticActionInOut::create(construct<DynamicIconProgress>(duration, targetProgress));
			a->setTag(1);
			stopActionByTag(1);
			runAction(a);
		} else {
			stopActionByTag(1);
			setProgress(targetProgress);
		}
	}
}

void DynamicIcon::redraw(float progress, float diff) {
	if (_icon) {
		_icon->redraw(progress, diff);
	}
}

bool DynamicIcon::init(int width, int height) {
	if (!stappler::draw::PathNode::init(width, height, stappler::draw::Format::A8)) {
		return false;
	}

	return true;
}

bool DynamicIcon::init() {
	if (!stappler::draw::PathNode::init(24, 24, stappler::draw::Format::A8)) {
		return false;
	}

	return true;
}

void DynamicIcon::onEnter() {
	PathNode::onEnter();
	redraw(_progress, 0.0f);
}

NS_MD_END
