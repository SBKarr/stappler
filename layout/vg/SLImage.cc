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
#include "SPHtmlParser.h"
#include "SLImage.h"

NS_LAYOUT_BEGIN

/*
	matrix(<a> <b> <c> <d> <e> <f>), which specifies a transformation in the form
		of a transformation matrix of six values. matrix(a,b,c,d,e,f) is equivalent
		to applying the transformation matrix [a b c d e f].

	translate(<tx> [<ty>]), which specifies a translation by tx and ty. If <ty> is
		not provided, it is assumed to be zero.

	scale(<sx> [<sy>]), which specifies a scale operation by sx and sy. If <sy> is
		not provided, it is assumed to be equal to <sx>.

	rotate(<rotate-angle> [<cx> <cy>]), which specifies a rotation by <rotate-angle>
		degrees about a given point.
		If optional parameters <cx> and <cy> are not supplied, the rotate is about
		the origin of the current user coordinate system. The operation corresponds
		to the matrix [cos(a) sin(a) -sin(a) cos(a) 0 0].
		If optional parameters <cx> and <cy> are supplied, the rotate is about the
		point (cx, cy). The operation represents the equivalent of the following
		specification: translate(<cx>, <cy>) rotate(<rotate-angle>)
		translate(-<cx>, -<cy>).

	skewX(<skew-angle>), which specifies a skew transformation along the x-axis.

	skewY(<skew-angle>), which specifies a skew transformation along the y-axis.
 */

static Mat4 svg_parseTransform(StringView &r) {
	Mat4 ret(Mat4::IDENTITY);
	while (!r.empty()) {
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		if (r.is("matrix(")) {
			r += "matrix("_len;
			float values[6] = { 0 };

			uint16_t i = 0;
			for (; i < 6; ++ i) {
				r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>, StringView::Chars<','>>();
				values[i] = r.readFloat();
				if (IsErrorValue(values[i])) {
					break;
				}
			}

			if (i != 6) {
				break;
			}

			ret *= Mat4(values[0], values[1], values[2], values[3], values[4], values[5]);

		} else if (r.is("translate(")) {
			r += "translate("_len;

			float tx = 0.0f, ty = 0.0f;
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			tx = r.readFloat();
			if (IsErrorValue(tx)) {
				break;
			}

			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>, StringView::Chars<','>>();
			if (!r.is(')')) {
				ty = r.readFloat();
				if (IsErrorValue(ty)) {
					break;
				}
			}

			ret.m[12] += tx;
			ret.m[13] += ty;

		} else if (r.is("scale(")) {
			r += "scale("_len;

			float sx = 0.0f, sy = 0.0f;
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			sx = r.readFloat();
			if (IsErrorValue(sx)) {
				break;
			}

			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>, StringView::Chars<','>>();
			if (!r.is(')')) {
				sy = r.readFloat();
				if (IsErrorValue(sy)) {
					break;
				}
			}

			ret.scale(sx, (sy == 0.0f) ? sx : sy, 1.0f);

		} else if (r.is("rotate(")) {
			r += "rotate("_len;

			float angle = 0.0f;
			float cx = 0.0f, cy = 0.0f;

			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			angle = r.readFloat();
			if (IsErrorValue(angle)) {
				break;
			}

			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>, StringView::Chars<','>>();
			if (!r.is(')')) {
				cx = r.readFloat();
				if (IsErrorValue(cx)) {
					break;
				}

				r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>, StringView::Chars<','>>();
				cy = r.readFloat();
				if (IsErrorValue(cx)) {
					break;
				}
			}

			if (cx == 0.0f && cy == 0.0f) {
				ret.rotateZ(to_rad(angle));
			} else {
				// optimize matrix translate operations
				ret.m[12] += cx;
				ret.m[13] += cy;

				ret.rotateZ(to_rad(angle));

				ret.m[12] -= cx;
				ret.m[13] -= cy;
			}

		} else if (r.is("skewX(")) {
			r += "skewX("_len;

			float angle = 0.0f;
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			angle = r.readFloat();
			if (IsErrorValue(angle)) {
				break;
			}
			ret *= Mat4(1, 0, tanf(to_rad(angle)), 1, 0, 0);

		} else if (r.is("skewY(")) {
			r += "skewY("_len;

			float angle = 0.0f;
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			angle = r.readFloat();
			if (IsErrorValue(angle)) {
				break;
			}

			ret *= Mat4(1, tanf(to_rad(angle)), 0, 1, 0, 0);
		}
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		if (!r.is(')')) {
			break;
		} else {
			++ r;
		}
	}
	return ret;
}

