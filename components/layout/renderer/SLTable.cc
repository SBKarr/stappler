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

static uint16_t Cell_getColSpan(const Node &node) {
	if (auto colSpan = node.getAttribute("colspan")) {
		return min(uint16_t(32), uint16_t(StringView(*colSpan).readInteger().get(1)));
	}
	return 1;
}

static uint16_t Cell_getRowSpan(const Node &node) {
	if (auto colSpan = node.getAttribute("rowspan")) {
		return min(uint16_t(32), uint16_t(StringView(*colSpan).readInteger().get(1)));
	}
	return 1;
}

Table::Table(Layout *l, const Size &size, NodeId nodeId)
: parentSize(size), layout(l), phantomBody("tbody", String()) {
	phantomBody.setNodeId(nodeId);
}

void Table::addCol(const Node *group, const Node &node) {
	uint16_t colGroup = maxOf<uint16_t>();
	bool init = false;
	size_t idx = 0;
	for (auto &it : colGroups) {
		if (it.node == group) {
			init = true;
			colGroup = idx;
			break;
		}
		++ idx;
	}

	if (!init && group) {
		colGroups.emplace_back(ColGroup{uint16_t(colGroups.size()), group});
		colGroup = colGroups.size() - 1;
	}

	cols.emplace_back(Col{uint16_t(cols.size()), colGroup, &node});

	if (colGroup != maxOf<uint16_t>()) {
		colGroups[colGroup].cols.emplace_back(cols.size() - 1);
	}
}

void Table::addRow(const Node *group, const Node &node) {
	if (!group) {
		group = &phantomBody;
	}

	uint16_t rowGroup = maxOf<uint16_t>();
	bool init = false;
	size_t idx = 0;
	for (auto &it : rowGroups) {
		if (it.node == group) {
			init = true;
			rowGroup = idx;
			break;
		}
		++ idx;
	}

	if (!init) {
		rowGroups.emplace_back(RowGroup{uint16_t(rowGroups.size()), group});
		rowGroup = rowGroups.size() - 1;
	}

	rows.emplace_back(Row{uint16_t(rows.size()), rowGroup, &node});

	if (rowGroup != maxOf<uint16_t>()) {
		rowGroups[rowGroup].rows.emplace_back(rows.size() - 1);
	}
}

static uint16_t nextRowIndex(const Table::Row &row, uint16_t idx) {
	while (idx < row.cells.size() && row.cells[idx] != nullptr) {
		++ idx;
	}
	return idx;
}

void Table::processRow(const Row &row, size_t colsCount) {
	auto rowsCount = uint16_t(rows.size());
	uint16_t colIndex = nextRowIndex(row, 0);
	for (auto &it : row.node->getNodes()) {
		if (it.getHtmlName() == "td" || it.getHtmlName() == "th") {
			auto rowIndex = row.index;
			auto colSpan = Cell_getColSpan(it);
			auto rowSpan = Cell_getRowSpan(it);

			cells.emplace_back(Cell{&it, rowIndex, colIndex, rowSpan, colSpan});

			for (uint16_t i = 0; i < colSpan; ++ i) {
				if (colIndex + i < colsCount) {
					for (uint16_t j = 0; j < rowSpan; ++ j) {
						if (rowIndex + j < rowsCount) {
							rows[rowIndex + j].cells[colIndex + i] = &cells.back();
							cols[colIndex + i].cells[rowIndex + j] = &cells.back();
						}
					}
				}
			}

			colIndex = nextRowIndex(row, colIndex + colSpan);
		}
	}
}

