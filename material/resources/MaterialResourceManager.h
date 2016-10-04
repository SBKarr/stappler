/*
 * MaterialResourceManager.h
 *
 *  Created on: 13 нояб. 2014 г.
 *      Author: sbkarr
 */

#ifndef CLASSES_MATERIAL_MATERIALRESOURCEMANAGER_H_
#define CLASSES_MATERIAL_MATERIALRESOURCEMANAGER_H_

#include "Material.h"
#include "MaterialFont.h"
#include "base/CCVector.h"
#include "base/CCMap.h"
#include "MaterialIconSprite.h"

#include "SPEventHeader.h"
#include "SPEventHandler.h"
#include "SPRichTextStyle.h"

NS_MD_BEGIN

class ResourceManager : public cocos2d::Ref, stappler::EventHandler {
public:
	using LightLevel = stappler::rich_text::style::LightLevel;
	using TextureCallback = std::function<void(cocos2d::Texture2D *tex)>;
	using TextureLoaderCallback = std::function<void(const TextureCallback &, bool reload)>;

public:
	static stappler::EventHeader onLoaded;
	static stappler::EventHeader onSystemFonts;
	static stappler::EventHeader onSystemFontsReload;
	static stappler::EventHeader onTextFonts;
	static stappler::EventHeader onTextFontsReload;
	static stappler::EventHeader onLightLevel;

public:
	static ResourceManager *getInstance();

	ResourceManager();
	~ResourceManager();

	void setSystemFontScale(float scale);
	float getSystemFontScale() const;

	void setTextFontScale(float scale);
	float getTextFontScale() const;

	void setLightLevel(LightLevel);
	LightLevel getLightLevel() const;

	void setLocale(const std::string &);
	const std::string &getLocale() const;

	bool isLoaded();
	const material::Font * getSystemFont(Font::Type type) const;
	material::FontSet *getSystemFontSet() const;

	Icon getIcon(IconName name);

protected:
	void update();
	void saveUserData();

	void updateSystemFonts();
	void updateTextFonts();

	std::string _locale;
	bool _localeCustom = false;

	Rc<stappler::IconSet>_iconSet;
	Rc<material::FontSet>_systemFonts;

	float _systemFontScale = 1.0f;
	float _textFontScale = 1.0f;

	LightLevel _lightLevel = LightLevel::Normal;

	bool _init = false;
};

NS_MD_END

#endif /* CLASSES_MATERIAL_MATERIALRESOURCEMANAGER_H_ */
