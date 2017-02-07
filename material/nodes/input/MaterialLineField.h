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

#include "MaterialFormField.h"

NS_MD_BEGIN

class LineField : public FormField {
public:
	static float getMaxLabelHeight(bool dense = false);

	virtual bool onSwipeBegin(const Vec2 &, const Vec2 &) override;
	virtual bool onSwipe(const Vec2 &, const Vec2 &) override;
	virtual bool onSwipeEnd(const Vec2 &) override;

	virtual void update(float dt) override;

protected:
	virtual void onInput() override;
	virtual bool onInputString(const WideString &str, const Cursor &c) override;

	virtual void onMenuVisible() override;

	enum Adjust {
		None,
		Left,
		Right
	};

	virtual void runAdjust(float);
	virtual void scheduleAdjust(Adjust, const Vec2 &, float pos);

	virtual InputLabel *makeLabel(FontType) override;

	Adjust _adjust = None;
	Vec2 _adjustValue;
	float _adjustPosition = 0.0f;
	bool _swipeCaptured = false;
};

NS_MD_END

#endif /* MATERIAL_NODES_INPUT_MATERIALLINEFIELD_H_ */
