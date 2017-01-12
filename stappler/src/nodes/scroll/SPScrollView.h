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

#ifndef __stappler__SPScrollView__
#define __stappler__SPScrollView__

#include "SPScrollViewBase.h"

NS_SP_BEGIN

class ScrollView : public ScrollViewBase {
public:
	using TapCallback = std::function<void(int count, const Vec2 &loc)>;
	using AnimationCallback = std::function<void()>;

	virtual bool init(Layout l) override;

    virtual void onContentSizeDirty() override;

	virtual void setOverscrollColor(const Color3B &);
	virtual const Color3B & getOverscrollColor() const;

	virtual void setOverscrollOpacity(uint8_t);
	virtual uint8_t getOverscrollOpacity() const;

	virtual void setOverscrollVisible(bool value);
	virtual bool isOverscrollVisible() const;

	virtual void setIndicatorColor(const Color3B &);
	virtual const Color3B & getIndicatorColor() const;

	virtual void setIndicatorOpacity(uint8_t);
	virtual uint8_t getIndicatorOpacity() const;

	virtual void setIndicatorVisible(bool value);
	virtual bool isIndicatorVisible() const;

	virtual void setPadding(const Padding &) override;

	virtual void setOverscrollFrontOffset(float value);
	virtual float getOverscrollFrontOffset() const;

	virtual void setOverscrollBackOffset(float value);
	virtual float getOverscrollBackOffset() const;

	virtual void setIndicatorIgnorePadding(bool value);
	virtual bool isIndicatorIgnorePadding() const;

	virtual void setTapCallback(const TapCallback &);
	virtual const TapCallback &getTapCallback() const;

	virtual void setAnimationCallback(const AnimationCallback &);
	virtual const AnimationCallback &getAnimationCallback() const;

	virtual void update(float dt) override;

	enum class Adjust {
		None,
		Front,
		Back
	};

	virtual void runAdjust(float pos, float factor = 1.0f);
	virtual void scheduleAdjust(Adjust, float value);

protected:
	virtual void onOverscroll(float delta) override;
	virtual void onScroll(float delta, bool finished) override;
	virtual void onTap(int count, const Vec2 &loc) override;
	virtual void onAnimationFinished() override;

	virtual void updateIndicatorPosition();

	Overscroll *_overflowFront = nullptr;
	Overscroll *_overflowBack = nullptr;

	RoundedSprite *_indicator = nullptr;
	bool _indicatorVisible = true;
	bool _indicatorIgnorePadding = false;

	float _overscrollFrontOffset = 0.0f;
	float _overscrollBackOffset = 0.0f;

	TapCallback _tapCallback = nullptr;
	AnimationCallback _animationCallback = nullptr;

	Adjust _adjust = Adjust::None;
	float _adjustValue = 0.0f;
};

NS_SP_END

#endif /* defined(__stappler__SPScrollView__) */
