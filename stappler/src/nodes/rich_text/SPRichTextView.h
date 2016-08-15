/*
 * SPRichTextView.h
 *
 *  Created on: 31 июля 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_NODES_RICH_TEXT_SPRICHTEXTVIEW_H_
#define LIBS_STAPPLER_NODES_RICH_TEXT_SPRICHTEXTVIEW_H_

#include "SPRichTextSource.h"
#include "SPScrollView.h"
#include "SPRichTextResult.h"

NS_SP_EXT_BEGIN(rich_text)

class View : public ScrollView {
public:
	using ResultCallback = std::function<void(Result *)>;
	using PageData = Result::PageData;

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

	virtual ~View();

	virtual bool init(Layout = Vertical, Source * = nullptr, const Vector<String> &ids = {});
	virtual void setLayout(Layout l) override;

    virtual void onContentSizeDirty() override;

    virtual void setHyphens(rich_text::HyphenMap *);

	virtual void setSource(Source *);
	virtual Source *getSource() const;

	virtual Renderer *getRenderer() const;
	virtual Document *getDocument() const;
	virtual Result *getResult() const;

	virtual void refresh();

	virtual void setResultCallback(const ResultCallback &);
	virtual const ResultCallback &getResultCallback() const;

	virtual void setScrollRelativePosition(float value) override;

	virtual bool showNextPage();
	virtual bool showPrevPage();

protected:
	virtual Vec2 convertToObjectSpace(const Vec2 &) const;
	virtual bool isObjectActive(const Object &) const;
	virtual bool isObjectTapped(const Vec2 &, const Object &) const;
	virtual void onObjectPressBegin(const Vec2 &, const Object &); // called with original world location
	virtual void onObjectPressEnd(const Vec2 &, const Object &); // called with original world location

	virtual bool onPressBegin(const Vec2 &) override;
	virtual bool onPressEnd(const Vec2 &, const TimeInterval &) override;
	virtual bool onPressCancel(const Vec2 &, const TimeInterval &) override;

	virtual void onRenderer(Result *, bool);

	virtual void onPageData(Result *, float offset);
	virtual PageData getPageData(size_t) const;
	virtual cocos2d::Node * onPageNode(size_t);

	virtual cocos2d::ActionInterval *onSwipeFinalizeAction(float velocity) override;

	Margin _pageMargin;
	Renderer *_renderer = nullptr;
	Layer *_background = nullptr;

	float _density = 1.0f;
	Rc<Source> _source;

	ResultCallback _callback;

	Vector<PageData> _pages;
	float _objectsOffset = 0.0f;
};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_NODES_RICH_TEXT_SPRICHTEXTVIEW_H_ */
