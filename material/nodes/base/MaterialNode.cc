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
#include "MaterialNode.h"
#include "MaterialResourceManager.h"

#include "SPShadowSprite.h"
#include "SPRoundedSprite.h"
#include "SPClippingNode.h"
#include "SPEventListener.h"
#include "SPLayer.h"

#include "2d/CCActionInterval.h"
#include "base/CCDirector.h"
#include "SPDataListener.h"

NS_MD_BEGIN

static constexpr float MATERIAL_SHADOW_AMBIENT_MOD = 6.0f;
static constexpr float MATERIAL_SHADOW_KEY_MOD = 9.0f;

class ShadowAction : public cocos2d::ActionInterval {
public:
    /** creates the action with separate rotation angles */
    static ShadowAction* create(float duration, float targetIndex) {
    	ShadowAction *action = new ShadowAction();
    	if (action->init(duration, targetIndex)) {
    		action->autorelease();
    		return action;
    	} else {
    		delete action;
    		return nullptr;
    	}
    }

    virtual bool init(float duration, float targetIndex) {
    	if (!ActionInterval::initWithDuration(duration)) {
    		return false;
    	}

    	_destIndex = targetIndex;
    	return true;
    }

    virtual void startWithTarget(cocos2d::Node *t) override {
    	if (auto target = dynamic_cast<MaterialNode *>(t)) {
        	_sourceIndex = target->getShadowZIndex();
    	}
    	ActionInterval::startWithTarget(t);
    }

    virtual void update(float time) override {
    	auto target = dynamic_cast<MaterialNode *>(_target);
    	target->setShadowZIndex(_sourceIndex + (_destIndex - _sourceIndex) * time);

    }

    virtual void stop() override {
		ActionInterval::stop();
    }

protected:
    float _sourceIndex = 0;
    float _destIndex = 0;
};

bool MaterialNode::init() {
	if (!Node::init()) {
		return false;
	}

	auto backgroundClipper = Rc<RoundedSprite>::create((uint32_t)_borderRadius, screen::density());
	backgroundClipper->setAnchorPoint(Vec2(0, 0));
	backgroundClipper->setPosition(0, 0);
	backgroundClipper->setOpacity(255);
	backgroundClipper->setColor(Color::Black);
	backgroundClipper->setAlphaTest(AlphaTest::GreatherThen, 1);
	_backgroundClipper = backgroundClipper;

	auto shadowClipper = Rc<ClippingNode>::create(_backgroundClipper);
	shadowClipper->setAnchorPoint(Vec2(0, 0));
	shadowClipper->setPosition(0, 0);
	shadowClipper->setInverted(true);
	shadowClipper->setEnabled(false);
	shadowClipper->setCascadeColorEnabled(true);
	_shadowClipper = addChildNode(shadowClipper, -1);

	auto content = Rc<ClippingNode>::create(_backgroundClipper);
	content->setAnchorPoint(Vec2(0, 0));
	content->setPosition(0, 0);
	content->setInverted(false);
	content->setEnabled(false);
	_content = addChildNode(content, 0);

	auto background = Rc<RoundedSprite>::create((uint32_t)_borderRadius, screen::density());
	background->setAnchorPoint(Vec2(0, 0));
	background->setPosition(0, 0);
	background->setOpacity(255);
	background->setColor(Color::White);
	_background = _content->addChildNode(background, 0);

	setCascadeOpacityEnabled(true);

	ignoreAnchorPointForPosition(false);
	setAnchorPoint(Vec2(0, 0));

	auto ambientShadow = Rc<ShadowSprite>::create(_shadowIndex * MATERIAL_SHADOW_AMBIENT_MOD + _borderRadius,
			std::max(0.0f, _borderRadius - (_shadowIndex) * MATERIAL_SHADOW_AMBIENT_MOD));
	ambientShadow->setOpacity(64);
	ambientShadow->setAnchorPoint(Vec2(0, 0));
	ambientShadow->setVisible(false);
	_ambientShadow = _shadowClipper->addChildNode(ambientShadow, -1);

	auto keyShadow = Rc<ShadowSprite>::create(_shadowIndex * MATERIAL_SHADOW_KEY_MOD + _borderRadius,
			std::max(0.0f, _borderRadius - (_shadowIndex) * MATERIAL_SHADOW_KEY_MOD));
	keyShadow->setOpacity(128);
	keyShadow->setAnchorPoint(Vec2(0, 0));
	keyShadow->setVisible(false);
	_keyShadow = _shadowClipper->addChildNode(keyShadow, -2);

	return true;
}

void MaterialNode::onEnter() {
	Node::onEnter();
	if (isAutoLightLevel()) {
		onLightLevel();
	}
}

void MaterialNode::onTransformDirty() {
	Node::onTransformDirty();
	layoutShadows();
}

void MaterialNode::onContentSizeDirty() {
	Node::onContentSizeDirty();
	layoutShadows();
}

