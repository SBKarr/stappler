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

#include "SPLayout.h"
#include "SPFilesystem.h"
#include "SLImage.h"
#include "SLSvgReader.h"

NS_LAYOUT_BEGIN

bool Image::isSvg(const StringView &str) {
	return Bitmap::check(Bitmap::FileFormat::Svg, (const uint8_t *)str.data(), str.size());
}

bool Image::isSvg(const Bytes &data) {
	return Bitmap::check(Bitmap::FileFormat::Svg, data.data(), data.size());
}

bool Image::isSvg(const FilePath &file) {
	auto d = filesystem::readIntoMemory(file.get(), 0, 512);
	return Bitmap::check(Bitmap::FileFormat::Svg, d.data(), d.size());
}

bool Image::init(const StringView &data) {
	String tmp = data.str();
	SvgReader reader;
	html::parse<SvgReader, StringView, SvgTag>(reader, StringView(tmp));

	if (!reader._paths.empty()) {
		_width = uint16_t(ceilf(reader._width));
		_height = uint16_t(ceilf(reader._height));
		_drawOrder = std::move(reader._drawOrder);
		_paths = std::move(reader._paths);
		_nextId = reader._nextId;

		if (!reader._viewBox.equals(Rect::ZERO)) {
			const float scaleX = reader._width / reader._viewBox.size.width;
			const float scaleY = reader._height / reader._viewBox.size.height;
			_viewBoxTransform = Mat4::IDENTITY;
			_viewBoxTransform.scale(scaleX, scaleY, 1.0f);
			_viewBoxTransform.translate(-reader._viewBox.origin.x, -reader._viewBox.origin.y, 0.0f);
			_viewBox = Rect(reader._viewBox.origin.x * scaleX, reader._viewBox.origin.y * scaleY,
					reader._viewBox.size.width * scaleX, reader._viewBox.size.height * scaleY);
		} else {
			_viewBox = Rect(0, 0, _width, _height);
		}
		return true;
	} else {
		log::text("layout::Image", "No paths found in input string");
	}

	return false;
}

bool Image::init(const Bytes &data) {
	SvgReader reader;
	html::parse<SvgReader, StringView, SvgTag>(reader, StringView((const char *)data.data(), data.size()));

	if (!reader._paths.empty()) {
		_width = uint16_t(ceilf(reader._width));
		_height = uint16_t(ceilf(reader._height));
		_drawOrder = std::move(reader._drawOrder);
		_paths = std::move(reader._paths);
		_nextId = reader._nextId;

		if (!reader._viewBox.equals(Rect::ZERO)) {
			const float scaleX = reader._width / reader._viewBox.size.width;
			const float scaleY = reader._height / reader._viewBox.size.height;
			_viewBoxTransform = Mat4::IDENTITY;
			_viewBoxTransform.scale(scaleX, scaleY, 1.0f);
			_viewBoxTransform.translate(-reader._viewBox.origin.x, -reader._viewBox.origin.y, 0.0f);
			_viewBox = Rect(reader._viewBox.origin.x * scaleX, reader._viewBox.origin.y * scaleY,
					reader._viewBox.size.width * scaleX, reader._viewBox.size.height * scaleY);
		} else {
			_viewBox = Rect(0, 0, _width, _height);
		}
		return true;
	} else {
		log::text("layout::Image", "No paths found in input data");
	}

	return false;
}

bool Image::init(FilePath &&path) {
	return init(filesystem::readTextFile(path.get()));
}

bool Image::init(const Image &image) {
	_isAntialiased = image._isAntialiased;
	_nextId = image._nextId;
	_width = image._width;
	_height = image._height;
	_viewBox = image._viewBox;
	_viewBoxTransform = image._viewBoxTransform;
	_drawOrder = image._drawOrder;
	_paths = image._paths;
	return true;
}

uint16_t Image::getWidth() const {
	return _width;
}
uint16_t Image::getHeight() const {
	return _height;
}

Rect Image::getViewBox() const {
	return _viewBox;
}

Image::~Image() {
	clearRefs();
}

bool Image::init(uint16_t width, uint16_t height, const String &data) {
	Path path;
	if (!path.init(data)) {
		return false;
	}
	return init(width, height, std::move(path));
}

