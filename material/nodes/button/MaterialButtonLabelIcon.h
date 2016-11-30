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

#ifndef MATERIAL_NODES_BUTTON_MATERIALBUTTONLABELICON_H_
#define MATERIAL_NODES_BUTTON_MATERIALBUTTONLABELICON_H_

#include "MaterialButtonIcon.h"
#include "MaterialLabel.h"

NS_MD_BEGIN

class ButtonLabelIcon : public ButtonIcon {
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

#endif /* MATERIAL_NODES_BUTTON_MATERIALBUTTONLABELICON_H_ */
