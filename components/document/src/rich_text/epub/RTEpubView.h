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

#ifndef RICH_TEXT_EPUB_RTEPUBVIEW_H_
#define RICH_TEXT_EPUB_RTEPUBVIEW_H_

#include "RTSource.h"
#include "RTEpubContentsView.h"
#include "RTView.h"

#include "MaterialToolbarLayout.h"
#include "MaterialToolbar.h"

#include "SLFontFormatter.h"
#include "SLResult.h"

NS_RT_BEGIN

class EpubView : public material::ToolbarLayout {
public:
	virtual bool init(Source *, const StringView &title);
	virtual void onContentSizeDirty() override;
    virtual void visit(cocos2d::Renderer *, const Mat4 &, uint32_t, ZPath &) override;

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual void setTitle(const StringView &) override;
	virtual StringView getTitle() const override;

	virtual void setLocalHeader(bool value);
	virtual bool isLocalHeader() const;

	virtual void onScroll(float delta, bool finished) override;

	virtual void setBookmarksSource(data::Source *);
	virtual void setLayout(View::Layout);

	virtual void setBookmarksEnabled(bool);
	virtual bool isBookmarksEnabled() const;

	virtual void setExtendedNavigationEnabled(bool);
	virtual bool isExtendedNavigationEnabled() const;

protected:
	virtual void onBaseNode(const node::Params &, const Padding &, float offset) override;

	virtual void onRefreshButton();
	virtual void onLayoutButton();

	virtual void onButtonLeft();
	virtual void onButtonRight();
	virtual void onReaderBackButton();

	virtual void onProgressPosition(float);

	virtual void showBackButton(float val);
	virtual void hideBackButton();

	virtual void onShowBackButton(float val);
	virtual void onHideBackButton();

	virtual void onContentsRef(const String &);
	virtual void onDocument(rich_text::Document *);
	virtual void onBookmarkData(size_t object, size_t position, float scroll);

	virtual void updateTitle();

	virtual void onSelection(bool enabled);

	virtual void showSelectionToolbar();
	virtual void hideSelectionToolbar();

protected:
	virtual Rc<View> constructTextView(Source *);
	virtual void onTextViewTapCallback(int count, const Vec2 &loc);
	virtual void onTextViewAnimationCallback();
	virtual void onTextViewPositionCallback(float val);

	virtual Rc<EpubContentsView> constructContentsView();

	virtual Rc<EpubNavigation> constructNavigationView();
	virtual void onNavigationViewCallback(float val);

	virtual void onCopyButton();
	virtual void onBookmarkButton();
	virtual void onEmailButton();
	virtual void onShareButton();
	virtual void onMisprintButton();

	bool _localHeader = false;

	enum BackButtonStatus : uint8_t {
		Hide, Show, Wait, None
	} _backButtonStatus = None;

	Rc<Source> _source;

	Rc<material::MenuSource> _viewSource;
	Rc<material::MenuSource> _selectionSource;

	View * _view = nullptr;
	material::MenuSourceButton *_layoutButton = nullptr;

	material::Button *_buttonRight = nullptr;
	material::Button *_buttonLeft = nullptr;
	material::FloatingActionButton *_backButton = nullptr;

	EpubNavigation *_navigation = nullptr;
	EpubContentsView *_contents = nullptr;
	material::SidebarLayout *_sidebar = nullptr;

	float _backButtonValue = nan();
	String _savedTitle;
	rich_text::Result::BoundIndex _currentBounds;

	float _tmpIconProgress = 0.0f;
	material::IconName _tmpIcon = material::IconName::None;
	std::function<void()> _tmpCallback;

	bool _extendedNavigation = true;
};

NS_MD_END

#endif /* RICH_TEXT_EPUB_RTEPUBVIEW_H_ */
