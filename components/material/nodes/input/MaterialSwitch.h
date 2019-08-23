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

#ifndef MATERIAL_NODES_INPUT_MATERIALSWITCH_H_
#define MATERIAL_NODES_INPUT_MATERIALSWITCH_H_

#include "Material.h"
#include "2d/CCNode.h"

NS_MD_BEGIN

class Switch : public cocos2d::Node {
public:
	using Callback = std::function<void(bool)>;

	virtual bool init(const Callback & = nullptr);
	virtual void onContentSizeDirty() override;

	virtual void toggle(float duration = 0.0f);

	virtual void setSelected(bool, float = 0.0f);
	virtual bool isSelected() const;

	virtual void setEnabled(bool);
	virtual bool isEnabled() const;

	virtual void setSelectedColor(const Color &);
	virtual const Color &getSelectedColor() const;

	virtual void setCallback(const Callback &);
	virtual const Callback &getCallback() const;

protected:
	virtual void onAnimation(float progress);

	virtual bool onPressBegin(const Vec2 &);
	virtual bool onPressEnded(const Vec2 &);
	virtual bool onPressCancelled(const Vec2 &);

	virtual bool onSwipeBegin(const Vec2 &);
	virtual bool onSwipe(const Vec2 &);
	virtual bool onSwipeEnded(const Vec2 &);

	gesture::Listener *_listener = nullptr;

	material::Color _selectedColor = Color::Teal_500;

	float _progress = 0.0f;
	bool _selected = false;
	bool _enabled = true;

	RoundedSprite *_thumb = nullptr;
	CustomCornerSprite *_shadow = nullptr;
	RoundedSprite *_track = nullptr;
	RoundedSprite *_select = nullptr;

	Callback _callback = nullptr;
};

NS_MD_END

#endif /* MATERIAL_NODES_INPUT_MATERIALSWITCH_H_ */
