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
#include "MaterialImage.h"
#include "MaterialResourceManager.h"
#include "SPDynamicSprite.h"
#include "SPNetworkSprite.h"
#include "SPRoundedSprite.h"
#include "SPActions.h"

NS_MD_BEGIN

MaterialImage::~MaterialImage() {
	if (_network) {
		_network->setCallback(nullptr);
	}
}

bool MaterialImage::init(const String &file, float density) {
	if (!MaterialNode::init()) {
		return false;
	}

	_background->setVisible(false);
	setCascadeColorEnabled(true);

	Rc<DynamicSprite> sprite;
	if (!file.empty()) {
		sprite = Rc<DynamicSprite>::create(file, Rect::ZERO, density);
	} else {
		sprite = Rc<DynamicSprite>::create(nullptr, Rect::ZERO, density);
	}

	sprite->setPosition(Vec2::ZERO);
	sprite->setAnchorPoint(Vec2(0.0f, 0.0f));
	sprite->setVisible(true);
	sprite->setAutofit(Autofit::Contain);
	sprite->setForceI8Texture(true);
	_content->addChild(sprite, 0);
	_sprite = sprite;

	auto network = Rc<NetworkSprite>::create("", density);
	network->setPosition(Vec2::ZERO);
	network->setAnchorPoint(Vec2(0.0f, 0.0f));
	network->setOpacity(0);
	network->setAutofit(Autofit::Contain);
	network->setVisible(false);
	network->setForceI8Texture(true);
	_content->addChild(network, 1);
	_network = network;

	return true;
}

void MaterialImage::setContentSize(const Size &size) {
	MaterialNode::setContentSize(size);

	_sprite->setContentSize(_content->getContentSize());
	_network->setContentSize(_content->getContentSize());
}

void MaterialImage::onContentSizeDirty() {
	_sprite->setContentSize(_content->getContentSize());
	_network->setContentSize(_content->getContentSize());
}

void MaterialImage::visit(cocos2d::Renderer *r, const Mat4 &t, uint32_t f, ZPath &zPath) {
	if (!_init) {
		_init = true;
		if (auto tex = getTexture()) {
			if (_callback) {
				_callback(tex->getPixelsWide(), tex->getPixelsHigh());
			}
		}
	}
	MaterialNode::visit(r, t, f, zPath);
}

void MaterialImage::onEnter() {
	MaterialNode::onEnter();

	_sprite->setCallback([this] (cocos2d::Texture2D *tex) {
		_contentSizeDirty = true;
	});
	_network->setCallback([this] (cocos2d::Texture2D *tex) {
		_contentSizeDirty = true;
		onNetworkSprite();
	});
	onNetworkSprite();
}

void MaterialImage::onExit() {
	_sprite->setCallback(nullptr);
	_network->setCallback(nullptr);

	MaterialNode::onExit();
}

void MaterialImage::setAutofit(Autofit value) {
	_sprite->setAutofit(value);
	_network->setAutofit(value);
}

void MaterialImage::setAutofitPosition(const cocos2d::Vec2 &vec) {
	_sprite->setAutofitPosition(vec);
	_network->setAutofitPosition(vec);
}

const String &MaterialImage::getUrl() const {
	return _network->getUrl();
}

void MaterialImage::setUrl(const String &url, bool force) {
	_network->setUrl(url, force);
}

void MaterialImage::setImageSizeCallback(const ImageSizeCallback &cb) {
	_callback = cb;
}

stappler::DynamicSprite * MaterialImage::getSprite() const {
	return _sprite;
}
stappler::NetworkSprite * MaterialImage::getNetworkSprite() const {
	return _network;
}

cocos2d::Texture2D *MaterialImage::getTexture() const {
	auto ret = _network->getTexture();
	if (!ret) {
		ret = _sprite->getTexture();
	}
	return ret;
}

void MaterialImage::onNetworkSprite() {
	auto tex = _network->getTexture();
	if (!_init || !tex) {
		if (tex) {
			_sprite->setVisible(false);
			_network->setVisible(true);
			_network->setOpacity(255);
		} else {
			_sprite->setVisible(true);
			_network->setVisible(false);
			_network->setOpacity(0);
		}
	} else {
		_network->setVisible(true);
		_network->stopActionByTag(126);
		_network->runAction(action::sequence(cocos2d::FadeIn::create(0.25f), [this] {
			_sprite->setVisible(false);
			_network->setVisible(true);
		}), 126);
	}
	if (!tex) {
		tex = _sprite->getTexture();
	}

	if (_callback && tex) {
		_callback(tex->getPixelsWide(), tex->getPixelsHigh());
	}
}

NS_MD_END
