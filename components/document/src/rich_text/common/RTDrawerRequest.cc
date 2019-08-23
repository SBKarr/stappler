/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include "RTDrawerRequest.h"
#include "RTDrawer.h"
#include "SPThread.h"
#include "SPTextureCache.h"

NS_RT_BEGIN

constexpr uint16_t ImageFillerWidth = 312;
constexpr uint16_t ImageFillerHeight = 272;

constexpr auto ImageFillerData = R"ImageFillerSvg(<svg xmlns="http://www.w3.org/2000/svg" height="272" width="312" version="1.1">
	<rect x="0" y="0" width="312" height="272" opacity="0.25"/>
	<g transform="translate(0 -780.4)">
		<path d="m104 948.4h104l-32-56-24 32-16-12zm-32-96v128h168v-128h-168zm16 16h136v96h-136v-96zm38 20a10 10 0 0 1 -10 10 10 10 0 0 1 -10 -10 10 10 0 0 1 10 -10 10 10 0 0 1 10 10z" fill-rule="evenodd" fill="#ccc"/>
	</g>
</svg>)ImageFillerSvg";

constexpr uint16_t ImageVideoWidth = 128;
constexpr uint16_t ImageVideoHeight = 128;

constexpr auto ImageVideoData = R"ImageVideoData(<svg xmlns="http://www.w3.org/2000/svg" height="128" width="128" version="1.1">
	<circle cx="64" cy="64" r="64"/>
	<path fill="#fff" d="m96.76 64-51.96 30v-60z"/>
</svg>)ImageVideoData";

constexpr float ImageVideoSize = 72.0f;


Rc<cocos2d::Texture2D> Request::make(Drawer *drawer, CommonSource *source, Result *result) {
	Request req;
	req.init(drawer, source, result, Rect(Vec2(0.0f, 0.0f), result->getContentSize()), nullptr, nullptr);

	return TextureCache::getInstance()->performWithGL([&] {
		return req.makeTexture();
	});
}

// draw normal texture
bool Request::init(Drawer *drawer, CommonSource *source, Result *result, const Rect &rect, const Callback &cb, cocos2d::Ref *ref) {
	_rect = rect;
	_density = result->getMedia().density;
	_drawer = drawer;
	_source = source;
	_result = result;
	_ref = ref;
	_callback = cb;

	_width = (uint16_t)ceilf(_rect.size.width * _result->getMedia().density);
	_height = (uint16_t)ceilf(_rect.size.height * _result->getMedia().density);
	return true;
}

// draw thumbnail texture, where scale < 1.0f - resample coef
bool Request::init(Drawer *drawer, CommonSource *source, Result *result, const Rect &rect, float scale, const Callback &cb, cocos2d::Ref *ref) {
	_rect = rect;
	_scale = scale;
	_density = result->getMedia().density * scale;
	_isThumbnail = true;
	_drawer = drawer;
	_source = source;
	_result = result;
	_ref = ref;
	_callback = cb;

	_width = (uint16_t)ceilf(_rect.size.width * _result->getMedia().density * _scale);
	_height = (uint16_t)ceilf(_rect.size.height * _result->getMedia().density * _scale);
	return true;
}

void Request::run() {
	_source->retainReadLock(this, std::bind(&Request::onAssetCaptured, this));
}

Rc<cocos2d::Texture2D> Request::makeTexture() {
	_font = _source->getSource();
	_font->unschedule();

	auto tex = Rc<cocos2d::Texture2D>::create(cocos2d::Texture2D::PixelFormat::RGBA8888, _width, _height, cocos2d::Texture2D::RenderTarget);
	draw( tex );
	return tex;
}

void Request::onAssetCaptured() {
	if (!_source->isActual()) {
		if (_callback) {
			_callback(nullptr);
		}
		_source->releaseReadLock(this);
		return;
	}

	_font = _source->getSource();
	_font->unschedule();

	_networkAssets = _source->getExternalAssets();

	Rc<cocos2d::Texture2D> *ptr = new Rc<cocos2d::Texture2D>(nullptr);

	TextureCache::thread().perform([this, ptr] (const Task &) -> bool {
		TextureCache::getInstance()->performWithGL([&] {
			*ptr = makeTexture();
		});
		return true;
	}, [this, ptr] (const Task &, bool) {
		onDrawed(*ptr);
		_source->releaseReadLock(this);
		delete ptr;
	}, this);
}

