/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#include "MDBTransaction.h"
#include "MDBManifest.h"
#include "MDBStorage.h"
#include "STStorageWorker.h"
#include "STStorageScheme.h"

NS_MDB_BEGIN

mem::Value Transaction::select(Worker &worker, const db::Query &query) {
	auto scheme = _manifest->getScheme(worker.scheme());
	if (!scheme) {
		return mem::Value();
	}

	if (auto target = query.getSingleSelectId()) {

	} else if (!query.getSelectIds().empty()) {

	}

	return mem::Value();
}

mem::Value Transaction::create(Worker &worker, const mem::Value &idata) {
	auto scheme = _manifest->getScheme(worker.scheme());
	if (!scheme) {
		return mem::Value();
	}

	auto perform = [&] (const mem::Value &data) -> mem::Value {
		uint64_t id = 0;

		// check unique
		// - not implemented

		id = _manifest->getNextOid();

		if (pushObject(*scheme, id, data)) {
			mem::Value ret(data);
			ret.setInteger(id, "__oid");
			if (worker.shouldIncludeNone() && worker.scheme().hasForceExclude()) {
				for (auto &it : worker.scheme().getFields()) {
					if (it.second.hasFlag(db::Flags::ForceExclude)) {
						ret.erase(it.second.getName());
					}
				}
			}
			return ret;
		}
		return mem::Value();
	};

	if (idata.isDictionary()) {
		return perform(idata);
	} else if (idata.isArray()) {
		mem::Value ret;
		for (auto &it : idata.asArray()) {
			if (auto v = perform(it)) {
				ret.addValue(std::move(v));
			}
		}
		return ret;
	}

	return mem::Value();
}

mem::Value Transaction::save(Worker &, uint64_t oid, const mem::Value &obj, const mem::Vector<mem::String> &fields) {
	return mem::Value();
}

mem::Value Transaction::patch(Worker &, uint64_t oid, const mem::Value &patch) {
	return mem::Value();
}

bool Transaction::remove(Worker &, uint64_t oid) {
	return false;
}

size_t Transaction::count(Worker &, const db::Query &) {
	return 0;
}



uint8_p Transaction::findTargetObject(const TreeStackFrame &frame, uint64_t oid) const {
	struct CellData {
		uint64_t value;
		uint8_p payload;
	};

	auto getCell = [] (const TreeStackFrame &frame, uint16_t offset) -> CellData {
		CellData ret;
		ret.value = readVarUint(uint8_p(frame.ptr) + offset, &ret.payload);
		return ret;
	};

	auto p = (TreePageHeader *)frame.ptr;
	uint16_t *begin = (uint16_t *) ( ((uint16_t *)frame.ptr) + sizeof(TreePageHeader) );

	if (p->ncells == 0) {
		return 0;
	} else if (p->ncells == 1) {
		auto cell = getCell(frame, *begin);
		if (oid == cell.value) {
			return cell.payload;
		} else {
			return 0;
		}
	}

	auto front = 0;
	auto back = p->ncells - 1;

	auto frontVal = getCell(frame, *begin);
	if (oid == frontVal.value) {
		return frontVal.payload;
	} else if (oid < frontVal.value) {
		return 0;
	}

	auto backVal = getCell(frame, *begin);
	if (oid == backVal.value) {
		return backVal.payload;
	} else if (oid > backVal.value) {
		return 0;
	}

	while (back > front) {
		auto mid = (front + back) / 2;
		auto cell = getCell(frame, *(begin + mid));
		if (cell.value == oid) {
			return cell.payload;
		} else if (oid < cell.value) {
			// left
			if (back == mid) {
				auto frontVal = getCell(frame, *(begin + front));
				if (oid == frontVal.value) {
					return frontVal.payload;
				} else if (oid < frontVal.value) {
					return 0;
				}
			} else {
				back = mid;
			}
		} else {
			// right
			if (front == mid) {
				auto backVal = getCell(frame, *(begin + back));
				if (oid == backVal.value) {
					return backVal.payload;
				} else if (oid > backVal.value) {
					return 0;
				}
			} else {
				front = mid;
			}
		}
	}

	return 0;
}