bool Image::init(uint16_t width, uint16_t height, Path && path) {
	_width = width;
	_height = height;
	_viewBox = Rect(0, 0, width, height);

	addPath(move(path));

	return true;
}

bool Image::init(uint16_t width, uint16_t height) {
	_width = width;
	_height = height;
	_viewBox = Rect(0, 0, width, height);

	return true;
}

Image::PathRef Image::addPath(const Path &path, const StringView & tag) {
	return addPath(Path(path), tag);
}

Image::PathRef Image::addPath(Path &&path, const StringView & tag) {
	Image::PathRef ref(definePath(move(path), tag));
	_drawOrder.emplace_back(PathXRef{ref.id.str()});
	return ref;
}

Image::PathRef Image::addPath(const StringView &tag) {
	return addPath(Path(), tag);
}

Image::PathRef Image::definePath(const Path &path, const StringView &tag) {
	return definePath(Path(path), tag);
}

Image::PathRef Image::definePath(Path &&path, const StringView &tag) {
	String idStr;
	StringView id(tag);
	if (id.empty()) {
		idStr = toString("auto-", _nextId);
		++ _nextId;
		id = idStr;
	}

	auto pathIt = _paths.emplace(id.str(), move(path)).first;
	pathIt->second.setAntialiased(_isAntialiased);

	setDirty();
	return PathRef(this, &pathIt->second, pathIt->first);
}

Image::PathRef Image::definePath(const StringView &tag) {
	return definePath(Path(), tag);
}

Image::PathRef Image::getPath(const StringView &tag) {
	auto it = _paths.find(tag);
	if (it != _paths.end()) {
		return PathRef(this, &it->second, it->first);
	}
	return PathRef();
}

void Image::removePath(const PathRef &path) {
	removePath(path.id);
}
void Image::removePath(const StringView &tag) {
	auto it = _paths.find(tag);
	if (it != _paths.end()) {
		_paths.erase(it);
		eraseRefs(tag);
	}
	setDirty();
}

void Image::clear() {
	_paths.clear();
	clearRefs();
	setDirty();
}

const Map<String, Path> &Image::getPaths() const {
	return _paths;
}

void Image::setAntialiased(bool value) {
	_isAntialiased = value;
}
bool Image::isAntialiased() const {
	return _isAntialiased;
}

void Image::setBatchDrawing(bool value) {
	_allowBatchDrawing = value;
}
bool Image::isBatchDrawing() const {
	return _allowBatchDrawing;
}

void Image::clearRefs() {
	for (auto &it : _refs) {
		it->path = nullptr;
		it->image = nullptr;
	}
	_refs.clear();
}

void Image::addRef(PathRef *ref) {
	_refs.emplace_back(ref);
}
void Image::removeRef(PathRef *ref) {
	ref->image = nullptr;
	ref->path = nullptr;
	_refs.erase(std::remove(_refs.begin(), _refs.end(), ref), _refs.end());
}
void Image::replaceRef(PathRef *original, PathRef *target) {
	auto it = std::find(_refs.begin(), _refs.end(), original);
	if (it != _refs.end()) {
		*it = target;
	} else {
		addRef(target);
	}
}
void Image::eraseRefs(const StringView &tag) {
	for (auto &it : _refs) {
		if (it->id == tag) {
			it->image = nullptr;
			it->path = nullptr;
		}
	}

	_refs.erase(std::remove_if(_refs.begin(), _refs.end(), [] (PathRef *ref) -> bool {
		if (ref->image == nullptr) {
			return true;
		}
		return false;
	}), _refs.end());
}

static bool colorIsBlack(const Color4B &c) {
	return c.r == 0 && c.g == 0 && c.b == 0 && c.a == 255;
}
static bool colorIsGray(const Color4B &c) {
	return c.r == c.g && c.b == c.r;
}

