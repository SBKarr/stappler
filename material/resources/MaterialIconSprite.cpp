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
#include "MaterialIconSprite.h"
#include "MaterialResourceManager.h"
#include "2d/CCSprite.h"
#include "2d/CCActionEase.h"
#include "2d/CCActionInstant.h"
#include "2d/CCActionManager.h"
#include "2d/CCComponent.h"

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

class IconProgress : public cocos2d::ActionInterval {
public:
	virtual bool init(float duration, float targetIndex);
	virtual void startWithTarget(cocos2d::Node *t) override;
	virtual void update(float time) override;
	virtual void stop() override;

	float getSourceProgress() const;
	float getDestProgress() const;

protected:
	float _sourceProgress = 0;
	float _destProgress = 0;
};


bool IconProgress::init(float duration, float targetProgress) {
	if (!ActionInterval::initWithDuration(duration)) {
		return false;
	}

	_destProgress = targetProgress;
	return true;
}

void IconProgress::startWithTarget(cocos2d::Node *t) {
	if (auto target = dynamic_cast<IconSprite *>(t)) {
    	_sourceProgress = target->getRawProgress();
	}
	ActionInterval::startWithTarget(t);
}

void IconProgress::update(float time) {
	if (auto target = dynamic_cast<IconSprite *>(_target)) {
		target->setProgress(_sourceProgress + (_destProgress - _sourceProgress) * time);
	}
}

void IconProgress::stop() {
	cocos2d::ActionInterval::stop();
}

float IconProgress::getSourceProgress() const {
	return _sourceProgress;
}
float IconProgress::getDestProgress() const {
	return _destProgress;
}

}

class IconSprite::DynamicIcon : public cocos2d::Component {
public:
	virtual void onAdded() override { Component::onAdded(); _image = Rc<draw::Image>::create(24, 24); }
	virtual void redraw(float progress, float diff) { }
	draw::Image * getImage() const { return _image; }

	virtual void animate() { }

protected:
	Rc<draw::Image> _image;
};

class IconSprite::NavIcon : public IconSprite::DynamicIcon {
public:
	virtual void onAdded() override;
	virtual void redraw(float pr, float diff) override;

protected:
	float _progress = -1.0f;
	draw::Image::PathRef _top;
	draw::Image::PathRef _center;
	draw::Image::PathRef _bottom;
};

class IconSprite::CircleLoaderIcon : public IconSprite::DynamicIcon {
public:
	virtual void onAdded() override;
	virtual void redraw(float pr, float diff) override;
	virtual void animate() override;

protected:
	draw::Image::PathRef _path;
};

void IconSprite::NavIcon::onAdded() {
	DynamicIcon::onAdded();
	_top = _image->addPath().setFillColor(Color4B(0, 0, 0, 255));
	_center = _image->addPath().setFillColor(Color4B(0, 0, 0, 255));
	_bottom = _image->addPath().setFillColor(Color4B(0, 0, 0, 255));
}