uint32_t Table::makeCells(NodeId nodeId) {
	if (rows.empty()) {
		return 0;
	}

	if (auto colsAttr = layout->node.node->getAttribute("cols")) {
		StringView(*colsAttr).readInteger().unwrap([&] (int64_t colNum) {
			if (colNum > 0 && colNum <= 32) {
				for (size_t i = cols.size(); i < size_t(colNum); ++ i) {
					cols.emplace_back(Col{uint16_t(cols.size()), maxOf<uint16_t>(), nullptr});
				}
			}
		});
	}

	size_t phantomColsCount = 0;

	size_t colsCount = cols.size();
	for (auto &it : rows) {
		size_t count = 0;
		for (auto &n : it.node->getNodes()) {
			if (n.getHtmlName() == "td" || n.getHtmlName() == "th") {
				count += Cell_getColSpan(n);
			}
		}
		colsCount = max(count, colsCount);
	}

	if (colsCount != cols.size()) {
		auto extra = colsCount - cols.size();
		for (size_t i = 0; i < extra; ++ i) {
			cols.emplace_back(Col{uint16_t(cols.size()), maxOf<uint16_t>(), nullptr});
			++ phantomColsCount;
		}
	}

	if (colsCount == 0) {
		return 0;
	}

	if (phantomColsCount > 0) {
		phantomCols.reserve(phantomColsCount);
	}

	for (auto &it : cols) {
		if (!it.node) {
			phantomCols.emplace_back("col", String());
			phantomCols.back().setNodeId(nodeId);
			it.node = &phantomCols.back();
			++ nodeId;
		}
	}

	size_t cellsCount = 0;

	for (auto &it : rows) {
		it.cells.resize(colsCount, nullptr);
		for (auto &iit : it.node->getNodes()) {
			if (iit.getHtmlName() == "td" || iit.getHtmlName() == "th") {
				++ cellsCount;
			}
		}
	}

	for (auto &it : cols) {
		it.cells.resize(rows.size(), nullptr);
	}

	cells.reserve(cellsCount);
	for (auto &it : rows) {
		processRow(it, colsCount);
	}

	return phantomColsCount;
}

bool Table::allocateSpace() {
	float calcMinWidth = 0.0f;
	float calcMaxWidth = 0.0f;
	if (isnan(width)) {
		for (auto &col : cols) {
			if (!isnan(col.info->width)) {
				calcMinWidth += col.info->width;
				calcMaxWidth += col.info->width;
			} else {
				calcMinWidth += col.info->minWidth;
				calcMaxWidth += col.info->maxWidth;
			}
		}

		if (!isnan(minWidth)) {
			calcMinWidth = max(minWidth, calcMinWidth);
		}

		if (!isnan(maxWidth)) {
			calcMaxWidth = max(maxWidth, calcMaxWidth);
		}

		if (isnan(origMinWidth)) {
			origMinWidth = calcMinWidth;
		}

		if (isnan(origMaxWidth)) {
			origMaxWidth = calcMaxWidth;
		}

		if (calcMinWidth == calcMaxWidth || layout->pos.size.width == calcMinWidth) {
			width = calcMinWidth;
		} else if (layout->pos.size.width < calcMinWidth) {
			widthScale *= (layout->pos.size.width / (calcMinWidth * 1.1f));

			for (auto &col : cols) {
				col.info->width = nan();
				col.info->minWidth = nan();
				col.info->maxWidth = nan();

				for (auto &cell : col.cells) {
					cell->info->width = nan();
					cell->info->minWidth = nan();
					cell->info->maxWidth = nan();
				}
			}

			return false;
		} else if (layout->pos.size.width <= calcMaxWidth) {
			width = layout->pos.size.width;
		} else {
			width = calcMaxWidth;
		}
	} else {
		origMinWidth = origMaxWidth = calcMinWidth = calcMaxWidth = width;
	}

	// calc extraspace
	float maxExtraSpace = 0.0f;
	for (auto &col : cols) {
		if (isnan(col.info->width)) {
			maxExtraSpace += col.info->maxWidth - col.info->minWidth;
		}
	}

	// allocate column space
	const float extraSpace = width - calcMinWidth;
	float xPos = 0.0f;
	for (auto &col : cols) {
		if (isnan(col.info->width)) {
			col.xPos = xPos;
			auto space = (maxExtraSpace != 0.0f) ? ((col.info->maxWidth - col.info->minWidth) / maxExtraSpace) * extraSpace : 0.0f;
			col.width = col.info->width = col.info->minWidth + space;
			xPos += col.width;
		}
	}

	// allocate cell space
	for (auto &cell : cells) {
		if (isnan(cell.info->width)) {
			cell.info->width = 0.0f;
			for (size_t i = cell.colIndex; i < cell.colIndex + cell.colSpan; ++ i) {
				cell.info->width += cols[i].info->width;
			}
			// log::format("Col", "%d %d %f", cell.colIndex, cell.rowIndex, cell.info->width);
		}
	}

	return true;
}

