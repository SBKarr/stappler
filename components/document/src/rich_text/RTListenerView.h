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

#ifndef RICH_TEXT_RTLISTENERTVIEW_H_
#define RICH_TEXT_RTLISTENERTVIEW_H_

#include "RTCommonView.h"
#include "RTSource.h"
#include "SPEventHeader.h"
#include "SPDynamicBatchNode.h"

NS_RT_BEGIN

class ListenerView : public CommonView {
public:
	static EventHeader onSelection;
	static EventHeader onExternalLink;

	enum class SelectMode {
		Full,
		Para,
		Indexed
	};

	struct SelectionPosition {
		size_t object;
		uint32_t position;
	};

	class Selection : public DynamicBatchNode {
	public:
		virtual bool init(ListenerView *);

		virtual void clearSelection();
		virtual void selectLabel(const rich_text::Object *, const Vec2 &);
		virtual void selectWholeLabel();

		virtual bool onTap(int, const Vec2 &);

		virtual bool onPressBegin(const Vec2 &);
		virtual bool onLongPress(const Vec2 &, const TimeInterval &, int);
		virtual bool onPressEnd(const Vec2 &, const TimeInterval &);
		virtual bool onPressCancel(const Vec2 &);

		virtual bool onSwipeBegin(const Vec2 &);
		virtual bool onSwipe(const Vec2 &, const Vec2 &);
		virtual bool onSwipeEnd(const Vec2 &);

		virtual void setEnabled(bool);
		virtual bool isEnabled() const;

		virtual void setMode(SelectMode);
		virtual SelectMode getMode() const;

		virtual bool hasSelection() const;

		virtual String getSelectedString(size_t maxWords) const;
		virtual Pair<SelectionPosition, SelectionPosition> getSelectionPosition() const;

		virtual StringView getSelectedHash() const;
		virtual size_t getSelectedSourceIndex() const;

	protected:
		virtual const Object *getSelectedObject(Result *, const Vec2 &) const;
		virtual const Object *getSelectedObject(Result *, const Vec2 &, size_t pos, int32_t offset) const;

		virtual void emplaceRect(const Rect &, size_t idx, size_t count);
		virtual void updateRects();

		ListenerView *_view = nullptr;
		const Object *_object = nullptr;
		size_t _index = 0;
		Pair<SelectionPosition, SelectionPosition> _selectionBounds;

		draw::PathNode *_markerStart = nullptr;
		draw::PathNode *_markerEnd = nullptr;
		draw::PathNode *_markerTarget = nullptr;
		bool _enabled = false;
		SelectMode _mode = SelectMode::Full;
	};

	virtual ~ListenerView();
	virtual bool init(Layout, CommonSource * = nullptr, const Vector<String> &ids = {}) override;

	virtual void onExit() override;

	virtual void setLayout(Layout l) override;
	virtual void setUseSelection(bool);

	virtual void disableSelection();
	virtual bool isSelectionEnabled() const;

	virtual void setSelectMode(SelectMode);
	virtual SelectMode getSelectMode() const;

	virtual String getSelectedString(size_t maxWords = maxOf<size_t>()) const;
	virtual Pair<SelectionPosition, SelectionPosition> getSelectionPosition() const;

	virtual StringView getSelectedHash() const;
	virtual size_t getSelectedSourceIndex() const;

protected:
	virtual void onTap(int count, const Vec2 &loc) override;
	virtual void onLightLevelChanged();

	virtual void onObjectPressEnd(const Vec2 &, const rich_text::Object &) override;
	virtual void onLink(const StringView &ref, const StringView &target, const Vec2 &);

	virtual bool onSwipeEventBegin(const Vec2 &loc, const Vec2 &d, const Vec2 &v) override;
	virtual bool onSwipeEvent(const Vec2 &loc, const Vec2 &d, const Vec2 &v) override;
	virtual bool onSwipeEventEnd(const Vec2 &loc, const Vec2 &d, const Vec2 &v) override;

	virtual bool onPressBegin(const Vec2 &) override;
	virtual bool onLongPress(const Vec2 &, const TimeInterval &, int count) override;
	virtual bool onPressEnd(const Vec2 &, const TimeInterval &) override;
	virtual bool onPressCancel(const Vec2 &, const TimeInterval &) override;

	virtual void onPosition() override;
	virtual void onRenderer(rich_text::Result *, bool) override;

	bool _useSelection = false;

	EventListener *_eventListener = nullptr;
	Selection *_selection = nullptr;
};

NS_RT_END

#endif /* RICH_TEXT_RTLISTENERTVIEW_H_ */