static Rect svg_readViewBox(StringView &r) {
	float values[4] = { 0 };

	uint16_t i = 0;
	for (; i < 4; ++ i) {
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>, StringView::Chars<','>>();
		values[i] = r.readFloat();
		if (IsErrorValue(values[i])) {
			return Rect();
		}
	}

	return Rect(values[0], values[1], values[2], values[3]);
}

static float svg_readCoordValue(StringView &source, float origin) {
	style::Metric m; m.metric = style::Metric::Px;
	if (style::readStyleValue(source, m, false, true)) {
		switch (m.metric) {
		case style::Metric::Px:
			return m.value;
			break;
		case style::Metric::Percent:
			return m.value * origin;
			break;
		default:
			break;
		}
	}
	return nan();
}

static void svg_readPointCoords(Path &target, StringView &source) {
	float x, y;
	while (!source.empty()) {
		source.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>, StringView::Chars<','>>();
		x = source.readFloat();
		if (IsErrorValue(x)) {
			return;
		}

		source.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>, StringView::Chars<','>>();
		y = source.readFloat();
		if (IsErrorValue(y)) {
			return;
		}

		if (target.empty()) {
			target.moveTo(x, y);
		} else {
			target.lineTo(x, y);
		}
	}
}

struct SvgTag : public html::Tag<StringView> {
	SvgTag(StringView &r) : Tag(r) {
		//path.setStyle(Path::Style::None);
	}

	enum Shape {
		None,
		Rect,
		Circle,
		Ellipse,
		Line,
		Polyline,
		Polygon,
		Use,
	} shape = None;

	// Coords layouts:
	// Rect: x, y, width, height, rx,ry
	// Circle: cx, cy, r
	// Ellipse: cx, cy, rx, ry
	// Line: x1, y1, x2, y2
	// Polyline: write directly to path
	// Polygon: write directly to path
	float coords[6] = { nan() };
	Path path;
	StringView id;
};

struct SvgReader {
	using Parser = html::Parser<SvgReader, StringView, SvgTag>;
	using Tag = SvgTag;
	using StringReader = Parser::StringReader;

	SvgReader() { }

	inline void onBeginTag(Parser &p, Tag &tag) {
		if (!p.tagStack.empty()) {
			tag.path.setParams(p.tagStack.back().path.getParams());
		}

		if (tag.name.compare("rect")) {
			tag.shape = SvgTag::Rect;
		} else if (tag.name.compare("circle")) {
			tag.shape = SvgTag::Circle;
		} else if (tag.name.compare("ellipse")) {
			tag.shape = SvgTag::Ellipse;
		} else if (tag.name.compare("line")) {
			tag.shape = SvgTag::Line;
		} else if (tag.name.compare("polyline")) {
			tag.shape = SvgTag::Polyline;
		} else if (tag.name.compare("polygon")) {
			tag.shape = SvgTag::Polygon;
		} else if (tag.name.compare("use")) {
			tag.shape = SvgTag::Use;
		}
	}

