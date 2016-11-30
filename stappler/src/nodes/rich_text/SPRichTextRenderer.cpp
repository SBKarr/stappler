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
#include "SPRichTextRenderer.h"
#include "SPRichTextDocument.h"

#include "SPFilesystem.h"
#include "SPThread.h"
#include "SPAsset.h"
#include "SPResource.h"

#include "2d/CCNode.h"

#include "SPRichTextBuilder.h"
#include "SPRichTextResult.h"
#include "SPString.h"

NS_SP_EXT_BEGIN(rich_text)

static constexpr const float PAGE_SPLIT_COEF = (4.0f / 3.0f);
static constexpr const float PAGE_SPLIT_WIDTH = (800.0f);

Renderer::~Renderer() {
	if (_drawer) {
		_drawer->free();
	}
}

bool Renderer::init(const Vector<String> &ids) {
	if (!Component::init()) {
		return false;
	}

	_drawer = Rc<Drawer>::create();

	_source.setCallback(std::bind(&Renderer::onSource, this));

	_ids = ids;

	pushVersionOptions();
	_media.density = screen::density();
	_media.dpi = screen::dpi();
	return true;
}

void Renderer::onVisit(cocos2d::Renderer *r, const Mat4& t, uint32_t f, const ZPath &zPath) {
	Component::onVisit(r, t, f, zPath);
	if (_renderingDirty && !_renderingInProgress && _enabled && _source) {
		requestRendering();
	}
	if (_drawer) {
		_drawer->update();
	}
}

void Renderer::setSource(Source *source) {
	if (_source != source) {
		_source = source;
		_renderingDirty = true;
	}
}

Renderer::Source *Renderer::getSource() const {
	return _source;
}

Document *Renderer::getDocument() const {
	if (_result) {
		return _result->getDocument();
	} else if (auto s = getSource()) {
		return s->getDocument();
	}
	return nullptr;
}

Result *Renderer::getResult() const {
	return _result;
}

Drawer *Renderer::getDrawer() const {
	return _drawer;
}

MediaResolver Renderer::getMediaResolver(const Vector<String> &opts) const {
	if (auto doc = getDocument()) {
		if (_result) {
			return MediaResolver(doc, _result->getMedia(), opts);
		} else {
			return MediaResolver(doc, _media, opts);
		}
	}
	return MediaResolver();
}

void Renderer::onContentSizeDirty() {
	_isPageSplitted = false;
	if (hasFlag(RenderFlag::PaginatedLayout)) {
		Size s = _owner->getContentSize();
		float coef = s.width / s.height;
		if (coef >= PAGE_SPLIT_COEF && s.width > PAGE_SPLIT_WIDTH) {
			s.width /= 2.0f;
			_isPageSplitted = true;
		}
		s.width -= _pageMargin.horizontal();
		s.height -= _pageMargin.vertical();
		setSurfaceSize(s);
	} else {
		setSurfaceSize(_owner->getContentSize());
	}
}

void Renderer::setSurfaceSize(const Size &size) {
	if (!_surfaceSize.equals(size)) {
		if (hasFlag(RenderFlag::PaginatedLayout) || !hasFlag(RenderFlag::NoHeightCheck) || size.width != _surfaceSize.width) {
			_renderingDirty = true;
		}
		_surfaceSize = size;
	}
	if (!_media.surfaceSize.equals(_surfaceSize)) {
		_media.surfaceSize = _surfaceSize;
	}
}

const Size &Renderer::getSurfaceSize() const {
	return _surfaceSize;
}

const Margin & Renderer::getPageMargin() const {
	return _pageMargin;
}

bool Renderer::isPageSplitted() const {
	return _isPageSplitted;
}

void Renderer::setDpi(int dpi) {
	if (_media.dpi != dpi) {
		_media.dpi = dpi;
		_renderingDirty = true;
	}
}
void Renderer::setDensity(float density) {
	if (_media.density != density) {
		_media.density = density;
		_renderingDirty = true;
	}
}

