/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef MATERIAL_NODES_LAYOUT_MATERIALMULTIVIEWLAYOUT_H_
#define MATERIAL_NODES_LAYOUT_MATERIALMULTIVIEWLAYOUT_H_

#include "MaterialToolbarLayout.h"
#include "2d/CCComponent.h"

NS_MD_BEGIN

class MultiViewLayout : public ToolbarLayout {
public:
	class Generator : public cocos2d::Component {
	public:
		using IndexViewCallback = Function< Rc<ScrollView>(int64_t) >;
		using SequenceViewCallback = Function< Rc<ScrollView>(ScrollView *, int64_t viewIndex, int8_t offset) >;

		using ViewCallback = Function< void(ScrollView *, int64_t) >;
		using ProgressCallback = Function< void(ScrollView *, ScrollView *, float progress) >;

		virtual bool init();
		virtual bool init(size_t, const IndexViewCallback &);
		virtual bool init(const SequenceViewCallback &);

		virtual bool isViewLocked(ScrollView *, int64_t index);

		virtual Rc<ScrollView> makeIndexView(int64_t viewIndex);
		virtual Rc<ScrollView> makeNextView(ScrollView *, int64_t viewIndex);
		virtual Rc<ScrollView> makePrevView(ScrollView *, int64_t viewIndex);

		virtual bool isInfinite() const;
		virtual int64_t getViewCount() const;

		virtual void setViewSelectedCallback(const ViewCallback &);
		virtual void setViewCreatedCallback(const ViewCallback &);
		virtual void setApplyViewCallback(const ViewCallback &);
		virtual void setApplyProgressCallback(const ProgressCallback &);

		virtual void onViewSelected(ScrollView *current, int64_t id);
		virtual void onViewCreated(ScrollView *current, int64_t id);

		virtual void onApplyView(ScrollView *current, int64_t id);
		virtual void onApplyProgress(ScrollView *current, ScrollView *n, float progress);

	protected:
		MultiViewLayout *getLayout();

		size_t _viewCount = maxOf<size_t>();
		IndexViewCallback _makeViewByIndex;
		SequenceViewCallback _makeViewSeq;
		ViewCallback _viewSelectedCallback;
		ViewCallback _viewCreatedCallback;
		ViewCallback _applyViewCallback;
		ProgressCallback _applyProgressCallback;
	};

	virtual bool init(Generator * = nullptr, ToolbarBase * = nullptr);

	virtual void onPush(ContentLayer *l, bool replace) override;

	void setGenerator(Generator *);
	Generator *getGenerator() const;

	int64_t getCurrentIndex() const;

	void showNextView(float val = 0.35f);
	void showPrevView(float val = 0.35f);

	void showIndexView(int64_t, float val = 0.35f);

protected:
	virtual Rc<ScrollView> makeInitialView();

	virtual bool onSwipeProgress();
	virtual bool beginSwipe(const Vec2 &diff);
	virtual bool setSwipeProgress(const Vec2 &delta);
	virtual bool endSwipeProgress(const Vec2 &delta, const Vec2 &velocity);
	virtual void onSwipeAction(cocos2d::Node *node);

	virtual void setNextView(int64_t id, float newProgress = 0.0f);
	virtual void setPrevView(int64_t id, float newProgress = 0.0f);

	virtual void onViewSelected(ScrollView *current, int64_t id);

	virtual void onPrevViewCreated(ScrollView *current, ScrollView *prev);
	virtual void onNextViewCreated(ScrollView *current, ScrollView *next);

	virtual void applyView(ScrollView *current);
	virtual void applyViewProgress(ScrollView *current, ScrollView *n, float progress);

	float _currentPos = 0.0f;
	float _swipeProgress = 0.0f;

	int64_t _currentViewIndex = 0;
	ScrollView *_currentView = nullptr;
	ScrollView *_nextView = nullptr;
	ScrollView *_prevView = nullptr;

	gesture::Listener *_swipeListener = nullptr;
	Generator *_generator = nullptr;
};

NS_MD_END

#endif /* MATERIAL_NODES_LAYOUT_MATERIALMULTIVIEWLAYOUT_H_ */
