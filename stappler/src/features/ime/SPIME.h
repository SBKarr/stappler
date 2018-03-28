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

#ifndef STAPPLER_SRC_FEATURES_IME_IME_H_
#define STAPPLER_SRC_FEATURES_IME_IME_H_

#include "SPDefine.h"
#include "SPEventHeader.h"

NS_SP_EXT_BEGIN(ime)

enum class InputType : int32_t {
	Empty				= 0,
	Date_Date			= 1,
	Date_DateTime		= 2,
	Date_Time			= 3,
	Date				= Date_DateTime,

	Number_Numbers		= 4,
	Number_Decimial		= 5,
	Number_Signed		= 6,
	Number				= Number_Numbers,

	Phone				= 7,

	Text_Text			= 8,
	Text_Search			= 9,
	Text_Punctuation	= 10,
	Text_Email			= 11,
	Text_Url			= 12,
	Text				= Text_Text,

	Default				= Text_Text,

	ClassMask			= 0b00011111,
	PasswordBit			= 0b00100000,
	MultiLineBit		= 0b01000000,
	AutoCorrectionBit	= 0b10000000,

	ReturnKeyMask		= 0b00001111 << 8,

	ReturnKeyDefault	= 1 << 8,
	ReturnKeyGo			= 2 << 8,
	ReturnKeyGoogle		= 3 << 8,
	ReturnKeyJoin		= 4 << 8,
	ReturnKeyNext		= 5 << 8,
	ReturnKeyRoute		= 6 << 8,
	ReturnKeySearch		= 7 << 8,
	ReturnKeySend		= 8 << 8,
	ReturnKeyYahoo		= 9 << 8,
	ReturnKeyDone		= 10 << 8,
	ReturnKeyEmergencyCall = 11 << 8,
};

SP_DEFINE_ENUM_AS_MASK(InputType);

using CursorPosition = ValueWrapper<uint32_t, class CursorPositionFlag>;
using CursorLength = ValueWrapper<uint32_t, class CursorStartFlag>;

struct Cursor {
	uint32_t start;
	uint32_t length;

	Cursor() : start(maxOf<uint32_t>()), length(0) { }
	Cursor(uint32_t pos) : start(pos), length(0) { }
	Cursor(uint32_t start, uint32_t length) : start(start), length(length) { }
	Cursor(CursorPosition pos) : start(pos.get()), length(0) { }
	Cursor(CursorPosition pos, CursorLength len) : start(pos.get()), length(len.get()) { }
	Cursor(CursorPosition first, CursorPosition last)
	: start(MIN(first.get(), last.get()))
	, length(((first > last)?(first - last).get():(last - first).get()) + 1) { }
};

struct Handler {
	Function<void(const WideString &, const Cursor &)> onText;
	Function<void(bool, const Rect &, float)> onKeyboard;
	Function<void(bool)> onInput;
	Function<void()> onEnded;
	Function<bool(const Vec2 &)> onTouchFilter;

	bool run(const WideString &str = u"", const Cursor & = Cursor(), int32_t = 0);
	void cancel();

	// only if this handler is active
	bool setString(const WideString &str, const Cursor & = Cursor());
	bool setCursor(const Cursor &);

	const WideString &getString() const;
	const Cursor &getCursor() const;

	bool isInputEnabled() const;
	bool isKeyboardVisible() const;
	const Rect &getKeyboardRect() const;

	bool isActive() const;

	~Handler();
};

extern EventHeader onKeyboard;
extern EventHeader onInput;

bool isInputEnabled();
void cancel();

bool isKeyboardVisible();
float getKeyboardDuration();
Rect getKeyboardRect();

WideString *getNativeStringPointer();
Cursor *getNativeCursorPointer();

NS_SP_EXT_END(ime)

#endif
