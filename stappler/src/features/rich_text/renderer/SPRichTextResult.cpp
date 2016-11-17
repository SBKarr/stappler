/*
 * SPRichTextRendererResult.cpp
 *
 *  Created on: 02 мая 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPRichTextResult.h"
#include "SPRichTextDocument.h"
#include "SPRichTextRenderer.h"

#include "SPFont.h"
#include "SPString.h"
#include "SPFilesystem.h"
#include "SPThread.h"
#include "SPScreen.h"

#include "SPTextureFBO.h"

#include "renderer/CCTexture2D.h"

NS_SP_EXT_BEGIN(rich_text)

Result::Result(const MediaParameters &media, font::Source *cfg, Document *doc) {
	_media = media;
	_fontSet = cfg;
	_document = doc;
	_size = _media.surfaceSize;
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

void Result::finalize() {
	if (_media.flags & RenderFlag::NoHeightCheck) {
		_numPages = 1;
	} else {
		_numPages = size_t(ceilf(_size.height / _media.surfaceSize.height));
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

size_t Result::getNumPages() const {
	return _numPages;
}

Result::PageData Result::getPageData(Renderer *r, size_t idx, float offset) const {
	auto surfaceSize = getSurfaceSize();
	if (_media.flags & RenderFlag::PaginatedLayout) {
		auto &m = r->getPageMargin();
		bool isSplitted = r->isPageSplitted();
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

NS_SP_EXT_END(rich_text)
