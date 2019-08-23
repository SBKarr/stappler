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

#include "SLTable.h"

NS_LAYOUT_BEGIN

static void Table_Borders_fillExternalBorder(Table::Borders &b, const BorderParams &params) {
	b.solidBorder = true;
	for (size_t i = 0; i < b.numRows; ++ i) {
		b.vertical.at(i * (b.numCols + 1)) = params;
		b.vertical.at((i + 1) * (b.numCols + 1) - 1) = params;
	}
	for (size_t i = 0; i < b.numCols; ++ i) {
		b.horizontal.at(i) = params;
		b.horizontal.at(i + b.numCols * b.numRows) = params;
	}
}

static void Table_Borders_fillExternalBorder(Table::Borders &b, const BorderParams &left, const BorderParams &top, const BorderParams &right, const BorderParams &bottom) {
	b.solidBorder = true;
	for (size_t i = 0; i < b.numRows; ++ i) {
		b.vertical.at(i * (b.numCols + 1)) = left;
		b.vertical.at((i + 1) * (b.numCols + 1) - 1) = right;
	}
	for (size_t i = 0; i < b.numCols; ++ i) {
		b.horizontal.at(i) = top;
		b.horizontal.at(i + b.numCols * b.numRows) = bottom;
	}
}

static void Table_Borders_fillCellBorder(Table::Borders &b, const Table::Cell &cell) {
	auto &media = cell.info->layout->builder->getMedia();
	auto style = cell.info->layout->node.style->compileOutline(cell.info->layout->builder);

	if (style.top.style != style::BorderStyle::None) {
		auto top = BorderParams{style.top.style, style.top.width.computeValueAuto(b.size.width, media), style.top.color};
		if (top.isVisible()) {
			for (size_t i = 0; i < cell.colSpan; ++ i) {
				if (b.horizontal.at(cell.rowIndex * b.numCols + cell.colIndex + i).merge(top)) {
					if (cell.rowIndex == 0) {
						b.solidBorder = false;
					}
				}
			}
		}
	}
	if (style.right.style != style::BorderStyle::None) {
		auto right = BorderParams{style.right.style, style.right.width.computeValueAuto(b.size.width, media), style.right.color};
		if (right.isVisible()) {
			for (size_t i = 0; i < cell.rowSpan; ++ i) {
				if (b.vertical.at((cell.rowIndex + i) * (b.numCols + 1) + cell.colIndex + cell.colSpan).merge(right)) {
					if (cell.colIndex + cell.colSpan > b.numCols) {
						b.solidBorder = false;
					}
				}
			}
		}
	}
	if (style.bottom.style != style::BorderStyle::None) {
		auto bottom = BorderParams{style.bottom.style, style.bottom.width.computeValueAuto(b.size.width, media), style.bottom.color};
		if (bottom.isVisible()) {
			for (size_t i = 0; i < cell.colSpan; ++ i) {
				if (b.horizontal.at((cell.rowIndex + cell.rowSpan) * b.numCols + cell.colIndex + i).merge(bottom)) {
					if (cell.rowIndex + cell.rowSpan > b.numRows) {
						b.solidBorder = false;
					}
				}
			}
		}
	}
	if (style.left.style != style::BorderStyle::None) {
		auto left = BorderParams{style.left.style, style.left.width.computeValueAuto(b.size.width, media), style.left.color};
		if (left.isVisible()) {
			for (size_t i = 0; i < cell.rowSpan; ++ i) {
				if (b.vertical.at((cell.rowIndex + i) * (b.numCols + 1) + cell.colIndex).merge(left)) {
					if (cell.colIndex == 0) {
						b.solidBorder = false;
					}
				}
			}
		}
	}
}

