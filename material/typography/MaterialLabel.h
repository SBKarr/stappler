/*
 * MaterialLabel.h
 *
 *  Created on: 13 нояб. 2014 г.
 *      Author: sbkarr
 */

#ifndef CLASSES_MATERIAL_TYPOGRAPHY_MATERIALLABEL_H_
#define CLASSES_MATERIAL_TYPOGRAPHY_MATERIALLABEL_H_

#include "SPRichLabel.h"
#include "MaterialFont.h"

NS_MD_BEGIN

class Label : public stappler::RichLabel {
public:
	virtual bool init(const Font *, Alignment = Alignment::Left, float = 0);
	virtual bool init(Font::Type, Alignment = Alignment::Left, float = 0);

	virtual void onEnter() override;

	virtual void setMaterialFont(const material::Font *);
	virtual const material::Font *getMaterialFont();

	virtual void setAutoLightLevel(bool);
	virtual bool isAutoLightLevel() const;

	virtual void setDimColor(const Color &);
	virtual const Color &getDimColor() const;

	virtual void setNormalColor(const Color &);
	virtual const Color &getNormalColor() const;

	virtual void setWashedColor(const Color &);
	virtual const Color &getWashedColor() const;

protected:
	virtual void onLightLevel();

	const material::Font *_materialFont;

	EventListener *_lightLevelListener = nullptr;

	Color _dimColor = Color::White;
	Color _normalColor = Color::Black;
	Color _washedColor = Color::Black;
};

NS_MD_END

#endif /* CLASSES_MATERIAL_TYPOGRAPHY_MATERIALLABEL_H_ */