Bitmap::PixelFormat Image::detectFormat() const {
	bool black = true;
	bool grey = true;

	for (auto &it : _paths) {
		if ((it.second.getStyle() & Path::Style::Fill) != DrawStyle::None) {
			if (!colorIsBlack(it.second.getFillColor())) {
				black = false;
			}
			if (!colorIsGray(it.second.getFillColor())) {
				black = false;
				grey = false;
			}
		}
		if ((it.second.getStyle() & Path::Style::Stroke) != DrawStyle::None) {
			if (!colorIsBlack(it.second.getStrokeColor())) {
				black = false;
			}
			if (!colorIsGray(it.second.getStrokeColor())) {
				black = false;
				grey = false;
			}
		}
	}

	if (black) {
		return Bitmap::PixelFormat::A8;
	} else if (grey) {
		return Bitmap::PixelFormat::IA88;
	}
	return Bitmap::PixelFormat::RGBA8888;
}

const Vector<PathXRef> &Image::getDrawOrder() const {
	return _drawOrder;
}
void Image::setDrawOrder(const Vector<PathXRef> &vec) {
	_drawOrder = vec;
}
void Image::setDrawOrder(Vector<PathXRef> &&vec) {
	_drawOrder = move(vec);
}

PathXRef Image::getDrawOrderPath(size_t size) const {
	if (size >= _drawOrder.size()) {
		return _drawOrder[size];
	}
	return PathXRef();
}
PathXRef Image::addDrawOrderPath(const StringView &id, const Vec2 &pos) {
	auto it = _paths.find(id);
	if (it != _paths.end()) {
		Mat4 mat; Mat4::createTranslation(pos.x, pos.y, 0.0f, &mat);
		_drawOrder.emplace_back(PathXRef{id.str(), mat});
		return _drawOrder.back();
	}
	return PathXRef();
}

void Image::clearDrawOrder() {
	_drawOrder.clear();
}

Path *Image::getPathById(const StringView &id) {
	auto it = _paths.find(id);
	if (it != _paths.end()) {
		return &it->second;
	}
	return nullptr;
}

void Image::setViewBoxTransform(const Mat4 &m) {
	_viewBoxTransform = m;
}

const Mat4 &Image::getViewBoxTransform() const {
	return _viewBoxTransform;
}

Rc<Image> Image::clone() const {
	return Rc<Image>::create(*this);
}

Image::PathRef::~PathRef() {
	if (image) {
		image->removeRef(this);
		image = nullptr;
	}
}
Image::PathRef::PathRef(Image *img, Path *path, const StringView &id) : id(id), path(path), image(img) {
	image->addRef(this);
}

Image::PathRef::PathRef() { }

Image::PathRef::PathRef(PathRef && ref) : id(ref.id), path(ref.path), image(ref.image) {
	if (image) {
		image->replaceRef(&ref, this);
	}
	ref.image = nullptr;
}
Image::PathRef::PathRef(const PathRef &ref) : id(ref.id), path(ref.path), image(ref.image) {
	if (image) {
		image->addRef(this);
	}
}
Image::PathRef::PathRef(nullptr_t) { }

Image::PathRef & Image::PathRef::operator=(PathRef &&ref) {
	invalidate();
	image = ref.image;
	path = ref.path;
	id = ref.id;
	if (image) {
		image->replaceRef(&ref, this);
		ref.image = nullptr;
	}
	return *this;
}
Image::PathRef & Image::PathRef::operator=(const PathRef &ref) {
	invalidate();
	image = ref.image;
	path = ref.path;
	id = ref.id;
	if (image) {
		image->addRef(this);
	}
	return *this;
}
Image::PathRef & Image::PathRef::operator=(nullptr_t) {
	invalidate();
	return *this;
}
size_t Image::PathRef::count() const {
	return path?path->count():0;
}

Image::PathRef & Image::PathRef::moveTo(float x, float y) {
	if (path) {
		path->moveTo(x, y);
		image->setDirty();
	}
	return *this;
}
Image::PathRef & Image::PathRef::lineTo(float x, float y) {
	if (path) {
		path->lineTo(x, y);
		image->setDirty();
	}
	return *this;
}
Image::PathRef & Image::PathRef::quadTo(float x1, float y1, float x2, float y2) {
	if (path) {
		path->quadTo(x1, y1, x2, y2);
		image->setDirty();
	}
	return *this;
}
Image::PathRef & Image::PathRef::cubicTo(float x1, float y1, float x2, float y2, float x3, float y3) {
	if (path) {
		path->cubicTo(x1, y1, x2, y2, x2, y3);
		image->setDirty();
	}
	return *this;
}
Image::PathRef & Image::PathRef::arcTo(float rx, float ry, float rotation, bool largeFlag, bool sweepFlag, float x, float y) {
	if (path) {
		path->arcTo(rx, ry, rotation, largeFlag, sweepFlag, x, y);
		image->setDirty();
	}
	return *this;
}

