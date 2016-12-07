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

#ifndef LIBS_STAPPLER_NODES_BATCHED_SPLAYER_H_
#define LIBS_STAPPLER_NODES_BATCHED_SPLAYER_H_

#include "SPCustomCornerSprite.h"

NS_SP_BEGIN

struct Gradient {
	using Color = Color4B;
	using ColorRef = const Color &;

	static const cocos2d::Vec2 Vertical;
	static const cocos2d::Vec2 Horizontal;

	Gradient();
	Gradient(ColorRef start, ColorRef end, const cocos2d::Vec2 & = Vertical);
	Gradient(ColorRef bl, ColorRef br, ColorRef tl, ColorRef tr);

	cocos2d::Color4B colors[4]; // bl - br - tl - tr
};

class Layer: public DynamicBatchNode {
public:
	virtual bool init(const cocos2d::Color4B & = cocos2d::Color4B(255, 255, 255, 255));

	virtual void onContentSizeDirty() override;

	virtual void setGradient(const Gradient &);
	virtual const Gradient &getGradient() const;

protected:
	virtual void updateSprites();
	virtual void updateColor() override;

	Gradient _gradient;
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_BATCHED_SPLAYER_H_ */
