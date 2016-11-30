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
#include "MaterialSingleLineField.h"
#include "SPGestureListener.h"

#include "SPLayer.h"
#include "SPString.h"

NS_MD_BEGIN

bool SingleLineField::init(FontType font, float width) {
	if (!TextField::init(font, width)) {
		return false;
	}

	_underlineLayer = construct<Layer>(material::Color::Grey_500);
	_underlineLayer->setOpacity(168);
	addChild(_underlineLayer, 1);

	return true;
}

void SingleLineField::onContentSizeDirty() {
	TextField::onContentSizeDirty();

	_underlineLayer->setContentSize(Size(_contentSize.width - _padding.horizontal(), 2.0f));
	_underlineLayer->setPosition(_padding.left, _padding.bottom / 2.0f);
}

void SingleLineField::setInputCallback(const Callback &cb) {
	_onInput = cb;
}
const SingleLineField::Callback &SingleLineField::getInputCallback() const {
	return _onInput;
}

void SingleLineField::onError(Error err) {
	TextField::onError(err);
	_underlineLayer->setColor(_errorColor);
}

void SingleLineField::updateFocus() {
	TextField::updateFocus();
	if (_underlineLayer) {
		if (_handler.isActive()) {
			_underlineLayer->setOpacity(168);
			_underlineLayer->setColor(_normalColor);
		} else {
			_underlineLayer->setOpacity(64);
			_underlineLayer->setColor(Color::Grey_500);
		}
	}
}

bool SingleLineField::updateString(const std::u16string &nstr, const Cursor &nc) {
	auto str = nstr;
	auto c = nc;

	auto pos = MIN(str.find(u'\n'), str.find(u'\r'));
	if (pos != std::u16string::npos) {
		str = str.substr(0, pos);
		c = Cursor(str.length(), 0);
		releaseInput();
		if (_onInput) {
			_onInput();
		}
		return true;
	}

	return TextField::updateString(str, c);
}

NS_MD_END