	inline void onEndTag(Parser &p, Tag &tag) {
		if (tag.name.compare("svg")) {
			_squareLength = sqrtf((_width * _width + _height * _height) / 2.0f);
		}

		switch (tag.shape) {
		case SvgTag::Rect:
			if (!isnan(tag.coords[0]) && !isnan(tag.coords[1])
					&& !isnan(tag.coords[2]) && tag.coords[2] > 0.0f && !isnan(tag.coords[3]) && tag.coords[3] > 0.0f
					&& (isnan(tag.coords[4]) || tag.coords[4] >= 0.0f) && (isnan(tag.coords[5]) || tag.coords[5] >= 0.0f)) {
				tag.path.addRect(tag.coords[0], tag.coords[1], tag.coords[2], tag.coords[3], tag.coords[4], tag.coords[5]);
			}
			break;
		case SvgTag::Circle:
			if (!isnan(tag.coords[0]) && !isnan(tag.coords[1]) && !isnan(tag.coords[2]) && tag.coords[2] >= 0.0f) {
				tag.path.addCircle(tag.coords[0], tag.coords[1], tag.coords[2]);
			}
			break;
		case SvgTag::Ellipse:
			if (!isnan(tag.coords[0]) && !isnan(tag.coords[1]) && !isnan(tag.coords[2]) && !isnan(tag.coords[3]) && tag.coords[2] >= 0.0f && tag.coords[3] >= 0.0f) {
				tag.path.addEllipse(tag.coords[0], tag.coords[1], tag.coords[2], tag.coords[3]);
			}
			break;
		case SvgTag::Line:
			if (!isnan(tag.coords[0]) && !isnan(tag.coords[1]) && !isnan(tag.coords[2]) && !isnan(tag.coords[3])) {
				tag.path.moveTo(tag.coords[0], tag.coords[1]);
				tag.path.lineTo(tag.coords[2], tag.coords[3]);
			}
			break;
		case SvgTag::Polygon:
			if (!tag.path.empty()) {
				tag.path.closePath();
			}
			break;
		default:
			break;
		}
	}

	inline void onStyleParameter(Tag &tag, StringReader &name, StringReader &value) {
		if (name.compare("opacity")) {
			const float op = value.readFloat();
			if (!IsErrorValue(op)) {
				if (op <= 0.0f) {
					tag.path.setFillOpacity(0);
					tag.path.setStrokeOpacity(0);
				} else if (op >= 1.0f) {
					tag.path.setFillOpacity(255);
					tag.path.setStrokeOpacity(255);
				} else {
					tag.path.setFillOpacity(255 * op);
					tag.path.setStrokeOpacity(255 * op);
				}
			}
		} else if (name.compare("fill")) {
			if (value.compare("none")) {
				tag.path.setStyle(tag.path.getStyle() & (~DrawStyle::Fill));
			} else {
				Color3B color;
				if (style::readColor(value, color)) {
					tag.path.setFillColor(color, true);
					tag.path.setStyle(tag.path.getStyle() | DrawStyle::Fill);
				}
			}
		} else if (name.compare("fill-rule")) {
			if (value.compare("nonzero")) {
				tag.path.setWindingRule(Winding::NonZero);
			} else if (value.compare("evenodd")) {
				tag.path.setWindingRule(Winding::EvenOdd);
			}
		} else if (name.compare("fill-opacity")) {
			const float op = value.readFloat();
			if (!IsErrorValue(op)) {
				if (op <= 0.0f) {
					tag.path.setFillOpacity(0);
				} else if (op >= 1.0f) {
					tag.path.setFillOpacity(255);
				} else {
					tag.path.setFillOpacity(255 * op);
				}
			}
		} else if (name.compare("stroke")) {
			if (value.compare("none")) {
				tag.path.setStyle(tag.path.getStyle() & (~DrawStyle::Stroke));
			} else {
				Color3B color;
				if (style::readColor(value, color)) {
					tag.path.setStrokeColor(color, true);
					tag.path.setStyle(tag.path.getStyle() | DrawStyle::Stroke);
				}
			}
		} else if (name.compare("stroke-opacity")) {
			const float op = value.readFloat();
			if (!IsErrorValue(op)) {
				if (op <= 0.0f) {
					tag.path.setStrokeOpacity(0);
				} else if (op >= 1.0f) {
					tag.path.setStrokeOpacity(255);
				} else {
					tag.path.setStrokeOpacity(255 * op);
				}
			}
		} else if (name.compare("stroke-width")) {
			auto val = svg_readCoordValue(value, _squareLength);
			if (!isnan(val)) {
				tag.path.setStrokeWidth(val);
				tag.path.setStyle(tag.path.getStyle() | DrawStyle::Stroke);
			}
		} else if (name.compare("stroke-linecap")) {
			if (value.compare("butt")) {
				tag.path.setLineCup(LineCup::Butt);
			} else if (value.compare("round")) {
				tag.path.setLineCup(LineCup::Round);
			} else if (value.compare("square")) {
				tag.path.setLineCup(LineCup::Square);
			}
		} else if (name.compare("stroke-linejoin")) {
			if (value.compare("miter")) {
				tag.path.setLineJoin(LineJoin::Miter);
			} else if (value.compare("round")) {
				tag.path.setLineJoin(LineJoin::Round);
			} else if (value.compare("bevel")) {
				tag.path.setLineJoin(LineJoin::Bevel);
			}
		} else if (name.compare("stroke-miterlimit")) {
			const float op = value.readFloat();
			if (!IsErrorValue(op) && op > 1.0f) {
				tag.path.setMiterLimit(op);
			}
		}
	}

