// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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
