/*
 * MaterialSingleLineInput.cpp
 *
 *  Created on: 13 окт. 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialSingleLineField.h"
#include "SPGestureListener.h"

#include "SPLayer.h"
#include "SPString.h"

NS_MD_BEGIN

bool SingleLineField::init(const Font *font, float width) {
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