void IconSprite::NavIcon::redraw(float pr, float diff) {
	float p = pr;

	if (p <= 1.0f) {
		_top.clear()
				.moveTo( progress(2.0f, 13.0f, p),						progress(5.0f, 3.0f, p) )
				.lineTo( progress(2.0f, 13.0f - (float)M_SQRT2, p),		progress(7.0f, 3.0f + (float)M_SQRT2, p) )
				.lineTo( progress(22.0f, 22.0f - (float)M_SQRT2, p),	progress(7.0f, 12.0f + (float)M_SQRT2, p) )
				.lineTo( progress(22.0f, 22.0f, p),						progress(5.0f, 12.0f, p) )
				.closePath();

		_center.clear()
				.moveTo( progress(2.0f, 3.0f, p),	11 )
				.lineTo( progress(22.0f, 20.0f, p), 11 )
				.lineTo( progress(22.0f, 20.0f, p), 13 )
				.lineTo( progress(2.0f, 3.0f, p),	13 )
				.closePath();

		_bottom.clear()
				.moveTo( progress(2.0f, 13.0f - (float)M_SQRT2, p),		progress(17.0f, 21.0f - (float)M_SQRT2, p) )
				.lineTo( progress(22.0f, 22.0f - (float)M_SQRT2, p),	progress(17.0f, 12.0f - (float)M_SQRT2, p) )
				.lineTo( progress(22.0f, 22.0f, p),						progress(19.0f, 12.0f, p) )
				.lineTo( progress(2.0f, 13.0f, p),						progress(19.0f, 21.0f, p) )
				.closePath();

		float rotation = _owner->getRotation();

		if (pr == 0.0f) {
			_owner->setRotation(0);
		} else if (pr == 1.0f) {
			_owner->setRotation(180);
		} else if ((diff < 0 && fabsf(rotation) == 180) || rotation < 0) {
			_owner->setRotation(progress(0, -180, pr));
		} else if ((diff > 0 && rotation == 0) || rotation > 0) {
			_owner->setRotation(progress(0, 180, pr));
		}
	} else {
		p = p - 1.0f;

		_top.clear()
				.moveTo( 13.0f, progress(3.0f, 4.0f, p) )
				.lineTo( progress(13.0f - (float)M_SQRT2, 11.0f, p), progress(3.0f + (float)M_SQRT2, 4.0f, p) )
				.lineTo( progress(22.0f - (float)M_SQRT2, 11.0f, p), progress(12.0f + (float)M_SQRT2, 12.0f, p) )
				.lineTo( progress(22.0f, 13.0f, p), 12.0f )
				.closePath();

		_center.clear()
				.moveTo( progress(3.0f, 4.0f, p), 11 )
				.lineTo( progress(20.0f, 20.0f, p), 11 )
				.lineTo( progress(20.0f, 20.0f, p), 13 )
				.lineTo( progress(3.0f, 4.0f, p), 13 )
				.closePath();

		_bottom.clear()
				.moveTo( progress(13.0f - (float)M_SQRT2, 11.0f, p), progress(21.0f - (float)M_SQRT2, 20.0f, p) )
				.lineTo( progress(22.0f - (float)M_SQRT2, 11.0f, p), progress(12.0f - (float)M_SQRT2, 12.0f, p) )
				.lineTo( progress(22.0f, 13.0f, p), 12.0f )
				.lineTo( 13.0f, progress(21.0f, 20.0f, p) )
				.closePath();

		if (p == 0.0f) {
			_owner->setRotation(180);
		} else if (p == 1.0f) {
			_owner->setRotation(45);
		} else {
			_owner->setRotation(progress(180, 45, p));
		}
	}
}

void IconSprite::CircleLoaderIcon::onAdded() {
	DynamicIcon::onAdded();
	_path = _image->addPath(draw::Path().setStyle(stappler::draw::Path::Style::Stroke).setStrokeWidth(2.0f));
}

void IconSprite::CircleLoaderIcon::redraw(float pr, float diff) {
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

	_path.clear().addArc(cocos2d::Rect(4, 4, 16, 16), arcStart, arcLen);
}

void IconSprite::CircleLoaderIcon::animate() {
	if (_owner) {
		auto a = cocos2d::EaseBezierAction::create(construct<_icons::IconProgress>(1.2f, 1.0f));
		a->setBezierParamer(0.0, 0.050, 0.950, 1.0);
		auto b = cocos2d::CallFunc::create([this] {
			static_cast<IconSprite *>(_owner)->setProgress(0.0f);
		});
		auto c = cocos2d::RepeatForever::create(cocos2d::Sequence::create(a, b, nullptr));
		_owner->stopActionByTag("Animate"_tag);
		_owner->runAction(c, "Animate"_tag);
	}
}

bool IconSprite::init(IconName name) {
	if (!PathNode::init(24, 24, draw::Format::A8)) {
		return false;
	}

	setCascadeColorEnabled(true);
	setCascadeOpacityEnabled(true);
	setContentSize(Size(24.0f, 24.0f));

	setIconName(name);

	return true;
}
bool IconSprite::init(IconName name, uint32_t w, uint32_t h) {
	if (!PathNode::init(w, h, draw::Format::A8)) {
		return false;
	}

	setCascadeColorEnabled(true);
	setCascadeOpacityEnabled(true);
	setContentSize(Size(w, h));

	setIconName(name);

	return true;
}