static void Table_processTableChildsNodes(Table &table, const Node::NodeVec &nodes) {
	for (auto &it : nodes) {
		if (it.getHtmlName() == "caption") {
			table.caption = &it;
		} else if (it.getHtmlName() == "colgroup") {
			for (auto &iit : it.getNodes()) {
				if (iit.getHtmlName() == "col") {
					table.addCol(&it, iit);
				}
			}
		} else if (it.getHtmlName() == "col") {
			table.addCol(nullptr, it);
		} else if (it.getHtmlName() == "thead") {
			for (auto &iit : it.getNodes()) {
				if (iit.getHtmlName() == "tr") {
					table.addRow(&it, iit);
				}
			}
		} else if (it.getHtmlName() == "tfoot") {
			for (auto &iit : it.getNodes()) {
				if (iit.getHtmlName() == "tr") {
					table.addRow(&it, iit);
				}
			}
		} else if (it.getHtmlName() == "tbody") {
			for (auto &iit : it.getNodes()) {
				if (iit.getHtmlName() == "tr") {
					table.addRow(&it, iit);
				}
			}
		} else if (it.getHtmlName() == "tr") {
			table.addRow(nullptr, it);
		}
	}
}

template <typename PushCallback, typename PopCallback, typename CompileCallback>
static void Table_processTableChildsStyle(const RendererInterface *iface, Table &table, const PushCallback &push, const PopCallback &pop, const CompileCallback &compile) {
	uint16_t tmpColGroup = maxOf<uint16_t>();
	for (auto &col : table.cols) {
		if (col.group != tmpColGroup) {
			if (tmpColGroup != maxOf<uint16_t>()) {
				pop();
			}
			if (col.group != maxOf<uint16_t>()) {
				push(table.colGroups[col.group].node);
				table.colGroups[col.group].style = compile(table.colGroups[col.group].node);
			}
			tmpColGroup = col.group;
		}

		if (col.node) {
			push(col.node);

			const Style *colStyle = compile(col.node);
			col.info = Rc<Table::LayoutInfo>::alloc();
			col.info->node = Layout::NodeInfo(col.node, colStyle, iface);

			pop();
		}
	}

	if (tmpColGroup) {
		pop();
	}

	uint16_t tmpRowGroup = maxOf<uint16_t>();
	const Node *tmpRow = nullptr;
	for (Table::Cell &cell : table.cells) {
		auto &row = table.rows[cell.rowIndex];
		if (row.group != maxOf<uint16_t>() && row.group != tmpRowGroup) {
			if (tmpRow) {
				pop();
				tmpRow = nullptr;
			}
			if (tmpRowGroup != maxOf<uint16_t>()) {
				pop();
			}
			auto &g = table.rowGroups[row.group];
			push(g.node);
			g.style = compile(g.node);
			tmpRowGroup = row.group;
		}

		if (tmpRow != row.node) {
			if (tmpRow) {
				pop();
			}
			tmpRow = row.node;
			push(row.node);
			row.style = compile(row.node);
		}

		push(cell.node);

		const Style *cellStyle = compile(cell.node);
		cell.info = Rc<Table::LayoutInfo>::alloc();
		cell.info->node = Layout::NodeInfo(cell.node, cellStyle, iface);

		pop(); // cell
	}

	if (tmpRow) {
		pop();
	}
	if (tmpRowGroup) {
		pop();
	}
}

