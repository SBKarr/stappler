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
#include "SPScrollViewBase.h"

#include "SPThread.h"
#include "SPAsset.h"
#include "SPResource.h"
#include "SPFilesystem.h"
#include "SPString.h"

#include "2d/CCNode.h"
#include "RTRenderer.h"
#include "RTCommonSource.h"

#include "SLBuilder.h"
#include "SLResult.h"

NS_RT_BEGIN

static constexpr const float PAGE_SPLIT_COEF = (4.0f / 3.0f);
static constexpr const float PAGE_SPLIT_WIDTH = (800.0f);

bool Renderer::shouldSplitPages(const Size &s) {
	float coef = s.width / s.height;
	if (coef >= PAGE_SPLIT_COEF && s.width > PAGE_SPLIT_WIDTH) {
		return true;
	}
	return false;
}

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

void Renderer::setSource(CommonSource *source) {
	if (_source != source) {
		_source = source;
		_renderingDirty = true;
	}
}

CommonSource *Renderer::getSource() const {
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

MediaParameters Renderer::getMedia() const {
	return _media;
}

void Renderer::onContentSizeDirty() {
	_isPageSplitted = false;
	auto size = _owner->getContentSize();
	auto scroll = dynamic_cast<ScrollViewBase *>(_owner);
	if (scroll) {
		auto &padding = scroll->getPadding();
		if (scroll->isVertical()) {
			size.width -= padding.horizontal();
		}
	}
	if (hasFlag(layout::RenderFlag::PaginatedLayout)) {
		if (shouldSplitPages(size)) {
			size.width /= 2.0f;
			_isPageSplitted = true;
		}
		size.width -= _pageMargin.horizontal();
		size.height -= _pageMargin.vertical();
		setSurfaceSize(size);
	} else {
		setSurfaceSize(size);
	}
}

void Renderer::setSurfaceSize(const Size &size) {
	if (!_surfaceSize.equals(size)) {
		if (hasFlag(layout::RenderFlag::PaginatedLayout) || !hasFlag(layout::RenderFlag::NoHeightCheck) || size.width != _surfaceSize.width) {
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

void Renderer::setDefaultBackground(const Color4B &c) {
	if (_media.defaultBackground != c) {
		_media.defaultBackground = c;
		_renderingDirty = true;
	}
}

void Renderer::setMediaType(layout::style::MediaType value) {
	if (_media.mediaType != value) {
		_media.mediaType = value;
		_renderingDirty = true;
	}
}
void Renderer::setOrientationValue(layout::style::Orientation value) {
	if (_media.orientation != value) {
		_media.orientation = value;
		_renderingDirty = true;
	}
}
void Renderer::setPointerValue(layout::style::Pointer value) {
	if (_media.pointer != value) {
		_media.pointer = value;
		_renderingDirty = true;
	}
}
void Renderer::setHoverValue(layout::style::Hover value) {
	if (_media.hover != value) {
		_media.hover = value;
		_renderingDirty = true;
	}
}
void Renderer::setLightLevelValue(layout::style::LightLevel value) {
	if (_media.lightLevel != value) {
		_media.lightLevel = value;
		_renderingDirty = true;
	}
}
void Renderer::setScriptingValue(layout::style::Scripting value) {
	if (_media.scripting != value) {
		_media.scripting = value;
		_renderingDirty = true;
	}
}

void Renderer::setPageMargin(const Margin &margin) {
	if (_pageMargin != margin) {
		_pageMargin = margin;
		if (hasFlag(layout::RenderFlag::PaginatedLayout)) {
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

void Renderer::addFlag(layout::RenderFlag::Flag flag) {
	_media.flags |= (layout::RenderFlag::Mask)flag;
	_renderingDirty = true;
}
void Renderer::removeFlag(layout::RenderFlag::Flag flag) {
	_media.flags &= ~ (layout::RenderFlag::Mask)flag;
	_renderingDirty = true;
}
bool Renderer::hasFlag(layout::RenderFlag::Flag flag) const {
	return _media.flags & (layout::RenderFlag::Mask)flag;
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
	FontSource *fontSet = nullptr;
	Document *document = nullptr;
	if (s->isReady()) {
		fontSet = s->getSource();
		document = s->getDocument();
	}

	if (fontSet && document) {
		auto media = _media;
		media.fontScale = s->getFontScale();
		media.pageMargin = _pageMargin;
		if (_isPageSplitted) {
			media.flags |= layout::RenderFlag::SplitPages;
		}

		layout::Builder * impl = new layout::Builder(document, media, fontSet, _ids);
		impl->setExternalAssetsMeta(s->getExternalAssetMeta());
		impl->setHyphens(s->getHyphens());
		_renderingInProgress = true;
		if (_renderingCallback) {
			_renderingCallback(nullptr, true);
		}

		retain();
		auto &thread = resource::thread();
		thread.perform([impl] (const Task &) -> bool {
			impl->render();
			return true;
		}, [this, impl] (const Task &, bool) {
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

StringView Renderer::getLegacyBackground(const layout::Node &node, const StringView &opt) const {
	if (auto doc = _source->getDocument()) {
		auto root = doc->getRoot();

		layout::Style style;
		auto media = _media;
		media.addOption(opt.str());

		auto resolved = media.resolveMediaQueries(root->queries);

		layout::Builder::compileNodeStyle(style, root, node, Vector<const layout::Node *>(), media, resolved);
		layout::SimpleRendererInterface iface(&resolved, &root->strings);
		auto bg = style.compileBackground(&iface);

		return bg.backgroundImage;
	}
	return StringView();
}

void Renderer::pushVersionOptions() {
	auto v = layout::EngineVersion();
	for (uint32_t i = 0; i <= v; i++) {
		addOption(toString("stappler-v", i, "-plus"));
	}
	addOption(toString("stappler-v", v));
}

NS_RT_END
