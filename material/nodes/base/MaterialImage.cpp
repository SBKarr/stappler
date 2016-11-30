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
#include "SPFadeTo.h"
#include "2d/CCActionInstant.h"

NS_MD_BEGIN

MaterialImage::~MaterialImage() {
	if (_network) {
		_network->setCallback(nullptr);
	}
}

bool MaterialImage::init(const std::string &file, float density) {
	if (!MaterialNode::init()) {
		return false;
	}

	_background->setVisible(false);

	_sprite = construct<DynamicSprite>(file, cocos2d::Rect::ZERO, density);
	_sprite->setAnchorPoint(cocos2d::Vec2(0.0f, 0.0f));
	_sprite->setVisible(true);
	_sprite->setAutofit(Autofit::Contain);
	addChild(_sprite, 0);

	_network = construct<NetworkSprite>("", density);
	_network->setAnchorPoint(cocos2d::Vec2(0.0f, 0.0f));
	_network->setOpacity(0);
	_network->setAutofit(Autofit::Contain);
	_network->setVisible(false);
	addChild(_network, 1);

	return true;
}

void MaterialImage::setContentSize(const cocos2d::Size &size) {
	MaterialNode::setContentSize(size);
	_sprite->setContentSize(size);
	_network->setContentSize(size);
}

void MaterialImage::visit(cocos2d::Renderer *r, const cocos2d::Mat4 &t, uint32_t f, ZPath &zPath) {
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

const std::string &MaterialImage::getUrl() const {
	return _network->getUrl();
}

void MaterialImage::setUrl(const std::string &url, bool force) {
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
		auto a = cocos2d::Sequence::createWithTwoActions(cocos2d::FadeIn::create(0.25f), cocos2d::CallFunc::create([this] {
			_sprite->setVisible(false);
			_network->setVisible(true);
		}));
		a->setTag(126);
		_network->runAction(a);
	}
	if (!tex) {
		tex = _sprite->getTexture();
	}

	if (_callback && tex) {
		_callback(tex->getPixelsWide(), tex->getPixelsHigh());
	}
}

NS_MD_END
