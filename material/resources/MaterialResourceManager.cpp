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
#include "SPFilesystem.h"

#include "platform/CCImage.h"
#include "renderer/CCTexture2D.h"

NS_MD_BEGIN

SP_DECLARE_EVENT(ResourceManager, "Material", onLoaded);
SP_DECLARE_EVENT(ResourceManager, "Material", onLightLevel);

static ResourceManager *s_instance = nullptr;

ResourceManager *ResourceManager::getInstance() {
	if (!s_instance) {
		s_instance = new ResourceManager();
	}
	return s_instance;
}

ResourceManager::ResourceManager() {
	locale::define("ru-ru", {
		pair("SystemSearch", "Найти"),
		pair("SystemFontSize", "Размер шрифта"),
		pair("SystemTheme", "Оформление"),
		pair("SystemMore", "Ещё"),
	});

	locale::define("en-us", {
		pair("SystemSearch", "Search"),
		pair("SystemFontSize", "Font size"),
		pair("SystemTheme", "Theme"),
		pair("SystemMore", "More"),
	});

	using namespace font;
	_source = Rc<Source>::create(Source::FontFaceMap{
		pair("default", Vector<FontFace>{
			FontFace("Roboto-Black.woff", FontStyle::Normal, FontWeight::W800),
			FontFace("Roboto-BlackItalic.woff", FontStyle::Italic, FontWeight::W800),
			FontFace("Roboto-Bold.woff", FontStyle::Normal, FontWeight::W700),
			FontFace("Roboto-BoldItalic.woff", FontStyle::Italic, FontWeight::W700),
			FontFace("RobotoCondensed-Bold.woff", FontStyle::Normal, FontWeight::W700, FontStretch::Condensed),
			FontFace("RobotoCondensed-BoldItalic.woff", FontStyle::Italic, FontWeight::W700, FontStretch::Condensed),
			FontFace("RobotoCondensed-Italic.woff", FontStyle::Italic, FontWeight::Normal, FontStretch::Condensed),
			FontFace("RobotoCondensed-Light.woff", FontStyle::Normal, FontWeight::W200, FontStretch::Condensed),
			FontFace("RobotoCondensed-LightItalic.woff", FontStyle::Italic, FontWeight::W200, FontStretch::Condensed),
			FontFace("RobotoCondensed-Regular.woff", FontStyle::Normal, FontWeight::Normal, FontStretch::Condensed),
			FontFace("Roboto-Italic.woff", FontStyle::Italic),
			FontFace("Roboto-Light.woff", FontStyle::Normal, FontWeight::W200),
			FontFace("Roboto-LightItalic.woff", FontStyle::Italic, FontWeight::W200),
			FontFace("Roboto-Medium.woff", FontStyle::Normal, FontWeight::W500),
			FontFace("Roboto-MediumItalic.woff", FontStyle::Italic, FontWeight::W500),
			FontFace("Roboto-Regular.woff"),
			FontFace("Roboto-Thin.woff", FontStyle::Normal, FontWeight::W100),
			FontFace("Roboto-ThinItalic.woff", FontStyle::Italic, FontWeight::W100)
		})
	}, [this] (const String &file) -> Bytes {
		if (filesystem::exists(file)) {
			return filesystem::readFile(file);
		}

		auto path = filepath::merge("fonts/", file);
		if (filesystem::exists(path)) {
			return filesystem::readFile(path);
		}

		path = filepath::merge("common/fonts/", file);
		if (filesystem::exists(path)) {
			return filesystem::readFile(path);
		}

		return Bytes();
	});

	stappler::storage::get("Material.ResourceManager", [this] (const std::string &key, stappler::data::Value &value) {
		if (value.isDictionary()) {
			LightLevel level = (LightLevel)value.getInteger("LightLevel", (int)LightLevel::Normal);
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
}

ResourceManager::~ResourceManager() { }

void ResourceManager::update() {
    float density = stappler::screen::density();
    const auto &m = getMaterialIconSources();
    stappler::IconSet::generate(stappler::IconSet::Config{"material", getMaterialIconVersion(), &m,
    	48, 48, (uint16_t)(24 * density), (uint16_t)(24 * density)}, [this] (IconSet *set) {
    		_iconSet = set;
    		if (_iconSet) {
    			_init = true;
    			ResourceManager::onLoaded(this);
    		}
    	});
}

void ResourceManager::saveUserData() {
	data::Value val(stappler::data::Value::Type::DICTIONARY);

	val.setInteger((int)_lightLevel, "LightLevel");
	val.setString(_locale, "Locale");
	val.setBool(_localeCustom, "LocaleCustom");

	storage::set("Material.ResourceManager", val);
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

font::Source *ResourceManager::getFontSource() const {
	return _source;
}

Icon ResourceManager::getIcon(IconName name) {
	auto &names = getMaterialIconNames();
	auto it = names.find(name);
	if (it != names.end()) {
		if (_iconSet) {
			return _iconSet->getIcon(it->second);
		}
	}
	return Icon();
}

NS_MD_END
