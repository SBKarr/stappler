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

#include "MaterialIconStorage.h"
#include "Material.h"
#include "MaterialResourceManager.h"
#include "MaterialFontSource.h"

#include "SPFilesystem.h"
#include "SPLocale.h"
#include "SPData.h"
#include "SPScreen.h"
#include "SPResource.h"
#include "SPFont.h"
#include "SPDevice.h"
#include "SPStorage.h"
#include "SPString.h"
#include "SPThread.h"

#include "platform/CCImage.h"
#include "renderer/CCTexture2D.h"

NS_MD_BEGIN

struct StaticResources {
	Mutex mutex;
	Vector<FontResourceHeader *> resources;

	static StaticResources *getInstance() {
		static StaticResources s_resources;
		return &s_resources;
	}

	void addFont(FontResourceHeader *ptr) {
		mutex.lock();
		resources.emplace_back(ptr);
		mutex.unlock();
	}

	void removeFont(FontResourceHeader *ptr) {
		mutex.lock();
		auto it = std::find(resources.begin(), resources.end(), ptr);
		if (it != resources.end()) {
			resources.erase(it);
		}
		mutex.unlock();
	}
};

FontResourceHeader::FontResourceHeader(layout::FontSource::FontFaceMap &&map, bool user) : _userFont(user), _faceMap(move(map)) {
	StaticResources::getInstance()->addFont(this);
}

FontResourceHeader::~FontResourceHeader() {
	StaticResources::getInstance()->removeFont(this);
}

bool FontResourceHeader::isUserFont() const {
	return _userFont;
}

layout::FontSource::FontFaceMap &FontResourceHeader::get() {
	return _faceMap;
}

SP_DECLARE_EVENT(ResourceManager, "Material", onLoaded);
SP_DECLARE_EVENT(ResourceManager, "Material", onLightLevel);
SP_DECLARE_EVENT(ResourceManager, "Material", onUserFont);

static ResourceManager *s_instance = nullptr;

static Thread s_resourceThread("MaterialResourceThread");

