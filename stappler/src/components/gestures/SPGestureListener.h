/*
 * SPGestureListener.h
 *
 *  Created on: 12 марта 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_COMPONENTS_GESTURES_SPGESTURELISTENER_H_
#define LIBS_STAPPLER_COMPONENTS_GESTURES_SPGESTURELISTENER_H_

#include "SPGestureData.h"
#include "SPGestureRecognizer.h"
#include "2d/CCComponent.h"
#include "base/CCEventListenerTouch.h"
#include "base/CCEventListenerMouse.h"

NS_SP_EXT_BEGIN(gesture)

class Listener : public cocos2d::Component {
public: // aliases
	template<typename T>
	using Callback = std::function<bool(Event, const T &)>;

	using DefaultTouchFilter = std::function<bool(const cocos2d::Vec2 &)>;
	using DefaultGestureFilter = std::function<bool(const cocos2d::Vec2 &, Type)>;

	using TouchFilter = std::function<bool(const cocos2d::Vec2 &, const DefaultTouchFilter &)>;
	using GestureFilter = std::function<bool(const cocos2d::Vec2 &, Type, const DefaultGestureFilter &)>;

public: // callbacks
	void setTouchCallback(const Callback<Touch> &);
	void setTapCallback(const Callback<Tap> &);
	void setPressCallback(const Callback<Press> &);
	void setSwipeCallback(const Callback<Swipe> &, float threshold = nan(), bool sendThreshold = true);
	void setPinchCallback(const Callback<Pinch> &);
	void setRotateCallback(const Callback<Rotate> &);

	void setMouseWheelCallback(const Callback<Wheel> &);

	void setTouchFilter(const TouchFilter &);
	void setGestureFilter(const GestureFilter &);

	const Callback<Touch> & getTouchCallback() const { return _onTouch; }
	const Callback<Tap> & getTapCallback() const { return _onTap; }
	const Callback<Press> & getPressCallback() const { return _onPress; }
	const Callback<Swipe> & getSwipeCallback() const { return _onSwipe; }
	const Callback<Pinch> & getPinchCallback() const { return _onPinch; }
	const Callback<Rotate> & getRotateCallback() const { return _onRotate; }

	const TouchFilter & getTouchFilter() const { return _touchFilter; }
	const GestureFilter & getGestureFilter() const { return _gestureFilter; }

public: // options
	void setOpacityFilter(uint8_t filter);
	uint8_t getOpacityFilter() const;

	void setSwallowTouches(bool value);
	bool isSwallowTouches() const;

	void setPriority(int32_t value);
	int32_t getPriority();

public: // overrides
	virtual ~Listener();
	virtual bool init() override;
	virtual void setEnabled(bool value) override;
	virtual void onEnter() override;
	virtual void onEnterTransitionDidFinish() override;
	virtual void onExitTransitionDidStart() override;
	virtual void onExit() override;

	void update(float dt);

	bool hasGesture(Type) const;

protected:
	bool shouldProcessTouch(const cocos2d::Vec2 &loc);
	bool _shouldProcessTouch(const cocos2d::Vec2 &loc); // default realization

	bool shouldProcessGesture(const cocos2d::Vec2 &loc, Type type);
	bool _shouldProcessGesture(const cocos2d::Vec2 &loc, Type type); // default realization

	bool onTouchBegan(cocos2d::Touch *pTouch);
	void onTouchMoved(cocos2d::Touch *pTouch);
	void onTouchEnded(cocos2d::Touch *pTouch);
	void onTouchCancelled(cocos2d::Touch *pTouch);

	void onMouseUp(cocos2d::Event *ev);
	void onMouseDown(cocos2d::Event *ev);
	void onMouseMove(cocos2d::Event *ev);
	void onMouseScroll(cocos2d::Event *ev);

	void onTouch(TouchRecognizer *, Event, const Touch &);
	void onTap(TapRecognizer *, Event, const Tap &);
	void onPress(PressRecognizer *, Event, const Press &);
	void onSwipe(SwipeRecognizer *, Event, const Swipe &);
	void onPinch(PinchRecognizer *, Event, const Pinch &);
	void onRotate(RotateRecognizer *, Event, const Rotate &);
	void onWheel(WheelRecognizer *, Event, const Wheel &);

protected:
	void registerWithDispatcher();
	void unregisterWithDispatcher();

	void clearRecognizer(Type type);
	Rc<Recognizer> getRecognizer(Type type);
	void setRecognizer(Rc<Recognizer> &&, Type type);

protected:
	Callback<Touch> _onTouch = nullptr;
	Callback<Tap> _onTap = nullptr;
	Callback<Press> _onPress = nullptr;
	Callback<Swipe> _onSwipe = nullptr;
	Callback<Pinch> _onPinch = nullptr;
	Callback<Rotate> _onRotate = nullptr;
	Callback<Wheel> _onWheel = nullptr;

	TouchFilter _touchFilter;
	GestureFilter _gestureFilter;

	std::map<Type, Rc<Recognizer>> _recognizers;

	int32_t _priority = 0;
	uint8_t _opacityFilter = 0;

	bool _registred = false;
	Rc<cocos2d::EventListenerTouchOneByOne> _touchListener = nullptr;
	Rc<cocos2d::EventListenerMouse> _mouseListener = nullptr;
};

NS_SP_EXT_END(gesture)

#endif /* LIBS_STAPPLER_COMPONENTS_GESTURES_SPGESTURELISTENER_H_ */