Table::Borders::Borders(Table &table) {
	numRows = table.rows.size();
	numCols = table.cols.size();

	horizontal.resize(numCols * (numRows + 1));
	vertical.resize((numCols + 1) * numRows);

	size = table.layout->pos.size;
	auto &media = table.layout->builder->getMedia();
	auto style = table.layout->node.style->compileOutline(table.layout->builder);

	BorderParams left, top, right, bottom;
	if (style.top.style != style::BorderStyle::None) {
		top = BorderParams{style.top.style, style.top.width.computeValueAuto(size.width, media), style.top.color};
	}
	if (style.right.style != style::BorderStyle::None) {
		right = BorderParams{style.right.style, style.right.width.computeValueAuto(size.width, media), style.right.color};
	}
	if (style.bottom.style != style::BorderStyle::None) {
		bottom = BorderParams{style.bottom.style, style.bottom.width.computeValueAuto(size.width, media), style.bottom.color};
	}
	if (style.left.style != style::BorderStyle::None) {
		left = BorderParams{style.left.style, style.left.width.computeValueAuto(size.width, media), style.left.color};
	}

	if (left.compare(right) && top.compare(bottom) && left.compare(top)) {
		Table_Borders_fillExternalBorder(*this, left);
	} else {
		Table_Borders_fillExternalBorder(*this, left, top, right, bottom);
	}

	for (auto &it : table.cells) {
		Table_Borders_fillCellBorder(*this, it);
	}
}

static float Border_getLeftHorz(const Table::Borders &border, size_t row, size_t col) {
	if (col > 0) {
		auto b = border.getHorz(row, col - 1);
		if (b.isVisible()) { return b.width; }
	}
	return 0.0f;
}

static float Border_getRightHorz(const Table::Borders &border, size_t row, size_t col) {
	if (col < border.numCols - 1) {
		auto b = border.getHorz(row, col + 1);
		if (b.isVisible()) { return b.width; }
	}
	return 0.0f;
}

static float Border_getLeftTopVert(const Table::Borders &border, size_t row, size_t col) {
	if (row > 0) {
		auto b = border.getVert(row - 1, col);
		if (b.isVisible()) { return b.width; }
	}
	return 0.0f;
}

static float Border_getRightTopVert(const Table::Borders &border, size_t row, size_t col) {
	if (row > 0) {
		auto b = border.getVert(row - 1, col + 1);
		if (b.isVisible()) { return b.width; }
	}
	return 0.0f;
}

static float Border_getLeftBottomVert(const Table::Borders &border, size_t row, size_t col) {
	if (row < border.numRows) {
		auto b = border.getVert(row, col);
		if (b.isVisible()) { return b.width; }
	}
	return 0.0f;
}

static float Border_getRightBottomVert(const Table::Borders &border, size_t row, size_t col) {
	if (row < border.numRows) {
		auto b = border.getVert(row, col + 1);
		if (b.isVisible()) { return b.width; }
	}
	return 0.0f;
}

static float Border_getTopVert(const Table::Borders &border, size_t row, size_t col) {
	if (row > 0) {
		auto b = border.getVert(row - 1, col);
		if (b.isVisible()) { return b.width; }
	}
	return 0.0f;
}

static float Border_getBottomVert(const Table::Borders &border, size_t row, size_t col) {
	if (row < border.numRows - 1) {
		auto b = border.getVert(row + 1, col);
		if (b.isVisible()) { return b.width; }
	}
	return 0.0f;
}

static float Border_getTopLeftHorz(const Table::Borders &border, size_t row, size_t col) {
	if (col > 0) {
		auto b = border.getHorz(row, col - 1);
		if (b.isVisible()) { return b.width; }
	}
	return 0.0f;
}

static float Border_getBottomLeftHorz(const Table::Borders &border, size_t row, size_t col) {
	if (col > 0) {
		auto b = border.getHorz(row + 1, col - 1);
		if (b.isVisible()) { return b.width; }
	}
	return 0.0f;
}

static float Border_getTopRightHorz(const Table::Borders &border, size_t row, size_t col) {
	if (col < border.numCols) {
		auto b = border.getHorz(row, col);
		if (b.isVisible()) { return b.width; }
	}
	return 0.0f;
}

