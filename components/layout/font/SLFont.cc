// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPLayout.h"
#include "SLFont.h"
#include "SPFilesystem.h"

NS_LAYOUT_BEGIN

constexpr uint16_t LAYOUT_PADDING  = 1;

struct UVec2 {
	uint16_t x;
	uint16_t y;
};

struct URect {
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;

	UVec2 origin() const { return UVec2{x, y}; }
};

void FontCharString::addChar(char16_t c) {
	auto it = std::lower_bound(chars.begin(), chars.end(), c);
	if (it == chars.end() || *it != c) {
		chars.insert(it, c);
	}
}

void FontCharString::addString(const String &str) {
	addString(string::toUtf16(str));
}

void FontCharString::addString(const WideString &str) {
	addString(str.data(), str.size());
}

void FontCharString::addString(const char16_t *str, size_t len) {
	for (size_t i = 0; i < len; ++ i) {
		const char16_t &c = str[i];
		auto it = std::lower_bound(chars.begin(), chars.end(), c);
		if (it == chars.end() || *it != c) {
			chars.insert(it, c);
		}
	}
}

struct LayoutNodeMemory;

struct LayoutNodeMemoryStorage {
	const EmplaceCharInterface *interface = nullptr;
	LayoutNodeMemory *free = nullptr;
	memory::pool_t *pool = nullptr;

	LayoutNodeMemoryStorage(const EmplaceCharInterface *, memory::pool_t *);

	LayoutNodeMemory *alloc(const URect &rect);
	LayoutNodeMemory *alloc(const UVec2 &origin, void *c);
	void release(LayoutNodeMemory *node);
};

struct LayoutNodeMemory : memory::AllocPool {
	LayoutNodeMemory* _child[2];
	URect _rc;
	void * _char = nullptr;

	LayoutNodeMemory(const URect &rect);
	LayoutNodeMemory(const LayoutNodeMemoryStorage &, const UVec2 &origin, void *c);
	~LayoutNodeMemory();

	bool insert(LayoutNodeMemoryStorage &, void *c);
	size_t nodes() const;
	void finalize(LayoutNodeMemoryStorage &, uint8_t tex);
};

LayoutNodeMemoryStorage::LayoutNodeMemoryStorage(const EmplaceCharInterface *i, memory::pool_t *p)
: interface(i), pool(p) { }

LayoutNodeMemory *LayoutNodeMemoryStorage::alloc(const URect &rect) {
	if (free) {
		auto node = free;
		free = node->_child[0];
		return new (node) LayoutNodeMemory(rect);
	}
	return new (pool) LayoutNodeMemory(rect);
}

LayoutNodeMemory *LayoutNodeMemoryStorage::alloc(const UVec2 &origin, void *c) {
	if (free) {
		auto node = free;
		free = node->_child[0];
		return new (node) LayoutNodeMemory(*this, origin, c);
	}
	return new (pool) LayoutNodeMemory(*this, origin, c);
}

void LayoutNodeMemoryStorage::release(LayoutNodeMemory *node) {
	node->_child[0] = nullptr;
	node->_child[1] = nullptr;
	node->_rc = URect({0, 0, 0, 0});
	node->_char = nullptr;
	if (free) {
		node->_child[0] = free;
	}
	free = node;
}

LayoutNodeMemory::LayoutNodeMemory(const URect &rect) {
	_rc = rect;
	_child[0] = nullptr;
	_child[1] = nullptr;
}

LayoutNodeMemory::LayoutNodeMemory(const LayoutNodeMemoryStorage &storage, const UVec2 &origin, void *c) {
	_child[0] = nullptr;
	_child[1] = nullptr;
	_char = c;
	_rc = URect{origin.x, origin.y, storage.interface->getWidth(c), storage.interface->getHeight(c)};
}

LayoutNodeMemory::~LayoutNodeMemory() { }

