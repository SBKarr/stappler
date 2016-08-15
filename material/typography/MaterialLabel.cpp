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

bool Label::init(const Font *font, Label::Alignment alignment, float width) {
	if (!stappler::RichLabel::init(font->getFont(), "", width, alignment, stappler::screen::density())) {
		return false;
	}

	_materialFont = font;
	setOpacity(font->getOpacity());
	setColor(Color::Black);
	if (font->isUppercase()) {
		setTextTransform(RichLabel::Style::TextTransform::Uppercase);
	} else {
		setTextTransform(RichLabel::Style::TextTransform::None);
	}

	auto el = construct<EventListener>();
	el->onEvent(ResourceManager::onLoaded, [this] (const stappler::Event *) {
		setFont(_materialFont->getFont());
	});
	addComponent(el);

	return true;
}

bool Label::init(Font::Type t, Alignment a, float d) {
	return init(Font::systemFont(t), a, d);
}

void Label::onEnter() {
	RichLabel::onEnter();
	if (isAutoLightLevel()) {
		onLightLevel();
	}
}

void Label::setMaterialFont(const material::Font *font) {
	if (font) {
		setFont(font->getFont());
		setOpacity(font->getOpacity());
		if (font->isUppercase()) {
			setTextTransform(RichLabel::Style::TextTransform::Uppercase);
		} else {
			setTextTransform(RichLabel::Style::TextTransform::None);
		}
	} else {
		setFont(nullptr);
		setTextTransform(RichLabel::Style::TextTransform::None);
	}
	_materialFont = font;
}
const material::Font *Label::getMaterialFont() {
	return _materialFont;
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
