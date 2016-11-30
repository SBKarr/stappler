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

#ifndef MATERIAL_NODES_INPUT_MATERIALTEXTFIELD_H_
#define MATERIAL_NODES_INPUT_MATERIALTEXTFIELD_H_

#include "MaterialLabel.h"
#include "SPCharReader.h"
#include "SPIME.h"

NS_MD_BEGIN

class TextField : public cocos2d::Node {
public:
	using Handler = ime::Handler;
	using Cursor = ime::Cursor;
	using InputFilter = std::function<bool(char16_t)>;

	enum class PasswordMode {
		ShowAll,
		ShowChar,
		ShowNone,
	};

	enum class Error {
		OverflowChars,
		InvalidChar,
	};

public:
	virtual bool init(FontType, float width);
	virtual void onContentSizeDirty() override;
	virtual void onExit() override;

	virtual void setWidth(float);
	virtual float getWidth() const;

	virtual void setPadding(const Padding &);
	virtual const Padding &getPadding() const;

	virtual void setNormalColor(const Color &);
	virtual const Color &getNormalColor() const;

	virtual void setErrorColor(const Color &);
	virtual const Color &getErrorColor() const;

	virtual void setString(const WideString &);
	virtual void setString(const String &);
	virtual const WideString &getString() const;

	virtual void setPlaceholder(const WideString &);
	virtual void setPlaceholder(const String &);
	virtual const WideString &getPlaceholder() const;

	virtual void setInputFilter(const InputFilter &);
	virtual void setInputFilter(InputFilter &&);
	virtual const InputFilter &getInputFilter() const;

	virtual void setMaxChars(size_t);
	virtual size_t getMaxChars() const;

	virtual void setCursor(const Cursor &);
	virtual const Cursor &getCursor() const;

	virtual void setPasswordMode(PasswordMode);
	virtual PasswordMode getPasswordMode();

	virtual void setEnabled(bool);
	virtual bool isEnabled() const;

	virtual void acquireInput();
	virtual void releaseInput();

	virtual bool isPlaceholderEnabled() const;
	virtual bool empty() const;

protected:
	virtual bool onPressBegin(const Vec2 &);
	virtual bool onLongPress(const Vec2 &);
	virtual bool onPressEnd(const Vec2 &);
	virtual bool onPressCancel(const Vec2 &);

	virtual void onText(const std::u16string &, const Cursor &);
	virtual void onKeyboard(bool, const Rect &, float);
	virtual void onInput(bool);
	virtual void onEnded();

	virtual void onError(Error);

	virtual void updateSize();
	virtual void updateCursor();
	virtual bool updateString(const std::u16string &str, const Cursor &c);
	virtual void updateFocus();

	virtual void showLastChar();
	virtual void hideLastChar();

	Color _normalColor = Color::Blue_500;
	Color _errorColor = Color::Red_500;

	std::u16string _placeholder;
	std::u16string _string;

	Label *_label;
	Layer *_cursorLayer;

	Layer *_background;

	Cursor _cursor;

	gesture::Listener *_gestureListener;

	Handler _handler;

	bool _enabled = true;
	float _width;
	Padding _padding = Padding(12.0f, 0.0f);
	PasswordMode _password;
	InputFilter _inputFilter = nullptr;
};

NS_MD_END

#endif /* MATERIAL_NODES_INPUT_MATERIALTEXTFIELD_H_ */
