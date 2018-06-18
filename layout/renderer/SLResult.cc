// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SLResult.h"
#include "SLDocument.h"
#include "SPFilesystem.h"
#include "SPString.h"

NS_LAYOUT_BEGIN

bool Result::init(const MediaParameters &media, FontSource *cfg, Document *doc) {
	_media = media;
	_fontSet = cfg;
	_document = doc;
	_size = _media.surfaceSize;

	return true;
}

FontSource *Result::getFontSet() const {
	return _fontSet;
}

const MediaParameters &Result::getMedia() const {
	return _media;
}

Document *Result::getDocument() const {
	return _document;
}

void Result::pushIndex(const String &str, const Vec2 &pos) {
	_index.emplace(str, pos);
}

void Result::processContents(const Document::ContentRecord & rec, size_t level) {
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
			_bounds.emplace_back(BoundIndex{_bounds.size(), level, pos, _size.height, page, rec.label, rec.href});
		}
	}

	for (auto &it : rec.childs) {
		processContents(it, level + 1);
	}
}

void Result::finalize() {
	if (_media.flags & RenderFlag::NoHeightCheck) {
		_numPages = 1;
	} else {
		_numPages = size_t(ceilf(_size.height / _media.surfaceSize.height));
	}

	auto & toc = _document->getTableOfContents();

	_bounds.emplace_back(BoundIndex{0, 0, 0.0f, _size.height, 0,
		toc.label.empty() ? _document->getMeta("title") : toc.label,
		toc.href});

	for (auto &it : toc.childs) {
		processContents(it, 1);
	}
}

void Result::setBackgroundColor(const Color4B &c) {
	_background = c;
}
const Color4B & Result::getBackgroundColor() const {
	return _background;
}

void Result::setContentSize(const Size &s) {
	_size = s;
	_size.width = _media.surfaceSize.width;

	if (_media.flags & RenderFlag::NoHeightCheck) {
		_media.surfaceSize.height = _size.height;
	}

	_numPages = size_t(ceilf(_size.height / _media.surfaceSize.height)) + 1;
}

const Size &Result::getContentSize() const {
	return _size;
}

const Size &Result::getSurfaceSize() const {
	return _media.surfaceSize;
}

const Vector<Object *> & Result::getObjects() const {
	return _objects;
}

const Vector<Link *> & Result::getRefs() const {
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
			return BoundIndex{maxOf<size_t>(), 0, 0.0f, 0.0f, maxOf<int64_t>()};
		}
		return *it;
	}
	return BoundIndex{maxOf<size_t>(), 0, 0.0f, 0.0f, maxOf<int64_t>()};
}

Label *Result::emplaceLabel(const Layout &l, bool isBullet) {
	auto ret = &_labels.emplace();
	ret->type = Object::Type::Label;
	ret->depth = l.depth;
	ret->index = _objects.size();

	if (!isBullet) {
		auto &node = l.node.node;
		if (auto hashPtr = node->getAttribute("x-data-hash")) {
			ret->hash = addString(*hashPtr);
		}

		if (auto indexPtr = node->getAttribute("x-data-index")) {
			StringView r(*indexPtr);
			r.readInteger().unwrap([&] (int64_t val) {
				ret->sourceIndex = size_t(val);
			});
		}
	}

	_objects.push_back(ret);
	return ret;
}

Background *Result::emplaceBackground(const Layout &l, const Rect &rect, const BackgroundStyle &style) {
	auto ret = &_backgrounds.emplace();
	ret->type = Object::Type::Background;
	ret->depth = l.depth;
	ret->bbox = rect;
	ret->background = style;
	ret->background.backgroundImage = addString(ret->background.backgroundImage);
	ret->index = _objects.size();
	_objects.push_back(ret);
	return ret;
}

Link *Result::emplaceLink(const Layout &l, const Rect &rect, const StringView &href, const StringView &target) {
	auto ret = &_links.emplace();
	ret->type = Object::Type::Link;
	ret->depth = l.depth;
	ret->bbox = rect;
	ret->target = addString(href);
	ret->mode = addString(target);
	ret->index = _refs.size();
	_refs.push_back(ret);
	return ret;
}

PathObject *Result::emplaceOutline(const Layout &l, const Rect &rect, const Color4B &color, float width, style::BorderStyle style) {
	auto ret = &_paths.emplace();
	ret->type = Object::Type::Path;
	ret->depth = l.depth;
	ret->bbox = rect;
	ret->drawOutline(rect, color, width, style);
	ret->index = _objects.size();
	_objects.push_back(ret);
	return ret;
}

void Result::emplaceBorder(Layout &l, const Rect &rect, const OutlineStyle &style, float width) {
	PathObject::makeBorder(this, l, rect, style, width, _media);
}

PathObject *Result::emplacePath(const Layout &l) {
	auto ret = &_paths.emplace();
	ret->type = Object::Type::Path;
	ret->depth = l.depth;
	ret->index = _objects.size();
	_objects.push_back(ret);
	return ret;
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

const Object *Result::getObject(size_t size) const {
	if (size < _objects.size()) {
		return _objects[size];
	}
	return nullptr;
}

const Label *Result::getLabelByHash(const StringView &hash, size_t idx) const {
	const Label *indexed = nullptr;
	for (auto &it : _objects) {
		if (auto l = it->asLabel()) {
			if (l->hash == hash) {
				return l;
			} else if (l->sourceIndex == idx) {
				indexed = l;
			}
		}
	}
	return indexed;
}

const Map<CssStringId, String> &Result::getStrings() const {
	return _strings;
}

StringView Result::addString(CssStringId id, const StringView &str) {
	auto it = _strings.emplace(id, str.str());
	return it.first->second;
}

StringView Result::addString(const StringView &str) {
	return addString(hash::hash32(str.data(), str.size()), str);
}

NS_LAYOUT_END
