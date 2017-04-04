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

#ifndef LIBS_STAPPLER_COMPONENTS_GESTURES_SPGESTUREDATA_H_
#define LIBS_STAPPLER_COMPONENTS_GESTURES_SPGESTUREDATA_H_

#include "SPDefine.h"

NS_SP_EXT_BEGIN(gesture)

/// Touch id - number, associated with this touch in cocos2d system
/// Special Values:
///  -1 		- invalid touch
///  INT_MAX	- touch was unregistered in system, but still has valid points
struct Touch {
    int id = -1;
    Vec2 startPoint;
    Vec2 prevPoint;
    Vec2 point;

    Touch &operator=(cocos2d::Touch *);
    Touch(cocos2d::Touch *);

    Touch() = default;

    operator bool() const;

    const Vec2 &location() const;
    String description() const;
    void cleanup();
};

struct Tap {
	Touch touch;
	int count = 0;

    const Vec2 &location() const;
    String description() const;
    void cleanup();
};

struct Press {
	Touch touch;
	TimeInterval limit;
	TimeInterval time;
	int count = 0;

    const Vec2 &location() const;
    String description() const;
    void cleanup();
};

struct Swipe {
	Touch firstTouch;
	Touch secondTouch;
	Vec2 midpoint;
	Vec2 delta;
	Vec2 velocity;

    const Vec2 &location() const;
    String description() const;
    void cleanup();
};

struct Pinch {
	Touch first;
	Touch second;
	Vec2 center;
	float startDistance = 0.0f;
	float prevDistance = 0.0f;
	float distance = 0.0f;
	float scale = 0.0f;
	float velocity = 0.0f;

	void set(cocos2d::Touch *, cocos2d::Touch *, float vel);
    const Vec2 &location() const;
    String description() const;
    void cleanup();
};

struct Rotate {
	Touch first;
	Touch second;
	Vec2 center;
	float startAngle;
	float prevAngle;
	float angle;
	float velocity;

    const Vec2 &location() const;
    void cleanup();
};

struct Wheel {
	Touch position;
	Vec2 amount;

    const Vec2 &location() const;
    void cleanup();
};

enum class Type {
	Touch = 1,
	Press = 2,
	Swipe = 4,
	Tap = 8,
	Pinch = 16,
	Rotate = 32,
	Wheel = 64,
};

enum class Event {
	/** Action just started, listener should return true if it want to "capture" it.
	 * Captured actions will be automatically propagated to end-listener
	 * Other listeners branches will not recieve updates on action, that was not captured by them.
	 * Only one listener on every level can capture action. If one of the listeners return 'true',
	 * action will be captured by this listener, no other listener on this level can capture this action.
	 */
	Began,

	/** Action was activated:
	 * on Raw Touch - touch was moved
	 * on Tap - n-th  tap was recognized
	 * on Press - long touch was recognized
	 * on Swipe - touch was moved
	 * on Pinch - any of two touches was moved, scale was changed
	 * on Rotate - any of two touches was moved, rotation angle was changed
	 */
	Activated,
	Moved = Activated,
	OnLongPress = Activated,

	/** Action was successfully ended, no recognition errors was occurred */
	Ended,

	/** Action was not successfully ended, recognizer detects error in action
	 * pattern and failed to continue recognition */
	Cancelled,
};

NS_SP_EXT_END(gesture)

#endif /* LIBS_STAPPLER_COMPONENTS_GESTURES_SPGESTUREDATA_H_ */
