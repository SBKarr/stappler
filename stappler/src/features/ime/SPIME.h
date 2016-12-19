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

#ifndef __stappler__SPIME__
#define __stappler__SPIME__

#include "SPDefine.h"

NS_SP_EXT_BEGIN(ime)

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
	std::function<void(const std::u16string &, const Cursor &)> onText;
	std::function<void(bool, const Rect &, float)> onKeyboard;
	std::function<void(bool)> onInput;
	std::function<void()> onEnded;

	bool run(const std::u16string &str = u"", const Cursor & = Cursor());
	void cancel();

	// only if this handler is active
	bool setString(const std::u16string &str, const Cursor & = Cursor());
	bool setCursor(const Cursor &);

	const std::u16string &getString() const;
	const Cursor &getCursor() const;

	bool isInputEnabled() const;
	bool isKeyboardVisible() const;
	const cocos2d::Rect &getKeyboardRect() const;

	bool isActive() const;

	~Handler();
};

bool isInputEnabled();

NS_SP_EXT_END(ime)

#endif /* defined(__stappler__SPIME__) */
