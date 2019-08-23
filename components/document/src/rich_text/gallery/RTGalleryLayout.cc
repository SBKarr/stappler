// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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
#include "MaterialGalleryScroll.h"
#include "MaterialToolbar.h"
#include "MaterialScene.h"
#include "MaterialButtonLabelSelector.h"

#include "RTGalleryLayout.h"

#include "SPUrl.h"
#include "SPThread.h"
#include "SPTextureCache.h"

NS_RT_BEGIN

Thread s_galleryRendererThread("RichTextGalleryLayout");

bool GalleryLayout::init(CommonSource *source, const StringView &name, const StringView &sel) {
	if (!ToolbarLayout::init()) {
		return false;
	}

	_source = source;

	auto &g = source->getDocument()->getGalleryMap();
	auto it = g.find(name);
	if (it == g.end()) {
		return false;
	}

	auto images = it->second;
	size_t selected = 0;
	for (size_t i = 0; i < images.size(); ++ i) {
		StringView r(images[i]);
		std::string image = r.readUntil<StringView::Chars<'?'>>().str();
		if (!r.empty()) {
			data::Value args = Url::parseDataArgs(r.str());
			std::string title = args.getString("title");
			_title.emplace_back(title);
			log::text("Data", data::toString(args, true));
		} else {
			_title.emplace_back(std::string());
		}
		images[i] = image;
		if (sel == images[i]) {
			selected = i;
		}
	}

	setTitle(_title[selected]);

	auto scroll = Rc<material::GalleryScroll>::create(std::bind(&GalleryLayout::onImage, this, std::placeholders::_1, std::placeholders::_2),
			images, selected);
	scroll->setActionCallback([this] (material::GalleryScroll::Action a) {
		if (a == material::GalleryScroll::Tap) {
			if (getCurrentFlexibleHeight() == 0.0f) {
				setFlexibleLevelAnimated(1.0f, 0.35f);
			} else {
				setFlexibleLevelAnimated(0.0f, progress(0.0f, 0.35f, getFlexibleLevel()));
			}
		} else if (getCurrentFlexibleHeight() != 0.0f) {
			setFlexibleLevelAnimated(0.0f, progress(0.0f, 0.35f, getFlexibleLevel()));
		}
	});
	scroll->setPositionCallback([this] (float val) {
		onPosition(val);
	});
	_scroll = addChildNode(scroll, -1);

	return true;
}

void GalleryLayout::onPosition(float val) {
	auto n = (size_t)roundf(val);
	auto title = _title.at(n);
	if (!title.empty() && getTitle() != title) {
		setTitle(_title.at(n));
		log::text("SetTitle", title);
	}

	float diff = fabs(val - n) * 2.0f;
	auto btn =  static_cast<material::Toolbar *>(_toolbar)->getTitleNode();
	btn->setLabelOpacity((uint8_t)(diff * 255.0f));
}

void GalleryLayout::onContentSizeDirty() {
	ToolbarLayout::onContentSizeDirty();

	_scroll->setContentSize(getContentSize());
}

void GalleryLayout::onEnter() {
	ToolbarLayout::onEnter();
	setFlexibleLevel(0.0f);
}

void GalleryLayout::onForegroundTransitionBegan(material::ContentLayer *l, Layout *overlay) {
	ToolbarLayout::onForegroundTransitionBegan(l, overlay);
	if (auto t = dynamic_cast<ToolbarLayout *>(overlay)) {
		auto toolbar = t->getToolbar();
		setToolbarColor(toolbar->getBackgroundColor(), toolbar->getTextColor());
		setStatusBarBackgroundColor(t->getStatusBarBackgroundColor());
	}
}

void GalleryLayout::onPush(material::ContentLayer *l, bool replace) {
	auto prev = l->getPrevNode();
	if (auto t = dynamic_cast<ToolbarLayout *>(prev)) {
		auto toolbar = t->getToolbar();
		setToolbarColor(toolbar->getBackgroundColor(), toolbar->getTextColor());
		setStatusBarBackgroundColor(t->getStatusBarBackgroundColor());
	}
}

void GalleryLayout::onImage(const String &image, const Function<void(cocos2d::Texture2D *)> &tex) {
	_source->retainReadLock(this, std::bind(&GalleryLayout::onAssetCaptured, this, image, tex));
}

void GalleryLayout::onAssetCaptured(const String &imageLink, const Function<void(cocos2d::Texture2D *)> &cb) {
	StringView r(imageLink);
	std::string image = r.readUntil<StringView::Chars<'?'>>().str();
	if (!_source->isActual()) {
		cb(nullptr);
		_source->releaseReadLock(this);
		return;
	}

	Rc<cocos2d::Texture2D> *tex = new Rc<cocos2d::Texture2D>();
	s_galleryRendererThread.perform([this, image, tex] (const Task &) -> bool {
		auto doc = _source->getDocument();
		(*tex) = TextureCache::uploadTexture(doc->getImageData(image));
		return true;
	}, [this, tex, cb] (const Task &, bool) {
		_source->releaseReadLock(this);

		cb(*tex);

		delete tex;
	}, this);
}

NS_RT_END
