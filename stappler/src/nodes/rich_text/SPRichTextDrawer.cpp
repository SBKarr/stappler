/*
 * SPRichTextDrawer.cpp
 *
 *  Created on: 03 авг. 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPRichTextDrawer.h"
#include "SPTexture.h"

#include "renderer/CCTexture2D.h"
#include "platform/CCImage.h"
#include "base/CCDirector.h"
#include "base/CCScheduler.h"

#include "SPBitmap.h"
#include "SPString.h"
#include "SPDrawCanvas.h"

#include "cairo.h"

NS_SP_EXT_BEGIN(rich_text)

struct DrawerCanvas;
struct DrawerCache;

Thread s_softwareRendererThread("RichTextSoftwareRenderer");

struct DrawerCanvas {
	DrawerCanvas(cairo_t *c, Source *s, const cocos2d::Rect &rect, float d, uint16_t width, uint16_t height, uint8_t *data, float scale, bool thumb);

	Rect getRect(const Rect &) const;

	void drawRef(const cocos2d::Rect &bbox);
	void drawOutline(const cocos2d::Rect &bbox, const Outline &);
	void drawBitmap(const cocos2d::Rect &bbox, const Bitmap &bmp, cairo_surface_t *surf, const Background &bg);
	void drawBackgroundImage(const cocos2d::Rect &bbox, const Background &bg);
	void drawBackgroundColor(const cocos2d::Rect &bbox, const Background &bg);
	void drawBackground(const cocos2d::Rect &bbox, const Background &bg);
	void drawLabel(const cocos2d::Rect &bbox, const Label &l);
	void drawObject(const Object &obj);

	cairo_t *canvas;
	Source *source;
	const cocos2d::Rect &texRect;
	float density;
	float scale;
	bool thumb;
	Texture<PixelRGBA8888> tex;
	bool highlightRefs = false;
};

static DrawerCache * s_cache = nullptr;
static bool s_cacheScheduled = false;
static bool s_cacheUpdated = true;

struct DrawerCache {
	static void schedule();
	static DrawerCache *getInstance();

	std::string getKey(const std::string &, Document *);
	void addBitmap(const std::string &str, Document *doc, const Rc<Bitmap> &bmp);
	Rc<Bitmap> getBitmap(const std::string &str, Document *doc);
	void update();

	std::map<std::string, std::pair<Rc<Bitmap>, Time>> map;
};

void DrawerCache::schedule() {
	if (s_cacheScheduled) {
		return;
	}

	auto s = cocos2d::Director::getInstance()->getScheduler();
	s->schedule([] (float t) {
		if (!s_cacheUpdated) {
			return;
		}

		s_cacheUpdated = false;
		s_softwareRendererThread.perform(std::bind([] () -> bool {
			DrawerCache::getInstance()->update();
			return true;
		}), std::bind([] {
			s_cacheUpdated = true;
		}));
	}, s, 1.0f, false, "SP.DrawerCache");
	s_cacheScheduled = true;
}

DrawerCache *DrawerCache::getInstance() {
	if (!s_cache) {
		s_cache = new DrawerCache;
	}
	return s_cache;
}

std::string DrawerCache::getKey(const std::string &str, Document *doc) {
	return toString("0x", (ptrdiff_t)doc, ":", str);
}

void DrawerCache::addBitmap(const std::string &str, Document *doc, const Rc<Bitmap> &bmp) {
	std::string key = getKey(str, doc);
	map.insert(std::make_pair(key, std::make_pair(bmp, Time::now())));
	//log("added bitmap %s", key.c_str());
}

Rc<Bitmap> DrawerCache::getBitmap(const std::string &str, Document *doc) {
	std::string key = getKey(str, doc);
	auto it = map.find(key);
	if (it != map.end()) {
		//log("updated bitmap %s", key.c_str());
		it->second.second = Time::now();
		return it->second.first;
	}
	return nullptr;
}

void DrawerCache::update() {
	if (map.empty()) {
		return;
	}

	auto time = Time::now();
	std::vector<std::string> keys;
	for (auto &it : map) {
		if (it.second.second - time > TimeInterval::seconds(6)) {
			keys.push_back(it.first);
		}
	}

	for (auto &it : keys) {
		map.erase(it);
		//log("removed bitmap %s", it.c_str());
	}
}

DrawerCanvas::DrawerCanvas(cairo_t *c, Source *s, const cocos2d::Rect &rect, float d, uint16_t width, uint16_t height, uint8_t *data, float sc, bool t)
: canvas(c), source(s), texRect(rect), density(d * sc), scale(sc), thumb(t), tex(width, height, data, Texture<PixelRGBA8888>::DataPolicy::Borrowing) { }

Rect DrawerCanvas::getRect(const Rect &rect) const {
	return Rect(
			(rect.origin.x - texRect.origin.x) * density,
			(rect.origin.y - texRect.origin.y) * density,
			rect.size.width * density,
			rect.size.height * density);
}

static void helper_rectangle(cairo_t *cr, const Rect &rect, const Rect &tex, float density, draw::Style s) {
	cairo_rectangle(cr, (rect.origin.x - tex.origin.x) * density, (rect.origin.y - tex.origin.y) * density,
			rect.size.width * density, rect.size.height * density);
	switch (s) {
	case draw::Style::Fill:
		cairo_fill(cr);
		break;
	case draw::Style::Stroke:
		cairo_stroke(cr);
		break;
	case draw::Style::FillAndStroke:
		cairo_fill_preserve(cr);
		cairo_stroke(cr);
		break;
	}
}

static void helper_color(cairo_t *cr, const Color4B &color) {
	cairo_set_source_rgba(cr, color.b / 255.0, color.g / 255.0, color.r / 255.0, color.a / 255.0);
}

void DrawerCanvas::drawRef(const cocos2d::Rect &bbox) {
	if (thumb) {
		return;
	}
	if (highlightRefs) {
		auto rect = getRect(bbox);

		helper_color(canvas, Color4B(127, 255, 0, 64));
		helper_rectangle(canvas, bbox, texRect, density, draw::Style::Fill);
	}
}

static void DrawerCanvas_prepareOutline(cairo_t *canvas, const Outline::Params &outline, float density) {
	double dashes[] = { 0.0, 0.0 };
	int ndash = sizeof (dashes)/sizeof(dashes[0]);
	switch (outline.style) {
	case style::BorderStyle::Solid:
		cairo_set_dash (canvas, nullptr, 0, 0.0);
		cairo_set_line_cap(canvas, CAIRO_LINE_CAP_BUTT);
		break;
	case style::BorderStyle::Dotted:
		dashes[0] = 1.0f * density; dashes[1] = outline.width * density * 2;
		cairo_set_dash (canvas, dashes, ndash, outline.width * density / 2);
		cairo_set_line_cap(canvas, CAIRO_LINE_CAP_ROUND);
		break;
	case style::BorderStyle::Dashed:
		dashes[0] = outline.width * density * 4.0f; dashes[1] = outline.width * density;
		cairo_set_dash (canvas, dashes, ndash, outline.width * density / 2);
		cairo_set_line_cap(canvas, CAIRO_LINE_CAP_BUTT);
		break;
	default:
		break;
	}
	helper_color(canvas, outline.color);
	cairo_set_line_width(canvas, outline.width * density);
}

void DrawerCanvas::drawOutline(const cocos2d::Rect &bbox, const Outline &outline) {
	if (thumb) {
		return;
	}
	bool drawLines = true;
	auto lines = outline.getNumLines();
	if (outline.isMono()) {
		if (lines == 4) {
			DrawerCanvas_prepareOutline(canvas, outline.top, density);
			helper_rectangle(canvas, bbox, texRect, density, draw::Style::Stroke);
			drawLines = false;
		} else if (lines == 3) {
			if (!outline.hasTopLine()) {
				DrawerCanvas_prepareOutline(canvas, outline.left, density);
				cairo_move_to (canvas, (bbox.origin.x - texRect.origin.x) * density + bbox.size.width * density, (bbox.origin.y - texRect.origin.y) * density);
				cairo_rel_line_to (canvas, 0, bbox.size.height * density);
				cairo_rel_line_to (canvas, -bbox.size.width * density, 0);
				cairo_rel_line_to (canvas, 0, -bbox.size.height * density);
				cairo_stroke(canvas);
			} else if (!outline.hasRightLine()) {
				DrawerCanvas_prepareOutline(canvas, outline.top, density);
				cairo_move_to (canvas, (bbox.origin.x - texRect.origin.x) * density + bbox.size.width * density, (bbox.origin.y - texRect.origin.y) * density + bbox.size.height * density);
				cairo_rel_line_to (canvas, -bbox.size.width * density, 0);
				cairo_rel_line_to (canvas, 0, -bbox.size.height * density);
				cairo_rel_line_to (canvas, bbox.size.width * density, 0);
				cairo_stroke(canvas);
			} else if (!outline.hasBottomLine()) {
				DrawerCanvas_prepareOutline(canvas, outline.top, density);
				cairo_move_to (canvas, (bbox.origin.x - texRect.origin.x) * density, (bbox.origin.y - texRect.origin.y) * density + bbox.size.height * density);
				cairo_rel_line_to (canvas, 0, -bbox.size.height * density);
				cairo_rel_line_to (canvas, bbox.size.width * density, 0);
				cairo_rel_line_to (canvas, 0, bbox.size.height * density);
				cairo_stroke(canvas);
			} else if (!outline.hasLeftLine()) {
				DrawerCanvas_prepareOutline(canvas, outline.top, density);
				cairo_move_to (canvas, (bbox.origin.x - texRect.origin.x) * density, (bbox.origin.y - texRect.origin.y) * density);
				cairo_rel_line_to (canvas, bbox.size.width * density, 0);
				cairo_rel_line_to (canvas, 0, bbox.size.height * density);
				cairo_rel_line_to (canvas, -bbox.size.width * density, 0);
				cairo_stroke(canvas);
			}
			drawLines = false;
		} else if (lines == 2) {
			if (!((outline.hasBottomLine() && outline.hasTopLine()) || (outline.hasLeftLine() && outline.hasRightLine()))) {
				if (outline.hasBottomLine() && outline.hasRightLine()) {
					DrawerCanvas_prepareOutline(canvas, outline.bottom, density);
					cairo_move_to (canvas, (bbox.origin.x - texRect.origin.x) * density + bbox.size.width * density, (bbox.origin.y - texRect.origin.y) * density);
					cairo_rel_line_to (canvas, 0, bbox.size.height * density);
					cairo_rel_line_to (canvas, -bbox.size.width * density, 0);
					cairo_stroke(canvas);
				} else if (outline.hasRightLine() && outline.hasTopLine()) {
					DrawerCanvas_prepareOutline(canvas, outline.right, density);
					cairo_move_to (canvas, (bbox.origin.x - texRect.origin.x) * density, (bbox.origin.y - texRect.origin.y) * density);
					cairo_rel_line_to (canvas, bbox.size.width * density, 0);
					cairo_rel_line_to (canvas, 0, bbox.size.height * density);
					cairo_stroke(canvas);
				} else if (outline.hasTopLine() && outline.hasLeftLine()) {
					DrawerCanvas_prepareOutline(canvas, outline.top, density);
					cairo_move_to (canvas, (bbox.origin.x - texRect.origin.x) * density, (bbox.origin.y - texRect.origin.y) * density + bbox.size.height * density);
					cairo_rel_line_to (canvas, 0, -bbox.size.height * density);
					cairo_rel_line_to (canvas, bbox.size.width * density, 0);
					cairo_stroke(canvas);
				} else if (outline.hasLeftLine() && outline.hasBottomLine()) {
					DrawerCanvas_prepareOutline(canvas, outline.left, density);
					cairo_move_to (canvas, (bbox.origin.x - texRect.origin.x) * density + bbox.size.width * density, (bbox.origin.y - texRect.origin.y) * density + bbox.size.height * density);
					cairo_rel_line_to (canvas, -bbox.size.width * density, 0);
					cairo_rel_line_to (canvas, 0, -bbox.size.height * density);
					cairo_stroke(canvas);
				}
				drawLines = false;
			}
		}
	}

	if (drawLines) {
		if (outline.hasTopLine()) {
			DrawerCanvas_prepareOutline(canvas, outline.top, density);
			cairo_move_to (canvas, (bbox.origin.x - texRect.origin.x) * density, (bbox.origin.y - texRect.origin.y) * density);
			cairo_rel_line_to (canvas, bbox.size.width * density, 0);
			cairo_stroke(canvas);
		}
		if (outline.hasRightLine()) {
			DrawerCanvas_prepareOutline(canvas, outline.right, density);
			cairo_move_to (canvas, (bbox.origin.x - texRect.origin.x) * density + bbox.size.width * density, (bbox.origin.y - texRect.origin.y) * density);
			cairo_rel_line_to (canvas, 0, bbox.size.height * density);
			cairo_stroke(canvas);
		}
		if (outline.hasBottomLine()) {
			DrawerCanvas_prepareOutline(canvas, outline.bottom, density);
			cairo_move_to (canvas, (bbox.origin.x - texRect.origin.x) * density, (bbox.origin.y - texRect.origin.y) * density + bbox.size.height * density);
			cairo_rel_line_to (canvas, bbox.size.width * density, 0);
			cairo_stroke(canvas);
		}
		if (outline.hasLeftLine()) {
			DrawerCanvas_prepareOutline(canvas, outline.left, density);
			cairo_move_to (canvas, (bbox.origin.x - texRect.origin.x) * density, (bbox.origin.y - texRect.origin.y) * density);
			cairo_rel_line_to (canvas, 0, bbox.size.height * density);
			cairo_stroke(canvas);
		}
	}
	cairo_set_line_width(canvas, 1.0f * density);
	cairo_set_dash (canvas, nullptr, 0, 0.0);
	cairo_set_line_cap(canvas, CAIRO_LINE_CAP_BUTT);
}

void DrawerCanvas::drawBitmap(const cocos2d::Rect &origBbox, const Bitmap &bmp, cairo_surface_t *surf, const Background &bg) {
	cocos2d::Rect bbox = origBbox;
	float coverRatio = 1.0f, containRatio = 1.0f;
	cocos2d::Size coverSize, containSize;

	coverRatio = MAX(bbox.size.width / bmp.width(), bbox.size.height / bmp.height());
	containRatio = MIN(bbox.size.width / bmp.width(), bbox.size.height / bmp.height());

	coverSize = cocos2d::Size(bmp.width() * coverRatio, bmp.height() * coverRatio);
	containSize = cocos2d::Size(bmp.width() * containRatio, bmp.height() * containRatio);

	float width = 0.0f, height = 0.0f;
	switch (bg.backgroundSizeWidth.metric) {
	case style::Size::Metric::Contain: width = containSize.width; break;
	case style::Size::Metric::Cover: width = coverSize.width; break;
	case style::Size::Metric::Percent: width = bbox.size.width * bg.backgroundSizeWidth.value; break;
	case style::Size::Metric::Px: width = bg.backgroundSizeWidth.value; break;
	default: width = bbox.size.width; break;
	}

	switch (bg.backgroundSizeHeight.metric) {
	case style::Size::Metric::Contain: height = containSize.height; break;
	case style::Size::Metric::Cover: height = coverSize.height; break;
	case style::Size::Metric::Percent: height = bbox.size.height * bg.backgroundSizeHeight.value; break;
	case style::Size::Metric::Px: height = bg.backgroundSizeHeight.value; break;
	default: height = bbox.size.height; break;
	}

	if (bg.backgroundSizeWidth.metric == style::Size::Metric::Auto
			&& bg.backgroundSizeHeight.metric == style::Size::Metric::Auto) {
		width = bmp.width();
		height = bmp.height();
	} else if (bg.backgroundSizeWidth.metric == style::Size::Metric::Auto) {
		width = height * ((float)bmp.width() / (float)bmp.height());
	} else if (bg.backgroundSizeHeight.metric == style::Size::Metric::Auto) {
		height = width * ((float)bmp.height() / (float)bmp.width());
	}

	float availableWidth = bbox.size.width - width, availableHeight = bbox.size.height - height;
	float xOffset = 0.0f, yOffset = 0.0f;

	switch (bg.backgroundPositionX.metric) {
	case style::Size::Metric::Percent: xOffset = availableWidth * bg.backgroundPositionX.value; break;
	case style::Size::Metric::Px: xOffset = bg.backgroundPositionX.value; break;
	default: xOffset = availableWidth / 2.0f; break;
	}

	switch (bg.backgroundPositionY.metric) {
	case style::Size::Metric::Percent: yOffset = availableHeight * bg.backgroundPositionY.value; break;
	case style::Size::Metric::Px: yOffset = bg.backgroundPositionY.value; break;
	default: yOffset = availableHeight / 2.0f; break;
	}

	cocos2d::Rect contentBox(0, 0, bmp.width(), bmp.height());

	if (width < bbox.size.width) {
		bbox.size.width = width;
		bbox.origin.x += xOffset;
	} else if (width > bbox.size.width) {
		contentBox.size.width *= bbox.size.width / width;
		contentBox.origin.x -= xOffset * (contentBox.size.width / bbox.size.width);
	}

	if (height < bbox.size.height) {
		bbox.size.height = height;
		bbox.origin.y += yOffset;
	} else if (height > bbox.size.height) {
		contentBox.size.height *= bbox.size.height / height;
		contentBox.origin.y -= yOffset * (contentBox.size.height / bbox.size.height);
	}

	bbox = getRect(bbox);


	cairo_matrix_t save_matrix;
	cairo_get_matrix(canvas, &save_matrix);

	cairo_rectangle(canvas, bbox.origin.x, bbox.origin.y, bbox.size.width, bbox.size.height);
	cairo_clip(canvas);

	cairo_translate(canvas, bbox.getMidX(), bbox.getMidY());
	cairo_scale(canvas, bbox.size.width / contentBox.size.width, bbox.size.height / contentBox.size.height);
	cairo_set_source_surface(canvas, surf, -(float)bmp.width() / 2.0f, -(float)bmp.height() / 2.0f);
	cairo_paint(canvas);

	cairo_reset_clip(canvas);
	cairo_set_matrix(canvas, &save_matrix);
}

void DrawerCanvas::drawBackgroundImage(const cocos2d::Rect &bbox, const Background &bg) {
	auto src = bg.backgroundImage;
	auto document = source->getDocument();
	if (!document->hasImage(src)) {
		return;
	}

	auto bmp = DrawerCache::getInstance()->getBitmap(src, document);
	if (!bmp) {
		auto strideFn = [] (Bitmap::Format f, uint32_t width) -> uint32_t {
			switch (f) {
			case Bitmap::Format::A8:
			case Bitmap::Format::I8:
				return cairo_format_stride_for_width(CAIRO_FORMAT_A8, width);
				break;
			case Bitmap::Format::IA88: return 0; break;
			case Bitmap::Format::RGB888: return 0; break;
			case Bitmap::Format::RGBA8888:
				return cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
				break;
			}
			return 0;
		};

		Bitmap bmpSource(document->getImageBitmap(src, strideFn));
		if (!bmpSource) {
			return;
		}

		// IA88 is not supported by cairo
		if (bmpSource.format() == Bitmap::Format::IA88 || bmpSource.format() == Bitmap::Format::RGB888) {
			bmpSource.convert(Bitmap::Format::RGBA8888, strideFn);
		}

		if (bmpSource.empty()) {
			return;
		}

		bmp = Rc<Bitmap>::create(std::move(bmpSource));
		DrawerCache::getInstance()->addBitmap(src, document,  bmp);
	}

	auto imgData = bmp->dataPtr();

	cairo_format_t imgColorType = CAIRO_FORMAT_ARGB32;

	switch (bmp->format()) {
	case Bitmap::Format::A8: imgColorType = CAIRO_FORMAT_A8; break;
	case Bitmap::Format::I8: imgColorType = CAIRO_FORMAT_A8; break;
	case Bitmap::Format::RGB888: imgColorType = CAIRO_FORMAT_RGB24; break;
	case Bitmap::Format::RGBA8888: imgColorType = CAIRO_FORMAT_ARGB32; break;
	default: return; break;
	}

	cairo_surface_t *surf = cairo_image_surface_create_for_data(imgData, imgColorType,
			bmp->width(), bmp->height(), bmp->stride());

	drawBitmap(bbox, *bmp, surf, bg);

	cairo_surface_destroy(surf);
}

void DrawerCanvas::drawBackgroundColor(const cocos2d::Rect &bbox, const Background &bg) {
	auto &color = bg.backgroundColor;
	helper_color(canvas, color);
	helper_rectangle(canvas, bbox, texRect, density, draw::Style::Fill);
}

void DrawerCanvas::drawBackground(const cocos2d::Rect &bbox, const Background &bg) {
	if (thumb) {
		return;
	}
	if (bg.backgroundColor.a != 0) {
		drawBackgroundColor(bbox, bg);
	}
	if (!bg.backgroundImage.empty()) {
		drawBackgroundImage(bbox, bg);
	}
}

void DrawerCanvas::drawLabel(const cocos2d::Rect &bbox, const Label &l) {
	if (l.chars.empty()) {
		return;
	}

	const float xOffset = (bbox.origin.x - texRect.origin.x) * density;
	const float yOffset = (bbox.origin.y - texRect.origin.y) * density;

	if (thumb) {
		helper_color(canvas, Color4B(127, 127, 127, 127));
		for (auto &c : l.chars) {
			if (c.drawable()) {
				const float posX = xOffset + (c.posX + c.uCharPtr->xOffset) * scale;
				const float posY = yOffset + (c.posY + c.uCharPtr->yOffset + c.uCharPtr->font->getDescender()) * scale;

				const float width = c.uCharPtr->width() * scale;
				const float height = c.uCharPtr->height() * scale;

				cairo_rectangle(canvas, posX, posY, width, height);
				cairo_fill(canvas);
			}
		}
	} else {
		tex.drawChars(l.chars, xOffset, yOffset, false);
	}
}

void DrawerCanvas::drawObject(const Object &obj) {
	switch (obj.type) {
	case Object::Type::Background: drawBackground(obj.bbox, obj.value.background); break;
	case Object::Type::Label: drawLabel(obj.bbox, obj.value.label); break;
	case Object::Type::Outline: drawOutline(obj.bbox, obj.value.outline); break;
	case Object::Type::Ref: drawRef(obj.bbox); break;
	default: break;
	}
}

Thread &Drawer::thread() {
	return s_softwareRendererThread;
}

Drawer::~Drawer() { }
Drawer::Drawer() { }

bool Drawer::init(Source *source, Result *result, const Rect &rect, const Callback &cb, cocos2d::Ref *ref) {
	if (!cb) {
		return false;
	}

	DrawerCache::schedule();

	_rect = rect;
	_source = source;
	_result = result;
	_ref = ref;
	_callback = cb;

	_width = (uint16_t)ceilf(_rect.size.width * _result->getMedia().density);
	_height = (uint16_t)ceilf(_rect.size.height * _result->getMedia().density);
	_stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, _width);

	_source->retainReadLock(this, std::bind(&Drawer::onAssetCaptured, this));
	return true;
}

bool Drawer::init(Source *source, Result *result, const Rect &rect, float scale, const Callback &cb, cocos2d::Ref *ref) {
	if (!cb) {
		return false;
	}

	DrawerCache::schedule();

	_rect = rect;
	_scale = scale;
	_isThumbnail = true;
	_source = source;
	_result = result;
	_ref = ref;
	_callback = cb;

	_width = (uint16_t)ceilf(_rect.size.width * _result->getMedia().density * _scale);
	_height = (uint16_t)ceilf(_rect.size.height * _result->getMedia().density * _scale);
	_stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, _width);

	_source->retainReadLock(this, std::bind(&Drawer::onAssetCaptured, this));
	return true;
}

void Drawer::onAssetCaptured() {
	if (!_source->isActual()) {
		_callback(nullptr);
		_source->getAsset()->releaseReadLock(this);
		return;
	}

	uint8_t **ptr = new (uint8_t *)(nullptr);

	s_softwareRendererThread.perform([this, ptr] (cocos2d::Ref *) -> bool {
		(*ptr) = new uint8_t[_stride * _height];
		memset((*ptr), 0, _stride * _height * sizeof(uint8_t));
		draw( (*ptr) );
		return true;
	}, [this, ptr] (cocos2d::Ref *, bool) {
		onDrawed(*ptr);
		_source->releaseReadLock(this);
		delete [] (*ptr);
		delete ptr;
	}, this);
}

void Drawer::draw(uint8_t *data) const {
	auto canvas = Rc<draw::Canvas>::create(data, _width, _height, _stride, draw::Format::RGBA8888);

	if (!_isThumbnail) {
		auto bg = _result->getBackgroundColor();
		if (bg.a == 0) {
			bg.r = 255;
			bg.g = 255;
			bg.b = 255;
		}
		canvas->clear(bg);
	} else {
		canvas->clear(Color4B(0, 0, 0, 0));
	}

	DrawerCanvas drawer(canvas->getContext(), _source, _rect, _result->getMedia().density, _width, _height, data, _scale, _isThumbnail);

	if (!_isThumbnail) {
		_result->getFontSet()->getImage()->retainData();
	}

	auto &objs = _result->getObjects();
	for (auto &obj : objs) {
		if (obj.bbox.intersectsRect(_rect)) {
			drawer.drawObject(obj);
		}
	}

	if (!_isThumbnail) {
		_result->getFontSet()->getImage()->releaseData();
	}
}

void Drawer::onDrawed(uint8_t *data) {
	if (data) {
		auto tex = new cocos2d::Texture2D();
		tex->initWithData(data, _stride * _height, cocos2d::Texture2D::PixelFormat::RGBA8888, _width, _height, _stride, cocos2d::Size(_width, _height));
		tex->autorelease();
		tex->setAliasTexParameters();
		_callback(tex);
	}
}

NS_SP_EXT_END(rich_text)