template <typename Callback>
static void Table_prepareTableCell(const Builder *b, const Table &table, Table::Col &col, Table::Cell &cell, const MediaParameters &media, const Callback &cb) {
	cell.info->pos.padding.right = cell.info->node.block.paddingRight.computeValueAuto(0.0f, media);
	cell.info->pos.padding.left = cell.info->node.block.paddingLeft.computeValueAuto(0.0f, media);

	if (cell.info->node.block.width.isFixed()) {
		// fixed width - no need to calculate column width
		cell.info->minWidth = cell.info->maxWidth = cell.info->width = cell.info->node.block.width.computeValueStrong(0.0f, media);
		return;
	} else if (!isnan(table.width) && cell.info->node.block.width.metric == style::Metric::Percent) {
		cell.info->minWidth = cell.info->maxWidth = cell.info->width = table.width * cell.info->node.block.width.value;
		return;
	} else {
		if (cell.info->node.block.minWidth.isFixed()) {
			cell.info->minWidth = cell.info->node.block.minWidth.computeValueStrong(0.0f, media);
		} else if (!isnan(table.width) && cell.info->node.block.minWidth.metric == style::Metric::Percent) {
			cell.info->minWidth = table.width * cell.info->node.block.minWidth.value;
		}

		if (cell.info->node.block.maxWidth.isFixed()) {
			cell.info->maxWidth = cell.info->node.block.maxWidth.computeValueStrong(0.0f, media);
		} else if (!isnan(table.width) && cell.info->node.block.maxWidth.metric == style::Metric::Percent) {
			cell.info->maxWidth = table.width * cell.info->node.block.maxWidth.value;
		}

		cb(cell);
	}
}

