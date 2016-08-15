/*
 * MaterialSimpleGrid.cpp
 *
 *  Created on: 29 мая 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialSimpleGrid.h"
#include "SPScrollController.h"

NS_MD_BEGIN

bool ScrollHandlerGrid::init(Scroll *s) {
	if (!Handler::init(s)) {
		return false;
	}

	_padding = s->getPadding();

	return true;
}

bool ScrollHandlerGrid::init(Scroll *s, const Padding &p) {
	if (!Handler::init(s)) {
		return false;
	}

	_padding = p;

	return true;
}

Scroll::ItemMap ScrollHandlerGrid::run(Request t, DataMap &&data) {
	Scroll::ItemMap ret;
	auto size = _size;
	size.width -= (_padding.left + _padding.right);

	uint32_t cols = (uint32_t)floorf(size.width / _cellMinWidth);
	if (cols == 0) {
		cols = 1;
	}
	auto cellWidth = (_autoPaddings?(MIN(_cellMinWidth, size.width / cols)):(size.width / cols));
	auto cellHeight = cellWidth / _cellAspectRatio;

	_currentCellSize = cocos2d::Size(cellWidth, cellHeight);
	_currentCols = (uint32_t)cols;
	_widthPadding = (_size.width - _currentCellSize.width * _currentCols) / 2.0f;

	for (auto &it : data) {
		auto item = onItem(std::move(it.second), it.first);
		ret.insert(std::make_pair(it.first, item));
	}
	return ret;
}

void ScrollHandlerGrid::setCellMinWidth(float v) {
	_cellMinWidth = v;
}

void ScrollHandlerGrid::setCellAspectRatio(float v) {
	_cellAspectRatio = v;
}

void ScrollHandlerGrid::setAutoPaddings(bool value) {
	_autoPaddings = value;
}

bool ScrollHandlerGrid::isAutoPaddings() const {
	return _autoPaddings;
}

Rc<Scroll::Item> ScrollHandlerGrid::onItem(data::Value &&data, Scroll::Source::Id id) {
	Scroll::Source::Id::Type row = id.get() / _currentCols;
	Scroll::Source::Id::Type col = id.get() % _currentCols;

	Vec2 pos(col * _currentCellSize.width + _widthPadding, row * _currentCellSize.height);

	return Rc<Scroll::Item>::create(std::move(data), pos, _currentCellSize);
}

NS_MD_END