void MaterialNode::setShadowZIndex(float value) {
	if (_shadowIndex != value) {
		_shadowIndex = value;

		_ambientShadow->setTextureSize((_shadowIndex) * MATERIAL_SHADOW_AMBIENT_MOD + _borderRadius,
				std::max(0.0f, _borderRadius - (_shadowIndex) * MATERIAL_SHADOW_AMBIENT_MOD));
		_keyShadow->setTextureSize((_shadowIndex) * MATERIAL_SHADOW_KEY_MOD + _borderRadius,
				std::max(0.0f, _borderRadius - (_shadowIndex) * MATERIAL_SHADOW_KEY_MOD));

		if (value == 0.0f) {
			_ambientShadow->setVisible(false);
			_keyShadow->setVisible(false);
		} else {
			_ambientShadow->setVisible(true);
			_keyShadow->setVisible(true);
		}

		_ambientShadow->setOpacity(getOpacityForAmbientShadow(value));
		_keyShadow->setOpacity(getOpacityForKeyShadow(value));

		_contentSizeDirty = true;
	}
}

void MaterialNode::setShadowZIndexAnimated(float value, float duration) {
	if (_shadowIndex != value) {
		stopActionByTag(1);
		auto a = ShadowAction::create(duration, value);
		if (a) {
			a->setTag(1);
			runAction(a);
		}
	}
}

void MaterialNode::setBorderRadius(uint32_t value) {
	if (_borderRadius != value) {
		_borderRadius = value;

		_background->setTextureSize((uint32_t)_borderRadius);
		_backgroundClipper->setTextureSize((uint32_t)_borderRadius);

		_ambientShadow->setTextureSize((_shadowIndex) * MATERIAL_SHADOW_AMBIENT_MOD + _borderRadius,
				std::max(0.0f, _borderRadius - (_shadowIndex) * MATERIAL_SHADOW_AMBIENT_MOD));
		_keyShadow->setTextureSize((_shadowIndex) * MATERIAL_SHADOW_KEY_MOD + _borderRadius,
				std::max(0.0f, _borderRadius - (_shadowIndex) * MATERIAL_SHADOW_KEY_MOD));

		_contentSizeDirty = true;
	}
}
uint32_t MaterialNode::getBorderRadius() const {
	return _borderRadius;
}

void MaterialNode::setBackgroundOpacity(const uint8_t &op) {
	_background->setOpacity(op);
}
uint8_t MaterialNode::getBackgroundOpacity() const {
	return _background->getOpacity();
}

void MaterialNode::setBackgroundColor(const Color &c) {
	_background->setColor(c);
	//_backgroundClipper->setColor(c);
}
const Color3B &MaterialNode::getBackgroundColor() const {
	return _background->getColor();
}

void MaterialNode::setBackgroundVisible(bool value) {
	_background->setVisible(value);
}
bool MaterialNode::isBackgroundVisible() const {
	return _background->isVisible();
}

void MaterialNode::setUserData(const data::Value &d) {
	_userData = d;
}
void MaterialNode::setUserData(data::Value &&d) {
	_userData = std::move(d);
}
const data::Value &MaterialNode::getUserData() const {
	return _userData;
}

void MaterialNode::setClipContent(bool value) {
	static_cast<ClippingNode *>(_content)->setEnabled(value);
}
bool MaterialNode::isClipContent() const {
	return static_cast<const ClippingNode *>(_content)->isEnabled();
}

void MaterialNode::setPadding(const Padding &p) {
	if (p != _padding) {
		_padding = p;
		_contentSizeDirty = true;
	}
}
const Padding &MaterialNode::getPadding() const {
	return _padding;
}

void MaterialNode::layoutShadows() {
	if (_shadowIndex > 0) {
		_ambientShadow->setContentSize(getContentSizeForAmbientShadow(_shadowIndex));
		_ambientShadow->setPosition(getPositionForAmbientShadow(_shadowIndex));
		_ambientShadow->setVisible(true);

		_keyShadow->setContentSize(getContentSizeForKeyShadow(_shadowIndex));
		_keyShadow->setPosition(getPositionForKeyShadow(_shadowIndex));
		_keyShadow->setVisible(true);

		_ambientShadow->setOpacity(getOpacityForAmbientShadow(_shadowIndex));
		_keyShadow->setOpacity(getOpacityForKeyShadow(_shadowIndex));
	} else {
		_ambientShadow->setVisible(false);
		_keyShadow->setVisible(false);
	}

	auto size = getContentSizeWithPadding();
	auto vec = getAnchorPositionWithPadding();

	_backgroundClipper->setContentSize(size);
	_backgroundClipper->setPosition(Vec2::ZERO);

	_content->setContentSize(size);
	_content->setPosition(vec);

	_background->setContentSize(size);
	_background->setPosition(Vec2::ZERO);

	_shadowClipper->setContentSize(size);
	_shadowClipper->setPosition(vec);

	_positionsDirty = false;
}

