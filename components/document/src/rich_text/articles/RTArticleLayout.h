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

#ifndef RICH_TEXT_ARTICLE_RTARTICLELAYOUT_H_
#define RICH_TEXT_ARTICLE_RTARTICLELAYOUT_H_

#include "RTView.h"
#include "MaterialToolbarLayout.h"
#include "MaterialMenuSource.h"

NS_RT_BEGIN

class ArticleLayout : public material::ToolbarLayout {
public:
	enum class AssetFlags {
		None = 0,
		Open = 1 << 0,
		Read = 1 << 1,
		Offset = 1 << 2,
	};

	static EventHeader onOpened;
	static EventHeader onClosed;

	static EventHeader onImageLink;
	static EventHeader onExternalLink;
	static EventHeader onContentLink;

	class ArticleLoader;

	using ArticleCallback = Function<void(size_t num)>;
	using AssetCallback = Function<void(const ArticleLoader *, const Function<void(Asset *)> &)>;

	class ArticleLoader : public Ref {
	public:
		virtual bool init(size_t);
		virtual void setAsset(Asset *a);
		virtual void setCallback(const AssetCallback &);

		virtual Rc<Source> constructSource() const;
		virtual Rc<View> constructView() const;

		virtual size_t getNumber() const;

	protected:
		Rc<Asset> asset;
		AssetCallback callback;
		size_t number = 0;
	};

	virtual ~ArticleLayout();

	virtual bool init(size_t num, size_t count, font::HyphenMap * = nullptr);
	virtual bool init(Asset *a, font::HyphenMap * = nullptr);

	virtual void loadArticle(size_t num, Asset *a, const AssetCallback &cb = nullptr);
	virtual void loadArticle(const ArticleLoader *);

	virtual void setCount(size_t);
	virtual void setCloseCallback(const Function<void()> &);

	virtual void onEnter() override;
	virtual void onExit() override;
	virtual void onContentSizeDirty() override;

	virtual bool onBackButton() override;
	virtual void onScroll(float delta, bool finished) override;

	virtual void setArticleCallback(const ArticleCallback &);
	virtual const ArticleCallback &getArticleCallback() const;

	virtual void setAssetCallback(const AssetCallback &);
	virtual const AssetCallback &getAssetCallback() const;

protected:
	virtual void onDocument(View *);

	virtual void updateAssetInfo(SourceAsset *, AssetFlags flags, float pos);
	virtual void updateActionsMenu();

	virtual void onSidebar(material::SidebarLayout *);

	virtual Rc<ArticleLoader> constructLoader(size_t number);
	virtual Rc<View> loadView(const ArticleLoader *loader);

	virtual void onTap();

	virtual void onCopyButton(const String &);
	virtual void onBookmarkButton();
	virtual void onMisprintButton();

	virtual void onSelection(View *view, bool enabled);

	virtual void applyView(View *);
	virtual void applyViewProgress(View *, View *, float);

	void onSwipeProgress();
	void setSwipeProgress(const Vec2 &delta);
	void endSwipeProgress(const Vec2 &delta, const Vec2 &velocity);
	void onSwipeAction(cocos2d::Node *node);

	void setNextRichTextView(size_t id);
	void setPrevRichTextView(size_t id);
	virtual void onViewSelected(View *, size_t);

	void showSelectionToolbar();
	void hideSelectionToolbar();

	bool _documentOpened = false;

	float _currentPos = 0.0f;
	float _swipeProgress = 0.0f;

	Rc<font::HyphenMap> _hyphens;
	View *_richTextView = nullptr;
	View *_nextRichText = nullptr;
	View *_prevRichText = nullptr;

	material::SidebarLayout *_sidebar = nullptr;

	std::function<void()> _cancelCallback = nullptr;

	uint64_t _currentNumber = 0;
	uint64_t _count = 0;

	AssetCallback _assetCallback;
	ArticleCallback _articleCallback;

	float _tmpIconProgress = 0.0f;
	std::function<void()> _tmpCallback;

	Rc<material::MenuSource> _viewSource;
	size_t _viewSourceCount = 3;

	Rc<material::MenuSource> _selectionSource;
	size_t _selectionSourceCount = 2;
};

SP_DEFINE_ENUM_AS_MASK(ArticleLayout::AssetFlags);

NS_RT_END

#endif /* RICH_TEXT_ARTICLE_RTARTICLELAYOUT_H_ */
