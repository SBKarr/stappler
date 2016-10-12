/*
 * MaterialButtonLabel.h
 *
 *  Created on: 18 июня 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_BUTTON_MATERIALBUTTONLABEL_H_
#define LIBS_MATERIAL_NODES_BUTTON_MATERIALBUTTONLABEL_H_

#include "MaterialButtonIcon.h"
#include "MaterialLabel.h"

NS_MD_BEGIN

class ButtonLabel : public Button {
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

	virtual void setFont(FontType fnt);

protected:
	Label *_label = nullptr;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_BUTTON_MATERIALBUTTONLABEL_H_ */
