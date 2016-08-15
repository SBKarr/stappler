//
//  SPIME.h
//  stappler
//
//  Created by SBKarr on 13.08.14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

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
	, length((first > last)?(first - last).get():(last - first).get()) { }
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

NS_SP_EXT_END(ime)

#endif /* defined(__stappler__SPIME__) */
