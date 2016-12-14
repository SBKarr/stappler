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

#ifndef LIBS_STAPPLER_NODES_SCROLL_SPSCROLLVIEWBASE_H_
#define LIBS_STAPPLER_NODES_SCROLL_SPSCROLLVIEWBASE_H_

#include "SPStrictNode.h"
#include "SPPadding.h"

NS_SP_BEGIN

class ScrollViewBase : public StrictNode {
public:
	using ScrollFilterCallback = std::function<float (float delta)>;
	using ScrollCallback = std::function<void (float delta, bool finished)>;
	using OverscrollCallback = std::function<void (float delta)>;

	enum Layout {
		Vertical,
		Horizontal
	};

public:
	virtual bool init(Layout l);
	virtual void setLayout(Layout l);

    virtual void visit(cocos2d::Renderer *, const Mat4 &, uint32_t, ZPath &) override;

    virtual void onEnter() override;

	inline bool isVertical() const { return _layout == Vertical; }
	inline bool isHorizontal() const { return _layout == Horizontal; }
	virtual Layout getLayout() const { return _layout; }

    virtual void onContentSizeDirty() override;
    virtual void onTransformDirty() override;

    virtual void setEnabled(bool value);
    virtual bool isEnabled() const;

    virtual bool isTouched() const;
    virtual bool isMoved() const;

    virtual void setScrollCallback(const ScrollCallback & cb);
    virtual const ScrollCallback &getScrollCallback() const;

    virtual void setOverscrollCallback(const OverscrollCallback & cb);
    virtual const OverscrollCallback &getOverscrollCallback() const;

	virtual cocos2d::Node *getRoot() const;
	virtual gesture::Listener *getGestureListener() const;

	virtual bool addComponent(cocos2d::Component *) override;
	virtual void setController(ScrollController *c);
	virtual ScrollController *getController();

	virtual void setPadding(const Padding &);
	virtual const Padding &getPadding() const;

	virtual float getScrollableAreaOffset() const; // NaN by default
	virtual float getScrollableAreaSize() const; // NaN by default

	virtual Vec2 getPositionForNode(float scrollPos) const;
	virtual Size getContentSizeForNode(float size) const;
	virtual Vec2 getAnchorPointForNode() const;

	virtual float getNodeScrollSize(const Size &) const;
	virtual float getNodeScrollPosition(const Vec2 &) const;

	virtual bool addScrollNode(cocos2d::Node *, const Vec2 & pos, const Size & size, int z);
	virtual void updateScrollNode(cocos2d::Node *, const Vec2 & pos, const Size & size, int z);
	virtual bool removeScrollNode(cocos2d::Node *);

	virtual float getDistanceFromStart() const;

	virtual void setScrollMaxVelocity(float value);
	virtual float getScrollMaxVelocity() const;

	virtual cocos2d::Node * getFrontNode() const;
	virtual cocos2d::Node * getBackNode() const;

public: // internal, use with care
	virtual float getScrollMinPosition() const; // NaN by default
	virtual float getScrollMaxPosition() const; // NaN by default
	virtual float getScrollLength() const; // NaN by default
	virtual float getScrollSize() const;

	virtual void setScrollRelativePosition(float value);
	virtual float getScrollRelativePosition() const;
	virtual float getScrollRelativePosition(float pos) const;

	virtual void setScrollPosition(float pos);
	virtual float getScrollPosition() const;

	virtual Vec2 getPointForScrollPosition(float pos);

	virtual void setScrollDirty(bool value);
	virtual void updateScrollBounds();

protected: // common overload points
	virtual void onDelta(float delta);
	virtual void onOverscrollPerformed(float velocity, float pos, float boundary);

	virtual bool onSwipeEventBegin(const Vec2 &loc, const Vec2 &d, const Vec2 &v);
	virtual bool onSwipeEvent(const Vec2 &loc, const Vec2 &d, const Vec2 &v);
	virtual bool onSwipeEventEnd(const Vec2 &loc, const Vec2 &d, const Vec2 &v);

	virtual void onSwipeBegin();
	virtual bool onSwipe(float delta, float velocity, bool ended);
	virtual void onAnimationFinished();

	virtual cocos2d::ActionInterval *onSwipeFinalizeAction(float velocity);

	virtual void onScroll(float delta, bool finished);
	virtual void onOverscroll(float delta);
	virtual void onPosition();

	virtual void fixPosition();

	virtual bool onPressBegin(const Vec2 &);
	virtual bool onLongPress(const Vec2 &, const TimeInterval &, int count);
	virtual bool onPressEnd(const Vec2 &, const TimeInterval &);
	virtual bool onPressCancel(const Vec2 &, const TimeInterval &);

	virtual void onTap(int count, const Vec2 &loc);

protected:
	Layout _layout = Vertical;

	enum class Movement {
		None,
		Manual,
		Auto,
		Overscroll
	} _movement = Movement::None;

	float _scrollPosition = 0.0f; // current position of scrolling
	float _scrollSize = 0.0f; // size of scrolling area

	float _savedRelativePosition = std::numeric_limits<float>::quiet_NaN();

	float _scrollMin = std::numeric_limits<float>::quiet_NaN();
	float _scrollMax = std::numeric_limits<float>::quiet_NaN();

	float _maxVelocity = std::numeric_limits<float>::quiet_NaN();

    Vec2 _globalScale; // values to scale input gestures

    cocos2d::Node *_root = nullptr;
	ScrollFilterCallback _scrollFilter = nullptr;
    ScrollCallback _scrollCallback = nullptr;
    OverscrollCallback _overscrollCallback = nullptr;
    gesture::Listener *_listener = nullptr;
    ScrollController *_controller = nullptr;

    cocos2d::FiniteTimeAction *_animationAction = nullptr;
    Accelerated *_movementAction = nullptr;

    bool _bounce = (CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_MAC);
    bool _scrollDirty = true;
    bool _animationDirty = false;

    Padding _paddingGlobal;
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_SCROLL_SPSCROLLVIEWBASE_H_ */
