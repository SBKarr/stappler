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

#ifndef LIBS_MATERIAL_NODES_BUTTON_MATERIALFLOATINGACTIONBUTTON_H_
#define LIBS_MATERIAL_NODES_BUTTON_MATERIALFLOATINGACTIONBUTTON_H_

#include "MaterialIconStorage.h"
#include "MaterialNode.h"
#include "SPDrawPathNode.h"

NS_MD_BEGIN

class FloatingActionButton : public MaterialNode {
public:
	using TapCallback = std::function<void()>;

	virtual bool init(const TapCallback &tapCallback = nullptr, const TapCallback &longTapCallback = nullptr);
	virtual void onContentSizeDirty() override;
    virtual void setBackgroundColor(const Color &) override;

	virtual IconSprite *getIcon() const;
	virtual Label *getLabel() const;
	virtual stappler::draw::PathNode *getPathNode() const;

	virtual float getProgress() const;
	virtual void setProgress(float value);

	virtual void setEnabled(bool value);
	virtual bool isEnabled() const;

	virtual void setProgressColor(const Color &);
	virtual void setProgressWidth(float);

	virtual void setString(const std::string &);
	virtual void setLabelColor(const Color &);
	virtual void setLabelOpacity(uint8_t);

	virtual void setIcon(IconName name, bool animated = true);
	virtual void setIconColor(const Color &);
	virtual void setIconOpacity(uint8_t);

	virtual void setTapCallback(const TapCallback &);
	virtual void setLongTapCallback(const TapCallback &);
	virtual void setSwallowTouches(bool value);

	virtual void clearAnimations();

protected:
    virtual uint8_t getOpacityForAmbientShadow(float value) const override;
    virtual uint8_t getOpacityForKeyShadow(float value) const override;

	virtual void updateEnabled();

	virtual bool onPressBegin(const cocos2d::Vec2 &location);
    virtual void onLongPress();
	virtual bool onPressEnd();
	virtual void onPressCancel();

	virtual void animateSelection();
	virtual void animateDeselection();

	bool _enabled = true;
	TapCallback _tapCallback = nullptr;
	TapCallback _longTapCallback = nullptr;
	cocos2d::Component *_listener = nullptr;

	draw::Image::PathRef _tapAnimationPath;

	uint8_t _iconOpacity = 255;

	float _progress = 0.0f;
	draw::Image::PathRef _progressPath;
	Color _progressColor = Color::Black;
	float _progressWidth = 4.0f;

	IconSprite *_icon = nullptr;
	Label *_label = nullptr;
	draw::PathNode *_drawNode = nullptr;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_BUTTON_MATERIALFLOATINGACTIONBUTTON_H_ */
