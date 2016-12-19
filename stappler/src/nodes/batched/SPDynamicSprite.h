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

#ifndef LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICSPRITE_H_
#define LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICSPRITE_H_

#include "SPDynamicBatchNode.h"

NS_SP_BEGIN

class DynamicSprite : public DynamicBatchNode {
public:
	enum class Autofit {
		None,
		Width,
		Height,
		Cover,
		Contain,
	};

public:
	using Callback = std::function<void(cocos2d::Texture2D *)>;

	virtual bool init(cocos2d::Texture2D *tex = nullptr, const Rect & = Rect::ZERO, float density = 0.0f);
	virtual bool init(const std::string &, const Rect & = Rect::ZERO, float density = 0.0f);
	virtual bool init(const Bitmap &, const Rect & = Rect::ZERO, float density = 0.0f);

	virtual void onContentSizeDirty() override;
	virtual void setDensity(float value) override;

	virtual void setTexture(cocos2d::Texture2D *tex, const Rect & = Rect::ZERO);
	virtual void setTexture(const std::string &, const Rect & = Rect::ZERO);
	virtual void setTexture(const Bitmap &, const Rect & = Rect::ZERO);
	virtual cocos2d::Texture2D *getTexture() const override;

	virtual void setTextureRect(const cocos2d::Rect &);
	virtual const cocos2d::Rect &getTextureRect() const;

	virtual void setFlippedX(bool value);
	virtual bool isFlippedX() const;

	virtual void setFlippedY(bool value);
	virtual bool isFlippedY() const;

	virtual void setRotated(bool value);
	virtual bool isRotated() const;

	virtual void setAutofit(Autofit);
	virtual Autofit getAutofit() const;

	virtual void setAutofitPosition(const cocos2d::Vec2 &);
	virtual const cocos2d::Vec2 &getAutofitPosition() const;

	virtual void setCallback(const Callback &func);

protected:
	void updateQuads();

	virtual cocos2d::GLProgramState *getProgramStateI8() const override;
	virtual cocos2d::GLProgramState *getProgramStateAI88() const override;

	bool _flippedX = false;
	bool _flippedY = false;
	bool _rotated = false;

	cocos2d::Rect _textureRect;

	Autofit _autofit;
	cocos2d::Vec2 _autofitPos = cocos2d::Vec2(0.5f, 0.5f);

	cocos2d::Vec2 _textureOrigin;
	cocos2d::Size _textureSize;

	Callback _callback = nullptr;
	Time _textureTime;
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_BATCHED_SPDYNAMICSPRITE_H_ */