uint32_t Transaction::findTargetPage(const TreeStackFrame &frame, uint64_t oid) const {
	auto getCell = [] (const TreeStackFrame &frame, uint16_t offset) -> TreeTableInteriorCell {
		TreeTableInteriorCell ret;
		ret.pointer = * uint32_p(uint8_p(frame.ptr) + offset);
		ret.value = readVarUint(uint8_p(frame.ptr) + offset + sizeof(uint32_t));
		return ret;
	};

	auto p = (TreePageInteriorHeader *)frame.ptr;
	uint16_t *begin = (uint16_t *) ( ((uint16_t *)frame.ptr) + sizeof(TreePageInteriorHeader) );

	if (p->ncells == 0) {
		return 0;
	} else if (p->ncells == 1) {
		TreeTableInteriorCell val = getCell(frame, *begin);
		if (oid < val.value) {
			return val.pointer;
		} else {
			return p->right;
		}
	} else if (p->ncells == 2) {
		TreeTableInteriorCell front = getCell(frame, *begin);
		TreeTableInteriorCell back = getCell(frame, *(begin + 1));
		if (oid < front.value) {
			return front.pointer;
		} else if (oid < back.value) {
			return back.pointer;
		} else {
			return p->right;
		}
	}

	do {
		// check right first for insert;
		auto back = getCell(frame, *(begin + p->ncells - 1));
		if (oid >= back.value) {
			return p->right;
		}
	} while (0);

	auto front = 0;
	auto back = p->ncells - 1;
	uint32_t backPtr = 0;

	while (back - front != 1) {
		auto mid = (front + back) / 2;
		TreeTableInteriorCell val = getCell(frame, *(begin + mid));
		if (oid < val.value) {
			// left
			back = mid;
			backPtr = val.pointer;
		} else {
			// right
			front = mid;
		}
	}

	if (front == 0) {
		TreeTableInteriorCell front = getCell(frame, *begin);
		if (oid < front.value) {
			return front.pointer;
		} else {
			return backPtr;
		}
	} else if (back == p->ncells - 1) {
		TreeTableInteriorCell back = getCell(frame, *(begin + p->ncells - 1));
		if (oid < back.value) {
			return back.pointer;
		} else {
			return p->right;
		}
	} else {
		return backPtr;
	}
}

bool Transaction::pushObject(const Scheme &scheme, uint64_t oid, const mem::Value &data) const {
	// lock sheme on write
	std::unique_lock<std::shared_mutex> lock(scheme.scheme->mutex);
	TreeStack stack({&scheme, OpenMode::ReadWrite});
	if (!openTreeStack(stack, oid)) {
		return false;
	}

	if (!stack.frames.empty() || stack.frames.back().type != PageType::LeafTable) {
		return false;
	}

	auto cell = TreeCell(PageType::LeafTable, Oid(oid), &data);

	if (!pushCell(stack.frames.back(), nullptr, cell)) {
		pushCell(splitPage(stack, cell, true), nullptr, cell);
	}

	++ scheme.scheme->counter;
	scheme.scheme->dirty = true;
	return true;
}