static Size Request_getBitmapSize(const Rect &bbox, const Background &bg, uint32_t w, uint32_t h) {
	Size coverSize, containSize;

	const float coverRatio = std::max(bbox.size.width / w, bbox.size.height / h);
	const float containRatio = std::min(bbox.size.width / w, bbox.size.height / h);

	coverSize = Size(w * coverRatio, h * coverRatio);
	containSize = Size(w * containRatio, h * containRatio);

	float boxWidth = 0.0f, boxHeight = 0.0f;
	switch (bg.backgroundSizeWidth.metric) {
	case layout::style::Metric::Units::Contain: boxWidth = containSize.width; break;
	case layout::style::Metric::Units::Cover: boxWidth = coverSize.width; break;
	case layout::style::Metric::Units::Percent: boxWidth = bbox.size.width * bg.backgroundSizeWidth.value; break;
	case layout::style::Metric::Units::Px: boxWidth = bg.backgroundSizeWidth.value; break;
	default: boxWidth = bbox.size.width; break;
	}

	switch (bg.backgroundSizeHeight.metric) {
	case layout::style::Metric::Units::Contain: boxHeight = containSize.height; break;
	case layout::style::Metric::Units::Cover: boxHeight = coverSize.height; break;
	case layout::style::Metric::Units::Percent: boxHeight = bbox.size.height * bg.backgroundSizeHeight.value; break;
	case layout::style::Metric::Units::Px: boxHeight = bg.backgroundSizeHeight.value; break;
	default: boxHeight = bbox.size.height; break;
	}

	if (bg.backgroundSizeWidth.metric == layout::style::Metric::Units::Auto
			&& bg.backgroundSizeHeight.metric == layout::style::Metric::Units::Auto) {
		boxWidth = w;
		boxHeight = h;
	} else if (bg.backgroundSizeWidth.metric == layout::style::Metric::Units::Auto) {
		boxWidth = boxHeight * ((float)w / (float)h);
	} else if (bg.backgroundSizeHeight.metric == layout::style::Metric::Units::Auto) {
		boxHeight = boxWidth * ((float)h / (float)w);
	}

	return Size(boxWidth, boxHeight);
}

void Request::prepareBackgroundImage(const Rect &bbox, const Background &bg) {
	bool success = false;
	auto &src = bg.backgroundImage;
	if (_source->isFileExists(src)) {
		auto size = _source->getImageSize(src);
		Size bmpSize = Request_getBitmapSize(bbox, bg, size.first, size.second);
		if (!bmpSize.equals(Size::ZERO)) {
			auto bmp = _drawer->getBitmap(src, bmpSize);
			if (!bmp) {
				Bytes bmpSource = _source->getImageData(src);
				if (!bmpSource.empty()) {
					success = true;
					auto tex = TextureCache::uploadTexture(bmpSource, bmpSize, _density);
					_drawer->addBitmap(src, tex, bmpSize);
				}
			}
		}
	}

	if (!success) {
		Size bmpSize = Request_getBitmapSize(bbox, bg, ImageFillerWidth, ImageFillerHeight);
		auto bmp = _drawer->getBitmap("system://filler.svg", bmpSize);
		if (!bmp && !bmpSize.equals(Size::ZERO)) {
			Bytes bmpSource = Bytes((uint8_t *)ImageFillerData, (uint8_t *)(ImageFillerData + strlen(ImageFillerData)));
			auto tex = TextureCache::uploadTexture(bmpSource, bmpSize, _density);
			_drawer->addBitmap("system://filler.svg", tex, bmpSize);
		}
	}
}

