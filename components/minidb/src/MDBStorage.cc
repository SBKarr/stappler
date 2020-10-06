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

#include "MDBStorage.h"
#include "STStorageWorker.h"

NS_MDB_BEGIN

mem::Value Storage::select(Worker &, const db::Query &) {
	return mem::Value();
}

mem::Value Storage::create(Worker &worker, const mem::Value &idata) {
	bool isSchemeCompressed = worker.scheme().isCompressed();
	auto scheme = getScheme(worker.scheme());
	if (!scheme) {
		return mem::Value();
	}

	auto perform = [&] (const mem::Value &data) -> mem::Value {
		uint64_t id = 0;

		// check unique
		// - not implemented

		id = getNextOid();

		auto d = mem::writeData(data, isSchemeCompressed ? mem::EncodeFormat::CborCompressed : mem::EncodeFormat::Cbor);

		if (pushObject(*scheme, id, d)) {
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

mem::Value Storage::save(Worker &, uint64_t oid, const mem::Value &obj, const mem::Vector<mem::String> &fields) {
	return mem::Value();
}

mem::Value Storage::patch(Worker &, uint64_t oid, const mem::Value &patch) {
	return mem::Value();
}

bool Storage::remove(Worker &, uint64_t oid) {
	return false;
}

size_t Storage::count(Worker &, const db::Query &) {
	return 0;
}

bool Storage::isDeferredUpdate() const {
	return _deferredManifestUpdate.load();
}

void Storage::setDeferredUpdate(bool val) {
	if (val != isDeferredUpdate()) {
		_manifestLock.lock_shared();
		_deferredManifestUpdate.store(val);
		_manifestLock.unlock_shared();

		if (!val) {
			update();
		}
	}
}

void Storage::update() {
	auto val = UpdateFlags(_updateFlags.exchange(stappler::toInt(UpdateFlags::None)));
	if (val != UpdateFlags::None) {
		performManifestUpdate(val);
	}
}


uint8_p Storage::findTargetObject(const TreeStackFrame &frame, uint64_t oid) const {
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

uint32_t Storage::findTargetPage(const TreeStackFrame &frame, uint64_t oid) const {
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

bool Storage::pushObject(const Scheme &scheme, uint64_t oid, mem::BytesView data) {
	// lock sheme on write
	std::unique_lock<std::shared_mutex> lock(scheme.scheme->mutex);
	TreeStack stack({&scheme, OpenMode::ReadWrite});
	if (!openTreeStack(stack, oid)) {
		return false;
	}

	if (!stack.frames.empty() || stack.frames.back().type != PageType::LeafTable) {
		return false;
	}

	auto &frame = stack.frames.back();
	auto maxPayloadSize = (_pageSize - sizeof(TreePageHeader) - sizeof(uint16_t)) / 4;
	auto payloadSize = data.size() + getVarUintSize(oid) + sizeof(uint16_t); // with cell array record
	auto freeSpace = getFrameFreeSpace(stack.frames.back());

	std::array<uint8_t, 8> overflowData;
	if (payloadSize > maxPayloadSize) {
		uint32_t overflowPage = allocateOverflowPage();
		writeOverflow(overflowPage, data);

		memcpy(overflowData.data(), "Ovfl", 4);
		memcpy(overflowData.data() + 4, &overflowPage, sizeof(uint32_t));

		data = overflowData;
		payloadSize = data.size() + getVarUintSize(oid) + sizeof(uint16_t); // with cell array record
	}

	if (payloadSize > freeSpace.first) {
		// split page and update frame
	}

	// with incremental oids new cell will be at the end of cell pointer array
	auto h = (TreePageHeader *)stack.frames.back().ptr;
	auto target = uint16_p(uint8_p(frame.ptr) + sizeof(TreePageHeader) + h->ncells * sizeof(uint16_t));
	auto revOffset = freeSpace.second - payloadSize + sizeof(uint16_t);
	auto revTarget = uint8_p(frame.ptr) + revOffset;

	revTarget += writeVarUint(revTarget, oid);
	memcpy(revTarget, data.data(), data.size());

	*target = revOffset;
	h->ncells += 1;
	return true;
}

bool Storage::openTreeStack(TreeStack &stack, uint64_t oid) {
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

void Storage::closeTreeStack(TreeStack &stack) {
	for (auto &it : stack.frames) {
		closeFrame(it);
	}
	stack.frames.clear();
}

void Storage::dropTree(uint32_t root) {
	if (openPageForReading(root, [&] (void *mem, size_t) {
		auto h = (TreePageHeader *)mem;
		switch (PageType(h->type)) {
		case PageType::InteriorIndex:
		case PageType::InteriorTable:
			break;
		case PageType::LeafIndex:
		case PageType::LeafTable:
			break;
		case PageType::None:
			break;
		}
		return true;
	})) {
		dropPage(root);
	}
}

uint32_t Storage::createTree(PageType t) {
	auto page = allocatePage();
	openPageForWriting(page, [&] (void *mem, size_t) {
		auto h = (TreePageHeader *)mem;
		h->type = stappler::toInt(t);
		h->reserved = 0;
		h->ncells = 0;
		h->root = 0;
		h->prev = 0;
		h->next = 0;
		return true;
	}, 1);
	return page;
}

NS_MDB_END