bool Transaction::pushCell(const TreeStackFrame &frame, uint16_p cellTarget, TreeCell cell) const {
	const auto maxPayloadSize = (_storage->getPageSize() - sizeof(TreePageHeader) - sizeof(uint16_t)) / 4;

	// with incremental oids new cell will be at the end of cell pointer array
	auto h = (TreePageHeader *)frame.ptr;
	auto freeSpace = getFrameFreeSpace(frame);

	auto payloadSize = cell.size();

	bool isOverflow = false;

	if (payloadSize + sizeof(uint16_t) > maxPayloadSize) {
		isOverflow = true;

		switch (cell.type) {
		case PageType::InteriorIndex: payloadSize = sizeof(uint32_t) + 8; break;
		case PageType::InteriorTable: payloadSize = sizeof(uint32_t) + getVarUintSize(cell.oid); break;
		case PageType::LeafIndex: payloadSize = sizeof(uint32_t) + getVarUintSize(cell.oid) + 0; break;
		case PageType::LeafTable: payloadSize = getVarUintSize(cell.oid) + 8; break;
		default: break;
		}
	}

	if (payloadSize + sizeof(uint16_t) > freeSpace.first) {
		// split page and update frame - unbalanced split on insert
		return false;
	}

	auto cellStart = uint16_p( uint8_p(frame.ptr)
		+ ((PageType(h->type) == PageType::InteriorIndex || PageType(h->type) == PageType::InteriorTable)
			? sizeof(TreePageHeader)
			: sizeof(TreePageInteriorHeader)) );
	auto cellEnd = cellStart + sizeof(uint16_t) * h->ncells;

	if (cellTarget == nullptr) {
		// use last cell
		cellTarget = cellEnd;
	} else {
		// insert with move
		memmove(cellTarget + 1, cellTarget, sizeof(uint16_t) * (h->ncells - (cellTarget - cellStart)));
	}

	auto revOffset = freeSpace.second - payloadSize;
	auto revTarget = uint8_p(frame.ptr) + revOffset;

	switch (cell.type) {
	case PageType::InteriorIndex:
		memcpy(revTarget, &cell.page, sizeof(uint32_t)); revTarget += sizeof(uint32_t);
		if (isOverflow) {
			auto page = _manifest->writeOverflow(*this, cell.type, *cell.payload);
			memcpy(revTarget, "Ovfl", 4); revTarget += 4;
			memcpy(revTarget, &page, sizeof(uint32_t));
		} else {
			revTarget += writePayload(cell.type, revTarget, *cell.payload);
		}
		break;
	case PageType::InteriorTable:
		memcpy(revTarget, &cell.page, sizeof(uint32_t)); revTarget += sizeof(uint32_t);
		revTarget += writeVarUint(revTarget, cell.oid);
		break;
	case PageType::LeafIndex:
		memcpy(revTarget, &cell.page, sizeof(uint32_t)); revTarget += sizeof(uint32_t);
		revTarget += writeVarUint(revTarget, cell.oid);
		if (isOverflow) {
			auto page = _manifest->writeOverflow(*this, cell.type, *cell.payload);
			memcpy(revTarget, "Ovfl", 4); revTarget += 4;
			memcpy(revTarget, &page, sizeof(uint32_t));
		} else {
			revTarget += writePayload(cell.type, revTarget, *cell.payload);
		}
		break;
	case PageType::LeafTable:
		revTarget += writeVarUint(revTarget, cell.oid);
		if (isOverflow) {
			auto page = _manifest->writeOverflow(*this, cell.type, *cell.payload);
			memcpy(revTarget, "Ovfl", 4); revTarget += 4;
			memcpy(revTarget, &page, sizeof(uint32_t));
		} else {
			revTarget += writePayload(cell.type, revTarget, *cell.payload);
		}
		break;
	default: break;
	}

	*cellTarget = revOffset;
	h->ncells += 1;
	return true;
}

Transaction::TreeStackFrame Transaction::splitPage(TreeStack &stack, TreeCell cell, bool unbalanced) const {
	auto &frameToSplit = stack.frames.back();

	// key is greater then any other
	if (stack.frames.size() == 1) {
		// we at root, so, we need new root and new leaf
		auto newRoot = _manifest->allocatePage(*this);
		auto newLeaf = _manifest->allocatePage(*this);

		if (!newRoot || !newLeaf) {
			return TreeStackFrame{0, nullptr};
		}

		auto newRootFrane = openFrame(newRoot, OpenMode::Write);
		auto newLeafFrame = openFrame(newLeaf, OpenMode::Write);

		// form new root

		auto prevRootHeader = (TreePageHeader *)frameToSplit.ptr;
		auto rootHeader = (TreePageInteriorHeader *)newRootFrane.ptr;

		switch (PageType(prevRootHeader->type)) {
		case PageType::InteriorTable:
		case PageType::LeafTable:
			rootHeader->type = stappler::toInt(PageType::InteriorTable);
			break;
		case PageType::InteriorIndex:
		case PageType::LeafIndex:
			rootHeader->type = stappler::toInt(PageType::InteriorIndex);
			break;
		default: break;
		}

		rootHeader->root = 0;
		rootHeader->prev = 0;
		rootHeader->next = 0;
		rootHeader->right = newLeaf;

	}

}

bool Transaction::openTreeStack(TreeStack &stack, uint64_t oid) const {
	auto mode = stack.mode == OpenMode::Read ? OpenMode::Read : OpenMode::ReadWrite;
	auto type = PageType::InteriorTable;
	auto target = stack.scheme->scheme->idx;

	while (type != PageType::LeafTable && target) {
		if (auto frame = openFrame(target, mode)) {
			auto p = (TreePageHeader *)frame.ptr;
			frame.type = type = PageType(p->type);
			if (type == PageType::InteriorTable) {
				// find next page;
				if (auto next = findTargetPage(frame, oid)) {
					target = next;
					stack.frames.emplace_back(frame);
				} else {
					closeTreeStack(stack);
					return false;
				}
			}
		} else {
			closeTreeStack(stack);
			return false;
		}
	}

	if (target == 0) {
		closeTreeStack(stack);
		return false;
	}

	return true;
}

void Transaction::closeTreeStack(TreeStack &stack) const {
	for (auto &it : stack.frames) {
		closeFrame(it);
	}
	stack.frames.clear();
}

NS_MDB_END
