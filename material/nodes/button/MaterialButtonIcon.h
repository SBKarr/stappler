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

#ifndef LIBS_MATERIAL_NODES_BUTTON_MATERIALBUTTONICON_H_
#define LIBS_MATERIAL_NODES_BUTTON_MATERIALBUTTONICON_H_

#include "MaterialIconSprite.h"
#include "MaterialButton.h"

NS_MD_BEGIN

class ButtonIcon : public Button {
public:
	virtual bool init(IconName name = IconName::Empty, const TapCallback &tapCallback = nullptr, const TapCallback &longTapCallback = nullptr);
	virtual void onContentSizeDirty() override;

	virtual void setIconName(IconName name);
	virtual IconName getIconName() const;

	virtual void setIconProgress(float value, float animation = 0.0f);
	virtual float getIconProgress() const;

	virtual void setIconColor(const Color &);
	virtual const cocos2d::Color3B &getIconColor() const;

	virtual void setIconOpacity(uint8_t);
	virtual uint8_t getIconOpacity() const;

	virtual IconSprite *getIcon() const;

	virtual void setSizeHint(IconSprite::SizeHint);

protected:
	virtual void updateFromSource() override;

	IconSprite *_icon = nullptr;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_BUTTON_MATERIALBUTTONICON_H_ */
