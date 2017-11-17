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
#include "MaterialLabel.h"
#include "MaterialResourceManager.h"
#include "SPEventListener.h"

NS_MD_BEGIN

DynamicLabel::DescriptionStyle Label::getFontStyle(FontType t) {
	DescriptionStyle ret;
	ret.font.fontFamily = "system";
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
	case FontType::BodySmall:
		ret.font.fontSize = 12;
		ret.text.opacity = 222;
		break; // 12sp Regular 87%
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
	case FontType::ButtonSmall:
		ret.font.fontSize = 12;
		ret.font.fontWeight = font::FontWeight::W500;
		ret.text.opacity = 222;
		ret.text.textTransform = font::TextTransform::Uppercase;
		break;
	case FontType::InputDense:
		ret.font.fontSize = 13;
		ret.text.opacity = 222;
		break; // 13sp Regular 87%
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
	case FontType::Tab_Caption:
		ret.font.fontSize = 10;
		ret.text.opacity = 222;
		ret.text.textTransform = font::TextTransform::Uppercase;
		break;
	case FontType::Tab_Caption_Selected:
		ret.font.fontSize = 10;
		ret.font.fontWeight = font::FontWeight::W500;
		ret.text.textTransform = font::TextTransform::Uppercase;
		ret.text.opacity = 222;
		break;
	default: break;
	}
	return ret;
}

DynamicLabel::DescriptionStyle Label::getFontStyle(const String &name) {
	return ResourceManager::getInstance()->getFontStyle(name);
}

static font::FontSource *Label_getSourceForStyle(const DynamicLabel::DescriptionStyle &style) {
	auto m = ResourceManager::getInstance();
	if (style.font.fontFamily == "system") {
		return m->getSystemFontSource();
	} else {
		return m->getUserFontSource();
	}
}

void Label::preloadChars(FontType type, const Vector<char16_t> &chars) {
	auto style = getFontStyle(type);
	auto source = ResourceManager::getInstance()->getSystemFontSource();
	source->preloadChars(style.font, chars);
}
void Label::preloadChars(const String &type, const Vector<char16_t> &chars) {

}

bool Label::ExternalFormatter::init(bool userFont, float w) {
	auto m = ResourceManager::getInstance();
	auto source = (userFont?m->getUserFontSource():m->getSystemFontSource());
	if (!DynamicLabel::ExternalFormatter::init(source, w, screen::density())) {
		return false;
	}
	return true;
}

void Label::ExternalFormatter::addString(FontType style, const String &str, bool localized) {
	DynamicLabel::ExternalFormatter::addString(getFontStyle(style), string::toUtf16(str), localized);
}
void Label::ExternalFormatter::addString(FontType style, const WideString &str, bool localized) {
	DynamicLabel::ExternalFormatter::addString(getFontStyle(style), str, localized);
}
void Label::ExternalFormatter::addString(const String &style, const String &str, bool localized) {
	DynamicLabel::ExternalFormatter::addString(getFontStyle(style), string::toUtf16(str), localized);
}
void Label::ExternalFormatter::addString(const String &style, const WideString &str, bool localized) {
	DynamicLabel::ExternalFormatter::addString(getFontStyle(style), str, localized);
}
void Label::ExternalFormatter::addString(const DescriptionStyle &style, const String &str, bool localized) {
	DynamicLabel::ExternalFormatter::addString(style, string::toUtf16(str), localized);
}
void Label::ExternalFormatter::addString(const DescriptionStyle &style, const WideString &str, bool localized) {
	DynamicLabel::ExternalFormatter::addString(style, str, localized);
}

Size Label::getLabelSize(FontType t, const String &str, float w, float density, bool localized) {
	return getLabelSize(getFontStyle(t), string::toUtf16(str), w, density, localized);
}
Size Label::getLabelSize(FontType t, const WideString &str, float w, float density, bool localized) {
	return getLabelSize(getFontStyle(t), str, w, density, localized);
}

Size Label::getLabelSize(const String &style, const String &str, float w, float density, bool localized) {
	return getLabelSize(getFontStyle(style), string::toUtf16(str), w, density, localized);
}
Size Label::getLabelSize(const String &style, const WideString &str, float w, float density, bool localized) {
	return getLabelSize(getFontStyle(style), str, w, density, localized);
}
Size Label::getLabelSize(const DescriptionStyle &style, const WideString &str, float w, float density, bool localized) {
	return DynamicLabel::getLabelSize(Label_getSourceForStyle(style), style, str, w, density, localized);
}

float Label::getStringWidth(FontType t, const String &str, float density, bool localized) {
	return getStringWidth(getFontStyle(t), string::toUtf16(str), density, localized);
}
float Label::getStringWidth(FontType t, const WideString &str, float density, bool localized) {
	return getStringWidth(getFontStyle(t), str, density, localized);
}
float Label::getStringWidth(const String &t, const String &str, float density, bool localized) {
	return getStringWidth(getFontStyle(t), string::toUtf16(str), density, localized);
}
float Label::getStringWidth(const String &t, const WideString &str, float density, bool localized) {
	return getStringWidth(getFontStyle(t), str, density, localized);
}
float Label::getStringWidth(const DescriptionStyle &style, const WideString &str, float density, bool localized) {
	return DynamicLabel::getStringWidth(Label_getSourceForStyle(style), style, str, density, localized);
}

Label::~Label() { }

bool Label::init(FontType t, Alignment a, float w) {
	return init(getFontStyle(t), a, w);
}
bool Label::init(const String &str, Alignment a, float w) {
	return init(getFontStyle(str), a, w);
}
bool Label::init(const DescriptionStyle &style, Alignment a, float w) {
	auto source = Label_getSourceForStyle(style);
	if (!DynamicLabel::init(source, style, "", w, a)) {
		return false;
	}

	_listener->onEvent(ResourceManager::onUserFont, std::bind(&Label::onUserFont, this));

	return true;
}

void Label::setFont(FontType t) {
	setStyle(getFontStyle(t));
}

void Label::setStyle(const DescriptionStyle &style) {
	if (style != _style) {
		auto source = Label_getSourceForStyle(style);
		if (source != _source) {
			setSource(source);
		}
		_style = style;
		setOpacity(_style.text.opacity);
		_labelDirty = true;
	}
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

void Label::onUserFont() {
	if (_style.font.fontFamily != "system") {
		setSource(ResourceManager::getInstance()->getUserFontSource());
	}
}

void Label::onLightLevel() {
	if (isAutoLightLevel()) {
		auto level = material::ResourceManager::getInstance()->getLightLevel();
		switch(level) {
		case LightLevel::Dim:
			setColor(_dimColor);
			break;
		case LightLevel::Normal:
			setColor(_normalColor);
			break;
		case LightLevel::Washed:
			setColor(_washedColor);
			break;
		};
	}
}

NS_MD_END
