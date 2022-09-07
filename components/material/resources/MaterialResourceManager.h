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

#ifndef CLASSES_MATERIAL_MATERIALRESOURCEMANAGER_H_
#define CLASSES_MATERIAL_MATERIALRESOURCEMANAGER_H_

#include "Material.h"
#include "MaterialIconSprite.h"
#include "MaterialUserFontConfig.h"

#include "SPEventHeader.h"
#include "SPEventHandler.h"
#include "SPLabelParameters.h"

NS_MD_BEGIN

class FontResourceHeader {
public:
	FontResourceHeader(layout::FontSource::FontFaceMap &&, bool user = false);
	~FontResourceHeader();

	bool isUserFont() const;
	layout::FontSource::FontFaceMap &get();

protected:
	size_t _idx = 0;
	bool _userFont = false;
	layout::FontSource::FontFaceMap _faceMap;
};

class ResourceManager : public Ref, EventHandler {
public:
	using LightLevel = layout::style::LightLevel;
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
	font::FontSource *getSystemFontSource() const;

	IconStorage *getIconStorage(const Size &) const;

	font::FontSource *getUserFontSource() const;
	UserFontStyle getFontStyle(const String &) const;

	void addUserFontFaceMap(const UserFontConfig::FontFaceMap & map);
	void addUserFontFace(const String &, UserFontConfig::FontFace &&);

	void addFontStyleMap(FontStyleMap &&);
	void addFontStyle(const String &, const UserFontStyle &);

	template <typename ... Args>
	void addFontStyle(const String &name, const String &family, LabelParameters::FontSize size, Args && ... args) {
		addFontStyle(name, UserFontStyle::construct(family, size, std::forward<Args>(args)...));
	}

	void setUserFontScale(float scale);
	float getUserFontScale() const;

protected:
	void saveUserData();

	String _locale;
	bool _localeCustom = false;

	Rc<IconStorage> _iconStorage;
	Rc<IconStorage> _iconStorageSmall;
	Rc<IconStorage> _iconStorageLarge;
	Rc<font::FontSource> _source;
	Rc<UserFontConfig> _textFont;

	LightLevel _lightLevel = LightLevel::Normal;

	bool _init = false;

	FontStyleMap _namedFonts;
};

NS_MD_END

#endif /* CLASSES_MATERIAL_MATERIALRESOURCEMANAGER_H_ */
