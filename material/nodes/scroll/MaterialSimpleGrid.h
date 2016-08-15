/*
 * MaterialSimpleGrid.h
 *
 *  Created on: 29 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_SCROLL_MATERIALSIMPLEGRID_H_
#define LIBS_MATERIAL_NODES_SCROLL_MATERIALSIMPLEGRID_H_

#include "MaterialScroll.h"

NS_MD_BEGIN

class ScrollHandlerGrid : public Scroll::Handler {
public:
	virtual ~ScrollHandlerGrid() { }
	virtual bool init(Scroll *) override;
	virtual bool init(Scroll *, const Padding &p);
	virtual Scroll::ItemMap run(Request, DataMap &&) override;

	virtual void setCellMinWidth(float v);
	virtual void setCellAspectRatio(float v);

	virtual void setAutoPaddings(bool);
	virtual bool isAutoPaddings() const;

protected:
	virtual Rc<Scroll::Item> onItem(data::Value &&, Scroll::Source::Id);

	bool _autoPaddings = false;
	float _currentWidth = 0.0f;

	float _cellAspectRatio = 1.0f;
	float _cellMinWidth = 1.0f;

	float _widthPadding = 0.0f;
	Padding _padding;
	cocos2d::Size _currentCellSize;
	uint32_t _currentCols = 0;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_SCROLL_MATERIALSIMPLEGRID_H_ */
