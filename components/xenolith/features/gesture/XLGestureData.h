/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_XENOLITH_FEATURES_GESTURE_XLGESTUREDATA_H_
#define COMPONENTS_XENOLITH_FEATURES_GESTURE_XLGESTUREDATA_H_

#include "XLDefine.h"

namespace stappler::xenolith::gesture {

/// Touch id - number, associated with this touch in cocos2d system
/// Special Values:
///  -1 		- invalid touch
///  INT_MAX	- touch was unregistered in system, but still has valid points
struct Touch {
    int id = -1;
    Vec2 startPoint;
    Vec2 prevPoint;
    Vec2 point;

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

	void set(const Touch &, const Touch &, float vel);
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

enum class KeyCode {
    None = 0,
    Key_Pause,
    Key_ScrollLock,
    Key_Print,
    Key_SysReq,
    Key_Break,
    Key_Escape,
    Key_Back = Key_Escape,
    Key_Backspace,
    Key_Tab,
    Key_BackTab,
    Key_Return,
    Key_CapsLock,
    Key_Shift,
    Key_LeftShift= Key_Shift,
    Key_RightShift,
    Key_Ctrl,
    Key_LeftCtrl = Key_Ctrl,
    Key_RightCtrl,
    Key_Alt,
    Key_LeftAlt = Key_Alt,
    Key_RightAlt,
    Key_Menu,
    Key_Hyper,
    Key_Insert,
    Key_Home,
    Key_PgUp,
    Key_Delete,
    Key_End,
    Key_PgDown,
    Key_LeftArrow,
    Key_RightArrow,
    Key_UpArrow,
    Key_DownArrow,
    Key_NumLock,
    Key_NumPadPeriod,
    Key_NumPadPlus,
    Key_NumPadMinus,
    Key_NumPadMultiply,
    Key_NumPadDivide,
    Key_NumPadEnter,
    Key_NumPadHome,
    Key_NumPadUp,
    Key_NumPadPgUp,
    Key_NumPadPlLeft,
    Key_NumPadFive,
    Key_NumPadRight,
    Key_NumPadEnd,
    Key_NumPadDown,
    Key_NumPadPgDown,
    Key_NumPadInsert,
    Key_NumPadDelete,
	Key_NumPadEqual,
    Key_F1,
    Key_F2,
    Key_F3,
    Key_F4,
    Key_F5,
    Key_F6,
    Key_F7,
    Key_F8,
    Key_F9,
    Key_F10,
    Key_F11,
    Key_F12,
    Key_Space,
    Key_Exclam,
    Key_Quote,
    Key_Number,
    Key_Dollar,
    Key_Percent,
    Key_Circumflex,
    Key_Ampersand,
    Key_Apostrophe,
    Key_LeftParentesis,
    Key_RightParentesis,
    Key_Asterisk,
    Key_Plus,
    Key_Comma,
    Key_Minus,
    Key_Period,
    Key_Slash,
    Key_0,
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,
    Key_Colon,
    Key_Semicolon,
    Key_LessThen,
    Key_Equal,
    Key_GreaterThen,
    Key_Question,
    Key_At,
    Key_CapitalA,
    Key_CapitalB,
    Key_CapitalC,
    Key_CapitalD,
    Key_CapitalE,
    Key_CapitalF,
    Key_CapitalG,
    Key_CapitalH,
    Key_CapitalI,
    Key_CapitalJ,
    Key_CapitalK,
    Key_CapitalL,
    Key_CapitalM,
    Key_CapitalN,
    Key_CapitalO,
    Key_CapitalP,
    Key_CapitalQ,
    Key_CapitalR,
    Key_CapitalS,
    Key_CapitalT,
    Key_CapitalU,
    Key_CapitalV,
    Key_CapitalW,
    Key_CapitalX,
    Key_CapitalY,
    Key_CapitalZ,
    Key_LeftBracket,
    Key_BackSlash,
    Key_RightBracket,
    Key_Underscore,
    Key_Grave,
    Key_A,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,
    Key_LeftBrace,
    Key_Bar,
    Key_RightBrace,
    Key_Tilde,
    Key_Euro,
    Key_Pound,
    Key_Yen,
    Key_MiddleDot,
    Key_Search,
    Key_DpadLeft,
    Key_DpadRight,
    Key_DpadUp,
    Key_DpadDown,
    Key_DpadCenter,
    Key_Enter,
    Key_Play
};

}

#endif /* COMPONENTS_XENOLITH_FEATURES_GESTURE_XLGESTUREDATA_H_ */
