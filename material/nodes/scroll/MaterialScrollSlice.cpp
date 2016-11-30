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
#include "MaterialIconSprite.h"
#include "MaterialScrollSlice.h"
#include "SPThread.h"

#include "SPScrollController.h"
#include "SPRoundedSprite.h"

#include "2d/CCActionInterval.h"
#include "2d/CCActionInstant.h"

#include <unistd.h>

NS_MD_BEGIN

bool ScrollHandlerSlice::init(Scroll *s, const DataCallback &cb) {
	if (!Handler::init(s)) {
		return false;
	}

	auto &items = s->getItems();
	if (!items.empty()) {
		_originFront = items.begin()->second->getPosition();
		_originBack = items.rbegin()->second->getPosition();
		_originBack.y += items.rbegin()->second->getContentSize().height;
	}

	_dataCallback = cb;

	return true;
}

void ScrollHandlerSlice::setDataCallback(const DataCallback &cb) {
	_dataCallback = cb;
}

Scroll::ItemMap ScrollHandlerSlice::run(Request t, DataMap &&data) {
	Scroll::ItemMap ret;

	auto origin = getOrigin(t);

	if (t == Scroll::Request::Front) {
		for (auto it = data.rbegin(); it != data.rend(); it ++) {
			auto item = onItem(std::move(it->second), origin);

			item->setPosition(
					(_layout == Scroll::Layout::Vertical)
						?Vec2(origin.x, origin.y - item->getContentSize().height)
						:Vec2(origin.x - item->getContentSize().height, origin.y));

			if (_layout == Scroll::Layout::Vertical) {
				origin.y -= item->getContentSize().height;
			} else {
				origin.x -= item->getContentSize().width;
			}

			ret.insert(std::make_pair(it->first, item));
		}
	} else {
		for (auto &it : data) {
			auto item = onItem(std::move(it.second), origin);

			if (_layout == Scroll::Layout::Vertical) {
				origin.y += item->getContentSize().height;
			} else {
				origin.x += item->getContentSize().width;
			}

			ret.insert(std::make_pair(it.first, item));
		}
	}
	return ret;
}

cocos2d::Vec2 ScrollHandlerSlice::getOrigin(Request t) const {
	Vec2 origin = Vec2(0, 0);

	switch (t) {
	case Scroll::Request::Reset:
		origin = Vec2(0, 0);
		break;
	case Scroll::Request::Update:
	case Scroll::Request::Front:
		origin = _originFront;
		break;
	case Scroll::Request::Back:
		origin = _originBack;
		break;
	}

	return origin;
}

Rc<Scroll::Item> ScrollHandlerSlice::onItem(data::Value &&d, const Vec2 &o) {
	if (_dataCallback) {
		return _dataCallback(this, std::move(d), o);
	}
	return nullptr;
}

NS_MD_END