Thread &ResourceManager::thread() {
	return s_resourceThread;
}

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
		pair("SystemThemeLight", "Светлая тема"),
		pair("SystemThemeNeutral", "Нейтральная тема"),
		pair("SystemThemeDark", "Темная тема"),
		pair("SystemMore", "Ещё"),
		pair("SystemRestore", "Восстановить"),
		pair("SystemRemoved", "Удалено"),
		pair("SystemCopy", "Копировать"),
		pair("SystemCut", "Вырезать"),
		pair("SystemPaste", "Вставить"),
		pair("SystemTapExit", "Нажмите ещё раз для выхода"),

		pair("SystemErrorOverflowChars", "Слишком много символов"),
		pair("SystemErrorInvalidChar", "Недопустимый символ"),

		pair("Shortcut:Megabytes", "Мб"),
		pair("Shortcut:Pages", "с"),
	});

	locale::define("en-us", {
		pair("SystemSearch", "Search"),
		pair("SystemFontSize", "Font size"),
		pair("SystemTheme", "Theme"),
		pair("SystemThemeLight", "Light theme"),
		pair("SystemThemeNeutral", "Neutral theme"),
		pair("SystemThemeDark", "Dark theme"),
		pair("SystemMore", "More"),
		pair("SystemRestore", "Restore"),
		pair("SystemRemoved", "Removed"),
		pair("SystemCopy", "Copy"),
		pair("SystemCut", "Cut"),
		pair("SystemPaste", "Paste"),
		pair("SystemTapExit", "Tap one more time to exit"),

		pair("SystemErrorOverflowChars", "Too many characters"),
		pair("SystemErrorInvalidChar", "Invalid character"),

		pair("Shortcut:Megabytes", "Mb"),
		pair("Shortcut:Pages", "p"),
	});

	using namespace font;

	FontSource::FontFaceMap systemMap({
		pair("system", Vector<FontFace>({
			FontFace(getSystemFont(SystemFontName::Roboto_Black), FontStyle::Normal, FontWeight::W800),
			FontFace(getSystemFont(SystemFontName::Roboto_BlackItalic), FontStyle::Italic, FontWeight::W800),
			FontFace(getSystemFont(SystemFontName::Roboto_Bold), FontStyle::Normal, FontWeight::W700),
			FontFace(getSystemFont(SystemFontName::Roboto_BoldItalic), FontStyle::Italic, FontWeight::W700),
			FontFace(getSystemFont(SystemFontName::RobotoCondensed_Bold), FontStyle::Normal, FontWeight::W700, FontStretch::Condensed),
			FontFace(getSystemFont(SystemFontName::RobotoCondensed_BoldItalic), FontStyle::Italic, FontWeight::W700, FontStretch::Condensed),
			FontFace(getSystemFont(SystemFontName::RobotoCondensed_Italic), FontStyle::Italic, FontWeight::Normal, FontStretch::Condensed),
			FontFace(getSystemFont(SystemFontName::RobotoCondensed_Light), FontStyle::Normal, FontWeight::W200, FontStretch::Condensed),
			FontFace(getSystemFont(SystemFontName::RobotoCondensed_LightItalic), FontStyle::Italic, FontWeight::W200, FontStretch::Condensed),
			FontFace(getSystemFont(SystemFontName::RobotoCondensed_Regular), FontStyle::Normal, FontWeight::Normal, FontStretch::Condensed),
			FontFace(getSystemFont(SystemFontName::Roboto_Italic), FontStyle::Italic),
			FontFace(getSystemFont(SystemFontName::Roboto_Light), FontStyle::Normal, FontWeight::W200),
			FontFace(getSystemFont(SystemFontName::Roboto_LightItalic), FontStyle::Italic, FontWeight::W200),
			FontFace(getSystemFont(SystemFontName::Roboto_Medium), FontStyle::Normal, FontWeight::W500),
			FontFace(getSystemFont(SystemFontName::Roboto_MediumItalic), FontStyle::Italic, FontWeight::W500),
			FontFace(getSystemFont(SystemFontName::Roboto_Regular)),
			FontFace(getSystemFont(SystemFontName::Roboto_Thin), FontStyle::Normal, FontWeight::W100),
			FontFace(getSystemFont(SystemFontName::Roboto_ThinItalic), FontStyle::Italic, FontWeight::W100)
		}))
	});

	FontSource::FontFaceMap userMap({
		pair("default", Vector<FontFace>({
			FontFace(getSystemFont(SystemFontName::Roboto_Black), FontStyle::Normal, FontWeight::W800),
			FontFace(getSystemFont(SystemFontName::Roboto_BlackItalic), FontStyle::Italic, FontWeight::W800),
			FontFace(getSystemFont(SystemFontName::Roboto_Bold), FontStyle::Normal, FontWeight::W700),
			FontFace(getSystemFont(SystemFontName::Roboto_BoldItalic), FontStyle::Italic, FontWeight::W700),
			FontFace(getSystemFont(SystemFontName::RobotoCondensed_Bold), FontStyle::Normal, FontWeight::W700, FontStretch::Condensed),
			FontFace(getSystemFont(SystemFontName::RobotoCondensed_BoldItalic), FontStyle::Italic, FontWeight::W700, FontStretch::Condensed),
			FontFace(getSystemFont(SystemFontName::RobotoCondensed_Italic), FontStyle::Italic, FontWeight::Normal, FontStretch::Condensed),
			FontFace(getSystemFont(SystemFontName::RobotoCondensed_Light), FontStyle::Normal, FontWeight::W200, FontStretch::Condensed),
			FontFace(getSystemFont(SystemFontName::RobotoCondensed_LightItalic), FontStyle::Italic, FontWeight::W200, FontStretch::Condensed),
			FontFace(getSystemFont(SystemFontName::RobotoCondensed_Regular), FontStyle::Normal, FontWeight::Normal, FontStretch::Condensed),
			FontFace(getSystemFont(SystemFontName::Roboto_Italic), FontStyle::Italic),
			FontFace(getSystemFont(SystemFontName::Roboto_Light), FontStyle::Normal, FontWeight::W200),
			FontFace(getSystemFont(SystemFontName::Roboto_LightItalic), FontStyle::Italic, FontWeight::W200),
			FontFace(getSystemFont(SystemFontName::Roboto_Medium), FontStyle::Normal, FontWeight::W500),
			FontFace(getSystemFont(SystemFontName::Roboto_MediumItalic), FontStyle::Italic, FontWeight::W500),
			FontFace(getSystemFont(SystemFontName::Roboto_Regular)),
			FontFace(getSystemFont(SystemFontName::Roboto_Thin), FontStyle::Normal, FontWeight::W100),
			FontFace(getSystemFont(SystemFontName::Roboto_ThinItalic), FontStyle::Italic, FontWeight::W100)
		}))
	});

	auto res = StaticResources::getInstance();
	res->mutex.lock();
	for (auto &it : res->resources) {
		auto &m = it->get();
		if (it->isUserFont()) {
			for (auto &iit : m) {
				userMap.emplace(iit.first, move(iit.second));
			}
		} else {
			for (auto &iit : m) {
				systemMap.emplace(iit.first, move(iit.second));
			}
		}
	}
	res->mutex.unlock();

	_source = Rc<FontSource>::create(move(systemMap), nullptr, 1.0f, Vector<String>{ "fonts/", "common/fonts/" });
	_textFont = Rc<UserFontConfig>::create(move(userMap), 1.0f);

	_iconStorage = Rc<IconStorage>::create(stappler::screen::density());
	_iconStorageSmall = Rc<IconStorage>::create(stappler::screen::density(), 18, 18);
	_iconStorageLarge = Rc<IconStorage>::create(stappler::screen::density(), 32, 32);

	storage::get("Material.ResourceManager", [this] (const StringView &key, data::Value &&value) {
		if (value.isDictionary()) {
			float textFontScale = value.getDouble("TextFontScale", 1.0f);
			_textFont->setFontScale(textFontScale);
			_textFont->update(0.0f);

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

		_init = true;
		ResourceManager::onLoaded(this);
	});
}