static float Border_getBottomRightHorz(const Table::Borders &border, size_t row, size_t col) {
	if (col < border.numCols) {
		auto b = border.getHorz(row + 1, col);
		if (b.isVisible()) { return b.width; }
	}
	return 0.0f;
}


void Table::Borders::make(Table &table, Result *res) {
	auto pos = table.layout->pos.position;
	for (size_t i = 0; i < horizontal.size(); ++ i) {
		size_t row = i / numCols;
		size_t col = i % numCols;

		Vec2 origin = pos;
		if (row != 0) {
			origin.y += table.rows[row - 1].yPos + table.rows[row - 1].height;
		}
		origin.x += table.cols[col].xPos;

		auto &border = horizontal[i];
		if (horizontal[i].isVisible()) {
			auto path = res->emplacePath(*table.layout);
			path->depth = table.layout->depth + 5;
			path->drawHorizontalLineSegment(origin, table.cols[col].width, border.color, border.width, border.style,
				Border_getLeftBottomVert(*this, row, col), Border_getLeftHorz(*this, row, col), Border_getLeftTopVert(*this, row, col),
				Border_getRightTopVert(*this, row, col), Border_getRightHorz(*this, row, col), Border_getRightBottomVert(*this, row, col));
		}
	}

	for (size_t i = 0; i < vertical.size(); ++ i) {
		size_t row = i / (numCols + 1);
		size_t col = i % (numCols + 1);

		Vec2 origin = pos;
		if (col != 0) {
			origin.x += table.cols[col - 1].xPos + table.cols[col - 1].width;
		}
		origin.y += table.rows[row].yPos;

		auto &border = vertical[i];
		if (vertical[i].isVisible()) {
			auto path = res->emplacePath(*table.layout);
			path->depth = table.layout->depth + 5;
			path->drawVerticalLineSegment(origin, table.rows[row].height, border.color, border.width, border.style,
				Border_getTopLeftHorz(*this, row, col), Border_getTopVert(*this, row, col), Border_getTopRightHorz(*this, row, col),
				Border_getBottomRightHorz(*this, row, col), Border_getBottomVert(*this, row, col), Border_getBottomLeftHorz(*this, row, col));
		}
	}
}

BorderParams Table::Borders::getHorz(size_t row, size_t col) const {
	if (col < numCols && row <= numCols) {
		return horizontal[row * numCols + col];
	}
	return BorderParams();
}

BorderParams Table::Borders::getVert(size_t row, size_t col) const {
	if (col <= numCols && row < numCols) {
		return vertical[row * (numCols + 1) + col];
	}
	return BorderParams();
}

