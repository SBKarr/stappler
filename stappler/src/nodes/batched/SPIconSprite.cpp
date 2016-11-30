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

#include "SPDefine.h"
#include "SPIconSprite.h"
#include "SPIcon.h"

NS_SP_BEGIN

bool IconSprite::init() {
	if (!DynamicSprite::init()) {
		return false;
	}

	return true;
}

bool IconSprite::init(const Icon &icon) {
	if (!DynamicSprite::init(icon.getTexture(), icon.getTextureRect(), icon.getDensity())) {
		return false;
	}

	_icon = icon;

	return true;
}

void IconSprite::setIcon(const Icon &icon) {
	if (icon != _icon) {
		_icon = icon;

		if (icon) {
			setTexture(_icon.getTexture());
			setTextureRect(_icon.getTextureRect());
			setDensity(_icon.getDensity());
		} else {
			setTexture(nullptr);
		}
	}
}

const Icon &IconSprite::getIcon() const {
	return _icon;
}

NS_SP_END
