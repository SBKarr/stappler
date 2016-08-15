/*
 * MaterialButtonLabelIcon.h
 *
 *  Created on: 8 февр. 2016 г.
 *      Author: sbkarr
 */

#ifndef MATERIAL_NODES_BUTTON_MATERIALBUTTONLABELICON_H_
#define MATERIAL_NODES_BUTTON_MATERIALBUTTONLABELICON_H_

#include "MaterialButtonIcon.h"
#include "MaterialLabel.h"

NS_MD_BEGIN

class ButtonLabelIcon : public ButtonStaticIcon {
public:
	virtual bool init(const TapCallback &tapCallback = nullptr, const TapCallback &longTapCallback = nullptr) override;
	virtual void onContentSizeDirty() override;

	virtual void setString(const std::string &);
	virtual const std::string &getString() const;

	virtual void setWidth(float value);
	virtual float getWidth() const;

	virtual void setLabelColor(const Color &);
	virtual const cocos2d::Color3B &getLabelColor() const;

	virtual void setLabelOpacity(uint8_t);
	virtual uint8_t getLabelOpacity();

	virtual void setFont(const Font *fnt);
	virtual const Font *getFont() const;

protected:
	Label *_label = nullptr;
};

NS_MD_END

#endif /* MATERIAL_NODES_BUTTON_MATERIALBUTTONLABELICON_H_ */