void Renderer::setMediaType(style::MediaType value) {
	if (_media.mediaType != value) {
		_media.mediaType = value;
		_renderingDirty = true;
	}
}
void Renderer::setOrientationValue(style::Orientation value) {
	if (_media.orientation != value) {
		_media.orientation = value;
		_renderingDirty = true;
	}
}
void Renderer::setPointerValue(style::Pointer value) {
	if (_media.pointer != value) {
		_media.pointer = value;
		_renderingDirty = true;
	}
}
void Renderer::setHoverValue(style::Hover value) {
	if (_media.hover != value) {
		_media.hover = value;
		_renderingDirty = true;
	}
}
void Renderer::setLightLevelValue(style::LightLevel value) {
	if (_media.lightLevel != value) {
		_media.lightLevel = value;
		_renderingDirty = true;
	}
}
void Renderer::setScriptingValue(style::Scripting value) {
	if (_media.scripting != value) {
		_media.scripting = value;
		_renderingDirty = true;
	}
}
void Renderer::setHyphens(font::HyphenMap *map) {
	_hyphens = map;
}

void Renderer::setPageMargin(const Margin &margin) {
	if (_pageMargin != margin) {
		_pageMargin = margin;
		if (hasFlag(RenderFlag::PaginatedLayout)) {
			_renderingDirty = true;
		}
	}
}

void Renderer::addOption(const String &str) {
	_media.addOption(str);
	_renderingDirty = true;
}

void Renderer::removeOption(const String &str) {
	_media.removeOption(str);
	_renderingDirty = true;
}

bool Renderer::hasOption(const String &str) const {
	return _media.hasOption(str);
}

void Renderer::addFlag(RenderFlag::Flag flag) {
	_media.flags |= (RenderFlag::Mask)flag;
	_renderingDirty = true;
}
void Renderer::removeFlag(RenderFlag::Flag flag) {
	_media.flags &= ~ (RenderFlag::Mask)flag;
	_renderingDirty = true;
}
bool Renderer::hasFlag(RenderFlag::Flag flag) const {
	return _media.flags & (RenderFlag::Mask)flag;
}

void Renderer::onSource() {
	auto s = getSource();
	if (s && s->isReady()) {
		_drawer->clearCache();
		_renderingDirty = true;
		requestRendering();
	}
}
bool Renderer::requestRendering() {
	auto s = getSource();
	if (!_enabled || _renderingInProgress || _surfaceSize.equals(cocos2d::Size::ZERO) || !s) {
		return false;
	}
	font::Source *fontSet = nullptr;
	Document *document = nullptr;
	if (s->isReady()) {
		fontSet = s->getSource();
		document = s->getDocument();
	}

	if (fontSet && document) {
		_media.fontScale = s->getFontScale();
		Builder * impl = new Builder(document, _media, fontSet, _ids);
		impl->setHyphens(_hyphens);
		_renderingInProgress = true;
		if (_renderingCallback) {
			_renderingCallback(nullptr, true);
		}

		retain();
		auto &thread = resource::thread();
		thread.perform([impl] (cocos2d::Ref *) -> bool {
			impl->render();
			return true;
		}, [this, impl] (cocos2d::Ref *, bool) {
			auto result = impl->getResult();
			if (result) {
				onResult(result);
			}
			release();
			delete impl;
		}, this);

		_renderingDirty = false;
	}
	return false;
}

void Renderer::onResult(Result * result) {
	_renderingInProgress = false;
	if (_renderingDirty) {
		requestRendering();
	} else {
		_result = result;
	}

	if (!_renderingInProgress && _renderingCallback) {
		_renderingCallback(_result, false);
	}
}

void Renderer::setRenderingCallback(const RenderingCallback &cb) {
	_renderingCallback = cb;
}

void Renderer::pushVersionOptions() {
	auto v = RTEngineVersion();
	for (uint32_t i = 0; i <= v; i++) {
		addOption(toString("stappler-v", i, "-plus"));
	}
	addOption(toString("stappler-v", v));
}

NS_SP_EXT_END(rich_text)
