// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPDevice.h"
#include "SPEventListener.h"

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
	DynamicIcon() { }

	virtual void onAdded() override { Component::onAdded(); _image = Rc<draw::Image>::create(24, 24); }
	virtual void redraw(float progress, float diff) { }
	draw::Image * getImage() const { return _image; }

	virtual void animate() { }

protected:
	float _progress = -1.0f;
	Rc<draw::Image> _image;
};

class IconSprite::NavIcon : public IconSprite::DynamicIcon {
public:
	virtual void onAdded() override;
	virtual void redraw(float pr, float diff) override;

protected:
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

class IconSprite::ExpandIcon : public IconSprite::DynamicIcon {
public:
	virtual void onAdded() override;
	virtual void redraw(float pr, float diff) override;

protected:
	draw::Image::PathRef _path;
};

void IconSprite::NavIcon::onAdded() {
	DynamicIcon::onAdded();
	_top = _image->addPath().setFillColor(Color4B(255, 255, 255, 255));
	_center = _image->addPath().setFillColor(Color4B(255, 255, 255, 255));
	_bottom = _image->addPath().setFillColor(Color4B(255, 255, 255, 255));
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
	_path = _image->addPath(draw::Path().setStyle(stappler::draw::Path::Style::Stroke).setStrokeWidth(2.0f)).setFillColor(Color4B(255, 255, 255, 255));
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
		auto a = cocos2d::EaseBezierAction::create(Rc<_icons::IconProgress>::create(1.2f, 1.0f));
		a->setBezierParamer(0.0, 0.050, 0.950, 1.0);
		auto b = cocos2d::CallFunc::create([this] {
			static_cast<IconSprite *>(_owner)->setProgress(0.0f);
		});
		auto c = cocos2d::RepeatForever::create(cocos2d::Sequence::create(a, b, nullptr));
		_owner->stopActionByTag("Animate"_tag);
		_owner->runAction(c, "Animate"_tag);
	}
}

void IconSprite::ExpandIcon::onAdded() {
	DynamicIcon::onAdded();
	_path = _image->addPath(draw::Path()
		.setFillColor(Color4B(255, 255, 255, 255))
		.setStyle(draw::Path::Style::Fill)
		.moveTo(12.0f, 16.0f)
		.lineTo(18.0f, 10.0f)
		.lineTo(18.0f - float(M_SQRT2), 10.0f - float(M_SQRT2))
		.lineTo(12.0f, 14.0f)
		.lineTo(6.0f + float(M_SQRT2), 10.0f - float(M_SQRT2))
		.lineTo(6.0f, 10.0f)
		.closePath());
}

void IconSprite::ExpandIcon::redraw(float pr, float diff) {
	float p = pr;

	if (p <= 1.0f) {
		float rotation = _owner->getRotation();
		if (pr == 0.0f) {
			_owner->setRotation(0);
		} else if (pr == 1.0f) {
			_owner->setRotation(180);
		} else if ((diff < 0 && fabsf(rotation) == 180) || rotation > 0) {
			_owner->setRotation(progress(0, 180, pr));
		} else if ((diff > 0 && rotation == 0) || rotation < 0) {
			_owner->setRotation(progress(0, -180, pr));
		}
	}
}

bool IconSprite::init(IconName name, SizeHint s) {
	if (!PathNode::init(24, 24, draw::Format::A8)) {
		return false;
	}

	auto l = Rc<EventListener>::create();
	l->onEvent(IconStorage::onUpdate, [this] (const Event &) {
		onUpdate();
	});
	_listener = addComponentItem(l);

	setCascadeColorEnabled(true);
	setCascadeOpacityEnabled(true);
	setSizeHint(s);
	setIconName(name);

	return true;
}

void IconSprite::onContentSizeDirty() {
	PathNode::onContentSizeDirty();
	if (isStatic()) {
		auto storage = ResourceManager::getInstance()->getIconStorage(_contentSize);
		if (_storage != storage) {
			setIconName(_iconName);
		}
	}
}

void IconSprite::visit(cocos2d::Renderer *r, const Mat4 &t, uint32_t f, ZPath &z) {
	if (_diff != 0.0f) {
		_dynamicIcon->redraw(_progress, _diff);
		_diff = 0.0f;
	}
	PathNode::visit(r, t, f, z);
}

void IconSprite::onEnter() {
	PathNode::onEnter();
	if (_dynamicIcon) {
		_dynamicIcon->redraw(_progress, 0.0f);
	}
}

void IconSprite::setSizeHint(SizeHint s) {
	switch (s) {
	case SizeHint::Small: setContentSize(Size(18.0f, 18.0f)); break;
	case SizeHint::Normal: setContentSize(Size(24.0f, 24.0f)); break;
	case SizeHint::Large: setContentSize(Size(32.0f, 32.0f)); break;
	}
}

void IconSprite::setIconName(IconName name) {
	if (_iconName != name) {
		switch (name) {
		case IconName::Dynamic_Navigation:
			setDynamicIcon(Rc<NavIcon>::create());
			break;
		case IconName::Dynamic_Loader:
			setDynamicIcon(Rc<CircleLoaderIcon>::create());
			break;
		case IconName::Dynamic_Expand:
			setDynamicIcon(Rc<ExpandIcon>::create());
			break;
		case IconName::None:
		case IconName::Empty: {
			auto tmpSize = _contentSize;
			setDynamicIcon(nullptr);
			setContentSize(tmpSize);
			break;
		}
		default:
			setStaticIcon(name);
			break;
		}
		_iconName = name;
	}
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
	case IconName::Dynamic_Expand:
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
	case IconName::Dynamic_Expand:
	case IconName::None:
		return false;
		break;
	default:
		break;
	}
	return true;
}

void IconSprite::setProgress(float progress) {
	if (_dynamicIcon) {
		if (_progress != progress) {
			_diff = progress - _progress;
			_progress = progress;
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
			auto a = cocos2d::EaseQuadraticActionInOut::create(Rc<_icons::IconProgress>::create(duration, targetProgress));
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

void IconSprite::regenerate() {
	if (!isStatic()) {
		PathNode::regenerate();
	}
}

void IconSprite::updateCanvas(layout::Subscription::Flags f) {
	if (isDynamic() || !ResourceManager::getInstance()->getIconStorage(_contentSize)) {
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
	_storage = ResourceManager::getInstance()->getIconStorage(_contentSize);
	if (_storage) {
		auto tmpSize = _contentSize;
		setDynamicIcon(nullptr);
		_storage->addIcon(name);
		auto icon = _storage->getIcon(name);
		if (icon) {
			setImage(nullptr);
			setDensity(icon->getDensity());
			setTexture(_storage->getTexture(), icon->getTextureRect());
		} else {
			_contentSize = tmpSize;
		}
		_transformDirty = true;
	} else {
		setImage(Rc<layout::Image>::create(48, 48, IconStorage::getIconPath(name)));
	}
}

void IconSprite::onUpdate() {
	switch (_iconName) {
	case IconName::Dynamic_Navigation:
	case IconName::Dynamic_Loader:
	case IconName::Dynamic_Expand:
	case IconName::None:
	case IconName::Empty:
		break;
	default:
		setStaticIcon(_iconName);
		break;
	}
}

NS_MD_END