void IconSprite::onEnter() {
	PathNode::onEnter();
	if (_dynamicIcon) {
		_dynamicIcon->redraw(_progress, 0.0f);
	}
}

void IconSprite::setIconName(IconName name) {
	switch (name) {
	case IconName::Dynamic_Navigation:
		setDynamicIcon(Rc<NavIcon>::create());
		break;
	case IconName::Dynamic_Loader:
		setDynamicIcon(Rc<CircleLoaderIcon>::create());
		break;
	case IconName::None:
	case IconName::Empty:
		setDynamicIcon(nullptr);
		break;
	default:
		setStaticIcon(name);
		break;
	}
	_iconName = name;
}

IconName IconSprite::getIconName() const {
	return _iconName;
}

bool IconSprite::isEmpty() const {
	return _iconName == IconName::None || _iconName == IconName::Empty;
}
bool IconSprite::isDynamic() const {
	switch (_iconName) {
	case IconName::Dynamic_Navigation:
	case IconName::Dynamic_Loader:
		return true;
		break;
	default:
		break;
	}
	return false;
}
bool IconSprite::isStatic() const {
	switch (_iconName) {
	case IconName::Dynamic_Navigation:
	case IconName::Dynamic_Loader:
	case IconName::None:
		return false;
		break;
	default:
		break;
	}
	return false;
}

void IconSprite::setProgress(float progress) {
	if (_dynamicIcon) {
		if (_progress != progress) {
			float diff = progress - _progress;
			_progress = progress;
			if (diff != 0.0f) {
				_dynamicIcon->redraw(progress, diff);
			}
		}
	} else {
		_progress = 0.0f;
	}
}
float IconSprite::getProgress() const {
	auto a = _actionManager->getActionByTag("Animate"_tag, this);
	if (auto ease = dynamic_cast<cocos2d::EaseQuadraticActionInOut *>(a)) {
		if (auto inner = dynamic_cast<_icons::IconProgress *>(ease->getInnerAction())) {
			return inner->getDestProgress();
		}
	}
	return _progress;
}

float IconSprite::getRawProgress() const {
	return _progress;
}

void IconSprite::animate() {
	if (_dynamicIcon) {
		_dynamicIcon->animate();
	}
}
void IconSprite::animate(float targetProgress, float duration) {
	if (_dynamicIcon && _progress != targetProgress) {
		if (duration > 0.0f) {
			auto a = cocos2d::EaseQuadraticActionInOut::create(construct<_icons::IconProgress>(duration, targetProgress));
			stopActionByTag("Animate"_tag);
			runAction(a, "Animate"_tag);
		} else {
			stopActionByTag("Animate"_tag);
			setProgress(targetProgress);
		}
	} else {
		stopActionByTag("Animate"_tag);
	}
}

cocos2d::GLProgramState *IconSprite::getProgramStateI8() const {
	return getProgramStateA8();
}

void IconSprite::updateCanvas(layout::Subscription::Flags f) {
	if (isDynamic()) {
		PathNode::updateCanvas(f);
	}
}

void IconSprite::setDynamicIcon(DynamicIcon *i) {
	setDensity(screen::density());

	clear();
	stopActionByTag("Animate"_tag);
	setRotation(0);
	_progress = 0.0f;
	if (_dynamicIcon) {
		removeComponent(_dynamicIcon);
	}

	if (i) {
		addComponent(i);
		_dynamicIcon = i;
		setImage(_dynamicIcon->getImage());
		_dynamicIcon->redraw(0.0f, 0.0f);
	} else {
		setTexture(nullptr);
		_dynamicIcon = i;
	}
	setProgress(0.0f);
}

void IconSprite::setStaticIcon(IconName name) {
	setDynamicIcon(nullptr);
	auto icon = ResourceManager::getInstance()->getIcon(name);
	if (icon) {
		setImage(nullptr);
		setTexture(icon.getTexture());
		setTextureRect(icon.getTextureRect());
		setDensity(icon.getDensity());
	}
}

NS_MD_END
