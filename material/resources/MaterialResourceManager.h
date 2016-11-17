/*
 * MaterialResourceManager.h
 *
 *  Created on: 13 нояб. 2014 г.
 *      Author: sbkarr
 */

#ifndef CLASSES_MATERIAL_MATERIALRESOURCEMANAGER_H_
#define CLASSES_MATERIAL_MATERIALRESOURCEMANAGER_H_

#include "Material.h"
#include "base/CCVector.h"
#include "base/CCMap.h"
#include "MaterialIconSprite.h"
#include "MaterialUserFontConfig.h"

#include "SPEventHeader.h"
#include "SPEventHandler.h"
#include "SPRichTextStyle.h"
#include "SPLabelParameters.h"

NS_MD_BEGIN

class ResourceManager : public Ref, EventHandler {
public:
	using LightLevel = rich_text::style::LightLevel;
	using UserFontStyle = LabelParameters::DescriptionStyle;
	using FontStyleMap = Map<String, UserFontStyle>;

public:
	static stappler::EventHeader onLoaded;
	static stappler::EventHeader onLightLevel;
	static stappler::EventHeader onUserFont;

	static Thread &thread();

public:
	static ResourceManager *getInstance();

	ResourceManager();
	~ResourceManager();

	void setLightLevel(LightLevel);
	LightLevel getLightLevel() const;

	void setLocale(const String &);
	const String &getLocale() const;

	bool isLoaded();
	font::Source *getSystemFontSource() const;
	Icon getIcon(IconName name);

	font::Source *getUserFontSource() const;
	UserFontStyle getFontStyle(const String &) const;

	void addUserFontFaceMap(const UserFontConfig::FontFaceMap & map);
	void addUserFontFace(const String &, UserFontConfig::FontFace &&);

	void addFontStyleMap(FontStyleMap &&);
	void addFontStyle(const String &, const UserFontStyle &);

	template <typename ... Args>
	void addFontStyle(const String &name, const String &family, uint8_t size, Args && ... args) {
		addFontStyle(name, UserFontStyle::construct(family, size, std::forward<Args>(args)...));
	}

	void setUserFontScale(float scale);
	float getUserFontScale() const;

protected:
	void update();
	void saveUserData();

	String _locale;
	bool _localeCustom = false;

	Rc<IconSet>_iconSet;
	Rc<font::Source> _source;
	Rc<UserFontConfig> _textFont;

	LightLevel _lightLevel = LightLevel::Normal;

	bool _init = false;

	FontStyleMap _namedFonts;
};

NS_MD_END

#endif /* CLASSES_MATERIAL_MATERIALRESOURCEMANAGER_H_ */