	inline void onStyle(Tag &tag, StringReader &value) {
		while (!value.empty()) {
			auto n = value.readUntil<StringReader::Chars<':'>>();
			if (value.is(':')) {
				++ value;
				auto v = value.readUntil<StringReader::Chars<';'>>();
				if (value.is(';')) {
					++ value;
				}
				if (!n.empty() && !v.empty()) {
					onStyleParameter(tag, n, v);
				}
			}
		}
	}

	inline void onTagAttribute(Parser &p, Tag &tag, StringReader &name, StringReader &value) {
		if (tag.name.compare("svg")) {
			if (name.compare("height")) {
				auto val = svg_readCoordValue(value, 0.0f);
				if (!isnan(val)) {
					_height = val;
				}
			} else if (name.compare("width")) {
				auto val = svg_readCoordValue(value, 0.0f);
				if (!isnan(val)) {
					_width = val;
				}
			} else if (name.compare("viewbox")) {
				_viewBox = svg_readViewBox(value);
			}
		} else if (tag.name.compare("path")) {
			if (name.compare("d")) {
				tag.path.init(value);
			}
		}

		if (name.compare("fill") || name.compare("fill-rule") || name.compare("fill-opacity") || name.compare("stroke")
				|| name.compare("stroke-opacity") || name.compare("stroke-width") || name.compare("stroke-linecap")
				|| name.compare("stroke-linejoin") || name.compare("stroke-miterlimit") || name.compare("opacity")) {
			onStyleParameter(tag, name, value);
		} else if (name.compare("transform")) {
			tag.path.applyTransform(svg_parseTransform(value));
		} else if (name.compare("style")) {
			onStyle(tag, value);
		} else if (name.compare("id")) {
			tag.id = value;
		} else {
			switch (tag.shape) {
			case SvgTag::Rect:
				if (name.compare("x")) {
					tag.coords[0] = svg_readCoordValue(value, _width);
				} else if (name.compare("y")) {
					tag.coords[1] = svg_readCoordValue(value, _height);
				} else if (name.compare("width")) {
					tag.coords[2] = svg_readCoordValue(value, _width);
				} else if (name.compare("height")) {
					tag.coords[3] = svg_readCoordValue(value, _height);
				} else if (name.compare("rx")) {
					tag.coords[4] = svg_readCoordValue(value, _width);
				} else if (name.compare("ry")) {
					tag.coords[5] = svg_readCoordValue(value, _height);
				}
				break;
			case SvgTag::Circle:
				if (name.compare("cx")) {
					tag.coords[0] = svg_readCoordValue(value, _width);
				} else if (name.compare("cy")) {
					tag.coords[1] = svg_readCoordValue(value, _height);
				} else if (name.compare("r")) {
					tag.coords[2] = svg_readCoordValue(value, _width);
				}
				break;
			case SvgTag::Ellipse:
				if (name.compare("cx")) {
					tag.coords[0] = svg_readCoordValue(value, _width);
				} else if (name.compare("cy")) {
					tag.coords[1] = svg_readCoordValue(value, _height);
				} else if (name.compare("rx")) {
					tag.coords[2] = svg_readCoordValue(value, _width);
				} else if (name.compare("ry")) {
					tag.coords[3] = svg_readCoordValue(value, _height);
				}
				break;
			case SvgTag::Line:
				if (name.compare("x1")) {
					tag.coords[0] = svg_readCoordValue(value, _width);
				} else if (name.compare("y1")) {
					tag.coords[1] = svg_readCoordValue(value, _height);
				} else if (name.compare("x2")) {
					tag.coords[2] = svg_readCoordValue(value, _width);
				} else if (name.compare("y2")) {
					tag.coords[3] = svg_readCoordValue(value, _height);
				}
				break;
			case SvgTag::Polyline:
				if (name.compare("points")) {
					svg_readPointCoords(tag.path, value);
				}
				break;
			case SvgTag::Polygon:
				if (name.compare("points")) {
					svg_readPointCoords(tag.path, value);
				}
				break;
			case SvgTag::Use:
				if (name.compare("x")) {
					tag.coords[0] = svg_readCoordValue(value, _width);
				} else if (name.compare("y")) {
					tag.coords[1] = svg_readCoordValue(value, _height);
				} else if (name.compare("xlink:href")) {
					tag.id = value;
				}
				break;
			default:
				break;
			}
		}
	}

