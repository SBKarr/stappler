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

#include "MaterialIconStorage.h"

#include "SPDevice.h"
#include "SPThread.h"
#include "SPDrawCanvas.h"
#include "SPTextureCache.h"
#include "base/CCDirector.h"
#include "base/CCScheduler.h"

NS_MD_BEGIN

SP_DECLARE_EVENT_CLASS(IconStorage, onUpdate);

layout::Path IconStorage::getIconPath(IconName n) {
	auto dataIt = std::lower_bound(s_iconTable, s_iconTable + s_iconCount, n, [] (const IconDataStruct &d, IconName n) -> bool {
		return d.name < n;
	});

	layout::Path path;
	path.init(dataIt->data, dataIt->len);
	path.setFillColor(Color4B(255, 255, 255, 255));
	return path;
}

IconStorage::IconStorage() { }

IconStorage::~IconStorage() {
	unschedule();
}

bool IconStorage::init(float d, uint16_t w, int16_t h) {
	_width = w;
	_height = h;
	_density = d;
	onEvent(Device::onAndroidReset, [this] (const Event &) {
        if (_texture) {
            _texture->init(cocos2d::Texture2D::PixelFormat::A8, _texture->getPixelsWide(), _texture->getPixelsHigh(),
                           cocos2d::Texture2D::InitAs::RenderTarget);
        }
		_dirty = true;
	});
	schedule();
	return true;
}

void IconStorage::addIcon(IconName name) {
	auto it = std::lower_bound(_names.begin(), _names.end(), name);
	if (it == _names.end() || *it != name) {
		_names.insert(it, name);
		_dirty = true;
	}
}

const IconStorage::Icon * IconStorage::getIcon(IconName name) const {
	auto it = std::lower_bound(_icons.begin(), _icons.end(), name, [] (const Icon &i, IconName n) -> bool {
		return i._name < n;
	});

	if (it == _icons.end() || it->_name != name) {
		return nullptr;
	}
	return &(*it);
}
cocos2d::Texture2D *IconStorage::getTexture() const {
	return _texture;
}

void IconStorage::update(float dt) {
	if (_dirty) {
		reload();
	}
}

static void IconStorage_drawIcons(cocos2d::Texture2D *tex, const Vector<IconName> &names, Vector<IconStorage::Icon> &icons, float density, uint16_t iwidth, uint16_t iheight) {
	size_t count = names.size();
	uint16_t originalWidth = 48;
	uint16_t originalHeight = 48;
	uint16_t iconWidth = uint16_t(iwidth * density);
	uint16_t iconHeight = uint16_t(iheight * density);

	float scaleX = (float)iconWidth / (float)originalWidth;
	float scaleY = (float)iconHeight / (float)originalHeight;

	uint32_t cols = tex->getPixelsWide() / iconWidth;
	uint32_t rows = (uint32_t)(names.size() / cols + 1);
	while (rows * cols < count) {
		rows ++;
	}

	auto canvas = Rc<draw::Canvas>::create();

	canvas->setQuality(draw::Canvas::QualityLow);
	canvas->begin(tex, Color4B(0, 0, 0, 0));
	canvas->scale(scaleX, scaleY);
	canvas->beginBatch();

	uint16_t c, r, i = 0;
	for (auto &it : names) {
		c = i % cols;
		r = i / cols;

		auto iconIt = std::lower_bound(icons.begin(), icons.end(), it, [] (const IconStorage::Icon &i, IconName n) -> bool {
			return i._name < n;
		});

		if (iconIt == icons.end() || iconIt->_name != it) {
			auto dataIt = std::lower_bound(s_iconTable, s_iconTable + s_iconCount, it, [] (const IconDataStruct &d, IconName n) -> bool {
				return d.name < n;
			});

			layout::Path path;
			path.init(dataIt->data, dataIt->len);
			path.setFillColor(Color4B(255, 255, 255, 255));

			iconIt = icons.emplace(iconIt, IconStorage::Icon(it, c * iconWidth, r * iconHeight, iconWidth, iconHeight, density, std::move(path)));
		} else {
			iconIt->_x = c * iconWidth;
			iconIt->_y = r * iconHeight;
		}

		canvas->draw(iconIt->_path, iconIt->_x / scaleX, iconIt->_y / scaleY);

		++ i;
	}

	canvas->endBatch();
	canvas->end();
	tex->setAntiAliasTexParameters();
}

static Rc<cocos2d::Texture2D> IconStorage_updateIcons(const Vector<IconName> &names, Vector<IconStorage::Icon> &icons, float density, uint16_t iwidth, uint16_t iheight) {
	//Time time = Time::now();
	size_t count = names.size();
	uint16_t iconWidth = uint16_t(iwidth * density);
	uint16_t iconHeight = uint16_t(iheight * density);

	size_t globalSquare = count * (iconWidth * iconHeight);
	size_t sq2 = 1; uint16_t h = 1; uint16_t w = 1;
	for (bool s = true ;sq2 < globalSquare; sq2 *= 2, s = !s) {
		if (s) { w *= 2; } else { h *= 2; }
	}

	uint32_t cols = w / iconWidth;
	uint32_t rows = (uint32_t)(count / cols + 1);

	while (rows * cols < count) {
		rows ++;
	}

	uint32_t texWidth = w;
	uint32_t texHeight = rows * iconHeight;

	auto tex = Rc<cocos2d::Texture2D>::create(cocos2d::Texture2D::PixelFormat::A8, texWidth, texHeight,
			cocos2d::Texture2D::InitAs::RenderTarget);

	IconStorage_drawIcons(tex, names, icons, density, iwidth, iheight);

	return tex;
}

void IconStorage::reload() {
	if (_names.empty()) {
		_dirty = false;
		return;
	}

	if (_inUpdate) {
		return;
	}

	_inUpdate = true;
	_dirty = false;
	Thread & thread = TextureCache::thread();
	Vector<Icon> *icons = new Vector<Icon>(_icons);
	Vector<IconName> *names = new Vector<IconName>(_names);
	Rc<cocos2d::Texture2D> *texPtr = new Rc<cocos2d::Texture2D>;
	thread.perform([this, icons, names, texPtr] (const Task &) -> bool {
		TextureCache::getInstance()->performWithGL([&] {
			*texPtr = IconStorage_updateIcons(*names, *icons, _density, _width, _height);
		});
		return true;
	}, [this, icons, names, texPtr] (const Task &, bool) {
		if (*texPtr) {
			onUpdateTexture(*texPtr, std::move(*icons));
		}
		_inUpdate = false;
		delete icons;
		delete names;
		delete texPtr;
	});
}

bool IconStorage::matchSize(const Size &size) const {
	return size.width == float(_width) && size.height == float(_height);
}

void IconStorage::schedule() {
	if (!_scheduled) {
		cocos2d::Director::getInstance()->getScheduler()->scheduleUpdate(this, 0, false);
		_scheduled = true;
	}
}

void IconStorage::unschedule() {
	if (_scheduled) {
		_scheduled = false;
		cocos2d::Director::getInstance()->getScheduler()->unscheduleUpdate(this);
	}
}

void IconStorage::onUpdateTexture(cocos2d::Texture2D *tex, Vector<Icon> &&icons) {
	if (!_dirty) {
		_texture = tex;
		_icons = std::move(icons);
		onUpdate(this);
	}
}

NS_MD_END
