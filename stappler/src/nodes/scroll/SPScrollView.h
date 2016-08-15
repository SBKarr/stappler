//
//  SPScrollView.h
//  stappler
//
//  Created by SBKarr on 11/30/13.
//  Copyright (c) 2013 SBKarr. All rights reserved.
//

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
};

NS_SP_END

#endif /* defined(__stappler__SPScrollView__) */
