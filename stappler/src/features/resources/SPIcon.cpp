/*
 * SPIcon.cpp
 *
 *  Created on: 07 апр. 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPIcon.h"
#include "SPImage.h"
#include "renderer/CCTexture2D.h"
#include "SPFilesystem.h"
#include "SPResource.h"

NS_SP_BEGIN

Icon::Icon() : _id(0), _x(0), _y(0), _width(0), _height(0), _density(0.0f) { }

Icon::Icon(uint16_t id, uint16_t x, uint16_t y, uint16_t width, uint16_t height, float density, Image *image)
: _id(id), _x(x), _y(y), _width(width), _height(height), _density(density) {
	_image = image;
}

Icon::~Icon() { }

cocos2d::Texture2D *Icon::getTexture() const {
	auto tex = getImage()->retainTexture();
	tex->setAliasTexParameters();
	return tex;
}

cocos2d::Rect Icon::getTextureRect() const {
	return cocos2d::Rect(_x, _y, _width, _height);
}

void IconSet::generate(Config &&cfg, const std::function<void(IconSet *)> &callback) {
	resource::generateIconSet(std::move(cfg), callback);
}

IconSet::IconSet(Config &&cfg, Map<String, Icon> &&icons, Image *image)
: _config(std::move(cfg)), _image(image), _icons(std::move(icons)) {
	_image->retainData();

	_texWidth = _image->getWidth();
	_texHeight = _image->getHeight();
}

IconSet::~IconSet() {
	_image->releaseData();
}

Icon IconSet::getIcon(const std::string &key) const {
	auto it = _icons.find(key);
	if (it == _icons.end()) {
		return Icon();
	}
	return it->second;
}

NS_SP_END
