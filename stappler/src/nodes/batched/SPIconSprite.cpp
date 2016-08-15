/*
 * SPIconSprite.cpp
 *
 *  Created on: 08 апр. 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPIconSprite.h"
#include "SPIcon.h"

NS_SP_BEGIN

bool IconSprite::init(Icon *icon) {
	if (icon) {
		if (!DynamicSprite::init(icon->getTexture(), icon->getTextureRect(), icon->getDensity())) {
			return false;
		}
	} else {
		if (!DynamicSprite::init()) {
			return false;
		}
	}

	_icon = icon;

	return true;
}

void IconSprite::setIcon(Icon *icon) {
	if (icon != _icon) {
		_icon = icon;

		if (icon) {
			setTexture(_icon->getTexture());
			setTextureRect(_icon->getTextureRect());
			setDensity(_icon->getDensity());
		} else {
			setTexture(nullptr);
		}
	}

}
Icon *IconSprite::getIcon() const {
	return _icon;
}

NS_SP_END