bool LayoutNodeMemory::insert(LayoutNodeMemoryStorage &storage, void *c) {
	if (_child[0] && _child[1]) {
		return _child[0]->insert(storage, c) || _child[1]->insert(storage, c);
	} else {
		if (_char) {
			return false;
		}

		const auto iwidth = storage.interface->getWidth(c);
		const auto iheight = storage.interface->getHeight(c);

		if (_rc.width < iwidth || _rc.height < iheight) {
			return false;
		}

		if (_rc.width == iwidth || _rc.height == iheight) {
			if (_rc.height == iheight) {
				_child[0] = storage.alloc(_rc.origin(), c); // new (pool) LayoutNode(_rc.origin(), c);
				_child[1] = storage.alloc(URect{uint16_t(_rc.x + iwidth + LAYOUT_PADDING), _rc.y,
					(_rc.width > iwidth + LAYOUT_PADDING)?uint16_t(_rc.width - iwidth - LAYOUT_PADDING):uint16_t(0), _rc.height});
			} else if (_rc.width == iwidth) {
				_child[0] = storage.alloc(_rc.origin(), c); // new (pool) LayoutNode(_rc.origin(), c);
				_child[1] = storage.alloc(URect{_rc.x, uint16_t(_rc.y + iheight + LAYOUT_PADDING),
					_rc.width, (_rc.height > iheight + LAYOUT_PADDING)?uint16_t(_rc.height - iheight - LAYOUT_PADDING):uint16_t(0)});
			}
			return true;
		}

		//(decide which way to split)
		int16_t dw = _rc.width - iwidth;
		int16_t dh = _rc.height - iheight;

		if (dw > dh) {
			_child[0] = storage.alloc(URect{_rc.x, _rc.y, iwidth, _rc.height});
			_child[1] = storage.alloc(URect{uint16_t(_rc.x + iwidth + LAYOUT_PADDING), _rc.y, uint16_t(dw - LAYOUT_PADDING), _rc.height});
		} else {
			_child[0] = storage.alloc(URect{_rc.x, _rc.y, _rc.width, iheight});
			_child[1] = storage.alloc(URect{_rc.x, uint16_t(_rc.y + iheight + LAYOUT_PADDING), _rc.width, uint16_t(dh - LAYOUT_PADDING)});
		}

		_child[0]->insert(storage, c);

		return true;
	}
}

size_t LayoutNodeMemory::nodes() const {
	if (_char) {
		return 1;
	} else if (_child[0] && _child[1]) {
		return _child[0]->nodes() + _child[1]->nodes();
	} else {
		return 0;
	}
}

void LayoutNodeMemory::finalize(LayoutNodeMemoryStorage &storage, uint8_t tex) {
	if (_char) {
		storage.interface->setX(_char, _rc.x);
		storage.interface->setY(_char, _rc.y);
		storage.interface->setTex(_char, tex);
	} else {
		if (_child[0]) { _child[0]->finalize(storage, tex); }
		if (_child[1]) { _child[1]->finalize(storage, tex); }
	}
	 storage.release(this);
}

Extent2 emplaceChars(const EmplaceCharInterface &iface, const SpanView<void *> &layoutData, float totalSquare) {
	if (std::isnan(totalSquare)) {
		totalSquare = 0.0f;
		for (auto &it : layoutData) {
			totalSquare += iface.getWidth(it) * iface.getHeight(it);
		}
	}

	// find potential best match square
	bool s = true;
	uint16_t h = 128, w = 128; uint32_t sq2 = h * w;
	for (; sq2 < totalSquare; sq2 *= 2, s = !s) {
		if (s) { w *= 2; } else { h *= 2; }
	}

	LayoutNodeMemoryStorage storage(&iface, memory::pool::acquire());

	while (true) {
		auto l = storage.alloc(URect{0, 0, w, h});
		for (auto &it : layoutData) {
			if (!l->insert(storage, it)) {
				break;
			}
		}

		auto nodes = l->nodes();
		if (nodes == layoutData.size()) {
			l->finalize(storage, 0);
			storage.release(l);
			break;
		} else {
			if (s) { w *= 2; } else { h *= 2; }
			sq2 *= 2; s = !s;
		}
		storage.release(l);
	}

	return Extent2(w, h);
}

NS_LAYOUT_END