static void Table_processTableColSpan(Table &table, const Vector<Table::Cell *> &vec) {
	for (auto &cell : vec) {
		float minCellWidth = 0.0f;
		float maxCellWidth = 0.0f;
		float fixedWidth = 0.0f;

		struct ColInfo {
			Table::Col *col;
			float colMin;
			float colMax;
			bool fixed;
			size_t idx;
			float minWeight = 0.0f;
			float maxWeight = 0.0f;
		};

		bool allFixed = true;
		Vector<ColInfo> cols; cols.reserve(cell->colSpan);

		for (size_t i = 0; i < cell->colSpan; ++ i) {
			auto minW = table.cols[cell->colIndex + i].info->minWidth;
			auto maxW = table.cols[cell->colIndex + i].info->maxWidth;

			if (!isnan(table.cols[cell->colIndex + i].info->width)) {
				float f = table.cols[cell->colIndex + i].info->width;
				fixedWidth += f;
				cols.emplace_back(ColInfo{&table.cols[cell->colIndex + i], f, f, true, cell->colIndex + i});
			} else {
				cols.emplace_back(ColInfo{&table.cols[cell->colIndex + i], minW, maxW, false, cell->colIndex + i});
				allFixed = false;
			}

			if (!isnan(minW) && !isnan(minCellWidth)) {
				minCellWidth += minW;
			} else {
				minCellWidth = nan();
			}
			if (!isnan(maxW) && !isnan(maxCellWidth)) {
				maxCellWidth += maxW;
			} else {
				maxCellWidth = nan();
			}
		}

		if (allFixed) {
			// all fixed
			cell->info->maxWidth = cell->info->minWidth = cell->info->width = fixedWidth;
			continue;
		}

		if (!isnan(minCellWidth)) {
			if (minCellWidth < cell->info->minWidth) {
				float spaceToAllocate = cell->info->minWidth;
				spaceToAllocate -= fixedWidth;
				minCellWidth -= fixedWidth;
				for (auto &it : cols) {
					if (!it.fixed) {
						it.col->info->minWidth = (it.colMin / minCellWidth) * spaceToAllocate;
					}
				}
			}
		}

		if (!isnan(maxCellWidth)) {
			if (maxCellWidth < cell->info->maxWidth) {
				float spaceToAllocate = cell->info->maxWidth;
				spaceToAllocate -= fixedWidth;
				maxCellWidth -= fixedWidth;
				for (auto &it : cols) {
					if (!it.fixed) {
						it.col->info->maxWidth = (it.colMax / maxCellWidth) * spaceToAllocate;
					}
				}
			}
		}

		if (isnan(minCellWidth) || isnan(maxCellWidth)) {
			float minW = 0.0f; size_t minWeightCount = 0;
			float maxW = 0.0f; size_t maxWeightCount = 0;
			for (auto &it : cols) {
				if (!isnan(it.colMin)) {
					minW = std::max(minW, it.colMin);
					++ minWeightCount;
				}
				if (!isnan(it.colMax)) {
					maxW = std::max(maxW, it.colMax);
					++ maxWeightCount;
				}
			}

			float minWeightSum = 0.0f;
			float maxWeightSum = 0.0f;
			for (auto &it : cols) {
				if (!isnan(it.colMin)) {
					it.minWeight = (it.colMin / minW);
					minWeightSum += it.minWeight;
				}
				if (!isnan(it.colMax)) {
					it.maxWeight = (it.colMax / maxW);
					maxWeightSum += it.maxWeight;
				}
			}

			const float minMod = (minWeightCount > 0) ? (minWeightSum / float(minWeightCount)) / float(cell->colSpan) : 1.0f;
			const float maxMod = (maxWeightCount > 0) ? (maxWeightSum / float(maxWeightCount)) / float(cell->colSpan) : 1.0f;
			for (auto &it : cols) {
				if (!isnan(it.colMin)) {
					it.minWeight *= minMod;
				}
				if (!isnan(it.colMax)) {
					it.maxWeight *= maxMod;
				}
			}

			minWeightSum *= minMod;
			maxWeightSum *= maxMod;

			if (isnan(minCellWidth)) {
				// has undefined min-width
				float weightToAllocate = (1.0f - minWeightSum) / (cell->colSpan - minWeightCount);
				for (auto &it : cols) {
					const float w = isnan(it.colMin) ? cell->info->minWidth * weightToAllocate : cell->info->minWidth * it.minWeight;
					if (isnan(it.col->info->minWidth) || it.col->info->minWidth < w) {
						it.col->info->minWidth = w;
					}
				}
			}

			if (isnan(maxCellWidth)) {
				// has undefined max-width
				float weightToAllocate = (1.0f - maxWeightSum) / (cell->colSpan - maxWeightCount);
				for (auto &it : cols) {
					const float w = isnan(it.colMax) ? cell->info->maxWidth * weightToAllocate : cell->info->maxWidth * it.maxWeight;
					if (isnan(it.col->info->maxWidth) || it.col->info->maxWidth < w) {
						it.col->info->maxWidth = w;
					}
				}
			}
		}
	}
}

