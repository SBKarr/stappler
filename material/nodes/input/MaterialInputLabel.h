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

#ifndef MATERIAL_NODES_INPUT_MATERIALINPUTLABEL_H_
#define MATERIAL_NODES_INPUT_MATERIALINPUTLABEL_H_

#include "MaterialLabel.h"
#include "SPDynamicBatchNode.h"
#include "SPIME.h"
#include "SPStrictNode.h"

NS_MD_BEGIN

enum class InputError {
	OverflowChars,
	InvalidChar,
};

class InputLabelDelegate {
public:
	using Error = InputError;
	using Handler = ime::Handler;
	using Cursor = ime::Cursor;

	virtual ~InputLabelDelegate();
	virtual bool onInputChar(char16_t);
	virtual bool onInputString(const WideStringView &str, const Cursor &c);

	virtual void onCursor(const Cursor &);
	virtual void onInput();

	virtual void onActivated(bool);
	virtual void onError(Error);

	virtual void onPointer(bool);
};

class InputLabelContainer : public StrictNode {
public:
	template <typename T, typename ... Args>
	auto setLabel(const Rc<T> &ptr, Args && ... args) {
		setLabel(ptr.get(), forward<Args>(args)...);
		return ptr.get();
	}

	virtual void setLabel(InputLabel *, int zIndex = 0);
	virtual InputLabel *getLabel() const;

	virtual void update(float dt) override;
	virtual void onCursor();
	virtual void onInput();

	virtual bool onSwipeBegin(const Vec2 &, const Vec2 &);
	virtual bool onSwipe(const Vec2 &, const Vec2 &);
	virtual bool onSwipeEnd(const Vec2 &);

protected:
	enum Adjust {
		None,
		Left,
		Right
	};

	virtual void onLabelPosition();

	virtual void runAdjust(float);
	virtual void scheduleAdjust(Adjust, const Vec2 &, float pos);

	InputLabel *_label = nullptr;
	Adjust _adjust = None;
	Vec2 _adjustValue;
	float _adjustPosition = 0.0f;
	bool _swipeCaptured = false;
};

class InputLabel : public Label {
public:
	enum class PasswordMode {
		NotPassword,
		ShowAll,
		ShowChar,
		ShowNone,
	};

	using Error = InputError;
	using Handler = ime::Handler;
	using Cursor = ime::Cursor;

	using InputType = ime::InputType;

	class Selection : public DynamicBatchNode {
	public:
		virtual ~Selection();
		virtual bool init() override;
		virtual void clear();
		virtual void emplaceRect(const Rect &);
		virtual void updateColor() override;
	};

public:
	virtual bool init(FontType, float w = 0);
	virtual bool init(const String &, float w = 0);
	virtual bool init(const DescriptionStyle &, float w = 0);

	virtual void visit(cocos2d::Renderer *r, const Mat4& t, uint32_t f, ZPath &zPath) override;
	virtual void onContentSizeDirty() override;
	virtual void onExit() override;

	virtual Vec2 getCursorMarkPosition() const;

	virtual void setCursorColor(const Color &, bool pointer = true);
	virtual const Color &getCursorColor() const;

	virtual void setPointerColor(const Color &);
	virtual const Color &getPointerColor() const;

	virtual void setString(const StringView &) override;
	virtual void setString(const WideStringView &) override;
	virtual WideStringView getString() const override;

	virtual void setCursor(const Cursor &);
	virtual const Cursor &getCursor() const;

	virtual void setInputType(InputType);
	virtual InputType getInputType() const;

	virtual void setPasswordMode(PasswordMode);
	virtual PasswordMode getPasswordMode();

	virtual void setDelegate(InputLabelDelegate *);
	virtual InputLabelDelegate *getDelegate() const;

	virtual void setEnabled(bool);
	virtual bool isEnabled() const;

	virtual void setRangeAllowed(bool);
	virtual bool isRangeAllowed() const;

	virtual void setAllowMultiline(bool);
	virtual bool isAllowMultiline() const;

	virtual void setAllowAutocorrect(bool);
	virtual bool isAllowAutocorrect() const;

	virtual void setCursorAnchor(float);
	virtual float getCursorAnchor() const;

	virtual void acquireInput();
	virtual void releaseInput();

	virtual bool empty() const override;
	virtual bool isActive() const;
	virtual bool isPointerEnabled() const;

	virtual String getSelectedString() const;
	virtual void pasteString(const String &);
	virtual void pasteString(const WideString &);
	virtual void eraseSelection();

	virtual void setInputTouchFilter(const Function<bool(const Vec2 &)> &);
	virtual const Function<bool(const Vec2 &)> &getInputTouchFilter() const;

public:
	virtual draw::PathNode *getTouchedCursor(const Vec2 &, float = 4.0f);

	virtual bool onPressBegin(const Vec2 &);
	virtual bool onLongPress(const Vec2 &, const TimeInterval &, int count);
	virtual bool onPressEnd(const Vec2 &);
	virtual bool onPressCancel(const Vec2 &);

	virtual bool onSwipeBegin(const Vec2 &);
	virtual bool onSwipe(const Vec2 &, const Vec2 &);
	virtual bool onSwipeEnd(const Vec2 &);

public:
	Layer *getCursorLayer() const;
	draw::PathNode *getCursorPointer() const;
	draw::PathNode *getCursorStart() const;
	draw::PathNode *getCursorEnd() const;

protected:
	virtual void onText(const WideStringView &, const Cursor &);
	virtual void onKeyboard(bool, const Rect &, float);
	virtual void onInput(bool);
	virtual void onEnded();

	virtual void onError(Error);

	virtual void updateCursor();
	virtual bool updateString(const WideStringView &str, const Cursor &c);
	virtual void updateFocus();

	virtual void showLastChar();
	virtual void hideLastChar();

	virtual void scheduleCursorPointer();
	virtual void unscheduleCursorPointer();

	virtual void setPointerEnabled(bool);
	virtual void updatePointer();

	virtual int32_t getInputTypeValue() const;

	bool _enabled = true;
	bool _inputEnabled = false;
	bool _rangeAllowed = true;
	bool _isLongPress = false;
	bool _pointerEnabled = false;
	bool _cursorDirty = false;

	bool _allowMultiline = true;
	bool _allowAutocorrect = true;

	float _cursorAnchor = 1.2f;

	Color _selectionColor = Color::Blue_500;
	Color _cursorColor = Color::Blue_500;

	WideString _inputString;

	draw::PathNode * _selectedCursor = nullptr;
	Layer *_cursorLayer = nullptr;
	draw::PathNode *_cursorPointer = nullptr;
	draw::PathNode *_cursorStart = nullptr;
	draw::PathNode *_cursorEnd = nullptr;

	Selection *_cursorSelection = nullptr;

	InputType _inputType = InputType::Default;
	Cursor _cursor;
	Handler _handler;

	PasswordMode _password = PasswordMode::NotPassword;
	InputLabelDelegate *_delegate = nullptr;
};

NS_MD_END

#endif /* MATERIAL_NODES_INPUT_MATERIALINPUTLABEL_H_ */
