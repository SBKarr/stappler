/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef RICH_TEXT_RTTOOLTIP_H_
#define RICH_TEXT_RTTOOLTIP_H_

#include "MaterialNode.h"
#include "RTListenerView.h"

NS_RT_BEGIN

class Tooltip : public material::MaterialNode {
public:
	using Result = layout::Result;
	using CloseCallback = std::function<void()>;
	using CopyCallback = std::function<void(const String &)>;

	static EventHeader onCopy;

	virtual ~Tooltip();
	virtual bool init(CommonSource *, const Vector<String> &ids);
	virtual void onContentSizeDirty() override;
	virtual void onEnter() override;

	virtual void setMaxContentSize(const Size &);
	virtual void setOriginPosition(const Vec2 &pos, const Size &parentSize, const Vec2 &);

	virtual void setExpanded(bool value);

	virtual void pushToForeground();
	virtual void close();

	virtual material::Toolbar *getToolbar() const { return _toolbar; }
	virtual material::MenuSource *getActions() const { return _actions; }
	virtual material::ToolbarLayout *getLayout() const { return _layout; }

	virtual const cocos2d::Size &getDefaultSize() const { return _defaultSize; }

	virtual void onDelayedFadeOut();
	virtual void onFadeIn();
	virtual void onFadeOut();

	virtual void setCloseCallback(const CloseCallback &);

	virtual rich_text::Renderer *getRenderer() const;

	virtual String getSelectedString() const;

protected:
	virtual void onLightLevel() override;
	virtual void onResult(layout::Result *);

	virtual void copySelection();
	virtual void cancelSelection();
	virtual void onSelection();

	Vec2 _originPosition;
	Vec2 _worldPos;
	Size _parentSize;
	Size _maxContentSize;

	Size _defaultSize;

	cocos2d::Component *_listener = nullptr;

	Rc<material::MenuSource> _actions;
	Rc<material::MenuSource> _selectionActions;
	material::ToolbarLayout *_layout = nullptr;
	material::Toolbar *_toolbar = nullptr;

	ListenerView *_view = nullptr;
	CloseCallback _closeCallback;

	bool _expanded = true;
};

NS_RT_END

#endif /* RICH_TEXT_RTTOOLTIP_H_ */
