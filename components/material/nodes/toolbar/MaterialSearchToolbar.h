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

#ifndef MATERIAL_NODES_TOOLBAR_MATERIALSEARCHTOOLBAR_H_
#define MATERIAL_NODES_TOOLBAR_MATERIALSEARCHTOOLBAR_H_

#include "MaterialToolbar.h"
#include "MaterialInputLabel.h"

NS_MD_BEGIN

class SearchToolbar : public ToolbarBase, protected InputLabelDelegate {
public:
	using Handler = ime::Handler;
	using Cursor = ime::Cursor;

	using Callback = Function<void(const StringView &)>;

	enum Mode {
		Persistent,
		Expandable,
	};

	virtual bool init(Mode m = Mode::Persistent);
	virtual bool init(const Callback &, Mode m = Mode::Persistent);

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual void setFont(material::FontType);

	virtual void setTitle(const StringView &) override;
	virtual StringView getTitle() const override;

	virtual void setPlaceholder(const StringView &);
	virtual StringView getPlaceholder() const;

	virtual void setString(const StringView &);
	virtual StringView getString() const;

	virtual void setAllowAutocorrect(bool);
	virtual bool isAllowAutocorrect() const;

	virtual void onSearchMenuButton();
	virtual void onSearchIconButton();

	virtual void setColor(const Color &color) override;
	virtual void setTextColor(const Color &color) override;

	virtual void acquireInput();
	virtual void releaseInput();

	virtual InputLabel *getLabel() const;
	virtual MenuSource *getSearchMenu() const;
	virtual MenuSource *getCommonMenu() const;

	virtual void setInputTouchFilter(const Function<bool(const Vec2 &)> &);
	virtual void setInputTouchFilterEnabled(bool);

	virtual void setSearchEnabled(bool);
	virtual bool isSearchEnabled() const;

protected:
	virtual void layoutSubviews() override;

	virtual bool onInputString(const WideStringView &str, const Cursor &c) override;
	virtual void onInput() override;
	virtual void onCursor(const Cursor &) override;
	virtual void onActivated(bool) override;
	virtual void onPointer(bool) override;

	virtual bool onPressBegin(const Vec2 &);
	virtual bool onLongPress(const Vec2 &, const TimeInterval &, int count);
	virtual bool onPressEnd(const Vec2 &);
	virtual bool onPressCancel(const Vec2 &);

	virtual bool onSwipeBegin(const Vec2 &, const Vec2 &);
	virtual bool onSwipe(const Vec2 &, const Vec2 &);
	virtual bool onSwipeEnd(const Vec2 &);

	virtual void onClearButton();
	virtual void onVoiceButton();
	virtual void onSearchButton();

	virtual void updateMenu();

	virtual void onMenuCut();
	virtual void onMenuCopy();
	virtual void onMenuPaste();

	virtual bool onInputTouchFilter(const Vec2 &);

	virtual void onNavTapped() override;
	virtual void clearSearch();

	InputLabelContainer *_node = nullptr;
	Label *_placeholder = nullptr;
	InputLabel *_label = nullptr;
	InputMenu *_menu = nullptr;

	Rc<MenuSource> _searchMenu;
	Rc<MenuSource> _commonMenu;
	Rc<MenuSource> _altMenu;

	Callback _callback;
	Function<bool(const Vec2 &)> _touchFilterCallback;

	String _title;
	Mode _mode = Mode::Persistent;
	float _tmpNavProgress = 0.0f;
	bool _hasSwipe = false;
	bool _hasText = false;
	bool _searchEnabled = false;
	bool _canSearchBeEnabled = true;
};

NS_MD_END

#endif /* MATERIAL_NODES_TOOLBAR_MATERIALSEARCHTOOLBAR_H_ */