void Request::draw(cocos2d::Texture2D *data) {
	Vector<const Object *> drawObjects;
	auto &objs = _result->getObjects();
	for (auto &obj : objs) {
		if (obj->bbox.intersectsRect(_rect)) {
			drawObjects.push_back(obj);
			if (obj->type == Object::Type::Label) {
				auto l = obj->asLabel();
				for (auto &it : l->format.ranges) {
					_font->addTextureChars(it.layout->getName(), l->format.chars, it.start, it.count);
				}
			} else if (obj->type == Object::Type::Background && !_isThumbnail) {
				auto bg = obj->asBackground();
				if (!bg->background.backgroundImage.empty()) {
					prepareBackgroundImage(obj->bbox, bg->background);
				}
			}
		}
	}

	for (auto &obj : _result->getRefs()) {
		if (obj->bbox.intersectsRect(_rect) && obj->type == Object::Type::Ref) {
			auto link = obj->asLink();
			if (link->mode == "video" && !_isThumbnail) {
				drawObjects.push_back(obj);
				float size = (obj->bbox.size.width > ImageVideoSize && obj->bbox.size.height > ImageVideoSize)
						? ImageVideoSize : (std::min(obj->bbox.size.width, obj->bbox.size.height));

				Size texSize(size, size);
				Bytes bmpSource = Bytes((uint8_t *)ImageVideoData, (uint8_t *)(ImageVideoData + strlen(ImageVideoData)));
				auto tex = TextureCache::uploadTexture(bmpSource, texSize, _density);
				_drawer->addBitmap("system://video.svg", tex, texSize);
			}
		}
	}

	if (_font->isDirty()) {
		_font->update(0.0f);
	}

	if (!_isThumbnail) {
		auto bg = _result->getBackgroundColor();
		if (bg.a == 0) {
			bg.r = 255;
			bg.g = 255;
			bg.b = 255;
		}
		if (!_drawer->begin(data, bg, _density)) {
			return;
		}
	} else {
		if (!_drawer->begin(data, Color4B(0, 0, 0, 0), _density)) {
			return;
		}
	}

	std::sort(drawObjects.begin(), drawObjects.end(), [] (const Object *l, const Object *r) -> bool {
		if (l->depth != r->depth) {
			return l->depth < r->depth;
		} else if (l->type != r->type) {
			return toInt(l->type) < toInt(r->type);
		} else {
			return l->index < r->index;
		}
	});

	for (auto &obj : drawObjects) {
		drawObject(*obj);
	}

	_drawer->end();
}

Rect Request::getRect(const Rect &rect) const {
	return Rect(
			(rect.origin.x - _rect.origin.x) * _density,
			(rect.origin.y - _rect.origin.y) * _density,
			rect.size.width * _density,
			rect.size.height * _density);
}

void Request::drawRef(const Rect &bbox, const Link &l) {
	if (_isThumbnail) {
		return;
	}
	if (l.mode == "video") {
		float size = (bbox.size.width > ImageVideoSize && bbox.size.height > ImageVideoSize)
				? ImageVideoSize : (std::min(bbox.size.width, bbox.size.height));

		auto bmp = _drawer->getBitmap("system://video.svg", Size(size, size));
		if (bmp) {
			auto targetBbox = getRect(bbox);
			Rect drawBbox;
			if (size == ImageVideoSize) {
				drawBbox = Rect(targetBbox.origin.x, targetBbox.origin.y, ImageVideoSize * _density, ImageVideoSize * _density);
				drawBbox.origin.x += (targetBbox.size.width - drawBbox.size.width) / 2.0f;
				drawBbox.origin.y += (targetBbox.size.height - drawBbox.size.height) / 2.0f;
			}
			_drawer->setColor(Color4B(255, 255, 255, 160));
			_drawer->drawTexture(drawBbox, bmp, Rect(0, 0, bmp->getPixelsWide(), bmp->getPixelsHigh()));
		}
	}

	if (_highlightRefs) {
		drawRectangle(bbox, Color4B(127, 255, 0, 64));
	}
}

void Request::drawPath(const Rect &bbox, const layout::Path &path) {
	if (_isThumbnail) {
		return;
	}
	Rect rect(
		(bbox.origin.x - _rect.origin.x),
		(_rect.size.height - bbox.origin.y - bbox.size.height + _rect.origin.y),
		bbox.size.width * _density,
		bbox.size.height * _density);

	_drawer->drawPath(rect, path);
}

void Request::drawRectangle(const Rect &bbox, const Color4B &color) {
	layout::Path path; path.reserve(5, 1);
	path.setFillColor(color);
	path.addRect(0.0f, 0.0f, bbox.size.width, bbox.size.height);
	drawPath(bbox, path);
}

