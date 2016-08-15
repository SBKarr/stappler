/*
 * SPScrollViewController.h
 *
 *  Created on: 26 марта 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_COMPONENTS_SCROLL_SPSCROLLCONTROLLER_H_
#define LIBS_STAPPLER_COMPONENTS_SCROLL_SPSCROLLCONTROLLER_H_

#include "SPDefine.h"
#include "2d/CCComponent.h"
#include "base/CCVector.h"

NS_SP_BEGIN

class ScrollController : public cocos2d::Component {
public:
	struct Item;

	/// Callback for node creation
	using NodeFunction = std::function<cocos2d::Node *(const Item &)>;

	struct Item {
		Item(const NodeFunction &, const Vec2 &pos, const Size &size, int zIndex);

		NodeFunction nodeFunction = nullptr;
		Size size;
		Vec2 pos;
		int zIndex;

		cocos2d::Node *node = nullptr;
	};

public:
	virtual ~ScrollController();

	virtual void onAdded() override;
	virtual void onRemoved() override;
	virtual void onContentSizeDirty() override;

	/// Scroll view callbacks handlers
	virtual void onScrollPosition();
	virtual void onScroll(float delta, bool eneded);
	virtual void onOverscroll(float delta);

	virtual float getScrollMin();
	virtual float getScrollMax();

	virtual cocos2d::Node *getRoot() const;
	virtual ScrollViewBase *getScroll() const;

	virtual void clear(); /// remove all items and non-strict barriers
	virtual void update(float position, float size); /// update current scroll position and size
	virtual void reset(float position, float size); /// set new scroll position and size

	/// Scrollable area size and offset are strict limits of scrollable area
	/// It's usefull when scroll parameters (offset, size, items positions) are known
	/// If scroll parameters are dynamic or determined in runtime - use barriers

	virtual void setScrollableAreaOffset(float value);
	virtual float getScrollableAreaOffset() const; // NaN by default

	virtual void setScrollableAreaSize(float value);
	virtual float getScrollableAreaSize() const; // NaN by default

	/// you should return true if this call successfully rebuilds visible objects
	virtual bool rebuildObjects(bool force = false);

	float getNextPosition() const { return _nextPosition; }
	float getNextSize() const { return _nextSize; }

	virtual size_t size() const;
	virtual size_t addItem(const NodeFunction &, const Size &size, const Vec2 &pos, int zIndex = 0);
	virtual size_t addItem(const NodeFunction &, float size, float pos, int zIndex = 0);
	virtual size_t addItem(const NodeFunction &, float size, int zIndex = 0);

	virtual Item *getItem(size_t);
	virtual Item *getItem(cocos2d::Node *);

	virtual size_t getItemIndex(cocos2d::Node *);

	virtual void removeItem(size_t);

	virtual void setScrollRelativeValue(float value);

	virtual cocos2d::Node * getFrontNode() const;
	virtual cocos2d::Node * getBackNode() const;
	virtual cocos2d::Vector<cocos2d::Node *> getNodes();

protected:
	virtual void onNextObject(Item &); /// insert new object at specified position

	virtual void addScrollNode(Item &);
	virtual void updateScrollNode(Item &);
	virtual void removeScrollNode(Item &);

	ScrollViewBase *_scroll = nullptr;
	cocos2d::Node *_root = nullptr;

	float _scrollAreaOffset = 0.0f;
	float _scrollAreaSize = 0.0f;

	float _currentMin = 0.0f;
	float _currentMax = 0.0f;

	float _nextPosition = 0.0f;
	float _nextSize = 0.0f;

	float _currentPosition = 0.0f;
	float _currentSize = 0.0f;

	std::vector<Item> _nodes;

	bool _infoDirty = true;
};

NS_SP_END

#endif /* LIBS_STAPPLER_COMPONENTS_SCROLL_SPSCROLLCONTROLLER_H_ */
