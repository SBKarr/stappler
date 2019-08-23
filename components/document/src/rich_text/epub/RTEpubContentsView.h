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

#ifndef LIBS_LIBRARY_RICH_TEXT_EPUB_RTEPUBCONTENTSVIEW_H_
#define LIBS_LIBRARY_RICH_TEXT_EPUB_RTEPUBCONTENTSVIEW_H_

#include "RTCommon.h"
#include "MaterialNode.h"
#include "MaterialRecyclerScroll.h"

NS_RT_BEGIN

class EpubContentsNode : public material::MaterialNode {
public:
	using Callback = std::function<void(const String &)>;

	static Size compute(const data::Value &data, float width);
	virtual bool init(const data::Value &data, Callback & cb);
	virtual void onContentSizeDirty() override;

protected:
	bool _selected = false;
	bool _available = true;

	String _href;
	Callback _callback = nullptr;
	material::Label *_label = nullptr;
	material::Button *_button = nullptr;
};

class EpubContentsScroll : public material::Scroll {
public:
	using Callback = std::function<void(const String &)>;

	virtual bool init(const Callback &cb);
	virtual void onResult(layout::Result *res);
	virtual void setSelectedPosition(float value);

protected:
	Rc<layout::Result> _result;
	float _maxPos = nan();
	Callback _callback;
};

class EpubBookmarkNode : public material::MaterialNode {
public:
	using Callback = std::function<void(size_t object, float position, float scroll)>;

	static Size compute(const data::Value &data, float width);
	virtual bool init(const data::Value &data, const Size &size, const Callback &cb);
	virtual void onContentSizeDirty() override;

protected:
	virtual void onButton();

	size_t _object = 0;
	size_t _position = 0;
	float _scroll = nan();

	Callback _callback;
	material::Label *_name = nullptr;
	material::Label *_label = nullptr;
	material::Button *_button = nullptr;
};

class EpubBookmarkScroll : public material::RecyclerScroll {
public:
	using Callback = std::function<void(size_t object, float position, float scroll)>;

	virtual bool init(const Callback &cb);

protected:
	Callback _callback;
	data::Value _data;
};

class EpubContentsView : public material::MaterialNode {
public:
	using Callback = std::function<void(const String &)>;
	using BookmarkCallback = std::function<void(size_t object, size_t position, float scroll)>;

	virtual bool init(const Callback &, const BookmarkCallback &);
	virtual void onContentSizeDirty() override;

	virtual void onResult(layout::Result *);

	virtual void setSelectedPosition(float);
	virtual void setColors(const material::Color &bar, const material::Color &accent);

	virtual void setBookmarksSource(data::Source *);

	virtual void setBookmarksEnabled(bool);
	virtual bool isBookmarksEnabled() const;

protected:
	virtual void showBookmarks();
	virtual void showContents();

	virtual Rc<material::TabBar> constructTabBar();
	virtual Rc<EpubContentsScroll> constructContentsScroll(const Callback &cb);
	virtual Rc<EpubBookmarkScroll> constructBookmarkScroll(const BookmarkCallback &cb);

	bool _bookmarksEnabled = true;
	StrictNode *_contentNode = nullptr;
	material::TabBar *_tabBar = nullptr;
	material::Label *_title = nullptr;
	material::MaterialNode *_header = nullptr;
	EpubContentsScroll *_contentsScroll = nullptr;
	EpubBookmarkScroll *_bookmarksScroll = nullptr;
	float _progress = 0.0f;
};

NS_RT_END

#endif /* LIBS_LIBRARY_RICH_TEXT_EPUB_RTEPUBCONTENTSVIEW_H_ */
