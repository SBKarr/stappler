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

#ifndef MATERIAL_NODES_INPUT_MATERIALTEXTFIELD_H_
#define MATERIAL_NODES_INPUT_MATERIALTEXTFIELD_H_

#include "MaterialFormField.h"

NS_MD_BEGIN

class TextField : public FormField {
public:
	using CursorCallback = Function<void(const Vec2 &)>;
	using PositionCallback = Function<void(const Vec2 &, bool ended)>;

	virtual bool init(bool dense = false) override;
	virtual bool init(FormController *, const String &name, bool dense = false) override;

	virtual void setContentSize(const Size &) override;
	virtual void onContentSizeDirty() override;

	virtual bool onSwipe(const Vec2 &loc, const Vec2 &delta) override;
	virtual bool onSwipeEnd(const Vec2 &) override;

protected:
	virtual void onInput() override;
	virtual void onMenuVisible() override;

	material::ButtonIcon *_clearIcon = nullptr;
};

NS_MD_END

#endif /* MATERIAL_NODES_INPUT_MATERIALTEXTFIELD_H_ */