void Table::processTableChilds() {
	// build and fill grid
	Table_processTableChildsNodes(*this, layout->node.node->getNodes());

	auto &_media = layout->builder->getMedia();
	layout->builder->incrementNodeId(makeCells(layout->builder->getMaxNodeId()));

	// compile styles
	Table_processTableChildsStyle(layout->builder, *this,
		[this] (const Node *node) { layout->builder->pushNode(node);},
		[this] { layout->builder->popNode(); },
		[this] (const Node *node) { return layout->builder->compileStyle(*node); }
	);

	if (layout->node.block.width.isFixed()) {
		width = layout->pos.size.width;
	}

	auto calcSizes = [&] (float scale) {
		auto media = _media;
		media.fontScale *= scale;

		if (isnan(width)) {
			if (layout->node.block.minWidth.isFixed()) {
				minWidth = layout->node.block.minWidth.computeValueStrong(0.0f, media);
			}
			if (layout->node.block.maxWidth.isFixed()) {
				maxWidth = layout->node.block.maxWidth.computeValueStrong(0.0f, media);
			}
		}

		Vector<Cell *> colSpans;
		// calc column sizes
		for (auto &col : cols) {
			if (col.info->node.node) {
				if (col.info->node.block.width.isFixed()) {
					// fixed width - no need to calculate column width
					col.info->maxWidth = col.info->minWidth = col.info->width = col.info->node.block.width.computeValueStrong(0.0f, media);
					continue;
				} else if (!isnan(width) && col.info->node.block.width.metric == style::Metric::Percent) {
					col.info->maxWidth = col.info->minWidth = col.info->width = width * col.info->node.block.width.value;
				}

				if (col.info->node.block.minWidth.isFixed()) {
					col.info->minWidth = col.info->node.block.minWidth.computeValueStrong(0.0f, media);
				} else if (!isnan(width) && col.info->node.block.minWidth.metric == style::Metric::Percent) {
					col.info->minWidth = width * col.info->node.block.minWidth.value;
				}

				if (col.info->node.block.maxWidth.isFixed()) {
					col.info->maxWidth = col.info->node.block.maxWidth.computeValueStrong(0.0f, media);
				} else if (!isnan(width) && col.info->node.block.maxWidth.metric == style::Metric::Percent) {
					col.info->maxWidth = width * col.info->node.block.maxWidth.value;
				}
			}

			bool fixed = false;

			// check if some cell has fixed width
			for (auto &cell : col.cells) {
				if (!cell || !cell->node) {
					continue;
				}
				Table_prepareTableCell(layout->builder, *this, col, *cell, media, [&] (Table::Cell &cell) {
					auto &row = rows[cell.rowIndex];
					layout->builder->pushNode(rowGroups[row.group].node);
					layout->builder->pushNode(row.node);
					layout->builder->pushNode(cell.node);

					auto minW = ceilf(Layout::requestWidth(layout->builder, cell.info->node, Layout::ContentRequest::Minimize, media)) + 1.0f;
					auto maxW = ceilf(Layout::requestWidth(layout->builder, cell.info->node, Layout::ContentRequest::Maximize, media)) + 1.0f;

					auto minWidth = minW + cell.info->pos.padding.horizontal();
					auto maxWidth = maxW + cell.info->pos.padding.horizontal();

					//log::format("Table", "min: %f (%f + %f) max: %f (%f + %f)",
					//		minWidth, minW, cell.info->pos.padding.horizontal(),
					//		maxWidth, maxW, cell.info->pos.padding.horizontal());

					if (isnan(cell.info->minWidth) || minWidth > cell.info->minWidth) {
						cell.info->minWidth = minWidth;
					}

					if (isnan(cell.info->maxWidth) || maxWidth > cell.info->maxWidth) {
						cell.info->maxWidth = maxWidth;
					}

					layout->builder->popNode();
					layout->builder->popNode();
					layout->builder->popNode();
				});
				if (cell->colSpan == 1) {
					if (!isnan(cell->info->width)) {
						col.info->maxWidth = col.info->minWidth = col.info->width = cell->info->width;
						fixed = true;
						break;
					} else {
						if (isnan(col.info->minWidth) || col.info->minWidth < cell->info->minWidth) {
							col.info->minWidth = cell->info->minWidth;
						}
						if (isnan(col.info->maxWidth) || col.info->maxWidth < cell->info->maxWidth) {
							col.info->maxWidth = cell->info->maxWidth;
						}
					}
				} else {
					colSpans.emplace_back(cell);
				}
			}

			if (fixed) {
				continue;
			}
		}

		if (!colSpans.empty()) {
			Table_processTableColSpan(*this, colSpans);
		}
	};

	size_t limit = 4;
	do {
		calcSizes(widthScale);
		-- limit;
	} while (!allocateSpace() && limit > 0);
}