Image::PathRef & Image::PathRef::closePath() {
	if (path) {
		path->closePath();
		image->setDirty();
	}
	return *this;
}

Image::PathRef & Image::PathRef::addRect(const Rect& rect) {
	if (path) {
		path->addRect(rect);
		image->setDirty();
	}
	return *this;
}
Image::PathRef & Image::PathRef::addOval(const Rect& oval) {
	if (path) {
		path->addOval(oval);
		image->setDirty();
	}
	return *this;
}
Image::PathRef & Image::PathRef::addCircle(float x, float y, float radius) {
	if (path) {
		path->addCircle(x, y, radius);
		image->setDirty();
	}
	return *this;
}
Image::PathRef & Image::PathRef::addArc(const Rect& oval, float startAngleInRadians, float sweepAngleInRadians) {
	if (path) {
		path->addArc(oval, startAngleInRadians, sweepAngleInRadians);
		image->setDirty();
	}
	return *this;
}

Image::PathRef & Image::PathRef::setFillColor(const Color4B &color) {
	if (path) {
		path->setFillColor(color);
		image->setDirty();
	}
	return *this;
}
const Color4B &Image::PathRef::getFillColor() const {
	return path?path->getFillColor():Color4B::BLACK;
}

Image::PathRef & Image::PathRef::setStrokeColor(const Color4B &color) {
	if (path) {
		path->setStrokeColor(color);
		image->setDirty();
	}
	return *this;
}
const Color4B &Image::PathRef::getStrokeColor() const {
	return path?path->getStrokeColor():Color4B::BLACK;
}

Image::PathRef & Image::PathRef::setFillOpacity(uint8_t value) {
	if (path) {
		path->setFillOpacity(value);
		image->setDirty();
	}
	return *this;
}
uint8_t Image::PathRef::getFillOpacity() const {
	return path?path->getFillOpacity():0;
}

Image::PathRef & Image::PathRef::setStrokeOpacity(uint8_t value) {
	if (path) {
		path->setStrokeOpacity(value);
		image->setDirty();
	}
	return *this;
}
uint8_t Image::PathRef::getStrokeOpacity() const {
	return path?path->getStrokeOpacity():0;
}

Image::PathRef & Image::PathRef::setStrokeWidth(float width) {
	if (path) {
		path->setStrokeWidth(width);
		image->setDirty();
	}
	return *this;
}
float Image::PathRef::getStrokeWidth() const {
	return path?path->getStrokeWidth():0.0f;
}

Image::PathRef & Image::PathRef::setStyle(DrawStyle s) {
	if (path) {
		path->setStyle(s);
		image->setDirty();
	}
	return *this;
}
DrawStyle Image::PathRef::getStyle() const {
	return path?path->getStyle():DrawStyle::FillAndStroke;
}

Image::PathRef & Image::PathRef::setTransform(const Mat4 &t) {
	if (path) {
		path->setTransform(t);
		image->setDirty();
	}
	return *this;
}
Image::PathRef & Image::PathRef::applyTransform(const Mat4 &t) {
	if (path) {
		path->applyTransform(t);
		image->setDirty();
	}
	return *this;
}
const Mat4 &Image::PathRef::getTransform() const {
	return path?path->getTransform():Mat4::IDENTITY;
}

Image::PathRef & Image::PathRef::clear() {
	if (path) {
		path->clear();
		image->setDirty();
	}
	return *this;
}

StringView Image::PathRef::getId() const {
	return id;
}

bool Image::PathRef::empty() const {
	return path?path->empty():true;
}
bool Image::PathRef::valid() const {
	return path && image;
}

void Image::PathRef::invalidate() {
	if (image) {
		image->removeRef(this);
	}
	path = nullptr;
	image = nullptr;
}
Image::PathRef::operator bool() const {
	return valid() && !empty();
}

Path *Image::PathRef::getPath() const {
	return path;
}

NS_LAYOUT_END
