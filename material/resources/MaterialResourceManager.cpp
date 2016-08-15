/*
 * MaterialResourceManager.cpp
 *
 *  Created on: 13 нояб. 2014 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialResourceManager.h"
#include "MaterialIconSources.h"

#include "SPLocale.h"
#include "SPData.h"
#include "SPScreen.h"
#include "SPResource.h"
#include "SPFont.h"
#include "SPDevice.h"
#include "SPStorage.h"
#include "SPString.h"

#include "platform/CCImage.h"
#include "renderer/CCTexture2D.h"

NS_MD_BEGIN

SP_DECLARE_EVENT(ResourceManager, "Material", onLoaded);
SP_DECLARE_EVENT(ResourceManager, "Material", onSystemFonts);
SP_DECLARE_EVENT(ResourceManager, "Material", onSystemFontsReload);
SP_DECLARE_EVENT(ResourceManager, "Material", onTextFonts);
SP_DECLARE_EVENT(ResourceManager, "Material", onTextFontsReload);
SP_DECLARE_EVENT(ResourceManager, "Material", onLightLevel);

static ResourceManager *s_instance = nullptr;

ResourceManager *ResourceManager::getInstance() {
	if (!s_instance) {
		s_instance = new ResourceManager();
	}
	return s_instance;
}

ResourceManager::ResourceManager() {
	stappler::storage::get("Material.ResourceManager", [this] (const std::string &key, stappler::data::Value &value) {
		if (value.isDictionary()) {
			float systemFontScale = value.getDouble("SystemFontScale");
			float textFontScale = value.getDouble("TextFontScale");
			LightLevel level = (LightLevel)value.getInteger("LightLevel", (int)LightLevel::Normal);

			if (systemFontScale != 0.0) {
				_systemFontScale = systemFontScale;
			}

			if (textFontScale != 0.0) {
				_textFontScale = textFontScale;
			}

			_lightLevel = level;

			if (value.isString("Locale") && value.isBool("LocaleCustom")) {
				_locale = value.getString("Locale");
				_localeCustom = true;
			}
			if (_locale.empty()) {
				_locale = Device::getInstance()->getUserLanguage();
			}
			locale::setLocale(_locale);
		} else {
			_locale = Device::getInstance()->getUserLanguage();
			locale::setLocale(_locale);
		}

		update();
	});

	_iconNames = getMaterialIconsNames();
}

ResourceManager::~ResourceManager() { }

void ResourceManager::update() {
    float density = stappler::screen::density();
    stappler::IconSet::generate(stappler::IconSet::Config{"material", getMaterialIconVersion(), getMaterialIconSources(),
    	48, 48, (uint16_t)(24 * density), (uint16_t)(24 * density)}, [this] (stappler::IconSet *set) {
    		_iconSet = set;
    		if (_iconSet && _systemFonts) {
    			_init = true;
    			ResourceManager::onLoaded(this);
    		}
    	});

    std::string systemFontsName = toString(std::fixed, "material.SystemFonts.", density * _systemFontScale);

    stappler::FontSet::generate(stappler::FontSet::Config(systemFontsName, 2, FontSet::getRequest(density * _systemFontScale)),
    		[this] (stappler::FontSet *set) {
		_systemFonts = Rc<material::FontSet>::create(set);
		if (_iconSet && _systemFonts) {
			_init = true;
			ResourceManager::onLoaded(this);
		}
    });
}

void ResourceManager::updateSystemFonts() {
    float density = stappler::Screen::getInstance()->getDensity();

    std::string systemFontsName = toString(std::fixed, "material.SystemFonts.", density * _systemFontScale);
    stappler::FontSet::generate(stappler::FontSet::Config(systemFontsName, 2, FontSet::getRequest(density * _systemFontScale)),
    		[this] (stappler::FontSet *set) {
		_systemFonts = Rc<material::FontSet>::create(set);
		onSystemFonts(this);
		saveUserData();
    });

	onSystemFontsReload(this);
}

void ResourceManager::updateTextFonts() {
	onTextFontsReload(this);
	onTextFonts(this);
	saveUserData();
}

void ResourceManager::saveUserData() {
	stappler::data::Value val(stappler::data::Value::Type::DICTIONARY);

	val.setDouble(_systemFontScale, "SystemFontScale");
	val.setDouble(_textFontScale, "TextFontScale");
	val.setInteger((int)_lightLevel, "LightLevel");
	val.setString(_locale, "Locale");
	val.setBool(_localeCustom, "LocaleCustom");

	stappler::storage::set("Material.ResourceManager", val);
}

void ResourceManager::setSystemFontScale(float scale) {
	if (scale != _systemFontScale) {
		_systemFontScale = scale;
		updateSystemFonts();
	}
}
float ResourceManager::getSystemFontScale() const {
	return _systemFontScale;
}

void ResourceManager::setTextFontScale(float scale) {
	if (scale != _textFontScale) {
		_textFontScale = scale;
		updateTextFonts();
	}
}
float ResourceManager::getTextFontScale() const {
	return _textFontScale;
}

void ResourceManager::setLightLevel(LightLevel value) {
	if (value != _lightLevel) {
		_lightLevel = value;
		onLightLevel(this, (int64_t)_lightLevel);
		saveUserData();
	}
}
ResourceManager::LightLevel ResourceManager::getLightLevel() const {
	return _lightLevel;
}

void ResourceManager::setLocale(const std::string &loc) {
	if (_locale != loc) {
		_locale = loc;
		_localeCustom = true;
		locale::setLocale(loc);
		saveUserData();
	}
}

const std::string &ResourceManager::getLocale() const {
	return _locale;
}

bool ResourceManager::isLoaded() {
	return _init;
}

const material::Font * ResourceManager::getSystemFont(Font::Type type) const {
	return _systemFonts->getFont(type);
}

material::FontSet *ResourceManager::getSystemFontSet() const {
	return _systemFonts;
}

stappler::Icon *ResourceManager::getIcon(IconName name) {
	auto it = _iconNames.find(name);
	if (it != _iconNames.end()) {
		if (_iconSet) {
			return _iconSet->getIcon(it->second);
		}
	}
	return nullptr;
}

NS_MD_END
