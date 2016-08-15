/*
 * MaterialFonts.h
 *
 *  Created on: 12 нояб. 2014 г.
 *      Author: sbkarr
 */

#ifndef CATALOGS_CLASSES_MATERIAL_MATERIALFONTS_H_
#define CATALOGS_CLASSES_MATERIAL_MATERIALFONTS_H_

#include "Material.h"
#include "SPFont.h"

NS_MD_BEGIN

class Font : public cocos2d::Ref {
public:
	enum class Type {
		System_Headline, // 24sp Regular 87%
		System_Title, // 20sp Medium 87%
		System_Subhead, // 16sp Regular 87%
		System_Body_1, // 14sp Medium 87%
		System_Body_2, // 14sp Regular 87%
		System_Caption, // 12sp Regular 54%
		System_Button, // 14sp Medium CAPS 87%
	};

	static const Font *systemFont(Type type);

	Font (Type type, const std::string &name, uint16_t size, uint8_t opacity, bool caps = false);
	Font (Type type, const std::string &name, stappler::Font *font, uint8_t opacity, bool caps = false);

	stappler::Font *getFont() const { return _font; }
	uint8_t getOpacity() const { return _opacity; }

	const std::string &getName() const { return _name; }
	uint16_t getSize() const { return _size; }

	void setFont(stappler::Font *);
	bool isUppercase() const { return _caps; }
protected:
	friend class ResourceManager;

	Type _type;
	uint8_t _opacity = 0;
	bool _caps = 0;

	std::string _name;
	uint16_t _size = 0;

	Rc<stappler::Font>_font = nullptr;
};

class FontSet : public cocos2d::Ref {
public:
	static std::vector<stappler::FontRequest> getRequest(float density);

	FontSet();

	bool init(stappler::FontSet *);

	const material::Font * getFont(Font::Type type) const;

	const std::string &getName() const {return _name; }
	uint16_t getVersion() const { return _version; }

protected:
	std::string _name;
	uint16_t _version = 0;
	std::map<Font::Type, Rc<material::Font>> _fonts;
};

NS_MD_END

#endif /* CATALOGS_CLASSES_MATERIAL_MATERIALFONTS_H_ */