void MaterialNode::updateColor() {
	cocos2d::Node::updateColor();
	if (_displayedOpacity == 255) {
		_shadowClipper->setEnabled(false);
	} else {
		_shadowClipper->setEnabled(true);
	}
}

void MaterialNode::onDataRecieved(stappler::data::Value &) { }

Size MaterialNode::getContentSizeWithPadding() const {
	return Size(MAX(_contentSize.width - _padding.left - _padding.right, 0),
			MAX(_contentSize.height - _padding.top - _padding.bottom, 0));
}
Vec2 MaterialNode::getAnchorPositionWithPadding() const {
	return Vec2(_padding.left, _padding.bottom);
}

Rect MaterialNode::getContentRect() const {
	return Rect(getAnchorPositionWithPadding(), getContentSizeWithPadding());
}

cocos2d::Node *MaterialNode::getContentNode() const {
	return _content;
}

Size MaterialNode::getContentSizeForAmbientShadow(float index) const {
	float ambientSeed = index * MATERIAL_SHADOW_AMBIENT_MOD;
	auto ambientSize = getContentSizeWithPadding();
	ambientSize.width += ambientSeed;
	ambientSize.height += ambientSeed;
	return ambientSize;
}
Vec2 MaterialNode::getPositionForAmbientShadow(float index) const {
	float ambientSeed = index * MATERIAL_SHADOW_AMBIENT_MOD;
	return Vec2(-ambientSeed / 2, -ambientSeed / 2);
}

Size MaterialNode::getContentSizeForKeyShadow(float index) const {
	float keySeed = index * MATERIAL_SHADOW_KEY_MOD;
	auto keySize = getContentSizeWithPadding();
	keySize.width += keySeed;
	keySize.height += keySeed;
	return keySize;
}
Vec2 MaterialNode::getPositionForKeyShadow(float index) const {
	Vec2 vec = convertToWorldSpace(Vec2(_contentSize.width / 2, _contentSize.height / 2));
	Size screenSize = cocos2d::Director::getInstance()->getWinSize();
	Vec2 lightSource(screenSize.width / 2, screenSize.height);
	Vec2 normal = lightSource - vec;
	normal.normalize();

	normal = normal - Vec2(0, -1);

	float keySeed = index * MATERIAL_SHADOW_KEY_MOD;
	return Vec2(-keySeed / 2, -keySeed / 2) - normal * _shadowIndex * 0.5;
}

uint8_t MaterialNode::getOpacityForAmbientShadow(float value) const {
	uint8_t ret = 72;
	auto size = getContentSizeForAmbientShadow(value);
	auto texSize = _ambientShadow->getTextureSize() - _ambientShadow->getTextureRadius();
	float min = MIN(size.width, size.height) - texSize;
	if (min < 0) {
		ret = 0;
	} else if (min < texSize * 2) {
		ret = (float)ret * (min / (texSize * 2.0f));
	}
	return ret;
}
uint8_t MaterialNode::getOpacityForKeyShadow(float value) const {
	uint8_t ret = 128;
	auto size = getContentSizeForKeyShadow(value);
	auto texSize = _keyShadow->getTextureSize() - _keyShadow->getTextureRadius();
	float min = MIN(size.width, size.height) - texSize;
	if (min < 0) {
		ret = 0;
	} else if (min < texSize * 2) {
		ret = (float)ret * (min / (texSize * 2.0f));
	}
	return ret;
}

void MaterialNode::setAutoLightLevel(bool value) {
	if (value && !_lightLevelListener) {
		auto lightLevelListener = Rc<EventListener>::create();
		lightLevelListener->onEvent(ResourceManager::onLightLevel, std::bind(&MaterialNode::onLightLevel, this));
		_lightLevelListener = addComponentItem(lightLevelListener);
		if (isRunning()) {
			onLightLevel();
		}
	} else if (!value && _lightLevelListener) {
		removeComponent(_lightLevelListener);
		_lightLevelListener = nullptr;
	}
}
bool MaterialNode::isAutoLightLevel() const {
	return _lightLevelListener != nullptr;
}

void MaterialNode::setDimColor(const Color &c) {
	_dimColor = c;
	onLightLevel();
}
const Color &MaterialNode::getDimColor() const {
	return _dimColor;
}

void MaterialNode::setNormalColor(const Color &c) {
	_normalColor = c;
	onLightLevel();
}
const Color &MaterialNode::getNormalColor() const {
	return _normalColor;
}

void MaterialNode::setWashedColor(const Color &c) {
	_washedColor = c;
	onLightLevel();
}
const Color &MaterialNode::getWashedColor() const {
	return _washedColor;
}

void MaterialNode::onLightLevel() {
	if (isAutoLightLevel()) {
		auto level = material::ResourceManager::getInstance()->getLightLevel();
		switch(level) {
		case LightLevel::Dim:
			setBackgroundColor(_dimColor);
			break;
		case LightLevel::Normal:
			setBackgroundColor(_normalColor);
			break;
		case LightLevel::Washed:
			setBackgroundColor(_washedColor);
			break;
		};
	}
}

NS_MD_END