void Table::processTableBackground() {
	auto flushRect = [this] (Layout &l, const Rect &rect, const Color4B &color) {
		if (rect.size.width > 0.0f && rect.size.height > 0.0f) {
			auto path = layout->builder->getResult()->emplacePath(*layout);
			path->depth = layout->depth + 2;
			path->drawRect(rect, color);
			l.objects.emplace_back(path);
		}
	};

	for (auto &col : cols) {
		if (auto style = col.info->node.style) {
			auto bg = style->compileBackground(layout->builder);
			if (bg.backgroundColor.a != 0 && col.width > 0 && layout->pos.size.height > 0) {
				float yPos = 0.0f;
				float height = 0.0f;
				size_t rowIdx = 0;
				for (auto &cell : col.cells) {
					if (cell->rowIndex != rowIdx) {
						++ rowIdx;
						continue;
					}
					auto &pos = cell->info->layout->pos;
					auto rect = pos.getInsideBoundingBox();
					if (cell->colIndex == col.index) {
						if (cell->colSpan == 1) {
							height += rect.size.height;
						} else {
							flushRect(*layout, Rect(col.xPos, yPos, col.width, height), bg.backgroundColor);
							flushRect(*cell->info->layout, rect, bg.backgroundColor);
							yPos = height + rect.size.height;
							height = 0.0f;
						}
					} else {
						flushRect(*layout, Rect(col.xPos, yPos, col.width, height), bg.backgroundColor);
						yPos = height + rect.size.height;
						height = 0.0f;
					}
					++ rowIdx;
				}
				flushRect(*layout, Rect(col.xPos, yPos, col.width, height), bg.backgroundColor);
			}
		}
	}

	for (auto &row : rows) {
		if (auto style = row.style) {
			auto bg = style->compileBackground(layout->builder);
			if (bg.backgroundColor.a != 0 && row.height > 0 && layout->pos.size.width > 0) {
				float xPos = 0.0f;
				float bgWidth = 0.0f;
				size_t colIdx = 0;
				for (auto &cell : row.cells) {
					if (!cell) {
						continue;
					}
					if (cell->colIndex != colIdx) {
						++ colIdx;
						continue;
					}
					auto &pos = cell->info->layout->pos;
					auto rect = pos.getInsideBoundingBox();
					if (cell->rowIndex == row.index) {
						if (cell->rowSpan == 1) {
							bgWidth += rect.size.width;
						} else {
							flushRect(*layout, Rect(xPos, row.yPos, bgWidth, row.height), bg.backgroundColor);
							flushRect(*cell->info->layout, rect, bg.backgroundColor);
							xPos = bgWidth + rect.size.width;
							bgWidth = 0.0f;
						}
					} else {
						flushRect(*layout, Rect(xPos, row.yPos, bgWidth, row.height), bg.backgroundColor);
						xPos = bgWidth + rect.size.width;
						bgWidth = 0.0f;
					}
					++ colIdx;
				}
				flushRect(*layout, Rect(xPos, row.yPos, bgWidth, row.height), bg.backgroundColor);
			}
		}
	}

	if (layout->node.block.borderCollapse == style::BorderCollapse::Collapse) {
		Borders borders(*this);
		borders.make(*this, layout->builder->getResult());
	}

	if (widthScale < 1.0f) {
		String caption;
		if (captionLayout) {
			for (auto &it : captionLayout->objects) {
				if (it->type == Object::Type::Label) {
					caption = string::toUtf8(it->asLabel()->format.str());
				}
			}
		}

		auto href = caption.empty()
				? toString("#", layout->node.node->getHtmlId(), "?min=", size_t(ceilf(origMinWidth)), "&max=", size_t(ceilf(origMaxWidth)))
				: toString("#", layout->node.node->getHtmlId(), "?min=", size_t(ceilf(origMinWidth)), "&max=", size_t(ceilf(origMaxWidth)), "&caption=", caption);
		auto target = "table";
		auto res = layout->builder->getResult();
		if (!href.empty()) {
			layout->objects.emplace_back(res->emplaceLink(*layout,
				Rect(-layout->pos.padding.left, -layout->pos.padding.top,
					layout->pos.size.width + layout->pos.padding.left + layout->pos.padding.right,
					layout->pos.size.height + layout->pos.padding.top + layout->pos.padding.bottom),
					href, target));
		}

		layout->processRef();
	}
}

void Table::processCaption(style::CaptionSide side) {
	layout->builder->pushNode(caption);
	Vec2 parentPos = layout->pos.position;
	if (side == style::CaptionSide::Bottom) {
		parentPos.y += layout->pos.size.height;
	}

	if (captionLayout->init(parentPos, layout->pos.size, 0.0f)) {
		captionLayout->node.context = style::Display::TableCaption;

		layout->builder->processChilds(*captionLayout, *captionLayout->node.node);
		captionLayout->finalize(parentPos, 0.0f);

		layout->layouts.emplace_back(captionLayout);

		float nodeHeight =  captionLayout->getBoundingBox().size.height;
		if (side == style::CaptionSide::Top) {
			layout->pos.margin.top += nodeHeight;
			layout->pos.position.y += nodeHeight;
		} else {
			layout->pos.margin.bottom += nodeHeight;
		}
	}
	layout->builder->popNode();
}

NS_LAYOUT_END
