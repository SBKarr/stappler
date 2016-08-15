/*
 * MaterialScrollFixed.cpp
 *
 *  Created on: 24 мая 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialScrollFixed.h"
#include "SPScrollController.h"

NS_MD_BEGIN

bool ScrollHandlerFixed::init(Scroll *s, float size) {
	if (!Handler::init(s)) {
		return false;
	}

	_dataSize = size;

	return true;
}

Scroll::ItemMap ScrollHandlerFixed::run(Request t, DataMap &&data) {
	Scroll::ItemMap ret;
	Size size = (_layout == Scroll::Layout::Vertical)?Size(_size.width, _dataSize):Size(_dataSize, _size.height);
	for (auto &it : data) {
		Vec2 origin = (_layout == Scroll::Layout::Vertical)
				?Vec2(0.0f, it.first.get() * _dataSize)
				:Vec2(it.first.get() * _dataSize, 0.0f);

		auto item = Rc<Scroll::Item>::create(std::move(it.second), origin, size);
		ret.insert(std::make_pair(it.first, item));
	}
	return ret;
}

NS_MD_END