ResourceManager::~ResourceManager() { }

void ResourceManager::saveUserData() {
	data::Value val(data::Value::Type::DICTIONARY);

	val.setDouble(getUserFontScale(), "TextFontScale");
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

void ResourceManager::setLocale(const String &loc) {
	if (_locale != loc) {
		_locale = loc;
		_localeCustom = true;
		locale::setLocale(loc);
		saveUserData();
	}
}

const String &ResourceManager::getLocale() const {
	return _locale;
}

bool ResourceManager::isLoaded() {
	return _init;
}

font::FontSource *ResourceManager::getSystemFontSource() const {
	return _source;
}

IconStorage *ResourceManager::getIconStorage(const Size &size) const {
	if (_iconStorage->matchSize(size)) {
		return _iconStorage;
	} else if (_iconStorageLarge->matchSize(size)) {
		return _iconStorageLarge;
	} else if (_iconStorageSmall->matchSize(size)) {
		return _iconStorageSmall;
	}

	return nullptr;
}

font::FontSource *ResourceManager::getUserFontSource() const {
	return _textFont->getSafeSource();
}

void ResourceManager::addUserFontFaceMap(const UserFontConfig::FontFaceMap & map) {
	_textFont->addFontFaceMap(map);
}
void ResourceManager::addUserFontFace(const String &family, UserFontConfig::FontFace &&face) {
	_textFont->addFontFace(family, std::move(face));
}

void ResourceManager::addFontStyleMap(FontStyleMap &&map) {
	if (_namedFonts.empty()) {
		_namedFonts = std::move(map);
	} else {
		for (auto &it : map) {
			_namedFonts.emplace(std::move(it));
		}
	}
}

void ResourceManager::addFontStyle(const String &name, const UserFontStyle &style) {
	_namedFonts.emplace(name, style);
}

ResourceManager::UserFontStyle ResourceManager::getFontStyle(const String &name) const {
	auto it = _namedFonts.find(name);
	if (it != _namedFonts.end()) {
		return it->second;
	}
	return UserFontStyle();
}

void ResourceManager::setUserFontScale(float scale) {
	if (scale != getUserFontScale()) {
		_textFont->setFontScale(scale);
		saveUserData();
	}
}

float ResourceManager::getUserFontScale() const {
	return _textFont->getFontScale();
}

NS_MD_END
