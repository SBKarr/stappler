/*
 * MaterialScrollSlice.h
 *
 *  Created on: 26 авг. 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_SCROLL_MATERIALSCROLLSLICE_H_
#define LIBS_MATERIAL_NODES_SCROLL_MATERIALSCROLLSLICE_H_

#include "MaterialScroll.h"
#include "base/CCMap.h"

NS_MD_BEGIN

class ScrollHandlerSlice : public Scroll::Handler {
public:
	using DataCallback = std::function<Rc<Scroll::Item> (ScrollHandlerSlice *h, data::Value &&, const Vec2 &)>;

	virtual ~ScrollHandlerSlice() { }
	virtual bool init(Scroll *, const DataCallback & = nullptr);
	virtual void setDataCallback(const DataCallback &);

	virtual Scroll::ItemMap run(Request, DataMap &&) override;

protected:
	virtual cocos2d::Vec2 getOrigin(Request) const;
	virtual Rc<Scroll::Item> onItem(data::Value &&, const Vec2 &);

	Vec2 _originFront;
	Vec2 _originBack;
	DataCallback _dataCallback;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_SCROLL_MATERIALSCROLLSLICE_H_ */
