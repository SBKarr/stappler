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

#ifndef RICH_TEXT_COMMON_RTCOMMONTVIEW_H_
#define RICH_TEXT_COMMON_RTCOMMONTVIEW_H_

#include "RTCommonSource.h"
#include "SPScrollView.h"

NS_RT_BEGIN

class CommonView : public ScrollView {
public:
	using ResultCallback = Function<void(Result *)>;

	class Page : public cocos2d::Node {
	public:
		virtual bool init(const PageData &, float);
		virtual void setTexture(cocos2d::Texture2D *tex);

		virtual void setBackgroundColor(const Color3B &);
		virtual Vec2 convertToObjectSpace(const Vec2 &) const;

	protected:
		PageData _data;
		Layer *_background;
		DynamicSprite *_sprite;
	};

	virtual ~CommonView();

	virtual bool init(Layout = Vertical, CommonSource * = nullptr, const Vector<String> &ids = {});
	virtual void setLayout(Layout l) override;

    virtual void onContentSizeDirty() override;

	virtual void setSource(CommonSource *);
	virtual CommonSource *getSource() const;

	virtual Renderer *getRenderer() const;
	virtual Document *getDocument() const;
	virtual Result *getResult() const;

	virtual void refresh();

	virtual void setResultCallback(const ResultCallback &);
	virtual const ResultCallback &getResultCallback() const;

	virtual void setScrollRelativePosition(float value) override;

	virtual bool showNextPage();
	virtual bool showPrevPage();

	virtual float getObjectsOffset() const;

	virtual void setLinksEnabled(bool);
	virtual bool isLinksEnabled() const;

protected:
	virtual Vec2 convertToObjectSpace(const Vec2 &) const;
	virtual bool isObjectActive(const Object &) const;
	virtual bool isObjectTapped(const Vec2 &, const Object &) const;
	virtual void onObjectPressBegin(const Vec2 &, const Object &); // called with original world location
	virtual void onObjectPressEnd(const Vec2 &, const Object &); // called with original world location

	virtual void onSwipeBegin() override;

	virtual bool onPressBegin(const Vec2 &) override;
	virtual bool onPressEnd(const Vec2 &, const TimeInterval &) override;
	virtual bool onPressCancel(const Vec2 &, const TimeInterval &) override;

	virtual void onRenderer(Result *, bool);

	virtual void onPageData(Result *, float offset);
	virtual PageData getPageData(size_t) const;
	virtual Rc<cocos2d::Node> onPageNode(size_t);

	virtual Rc<Page> onConstructPageNode(const PageData &, float);

	virtual cocos2d::ActionInterval *onSwipeFinalizeAction(float velocity) override;

	virtual void onPosition() override;

	bool _linksEnabled = true;
	Margin _pageMargin;
	Renderer *_renderer = nullptr;
	Layer *_background = nullptr;

	float _density = 1.0f;
	Rc<CommonSource> _source;

	ResultCallback _callback;

	Vector<PageData> _pages;
	float _objectsOffset = 0.0f;
	float _gestureStart = nan();
};

NS_RT_END

#endif /* RICH_TEXT_COMMON_RTCOMMONTVIEW_H_ */