static void Table_procesRowCell(Table &table, Table::Row &row, Table::Cell &cell, Layout &newL, float xPos) {
	auto &parentPos = table.layout->pos.position;
	auto &_media = table.layout->builder->getMedia();
	float height = newL.node.block.height.computeValueStrong(table.layout->pos.size.height, _media);
	const float minHeight = newL.node.block.minHeight.computeValueStrong(table.layout->pos.size.height, _media);
	const float maxHeight = newL.node.block.maxHeight.computeValueStrong(table.layout->pos.size.height, _media);

	newL.pos.margin = Margin(0.0f);

	if (!std::isnan(height)) {
		if (!std::isnan(minHeight) && height < minHeight) {
			height = minHeight;
		}

		if (!std::isnan(maxHeight) && height > maxHeight) {
			height = maxHeight;
		}

		newL.pos.minHeight = nan();
		newL.pos.maxHeight = height;
	} else {
		newL.pos.minHeight = minHeight;
		newL.pos.maxHeight = maxHeight;
	}

	newL.pos.size = Size(cell.info->width - cell.info->pos.padding.horizontal(), height);
	newL.pos.padding.top = newL.node.block.paddingTop.computeValueAuto(newL.pos.size.width, _media);
	newL.pos.padding.bottom = newL.node.block.paddingBottom.computeValueAuto(newL.pos.size.width, _media);

	if (isnan(table.layout->pos.position.y)) {
		newL.pos.position = Vec2(parentPos.x + xPos + newL.pos.padding.left, newL.pos.padding.top);
	} else {
		newL.pos.position = Vec2(parentPos.x + xPos + newL.pos.padding.left, parentPos.y + newL.pos.padding.top);
	}

	newL.node.context = style::Display::Block;

	table.layout->builder->processChilds(newL, *newL.node.node);

	newL.finalizeInlineContext();

	if (!std::isnan(newL.pos.maxHeight) && newL.pos.size.height > newL.pos.maxHeight) {
		newL.pos.size.height = newL.pos.maxHeight;
	}
}

static void Table_processTableRowSpan(Table &table, Table::Cell &cell) {
	float fullHeight = 0.0f;
	size_t undefinedCount = 0;
	Vector<float> heights; heights.reserve(cell.rowSpan);
	for (size_t i = 0; i < cell.rowSpan; ++ i) {
		heights.emplace_back(table.rows[cell.rowIndex + i].height);
		fullHeight += table.rows[cell.rowIndex + i].height;
		if (heights.back() == 0) {
			++ undefinedCount;
		}
	}

	if (fullHeight < cell.info->layout->getBoundingBox().size.height) {
		const auto diff = cell.info->layout->getBoundingBox().size.height - fullHeight;
		if (undefinedCount > 0) {
			// allocate space to undefined rows;
			for (size_t i = 0; i < cell.rowSpan; ++ i) {
				if (table.rows[cell.rowIndex + i].height == 0.0f) {
					table.rows[cell.rowIndex + i].height = diff / undefinedCount;
				}
			}
		} else {
			// allocate to all rows;
			for (size_t i = 0; i < cell.rowSpan; ++ i) {
				table.rows[cell.rowIndex + i].height += diff * (heights[i] / fullHeight);
			}
		}
	}
}

static void Table_finalizeRowCell(Table &table, Layout &newL, float yPos, float height) {
	float diff = height - newL.getBoundingBox().size.height;

	style::VerticalAlign a = style::VerticalAlign::Middle;
	auto vAlign = newL.node.style->get(style::ParameterName::VerticalAlign, newL.builder);
	if (!vAlign.empty()) {
		a = vAlign.front().value.verticalAlign;
	}

	switch (a) {
	case style::VerticalAlign::Baseline:
	case style::VerticalAlign::Middle:
		newL.pos.padding.top += diff / 2.0f;
		newL.pos.padding.bottom += diff / 2.0f;
		newL.pos.position.y += diff / 2.0f;
		break;
	case style::VerticalAlign::Super:
	case style::VerticalAlign::Top:
		newL.pos.padding.bottom += diff;
		break;
	case style::VerticalAlign::Sub:
	case style::VerticalAlign::Bottom:
		newL.pos.padding.top += diff;
		newL.pos.position.y += diff;
		break;
	}

	newL.pos.origin.y = roundf(newL.pos.position.y * newL.builder->getMedia().density);

	newL.processBackground(table.layout->pos.position.y);
	newL.processOutline(table.layout->node.block.borderCollapse == style::BorderCollapse::Separate);
	newL.cancelInlineContext();

	newL.updatePosition(yPos);

	// log::format("Builder", "%f %f", newL.pos.position.x, newL.pos.position.y);

	table.layout->layouts.emplace_back(&newL);
}