	inline void onPushTag(Parser &p, Tag &tag) {
		if (tag.name == "defs") {
			_defs = true;
		}
	}
	inline void onPopTag(Parser &p, Tag &tag) {
		if (tag.name == "defs") {
			_defs = false;
		}
	}

	inline void onInlineTag(Parser &p, Tag &tag) {
		if (tag.shape == Tag::Shape::Use) {
			StringView ref(tag.id);
			if (ref.is('#')) { ++ ref; }
			auto pathIt = _paths.find(ref);
			if (pathIt != _paths.end()) {
				if (tag.coords[0] == 0.0f && tag.coords[1] == 0.0f) {
					_drawOrder.emplace_back(Image::PathXRef{ref.str()});
				} else {
					_drawOrder.emplace_back(Image::PathXRef{ref.str(), Vec2(tag.coords[0], tag.coords[1])});
				}
			}
		} else if (!tag.path.empty()) {
			String idStr;
			StringView id(tag.id);
			if (id.empty()) {
				idStr = toString("auto-", _nextId);
				++ _nextId;
				id = idStr;
			}

			_paths.emplace(id.str(), move(tag.path)).first;
			if (!_defs) {
				_drawOrder.emplace_back(Image::PathXRef{id.str()});
			}
		}
	}

	bool _defs = false;
	float _squareLength = 0.0f;
	float _width = 0;
	float _height = 0;
	uint16_t _nextId = 0;

	Rect _viewBox;
	Vector<Image::PathXRef> _drawOrder;
	Map<String, Path> _paths;
};

bool Image::isSvg(const StringView &str) {
	return Bitmap::check(Bitmap::FileFormat::Svg, (const uint8_t *)str.data(), str.size());
}

bool Image::isSvg(const Bytes &data) {
	return Bitmap::check(Bitmap::FileFormat::Svg, data.data(), data.size());
}

bool Image::isSvg(const FilePath &file) {
	auto d = filesystem::readFile(file.get(), 0, 512);
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
	}

	return false;
}

bool Image::init(FilePath &&path) {
	return init(filesystem::readTextFile(path.get()));
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
	return c.r == 0 && c.g == 0 && c.b == 0;
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

const Vector<Image::PathXRef> &Image::getDrawOrder() const {
	return _drawOrder;
}
void Image::setDrawOrder(const Vector<PathXRef> &vec) {
	_drawOrder = vec;
}
void Image::setDrawOrder(Vector<PathXRef> &&vec) {
	_drawOrder = move(vec);
}

Image::PathXRef Image::getDrawOrderPath(size_t size) const {
	if (size >= _drawOrder.size()) {
		return _drawOrder[size];
	}
	return PathXRef();
}
Image::PathXRef Image::addDrawOrderPath(const StringView &id, const Vec2 &pos) {
	auto it = _paths.find(id);
	if (it != _paths.end()) {
		_drawOrder.emplace_back(PathXRef{id.str(), pos});
		return _drawOrder.back();
	}
	return PathXRef();
}

void Image::clearDrawOrder() {
	_drawOrder.clear();
}

void Image::setViewBoxTransform(const Mat4 &m) {
	_viewBoxTransform = m;
}

const Mat4 &Image::getViewBoxTransform() const {
	return _viewBoxTransform;
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
