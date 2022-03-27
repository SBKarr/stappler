/**
 Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SLSvgReader.h"

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
				if (!r.readFloat().grab(values[i])) {
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
			if (!r.readFloat().grab(tx)) {
				break;
			}

			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>, StringView::Chars<','>>();
			if (!r.is(')')) {
				if (!r.readFloat().grab(ty)) {
					break;
				}
			}

			ret.m[12] += tx;
			ret.m[13] += ty;

		} else if (r.is("scale(")) {
			r += "scale("_len;

			float sx = 0.0f, sy = 0.0f;
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			if (!r.readFloat().grab(sx)) {
				break;
			}

			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>, StringView::Chars<','>>();
			if (!r.is(')')) {
				if (!r.readFloat().grab(sy)) {
					break;
				}
			}

			ret.scale(sx, (sy == 0.0f) ? sx : sy, 1.0f);

		} else if (r.is("rotate(")) {
			r += "rotate("_len;

			float angle = 0.0f;
			float cx = 0.0f, cy = 0.0f;

			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			if (!r.readFloat().grab(angle)) {
				break;
			}

			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>, StringView::Chars<','>>();
			if (!r.is(')')) {
				if (!r.readFloat().grab(cx)) {
					break;
				}

				r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>, StringView::Chars<','>>();

				if (!r.readFloat().grab(cy)) {
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
			if (!r.readFloat().grab(angle)) {
				break;
			}
			ret *= Mat4(1, 0, tanf(to_rad(angle)), 1, 0, 0);

		} else if (r.is("skewY(")) {
			r += "skewY("_len;

			float angle = 0.0f;
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			if (!r.readFloat().grab(angle)) {
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
		if (!r.readFloat().grab(values[i])) {
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
		if (!source.readFloat().grab(x)) {
			return;
		}

		source.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>, StringView::Chars<','>>();
		if (!source.readFloat().grab(y)) {
			return;
		}

		if (target.empty()) {
			target.moveTo(x, y);
		} else {
			target.lineTo(x, y);
		}
	}
}

Path &SvgTag::getPath() {
	return rpath;
}

SvgReader::SvgReader() { }

void SvgReader::onBeginTag(Parser &p, Tag &tag) {
	if (!p.tagStack.empty()) {
		tag.rpath.setParams(p.tagStack.back().rpath.getParams());
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
		tag.mat = Mat4::IDENTITY;
	}
}

void SvgReader::onEndTag(Parser &p, Tag &tag, bool isClosed) {
	if (tag.name.compare("svg")) {
		_squareLength = sqrtf((_width * _width + _height * _height) / 2.0f);
	}

	switch (tag.shape) {
	case SvgTag::Rect:
		if (!isnan(tag.mat.m[0]) && !isnan(tag.mat.m[1])
				&& !isnan(tag.mat.m[2]) && tag.mat.m[2] > 0.0f && !isnan(tag.mat.m[3]) && tag.mat.m[3] > 0.0f
				&& (isnan(tag.mat.m[4]) || tag.mat.m[4] >= 0.0f) && (isnan(tag.mat.m[5]) || tag.mat.m[5] >= 0.0f)) {
			tag.getPath().addRect(tag.mat.m[0], tag.mat.m[1], tag.mat.m[2], tag.mat.m[3], tag.mat.m[4], tag.mat.m[5]);
		}
		break;
	case SvgTag::Circle:
		if (!isnan(tag.mat.m[0]) && !isnan(tag.mat.m[1]) && !isnan(tag.mat.m[2]) && tag.mat.m[2] >= 0.0f) {
			tag.getPath().addCircle(tag.mat.m[0], tag.mat.m[1], tag.mat.m[2]);
		}
		break;
	case SvgTag::Ellipse:
		if (!isnan(tag.mat.m[0]) && !isnan(tag.mat.m[1]) && !isnan(tag.mat.m[2]) && !isnan(tag.mat.m[3]) && tag.mat.m[2] >= 0.0f && tag.mat.m[3] >= 0.0f) {
			tag.getPath().addEllipse(tag.mat.m[0], tag.mat.m[1], tag.mat.m[2], tag.mat.m[3]);
		}
		break;
	case SvgTag::Line:
		if (!isnan(tag.mat.m[0]) && !isnan(tag.mat.m[1]) && !isnan(tag.mat.m[2]) && !isnan(tag.mat.m[3])) {
			tag.getPath().moveTo(tag.mat.m[0], tag.mat.m[1]);
			tag.getPath().lineTo(tag.mat.m[2], tag.mat.m[3]);
		}
		break;
	case SvgTag::Polygon:
		if (!tag.getPath().empty()) {
			tag.getPath().closePath();
		}
		break;
	default:
		break;
	}
}

void SvgReader::onStyleParameter(Tag &tag, StringReader &name, StringReader &value) {
	if (name.compare("opacity")) {
		value.readFloat().unwrap([&] (float op) {
			if (op <= 0.0f) {
				tag.getPath().setFillOpacity(0);
				tag.getPath().setStrokeOpacity(0);
			} else if (op >= 1.0f) {
				tag.getPath().setFillOpacity(255);
				tag.getPath().setStrokeOpacity(255);
			} else {
				tag.getPath().setFillOpacity(255 * op);
				tag.getPath().setStrokeOpacity(255 * op);
			}
		});
	} else if (name.compare("fill")) {
		if (value.compare("none")) {
			tag.getPath().setStyle(tag.getPath().getStyle() & (~DrawStyle::Fill));
		} else {
			Color3B color;
			if (style::readColor(value, color)) {
				tag.getPath().setFillColor(color, true);
				tag.getPath().setStyle(tag.getPath().getStyle() | DrawStyle::Fill);
			}
		}
	} else if (name.compare("fill-rule")) {
		if (value.compare("nonzero")) {
			tag.getPath().setWindingRule(Winding::NonZero);
		} else if (value.compare("evenodd")) {
			tag.getPath().setWindingRule(Winding::EvenOdd);
		}
	} else if (name.compare("fill-opacity")) {
		value.readFloat().unwrap([&] (float op) {
			if (op <= 0.0f) {
				tag.getPath().setFillOpacity(0);
			} else if (op >= 1.0f) {
				tag.getPath().setFillOpacity(255);
			} else {
				tag.getPath().setFillOpacity(255 * op);
			}
		});
	} else if (name.compare("stroke")) {
		if (value.compare("none")) {
			tag.getPath().setStyle(tag.getPath().getStyle() & (~DrawStyle::Stroke));
		} else {
			Color3B color;
			if (style::readColor(value, color)) {
				tag.getPath().setStrokeColor(color, true);
				tag.getPath().setStyle(tag.getPath().getStyle() | DrawStyle::Stroke);
			}
		}
	} else if (name.compare("stroke-opacity")) {
		value.readFloat().unwrap([&] (float op) {
			if (op <= 0.0f) {
				tag.getPath().setStrokeOpacity(0);
			} else if (op >= 1.0f) {
				tag.getPath().setStrokeOpacity(255);
			} else {
				tag.getPath().setStrokeOpacity(255 * op);
			}
		});
	} else if (name.compare("stroke-width")) {
		auto val = svg_readCoordValue(value, _squareLength);
		if (!isnan(val)) {
			tag.getPath().setStrokeWidth(val);
		}
	} else if (name.compare("stroke-linecap")) {
		if (value.compare("butt")) {
			tag.getPath().setLineCup(LineCup::Butt);
		} else if (value.compare("round")) {
			tag.getPath().setLineCup(LineCup::Round);
		} else if (value.compare("square")) {
			tag.getPath().setLineCup(LineCup::Square);
		}
	} else if (name.compare("stroke-linejoin")) {
		if (value.compare("miter")) {
			tag.getPath().setLineJoin(LineJoin::Miter);
		} else if (value.compare("round")) {
			tag.getPath().setLineJoin(LineJoin::Round);
		} else if (value.compare("bevel")) {
			tag.getPath().setLineJoin(LineJoin::Bevel);
		}
	} else if (name.compare("stroke-miterlimit")) {
		value.readFloat().unwrap([&] (float op) {
			if (op > 1.0f) {
				tag.getPath().setMiterLimit(op);
			}
		});
	}
}

void SvgReader::onStyle(Tag &tag, StringReader &value) {
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

void SvgReader::onTagAttribute(Parser &p, Tag &tag, StringReader &name, StringReader &value) {
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
			tag.getPath().init(value);
		}
	}

	if (name.compare("fill") || name.compare("fill-rule") || name.compare("fill-opacity") || name.compare("stroke")
			|| name.compare("stroke-opacity") || name.compare("stroke-width") || name.compare("stroke-linecap")
			|| name.compare("stroke-linejoin") || name.compare("stroke-miterlimit") || name.compare("opacity")) {
		onStyleParameter(tag, name, value);
	} else if (name.compare("transform")) {
		tag.getPath().applyTransform(svg_parseTransform(value));
	} else if (name.compare("style")) {
		onStyle(tag, value);
	} else if (name.compare("id")) {
		tag.id = value;
	} else {
		switch (tag.shape) {
		case SvgTag::Rect:
			if (name.compare("x")) {
				tag.mat.m[0] = svg_readCoordValue(value, _width);
			} else if (name.compare("y")) {
				tag.mat.m[1] = svg_readCoordValue(value, _height);
			} else if (name.compare("width")) {
				tag.mat.m[2] = svg_readCoordValue(value, _width);
			} else if (name.compare("height")) {
				tag.mat.m[3] = svg_readCoordValue(value, _height);
			} else if (name.compare("rx")) {
				tag.mat.m[4] = svg_readCoordValue(value, _width);
			} else if (name.compare("ry")) {
				tag.mat.m[5] = svg_readCoordValue(value, _height);
			}
			break;
		case SvgTag::Circle:
			if (name.compare("cx")) {
				tag.mat.m[0] = svg_readCoordValue(value, _width);
			} else if (name.compare("cy")) {
				tag.mat.m[1] = svg_readCoordValue(value, _height);
			} else if (name.compare("r")) {
				tag.mat.m[2] = svg_readCoordValue(value, _width);
			}
			break;
		case SvgTag::Ellipse:
			if (name.compare("cx")) {
				tag.mat.m[0] = svg_readCoordValue(value, _width);
			} else if (name.compare("cy")) {
				tag.mat.m[1] = svg_readCoordValue(value, _height);
			} else if (name.compare("rx")) {
				tag.mat.m[2] = svg_readCoordValue(value, _width);
			} else if (name.compare("ry")) {
				tag.mat.m[3] = svg_readCoordValue(value, _height);
			}
			break;
		case SvgTag::Line:
			if (name.compare("x1")) {
				tag.mat.m[0] = svg_readCoordValue(value, _width);
			} else if (name.compare("y1")) {
				tag.mat.m[1] = svg_readCoordValue(value, _height);
			} else if (name.compare("x2")) {
				tag.mat.m[2] = svg_readCoordValue(value, _width);
			} else if (name.compare("y2")) {
				tag.mat.m[3] = svg_readCoordValue(value, _height);
			}
			break;
		case SvgTag::Polyline:
			if (name.compare("points")) {
				svg_readPointCoords(tag.getPath(), value);
			}
			break;
		case SvgTag::Polygon:
			if (name.compare("points")) {
				svg_readPointCoords(tag.getPath(), value);
			}
			break;
		case SvgTag::Use:
			if (name.compare("x")) {
				tag.mat.translate(svg_readCoordValue(value, _width), 0.0f, 0.0f);
			} else if (name.compare("y")) {
				tag.mat.translate(0.0f, svg_readCoordValue(value, _width), 0.0f);
			} else if (name.compare("transform")) {
				tag.mat.multiply(svg_parseTransform(value));
			} else if (name.compare("id")) {
				tag.id = value;
			} else if (name.compare("xlink:href")) {
				tag.ref = value;
			}
			break;
		default:
			break;
		}
	}
}

void SvgReader::onPushTag(Parser &p, Tag &tag) {
	if (tag.name == "defs") {
		_defs = true;
	}
}
void SvgReader::onPopTag(Parser &p, Tag &tag) {
	if (tag.name == "defs") {
		_defs = false;
	}
}

void SvgReader::onInlineTag(Parser &p, Tag &tag) {
	if (tag.shape == Tag::Shape::Use) {
		StringView ref(tag.ref);
		if (ref.is('#')) { ++ ref; }
		auto pathIt = _paths.find(ref);
		if (pathIt != _paths.end()) {
			if (_defs) {
				if (!tag.id.empty()) {
					Path npath(pathIt->second);
					npath.applyTransform(tag.mat);
					_paths.emplace(tag.id.str(), move(npath));
				}
			} else {
				if (tag.mat.isIdentity()) {
					_drawOrder.emplace_back(PathXRef{ref.str()});
				} else {
					_drawOrder.emplace_back(PathXRef{ref.str(), tag.mat});
				}
			}
		}
	} else if (tag.rpath) {
		String idStr;
		StringView id(tag.id);
		if (id.empty()) {
			idStr = toString("auto-", _nextId);
			++ _nextId;
			id = idStr;
		}

		_paths.emplace(id.str(), move(tag.rpath));
		if (!_defs) {
			_drawOrder.emplace_back(PathXRef{id.str()});
		}
	}
}

NS_LAYOUT_END
