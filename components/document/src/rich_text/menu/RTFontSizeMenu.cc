// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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
#include "RTFontSizeMenu.h"

NS_RT_BEGIN

float FontSizeMenuButton::FontScaleFactor() {
	return 1.0f / stappler::screen::density();
}

bool FontSizeMenuButton::init() {
	if (!MenuSourceButton::init()) {
		return false;
	}

	onEvent(material::ResourceManager::onUserFont, std::bind(&FontSizeMenuButton::onTextFontsChanged, this));

	auto fontConfig = Rc<material::MenuSource>::create();

	auto c = fontConfig->addCustom(24, std::bind(&FontSizeMenuButton::onLabel, this), 120);

	_buttons.push_back(fontConfig->addButton("200%"));
	_buttons.push_back(fontConfig->addButton("175%"));
	_buttons.push_back(fontConfig->addButton("150%"));
	_buttons.push_back(fontConfig->addButton("125%"));
	_buttons.push_back(fontConfig->addButton("100%"));

	uint32_t id = 0;
	for (auto &btn : _buttons) {
		btn->setCallback(std::bind(&FontSizeMenuButton::onMenuButton, this, id));
		id ++;
	}

	setNextMenu(fontConfig);
	setNameIcon(material::IconName::Stappler_text_format_100);
	setName("SystemFontSize"_locale);

	onTextFontsChanged();

	return true;
}

Rc<cocos2d::Node> FontSizeMenuButton::onLabel() {
	auto n = Rc<cocos2d::Node>::create();

	auto l = Rc<material::Label>::create(material::FontType::Caption);
	l->setLocaleEnabled(true);
	l->setAutoLightLevel(true);
	l->setString("SystemFontSize"_locale.to_string());
	l->setPosition(16, 12);
	l->setAnchorPoint(cocos2d::Vec2(0, 0.5f));

	n->addChild(l);
	return n;
}

void FontSizeMenuButton::onMenuButton(uint32_t id) {
	switch(id) {
	case 0:
		setNameIcon(material::IconName::Stappler_text_format_200);
		material::ResourceManager::getInstance()->setUserFontScale(1.0f + 1.0f * FontScaleFactor());
		break;
	case 1:
		setNameIcon(material::IconName::Stappler_text_format_175);
		material::ResourceManager::getInstance()->setUserFontScale(1.0f + 0.75f * FontScaleFactor());
		break;
	case 2:
		setNameIcon(material::IconName::Stappler_text_format_150);
		material::ResourceManager::getInstance()->setUserFontScale(1.0f + 0.5f * FontScaleFactor());
		break;
	case 3:
		setNameIcon(material::IconName::Stappler_text_format_125);
		material::ResourceManager::getInstance()->setUserFontScale(1.0f + 0.25f * FontScaleFactor());
		break;
	case 4:
		setNameIcon(material::IconName::Stappler_text_format_100);
		material::ResourceManager::getInstance()->setUserFontScale(1.0f + 0.0f * FontScaleFactor());
		break;
	default: break;
	}

	for (auto & it : _buttons) {
		it->setSelected(false);
	}
	_buttons.at(id)->setSelected(true);
}

void FontSizeMenuButton::onTextFontsChanged() {
	for (auto & it : _buttons) {
		it->setSelected(false);
	}

	auto scale = material::ResourceManager::getInstance()->getUserFontScale();
	if (scale < 1.0f + 0.125f * FontScaleFactor()) {
		setNameIcon(material::IconName::Stappler_text_format_100);
		_buttons.at(4)->setSelected(true);
	} else if (scale < 1.0f + 0.375f * FontScaleFactor()) {
		setNameIcon(material::IconName::Stappler_text_format_125);
		_buttons.at(3)->setSelected(true);
	} else if (scale < 1.0f + 0.625f * FontScaleFactor()) {
		setNameIcon(material::IconName::Stappler_text_format_150);
		_buttons.at(2)->setSelected(true);
	} else if (scale < 1.0f + 0.875f * FontScaleFactor()) {
		setNameIcon(material::IconName::Stappler_text_format_175);
		_buttons.at(1)->setSelected(true);
	} else {
		setNameIcon(material::IconName::Stappler_text_format_200);
		_buttons.at(0)->setSelected(true);
	}
}

NS_RT_END
