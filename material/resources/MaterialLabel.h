/*
 * MaterialLabel.h
 *
 *  Created on: 13 нояб. 2014 г.
 *      Author: sbkarr
 */

#ifndef CLASSES_MATERIAL_TYPOGRAPHY_MATERIALLABEL_H_
#define CLASSES_MATERIAL_TYPOGRAPHY_MATERIALLABEL_H_

#include "SPRichLabel.h"
#include "SPDynamicLabel.h"
#include "Material.h"

NS_MD_BEGIN

enum class FontType {
	Headline, // 24sp Regular 87%
	Title, // 20sp Medium 87%
	Subhead, // 16sp Regular 87%
	Body_1, // 14sp Medium 87%
	Body_2, // 14sp Regular 87%
	Caption, // 12sp Regular 54%
	Button, // 14sp Medium CAPS 87%

	Tab_Large,
	Tab_Large_Selected,
	Tab_Small,
	Tab_Small_Selected,

	System_Headline = Headline, // 24sp Regular 87%
	System_Title = Title, // 20sp Medium 87%
	System_Subhead = Subhead, // 16sp Regular 87%
	System_Body_1 = Body_1, // 14sp Medium 87%
	System_Body_2 = Body_2, // 14sp Regular 87%
	System_Caption = Caption, // 12sp Regular 54%
	System_Button = Button, // 14sp Medium CAPS 87%
};

class Label : public DynamicLabel {
public:
	static DescriptionStyle getFontStyle(FontType);

	static Size getLabelSize(FontType, const String &, float w = 0.0f, float density = 0.0f, bool localized = false);
	static Size getLabelSize(FontType, const WideString &, float w = 0.0f, float density = 0.0f, bool localized = false);

	static float getStringWidth(FontType, const String &, float density = 0.0f, bool localized = false);
	static float getStringWidth(FontType, const WideString &, float density = 0.0f, bool localized = false);

	virtual ~Label();

	virtual bool init(FontType, Alignment = Alignment::Left, float = 0);
	virtual bool init(const DescriptionStyle &, Alignment = Alignment::Left, float = 0);

	virtual void setFont(FontType);
    virtual void setStyle(const DescriptionStyle &) override;

	virtual void onEnter() override;

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

	EventListener *_lightLevelListener = nullptr;

	Color _dimColor = Color::White;
	Color _normalColor = Color::Black;
	Color _washedColor = Color::Black;
};

NS_MD_END

#endif /* CLASSES_MATERIAL_TYPOGRAPHY_MATERIALLABEL_H_ */
