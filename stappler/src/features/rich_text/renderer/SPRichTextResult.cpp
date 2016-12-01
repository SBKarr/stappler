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

#include "SPDefine.h"
#include "SPRichTextResult.h"
#include "SPRichTextDocument.h"
#include "SPRichTextRenderer.h"

#include "SPFont.h"
#include "SPString.h"
#include "SPFilesystem.h"
#include "SPThread.h"
#include "SPScreen.h"

#include "renderer/CCTexture2D.h"

NS_SP_EXT_BEGIN(rich_text)

bool Result::init(const MediaParameters &media, font::Source *cfg, Document *doc) {
	_media = media;
	_fontSet = cfg;
	_document = doc;
	_size = _media.surfaceSize;
	return true;
}

font::Source *Result::getFontSet() const {
	return _fontSet;
}

const MediaParameters &Result::getMedia() const {
	return _media;
}

Document *Result::getDocument() const {
	return _document;
}

void Result::pushObject(Object &&obj) {
	if (obj.bbox.origin.y + obj.bbox.size.height > _size.height) {
		_size.height = obj.bbox.origin.y + obj.bbox.size.height;
	}
	if (obj.type == Object::Type::Ref) {
		_refs.push_back(std::move(obj));
	} else {
		_objects.push_back(std::move(obj));
	}
}

void Result::pushIndex(const String &str, const Vec2 &pos) {
	_index.emplace(str, pos);
}

void Result::processContents(const Document::ContentRecord & rec) {
	auto it = _index.find(rec.href);
	if (it != _index.end()) {
		auto pos = it->second.y;
		if (!_bounds.empty()) {
			_bounds.back().end = pos;
		}

		int64_t page = maxOf<int64_t>();
		if (!isnan(pos) && (_media.flags & RenderFlag::PaginatedLayout)) {
			page = int64_t(roundf(pos / _media.surfaceSize.height));
		}

		if (!rec.label.empty()) {
			_bounds.emplace_back(BoundIndex{_bounds.size(), pos, _size.height, page, rec.label, rec.href});
		}
	}

	for (auto &it : rec.childs) {
		processContents(it);
	}
}

void Result::finalize() {
	if (_media.flags & RenderFlag::NoHeightCheck) {
		_numPages = 1;
	} else {
		_numPages = size_t(ceilf(_size.height / _media.surfaceSize.height));
	}

	auto & toc = _document->getTableOfContents();

	_bounds.emplace_back(BoundIndex{0, 0.0f, _size.height, 0, toc.label, toc.href});

	for (auto &it : toc.childs) {
		processContents(it);
	}
}

void Result::setBackgroundColor(const cocos2d::Color4B &c) {
	_background = c;
}
const cocos2d::Color4B & Result::getBackgroundColor() const {
	return _background;
}

void Result::setContentSize(const cocos2d::Size &s) {
	_size = s;
	_size.width = _media.surfaceSize.width;

	if (_media.flags & RenderFlag::NoHeightCheck) {
		_media.surfaceSize.height = _size.height;
	}

	_numPages = size_t(ceilf(_size.height / _media.surfaceSize.height)) + 1;
}
const cocos2d::Size &Result::getContentSize() const {
	return _size;
}

const Size &Result::getSurfaceSize() const {
	return _media.surfaceSize;
}

const Vector<Object> & Result::getObjects() const {
	return _objects;
}

const Vector<Object> & Result::getRefs() const {
	return _refs;
}

const Map<String, Vec2> & Result::getIndex() const {
	return _index;
}

const Vector<Result::BoundIndex> & Result::getBounds() const {
	return _bounds;
}

size_t Result::getNumPages() const {
	return _numPages;
}

Result::BoundIndex Result::getBoundsForPosition(float pos) const {
	if (!_bounds.empty()) {
		auto it = std::lower_bound(_bounds.begin(), _bounds.end(), pos, [] (const BoundIndex &idx, float pos) {
			return idx.end < pos;
		});
		if (it != _bounds.end() && fabs(pos - it->end) < std::numeric_limits<float>::epsilon()) {
			++ it;
		}
		if (it == _bounds.end()) {
			return _bounds.back();
		}
		if (it->start > pos) {
			return BoundIndex{maxOf<size_t>(), 0.0f, 0.0f, maxOf<int64_t>()};
		}
		return *it;
	}
	return BoundIndex{maxOf<size_t>(), 0.0f, 0.0f, maxOf<int64_t>()};
}

Result::PageData Result::getPageData(size_t idx, float offset) const {
	auto surfaceSize = getSurfaceSize();
	if (_media.flags & RenderFlag::PaginatedLayout) {
		auto m = _media.pageMargin;
		bool isSplitted = _media.flags & RenderFlag::SplitPages;
		auto pageSize = Size(surfaceSize.width + m.horizontal(), surfaceSize.height + m.vertical());
		return PageData{m, Rect(pageSize.width * idx, 0, pageSize.width, pageSize.height),
				Rect(0, surfaceSize.height * idx, surfaceSize.width, surfaceSize.height),
				idx, isSplitted};
	} else {
		if (idx == _numPages - 1) {
			return PageData{Margin(), Rect(0, surfaceSize.height * idx + offset, surfaceSize.width, _size.height - surfaceSize.height * idx),
					Rect(0, surfaceSize.height * idx, surfaceSize.width, _size.height - surfaceSize.height * idx),
					idx, false};
		} else if (idx >= _numPages - 1) {
			return PageData{Margin(), Rect::ZERO, Rect::ZERO, idx, false};
		} else {
			return PageData{Margin(), Rect(0, surfaceSize.height * idx + offset, surfaceSize.width, surfaceSize.height),
					Rect(0, surfaceSize.height * idx, surfaceSize.width, surfaceSize.height),
					idx, false};
		}
	}
}

size_t Result::getSizeInMemory() const {
	auto ret = sizeof(Result) + _objects.capacity() * sizeof(Object) + _refs.capacity() * sizeof(Object);
	for (const Object &it : _objects) {
		if (it.type == Object::Type::Label) {
			const auto &f = it.value.label._format;
			ret += (f.chars.capacity() * sizeof(font::CharSpec)
					+ f.lines.capacity() + sizeof(font::LineSpec)
					+ f.ranges.capacity() + sizeof(font::RangeSpec));
		}
	}
	return ret;
}

NS_SP_EXT_END(rich_text)