void Request::drawBitmap(const Rect &origBbox, cocos2d::Texture2D *bmp, const Background &bg, const Size &box) {
	Rect bbox = origBbox;

	const auto w = bmp->getPixelsWide();
	const auto h = bmp->getPixelsHigh();

	float availableWidth = bbox.size.width - box.width, availableHeight = bbox.size.height - box.height;
	float xOffset = 0.0f, yOffset = 0.0f;

	switch (bg.backgroundPositionX.metric) {
	case layout::style::Metric::Units::Percent: xOffset = availableWidth * bg.backgroundPositionX.value; break;
	case layout::style::Metric::Units::Px: xOffset = bg.backgroundPositionX.value; break;
	default: xOffset = availableWidth / 2.0f; break;
	}

	switch (bg.backgroundPositionY.metric) {
	case layout::style::Metric::Units::Percent: yOffset = availableHeight * bg.backgroundPositionY.value; break;
	case layout::style::Metric::Units::Px: yOffset = bg.backgroundPositionY.value; break;
	default: yOffset = availableHeight / 2.0f; break;
	}

	Rect contentBox(0, 0, w, h);

	if (box.width < bbox.size.width) {
		bbox.size.width = box.width;
		bbox.origin.x += xOffset;
	} else if (box.width > bbox.size.width) {
		contentBox.size.width = bbox.size.width * w / box.width;
		contentBox.origin.x -= xOffset * (w / box.width);
	}

	if (box.height < bbox.size.height) {
		bbox.size.height = box.height;
		bbox.origin.y += yOffset;
	} else if (box.height > bbox.size.height) {
		contentBox.size.height = bbox.size.height * h / box.height;
		contentBox.origin.y -= yOffset * (h / box.height);
	}

	bbox = getRect(bbox);
	_drawer->drawTexture(bbox, bmp, contentBox);
}

void Request::drawFillerImage(const Rect &bbox, const Background &bg) {
	auto box = Request_getBitmapSize(bbox, bg, ImageFillerWidth, ImageFillerHeight);
	auto bmp = _drawer->getBitmap("system://filler.svg", box);
	if (bmp) {
		drawBitmap(bbox, bmp, bg, box);
	}
}

void Request::drawBackgroundImage(const Rect &bbox, const Background &bg) {
	auto &src = bg.backgroundImage;
	bool success = false;
	if (_source->isFileExists(src)) {
		auto size = _source->getImageSize(src);
		Size box = Request_getBitmapSize(bbox, bg, size.first, size.second);

		auto bmp = _drawer->getBitmap(src, box);
		if (bmp) {
			_drawer->setColor(Color4B(layout::Color(_result->getBackgroundColor()).text(), 255));
			drawBitmap(bbox, bmp, bg, box);
			success = true;
		}
	}

	if (!success && !src.empty() && bbox.size.width > 0.0f && bbox.size.height > 0.0f) {
		_drawer->setColor(Color4B(168, 168, 168, 255));
		drawFillerImage(bbox, bg);
	}
}

void Request::drawBackgroundColor(const Rect &bbox, const Background &bg) {
	drawRectangle(bbox, bg.backgroundColor);
}

void Request::drawBackground(const Rect &bbox, const Background &bg) {
	if (_isThumbnail) {
		drawRectangle(bbox, Color4B(127, 127, 127, 127));
		return;
	}
	if (bg.backgroundColor.a != 0) {
		drawBackgroundColor(bbox, bg);
	}
	if (!bg.backgroundImage.empty()) {
		drawBackgroundImage(bbox, bg);
	}
}

void Request::drawLabel(const cocos2d::Rect &bbox, const Label &l) {
	if (l.format.chars.empty()) {
		return;
	}

	if (_isThumbnail) {
		_drawer->setColor(Color4B(127, 127, 127, 127));
		_drawer->drawCharRects(_font, l.format, getRect(bbox), _scale);
	} else {
		if (l.preview) {
			_drawer->setColor(Color4B(127, 127, 127, 127));
			_drawer->drawCharRects(_font, l.format, getRect(bbox), 1.0f);
		} else {
			_drawer->drawChars(_font, l.format, getRect(bbox));
		}
	}
}

void Request::drawObject(const Object &obj) {
	switch (obj.type) {
	case Object::Type::Background: drawBackground(obj.bbox, obj.asBackground()->background); break;
	case Object::Type::Label: drawLabel(obj.bbox, *obj.asLabel()); break;
	case Object::Type::Path: drawPath(obj.bbox, obj.asPath()->path); break;
	case Object::Type::Ref: drawRef(obj.bbox, *obj.asLink()); break;
	default: break;
	}
}

void Request::onDrawed(cocos2d::Texture2D *data) {
	if (data && _callback) {
		_callback(data);
	}
}

NS_RT_END