void Table::processTableLayouts() {
	if (caption) {
		captionLayout = &layout->builder->makeLayout(caption, layout->node.context, true);
	}

	if (width < layout->pos.size.width) {
		if (layout->node.block.marginLeft.metric == style::Metric::Auto && layout->node.block.marginRight.metric == style::Metric::Auto) {
			auto diff = layout->pos.size.width - width;
			layout->pos.margin.left += diff / 2.0f;
			layout->pos.margin.right += diff / 2.0f;
			layout->pos.position.x += diff / 2.0f;
		}
		layout->pos.size.width = width;
	}

	if (captionLayout && captionLayout->node.block.captionSide == style::CaptionSide::Top) {
		processCaption(captionLayout->node.block.captionSide);
	}

	bool hookMedia = false;
	if (widthScale != 1.0f) {
		auto newMedia = layout->builder->getMedia();
		newMedia.fontScale *= widthScale;
		layout->builder->hookMedia(newMedia);
		hookMedia = true;
	}

	Vector<Table::Cell *> rowSpans;
	for (auto &row : rows) {
		float xPos = 0.0f;

		// process layouts
		size_t colIdx = 0;
		for (auto &cell : row.cells) {
			if (!cell) {
				continue;
			}
			if (cell->rowIndex != row.index || cell->colIndex != colIdx) {
				++ colIdx;
				continue;
			}

			auto &newL = layout->builder->makeLayout(move(cell->info->node), move(cell->info->pos));
			newL.depth = layout->depth + 5; // colgroup + col + rowgroup + row
			cell->info->layout = &newL;
			Table_procesRowCell(*this, row, *cell, newL, xPos);

			xPos += cell->info->width;
			++ colIdx;

			if (cell->rowSpan == 1) {
				row.height = std::max(row.height, newL.getBoundingBox().size.height);
			} else {
				rowSpans.emplace_back(cell);
			}
		}
	}

	for (auto &cell : rowSpans) {
		Table_processTableRowSpan(*this, *cell);
	}

	// vertical align
	float currentYPos = 0.0f;
	for (auto &row : rows) {
		size_t colIdx = 0;
		row.yPos = currentYPos;
		for (auto &cell : row.cells) {
			if (!cell) {
				continue;
			}
			if (cell->rowIndex != row.index || cell->colIndex != colIdx) {
				++ colIdx;
				continue;
			}

			float yPos = row.yPos;
			float rowHeight = 0.0f;
			for (size_t i = 0; i < cell->rowSpan; ++ i) {
				rowHeight += rows[cell->rowIndex + i].height;
			}

			Table_finalizeRowCell(*this, *cell->info->layout, yPos, rowHeight);
			++ colIdx;
		}

		currentYPos += row.height;
	}

	layout->pos.size.height = currentYPos;

	float groupXPos = 0.0f;
	for (auto &it : colGroups) {
		it.xPos = groupXPos;
		float groupWidth = 0.0f;
		for (auto &col : it.cols) {
			groupWidth += cols[col].width;
		}
		it.width = groupWidth;
		groupXPos += groupWidth;
	}

	float groupYPos = 0.0f;
	for (auto &it : rowGroups) {
		it.yPos = groupYPos;
		float groupHeight = 0.0f;
		for (auto &row : it.rows) {
			groupHeight += rows[row].height;
		}
		it.height = groupHeight;
		groupYPos += groupHeight;
	}

	processTableBackground();

	if (hookMedia) {
		layout->builder->restoreMedia();
	}

	if (captionLayout && captionLayout->node.block.captionSide == style::CaptionSide::Bottom) {
		processCaption(captionLayout->node.block.captionSide);
	}
}

NS_LAYOUT_END
