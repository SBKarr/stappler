/*
 * MaterialLabel.cpp
 *
 *  Created on: 13 нояб. 2014 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialLabel.h"
#include "MaterialColors.h"
#include "MaterialResourceManager.h"
#include "SPEventListener.h"

NS_MD_BEGIN

DynamicLabel::DescriptionStyle Label::getFontStyle(FontType t) {
	DescriptionStyle ret;
	ret.font.fontFamily = "default";
	switch (t) {
	case FontType::Headline:
		ret.font.fontSize = 24;
		ret.text.opacity = 222;
		break;
	case FontType::Title:
		ret.font.fontSize = 20;
		ret.font.fontWeight = font::FontWeight::W500;
		ret.text.opacity = 222;
		break;
	case FontType::Subhead:
		ret.font.fontSize = 16;
		ret.text.opacity = 222;
		break; // 16sp Regular 87%
	case FontType::Body_1:
		ret.font.fontSize = 14;
		ret.font.fontWeight = font::FontWeight::W500;
		ret.text.opacity = 222;
		break; // 14sp Medium 87%
	case FontType::Body_2:
		ret.font.fontSize = 14;
		ret.text.opacity = 222;
		break; // 14sp Regular 87%
	case FontType::Caption:
		ret.font.fontSize = 12;
		ret.text.opacity = 138;
		break; // 12sp Regular 54%
	case FontType::Button:
		ret.font.fontSize = 14;
		ret.font.fontWeight = font::FontWeight::W500;
		ret.text.opacity = 222;
		ret.text.textTransform = font::TextTransform::Uppercase;
		break;
	case FontType::Tab_Large:
		ret.font.fontSize = 14;
		ret.text.opacity = 222;
		ret.text.textTransform = font::TextTransform::Uppercase;
		break;
	case FontType::Tab_Large_Selected:
		ret.font.fontSize = 14;
		ret.font.fontWeight = font::FontWeight::W500;
		ret.text.textTransform = font::TextTransform::Uppercase;
		ret.text.opacity = 222;
		break;
	case FontType::Tab_Small:
		ret.font.fontSize = 12;
		ret.text.opacity = 222;
		ret.text.textTransform = font::TextTransform::Uppercase;
		break;
	case FontType::Tab_Small_Selected:
		ret.font.fontSize = 12;
		ret.font.fontWeight = font::FontWeight::W500;
		ret.text.textTransform = font::TextTransform::Uppercase;
		ret.text.opacity = 222;
		break;
	default: break;
	}
	return ret;
}

Size Label::getLabelSize(FontType t, const String &str, float w, float density, bool localized) {
	return getLabelSize(t, string::toUtf16(str), w, density, localized);
}
Size Label::getLabelSize(FontType t, const WideString &str, float w, float density, bool localized) {
	auto source = ResourceManager::getInstance()->getFontSource();
	auto style = getFontStyle(t);
	return DynamicLabel::getLabelSize(source, style, str, w, density, localized);
}

float Label::getStringWidth(FontType t, const String &str, float density, bool localized) {
	return getStringWidth(t, string::toUtf16(str), density, localized);
}
float Label::getStringWidth(FontType t, const WideString &str, float density, bool localized) {
	auto source = ResourceManager::getInstance()->getFontSource();
	auto style = getFontStyle(t);
	return DynamicLabel::getStringWidth(source, style, str, density, localized);
}

Label::~Label() { }

bool Label::init(FontType t, Alignment a, float w) {
	return init(getFontStyle(t), a, w);
}
bool Label::init(const DescriptionStyle &style, Alignment a, float w) {
	auto source = ResourceManager::getInstance()->getFontSource();
	if (!DynamicLabel::init(source, style, "", w, a)) {
		return false;
	}

	return true;
}

void Label::setFont(FontType t) {
	setStyle(getFontStyle(t));
}

void Label::setStyle(const DescriptionStyle &style) {
	_style = style;
    setOpacity(_style.text.opacity);
	_labelDirty = true;
}

void Label::onEnter() {
	DynamicLabel::onEnter();
	if (isAutoLightLevel()) {
		onLightLevel();
	}
}

void Label::setAutoLightLevel(bool value) {
	if (value && !_lightLevelListener) {
		_lightLevelListener = construct<EventListener>();
		_lightLevelListener->onEvent(ResourceManager::onLightLevel, std::bind(&Label::onLightLevel, this));
		addComponent(_lightLevelListener);
		if (isRunning()) {
			onLightLevel();
		}
	} else if (!value && _lightLevelListener) {
		removeComponent(_lightLevelListener);
		_lightLevelListener = nullptr;
	}
}

bool Label::isAutoLightLevel() const {
	return _lightLevelListener != nullptr;
}

void Label::setDimColor(const Color &c) {
	_dimColor = c;
	onLightLevel();
}
const Color &Label::getDimColor() const {
	return _dimColor;
}

void Label::setNormalColor(const Color &c) {
	_normalColor = c;
	onLightLevel();
}
const Color &Label::getNormalColor() const {
	return _normalColor;
}

void Label::setWashedColor(const Color &c) {
	_washedColor = c;
	onLightLevel();
}
const Color &Label::getWashedColor() const {
	return _washedColor;
}

void Label::onLightLevel() {
	if (isAutoLightLevel()) {
		auto level = material::ResourceManager::getInstance()->getLightLevel();
		switch(level) {
		case rich_text::style::LightLevel::Dim:
			setColor(_dimColor);
			break;
		case rich_text::style::LightLevel::Normal:
			setColor(_normalColor);
			break;
		case rich_text::style::LightLevel::Washed:
			setColor(_washedColor);
			break;
		};
	}
}

NS_MD_END
