/*
 * MaterialSidebarLayout.h
 *
 *  Created on: 09 сент. 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_LAYOUT_MATERIALSIDEBARLAYOUT_H_
#define LIBS_MATERIAL_NODES_LAYOUT_MATERIALSIDEBARLAYOUT_H_

#include "MaterialLayout.h"

NS_MD_BEGIN

class SidebarLayout : public Layout {
public:
	enum Position {
		Left,
		Right
	};

	using WidthCallback = std::function<float(const cocos2d::Size &)>;
	using BoolCallback = std::function<void(bool)>;

	virtual bool init(Position);
	virtual void onContentSizeDirty() override;

	virtual void setNode(MaterialNode *m, int zOrder = 0);
	virtual MaterialNode *getNode() const;

	virtual void setNodeWidth(float);
	virtual float getNodeWidth() const;

	virtual void setNodeWidthCallback(const WidthCallback &);
	virtual const WidthCallback &getNodeWidthCallback() const;

	virtual void setSwallowTouches(bool);
	virtual bool isSwallowTouches() const;

	virtual void setEdgeSwipeEnabled(bool);
	virtual bool isEdgeSwipeEnabled() const;

	virtual void setBackgroundColor(const Color &);
	virtual void setBackgroundActiveOpacity(uint8_t);
	virtual void setBackgroundPassiveOpacity(uint8_t);

	virtual void show();
	virtual void hide(float factor = 1.0f);

	virtual float getProgress() const;

	virtual bool isNodeVisible() const;
	virtual bool isNodeEnabled() const;

	virtual void setNodeVisibleCallback(const BoolCallback &);
	virtual void setNodeEnabledCallback(const BoolCallback &);

protected:
	virtual void setProgress(float);

	virtual void onSwipeDelta(float);
	virtual void onSwipeFinished(float);

	virtual void onNodeEnabled(bool value);
	virtual void onNodeVisible(bool value);

	virtual void stopNodeActions();

	WidthCallback _widthCallback;
	float _nodeWidth = 0.0f;

	Position _position;
	bool _swallowTouches = true;
	bool _edgeSwipeEnabled = true;
	uint8_t _backgroundActiveOpacity = 64;
	uint8_t _backgroundPassiveOpacity = 0;

	gesture::Listener *_listener;

	Layer *_background = nullptr;
	MaterialNode *_node = nullptr;

	BoolCallback _visibleCallback = nullptr;
	BoolCallback _enabledCallback = nullptr;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_LAYOUT_MATERIALSIDEBARLAYOUT_H_ */
