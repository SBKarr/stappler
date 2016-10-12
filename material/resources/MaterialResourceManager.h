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

#include "SPEventHeader.h"
#include "SPEventHandler.h"
#include "SPRichTextStyle.h"

NS_MD_BEGIN

class ResourceManager : public Ref, EventHandler {
public:
	using LightLevel = stappler::rich_text::style::LightLevel;

public:
	static stappler::EventHeader onLoaded;
	static stappler::EventHeader onLightLevel;

public:
	static ResourceManager *getInstance();

	ResourceManager();
	~ResourceManager();

	void setLightLevel(LightLevel);
	LightLevel getLightLevel() const;

	void setLocale(const std::string &);
	const std::string &getLocale() const;

	bool isLoaded();
	font::Source *getFontSource() const;
	Icon getIcon(IconName name);

protected:
	void update();
	void saveUserData();

	String _locale;
	bool _localeCustom = false;

	Rc<stappler::IconSet>_iconSet;
	Rc<font::Source> _source;

	LightLevel _lightLevel = LightLevel::Normal;

	bool _init = false;
};

NS_MD_END

#endif /* CLASSES_MATERIAL_MATERIALRESOURCEMANAGER_H_ */
