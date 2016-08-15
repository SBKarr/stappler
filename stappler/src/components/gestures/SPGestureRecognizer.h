/*
 * SPGestureRecognizer.h
 *
 *  Created on: 12 марта 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_COMPONENTS_GESTURES_SPGESTURERECOGNIZER_H_
#define LIBS_STAPPLER_COMPONENTS_GESTURES_SPGESTURERECOGNIZER_H_

#include "SPMovingAverage.h"
#include "SPGestureData.h"
#include "base/CCVector.h"

NS_SP_EXT_BEGIN(gesture)

class Recognizer : public cocos2d::Ref {
public:
	virtual ~Recognizer() { }

	virtual bool init();

	bool onTouchBegan(cocos2d::Touch *pTouch);
	void onTouchMoved(cocos2d::Touch *pTouch);
	void onTouchEnded(cocos2d::Touch *pTouch);
	void onTouchCancelled(cocos2d::Touch *pTouch);

	virtual void onMouseUp(cocos2d::EventMouse *ev);
	virtual void onMouseDown(cocos2d::EventMouse *ev);
	virtual void onMouseMove(cocos2d::EventMouse *ev);
	virtual void onMouseScroll(cocos2d::EventMouse *ev);

	virtual bool isTouchSupported() const { return false; }
	virtual bool isMouseSupported() const { return false; }

	uint32_t getTouchCount() const;
	bool hasTouch(cocos2d::Touch *touch) const;

	Event getEvent() const;
    bool isSuccessful() const;

	virtual void update(float dt);
	virtual cocos2d::Vec2 getLocation() const;
	virtual void cancel();

protected:
	bool hasTouch(cocos2d::Touch *touch);

	virtual bool addTouch(cocos2d::Touch *touch);
	virtual bool removeTouch(cocos2d::Touch *touch);
	virtual bool renewTouch(cocos2d::Touch *touch);

	virtual cocos2d::Touch *getTouchById(int id, int *index);
protected:
	bool _successfulEnd = true;

	cocos2d::Vector<cocos2d::Touch *> _touches;
	ssize_t _maxTouches = 0;
	Event _event = Event::Cancelled;
};

class TouchRecognizer : public Recognizer {
public:
	using Callback = std::function<void(TouchRecognizer *, Event, const Touch &)>;

public:
	TouchRecognizer();
	virtual ~TouchRecognizer() { }

	void setCallback(const Callback &func);
	void removeRecognizedTouch(const Touch &);

	virtual bool isTouchSupported() const override { return true; }

protected:
	virtual bool addTouch(cocos2d::Touch *touch) override;
	virtual bool removeTouch(cocos2d::Touch *touch) override;
	virtual bool renewTouch(cocos2d::Touch *touch) override;

	Callback _callback;
};

class TapRecognizer : public Recognizer {
public:
	using Callback = std::function<void(TapRecognizer *, const Tap &)>;

public:
	TapRecognizer();
	virtual ~TapRecognizer() { }

	void setCallback(const Callback &func);

	virtual cocos2d::Vec2 getLocation();
	virtual void update(float dt) override;
	virtual void cancel() override;

	virtual bool isTouchSupported() const override { return true; }

protected:
	virtual bool addTouch(cocos2d::Touch *touch) override;
	virtual bool removeTouch(cocos2d::Touch *touch) override;
	virtual bool renewTouch(cocos2d::Touch *touch) override;

	virtual void registerTap();

	Time _lastTapTime;

	Tap _gesture;
	Callback _callback;
};

class PressRecognizer : public Recognizer {
public:
	using Callback = std::function<void(PressRecognizer *, Event, const Press &)>;

public:
	PressRecognizer();
	virtual ~PressRecognizer() { }

	void setCallback(const Callback &func);

	virtual cocos2d::Vec2 getLocation();
	virtual void update(float dt) override;
	virtual void cancel() override;

	virtual bool isTouchSupported() const override { return true; }

protected:
	virtual bool addTouch(cocos2d::Touch *touch) override;
	virtual bool removeTouch(cocos2d::Touch *touch) override;
	virtual bool renewTouch(cocos2d::Touch *touch) override;

	Time _lastTime;
	bool _notified = false;

	Press _gesture;
	Callback _callback;
};

class SwipeRecognizer : public Recognizer {
public:
	using Callback = std::function<void(SwipeRecognizer *, Event, const Swipe &)>;

public:
	SwipeRecognizer();
	virtual ~SwipeRecognizer() { }

	virtual bool init(float threshold, bool sendThreshold);

	void setCallback(const Callback &func);

	virtual cocos2d::Vec2 getLocation();
	virtual void cancel() override;

	virtual bool isTouchSupported() const override { return true; }

protected:
	virtual bool addTouch(cocos2d::Touch *touch) override;
	virtual bool removeTouch(cocos2d::Touch *touch) override;
	virtual bool renewTouch(cocos2d::Touch *touch) override;

	Time _lastTime;
	MovingAverage _velocityX, _velocityY;

	bool _swipeBegin;
	cocos2d::Touch *_currentTouch = nullptr;

	Swipe _gesture;
	Callback _callback;

	float _threshold = 6.0f;
	bool _sendThreshold = true;
};

class PinchRecognizer : public Recognizer {
public:
	using Callback = std::function<void(PinchRecognizer *, Event, const Pinch &)>;

public:
	PinchRecognizer();
	virtual ~PinchRecognizer() { }

	void setCallback(const Callback &func);

	virtual cocos2d::Vec2 getLocation();
	virtual void cancel() override;

	virtual bool isTouchSupported() const override { return true; }

protected:
	virtual bool addTouch(cocos2d::Touch *touch) override;
	virtual bool removeTouch(cocos2d::Touch *touch) override;
	virtual bool renewTouch(cocos2d::Touch *touch) override;

	Time _lastTime;
	MovingAverage _velocity;

	Pinch _gesture;
	Callback _callback;
};

class RotateRecognizer : public Recognizer {
public:
	using Callback = std::function<void(RotateRecognizer *, Event, const Rotate &)>;

public:
	RotateRecognizer();
	virtual ~RotateRecognizer();

	void setCallback(const Callback &func);

	virtual cocos2d::Vec2 getLocation();
	virtual void update(float dt) override;
	virtual void cancel() override;

	virtual bool isTouchSupported() const override { return true; }

protected:
	virtual bool addTouch(cocos2d::Touch *touch) override;
	virtual bool removeTouch(cocos2d::Touch *touch) override;
	virtual bool renewTouch(cocos2d::Touch *touch) override;

	uint64_t _lastTime;
	MovingAverage _velocity;
	bool _rotateEnded;

	Rotate _gesture;
	Callback _callback;
};

class WheelRecognizer : public Recognizer {
public:
	using Callback = std::function<void(WheelRecognizer *, Event, const Wheel &)>;

public:
	virtual ~WheelRecognizer() { }

	virtual void onMouseScroll(cocos2d::EventMouse *ev) override;

	void setCallback(const Callback &func);

	virtual bool isMouseSupported() const override { return true; }

protected:
	Wheel _gesture;
	Callback _callback;
};

NS_SP_EXT_END(gesture)

#endif /* LIBS_STAPPLER_COMPONENTS_GESTURES_SPGESTURERECOGNIZER_H_ */
