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

#ifndef MATERIAL_NODES_INPUT_MATERIALLINEFIELD_H_
#define MATERIAL_NODES_INPUT_MATERIALLINEFIELD_H_

#include "MaterialInputField.h"

NS_MD_BEGIN

class LineField : public InputField {
public:
	virtual bool init(FontType) override;
	virtual void onContentSizeDirty() override;

	virtual void setPadding(const Padding &);
	virtual const Padding &getPadding() const;

	virtual bool onSwipeBegin(const Vec2 &) override;
	virtual bool onSwipe(const Vec2 &, const Vec2 &) override;
	virtual bool onSwipeEnd(const Vec2 &) override;

	virtual void update(float dt) override;

protected:
	virtual void onError(Error) override;
	virtual void onInput() override;
	virtual void onActivated(bool) override;
	virtual void onCursor(const Cursor &) override;
	virtual void onPointer(bool) override;
	virtual bool onInputString(const WideString &str, const Cursor &c) override;

	virtual void updateMenu();

	enum Adjust {
		None,
		Left,
		Right
	};

	virtual void runAdjust(float);
	virtual void scheduleAdjust(Adjust, const Vec2 &, float pos);

	Adjust _adjust = None;
	Vec2 _adjustValue;
	float _adjustPosition = 0.0f;

	bool _swipeCaptured = false;
	Padding _padding = Padding(12.0f, 8.0f);
	Layer *_underlineLayer = nullptr;
};

NS_MD_END

#endif /* MATERIAL_NODES_INPUT_MATERIALLINEFIELD_H_ */
