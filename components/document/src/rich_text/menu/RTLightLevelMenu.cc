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
#include "RTLightLevelMenu.h"

NS_RT_BEGIN

bool LightLevelMenuButton::init() {
	if (!MenuSourceButton::init()) {
		return false;
	}

	onEvent(material::ResourceManager::onLightLevel, std::bind(&LightLevelMenuButton::onLightLevelChanged, this));

	auto lightLevelConfig = Rc<material::MenuSource>::create();
	auto c = lightLevelConfig->addCustom(24, std::bind(&LightLevelMenuButton::onLabel, this), 170);

	_buttons.push_back(lightLevelConfig->addButton("SystemThemeLight"_locale, material::IconName::Image_brightness_7));
	_buttons.push_back(lightLevelConfig->addButton("SystemThemeNeutral"_locale, material::IconName::Image_brightness_5));
	_buttons.push_back(lightLevelConfig->addButton("SystemThemeDark"_locale, material::IconName::Image_brightness_3));

	uint32_t id = 0;
	for (auto &btn : _buttons) {
		btn->setCallback(std::bind(&LightLevelMenuButton::onMenuButton, this, id));
		id ++;
	}

	setNextMenu(lightLevelConfig);
	setNameIcon(material::IconName::Image_brightness_5);
	setName("SystemTheme"_locale);

	onLightLevelChanged();

	return true;
}

Rc<cocos2d::Node> LightLevelMenuButton::onLabel() {
	auto n = Rc<cocos2d::Node>::create();

	auto l = Rc<material::Label>::create(material::FontType::Caption);
	l->setLocaleEnabled(true);
	l->setAutoLightLevel(true);
	l->setString("SystemTheme"_locale.to_string());
	l->setPosition(16, 12);
	l->setAnchorPoint(cocos2d::Vec2(0, 0.5f));
	n->addChild(l);

	return n;
}

void LightLevelMenuButton::onMenuButton(uint32_t id) {
	switch(id) {
	case 0:
		setNameIcon(material::IconName::Image_brightness_7);
		material::ResourceManager::getInstance()->setLightLevel(material::ResourceManager::LightLevel::Washed);
		break;
	case 1:
		setNameIcon(material::IconName::Image_brightness_5);
		material::ResourceManager::getInstance()->setLightLevel(material::ResourceManager::LightLevel::Normal);
		break;
	case 2:
		setNameIcon(material::IconName::Image_brightness_3);
		material::ResourceManager::getInstance()->setLightLevel(material::ResourceManager::LightLevel::Dim);
		break;
	default: break;
	}

	for (auto & it : _buttons) {
		it->setSelected(false);
	}
	_buttons.at(id)->setSelected(true);
}

void LightLevelMenuButton::onLightLevelChanged() {
	for (auto & it : _buttons) {
		it->setSelected(false);
	}

	auto level = material::ResourceManager::getInstance()->getLightLevel();
	if (level == material::ResourceManager::LightLevel::Dim) {
		setNameIcon(material::IconName::Image_brightness_3);
		_buttons.at(2)->setSelected(true);
	} else if (level == material::ResourceManager::LightLevel::Normal) {
		setNameIcon(material::IconName::Image_brightness_5);
		_buttons.at(1)->setSelected(true);
	} else if (level == material::ResourceManager::LightLevel::Washed) {
		setNameIcon(material::IconName::Image_brightness_7);
		_buttons.at(0)->setSelected(true);
	}
}

NS_RT_END
