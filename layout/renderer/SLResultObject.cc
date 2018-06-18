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

#include "SLResultObject.h"

NS_LAYOUT_BEGIN

bool Object::isLink() const {
	return type == Type::Link;
}

bool Object::isLabel() const {
	return type == Type::Label;
}

bool Object::isBackground() const {
	return type == Type::Background;
}

const Link *Object::asLink() const {
	if (type == Type::Link) {
		return static_cast<const Link *>(this);
	}
	return nullptr;
}
const PathObject *Object::asPath() const {
	if (type == Type::Path) {
		return static_cast<const PathObject *>(this);
	}
	return nullptr;
}
const Label *Object::asLabel() const {
	if (type == Type::Label) {
		return static_cast<const Label *>(this);
	}
	return nullptr;
}
const Background *Object::asBackground() const {
	if (type == Type::Background) {
		return static_cast<const Background *>(this);
	}
	return nullptr;
}

bool BorderParams::merge(const BorderParams &p) {
	if (p.width > width) {
		*this = p;
		return true;
	}
	return false;
}

bool BorderParams::compare(const BorderParams &p) const {
	return p.style == style && p.width == width && p.color == color;
}

bool BorderParams::isVisible() const {
	return style != style::BorderStyle::None && width >= 0.5f && color.a > 0;
}

void PathObject::makeBorder(Result *res, Layout &l, const Rect &bb, const OutlineStyle &style, float w, const MediaParameters &media) {
	BorderParams left, top, right, bottom;
	if (style.top.style != style::BorderStyle::None) {
		top = BorderParams{style.top.style, style.top.width.computeValueAuto(w, media), style.top.color};
	}
	if (style.right.style != style::BorderStyle::None) {
		right = BorderParams{style.right.style, style.right.width.computeValueAuto(w, media), style.right.color};
	}
	if (style.bottom.style != style::BorderStyle::None) {
		bottom = BorderParams{style.bottom.style, style.bottom.width.computeValueAuto(w, media), style.bottom.color};
	}
	if (style.left.style != style::BorderStyle::None) {
		left = BorderParams{style.left.style, style.left.width.computeValueAuto(w, media), style.left.color};
	}

	if (left.compare(right) && top.compare(bottom) && left.compare(top)) {
		l.objects.emplace_back(res->emplaceOutline(l, Rect(
				bb.origin.x + left.width / 2.0f, bb.origin.y + left.width / 2.0f,
				bb.size.width - left.width, bb.size.height - left.width
			), left.color, left.width, left.style));
	} else {
		if (left.isVisible()) {
			auto p = res->emplacePath(l);
			p->drawVerticalLineSegment(bb.origin, bb.size.height,
				left.color, left.width, left.style,
				0.0f, 0.0f, top.width, bottom.width, 0.0f, 0.0f);
			l.objects.emplace_back(p);
		}
		if (top.isVisible()) {
			auto p = res->emplacePath(l);
			p->drawHorizontalLineSegment(bb.origin, bb.size.width,
				top.color, top.width, top.style,
				left.width, 0.0f, 0.0f, 0.0f, 0.0f, right.width);
			l.objects.emplace_back(p);
		}
		if (right.isVisible()) {
			auto p = res->emplacePath(l);
			p->drawVerticalLineSegment(Vec2(bb.origin.x + bb.size.width, bb.origin.y), bb.size.height,
				right.color, right.width, right.style,
				top.width, 0.0f, 0.0f, 0.0f, 0.0f, bottom.width);
			l.objects.emplace_back(p);
		}
		if (bottom.isVisible()) {
			auto p = res->emplacePath(l);
			p->drawHorizontalLineSegment(Vec2(bb.origin.x, bb.origin.y + bb.size.height), bb.size.width,
				bottom.color, bottom.width, bottom.style,
				0.0f, 0.0f, left.width, right.width, 0.0f, 0.0f);
			l.objects.emplace_back(p);
		}
	}
}

void PathObject::drawOutline(const Rect &bb, const Color4B &color, float width, style::BorderStyle style) {
	Rect rect(bb.origin.x - width, bb.origin.y - width, bb.size.width + width * 2.0f, bb.size.height + width * 2.0f);
	bbox = rect;
	path.setWindingRule(Winding::EvenOdd);
	path.setStyle(Path::Style::Fill);
	path.setFillColor(color);
	path.reserve(10, 1);

	path.moveTo(0.0f, 0.0f);
	path.lineTo(0.0f, rect.size.height);
	path.lineTo(rect.size.width, rect.size.height);
	path.lineTo(rect.size.width, 0.0f);
	path.closePath();

	path.moveTo(width, width);
	path.lineTo(rect.size.width - width, width);
	path.lineTo(rect.size.width - width, rect.size.height - width);
	path.lineTo(width, rect.size.height - width);
	path.closePath();
}

void PathObject::drawRect(const Rect &rect, const Color4B &color) {
	bbox = rect;
	path.reserve(5, 1);
	path.setStyle(Path::Style::Fill);
	path.setFillColor(color);
	path.addRect(0.0f, 0.0f, rect.size.width, rect.size.height);
}

void PathObject::drawVerticalLineSegment(const Vec2 &origin, float height, const Color4B &color, float border, style::BorderStyle style,
		float wTopLeft, float wTop, float wTopRight, float wBottomRight, float wBottom, float wBottomLeft) {
	path.setStyle(Path::Style::Fill);
	path.setFillColor(color);
	path.reserve(7, 1);

	const float extraTopSize = (wTop == 0.0f && ((wTopLeft == 0.0f && wTopRight != 0.0f) || (wTopLeft != 0.0f && wTopRight == 0.0f)))
			? max(wTopLeft, wTopRight) / 2.0f : 0.0f;
	const float extraBottomSize = (wBottom == 0.0f && ((wBottomLeft == 0.0f && wBottomRight != 0.0f) || (wBottomLeft != 0.0f && wBottomRight == 0.0f)))
			? max(wBottomLeft, wBottomRight) / 2.0f : 0.0f;

	const float originX = origin.x - border / 2.0f;
	const float originY = origin.y - extraTopSize;

	bbox = Rect( originX, originY, border, height + extraTopSize + extraBottomSize);

	if (wBottom == 0.0f) {
		if ((wBottomLeft == 0.0f && wBottomRight != 0.0f) || (wBottomLeft != 0.0f && wBottomRight == 0.0f)) {
			if (wBottomLeft == 0.0f) {
				path.moveTo(0.0f, 0.0f);
				path.lineTo(border, wBottomRight);
			} else {
				path.moveTo(0.0f, wBottomLeft);
				path.lineTo(border, 0.0f);
			}
		} else {
			path.moveTo(0.0f, (wBottomLeft == 0.0f) ? border / 2.0f : wBottomLeft / 2.0f);
			path.lineTo(border / 2.0f, 0.0f);
			path.lineTo(border, (wBottomRight == 0.0f) ? border / 2.0f : wBottomRight / 2.0f);
		}
	} else {
		if (wBottomLeft > 0.0f) {
			path.moveTo(0.0f, wBottomLeft / 2.0f);
		} else {
			path.moveTo(0.0f, 0.0f);
		}
		path.lineTo(border / 2.0f, 0.0f);
		if (wBottomRight > 0.0f) {
			path.lineTo(border, wBottomRight / 2.0f);
		} else {
			path.lineTo(border, 0.0f);
		}
	}

	if (wTop == 0.0f) {
		if ((wTopLeft == 0.0f && wTopRight != 0.0f) || (wTopLeft != 0.0f && wTopRight == 0.0f)) {
			if (wTopRight > 0.0f) {
				path.lineTo(border, bbox.size.height - wTopRight);
				path.lineTo(0.0f, bbox.size.height);
			} else {
				path.lineTo(border, bbox.size.height);
				path.lineTo(0.0f, bbox.size.height - wTopLeft);
			}
		} else {
			path.lineTo(border, bbox.size.height - ((wTopRight == 0.0f) ? border : wTopRight) / 2.0f);
			path.lineTo(border / 2.0f, bbox.size.height);
			path.lineTo(0.0f, bbox.size.height - ((wTopLeft == 0.0f) ? border : wTopLeft) / 2.0f);
		}
	} else {
		if (wTopRight > 0.0f) {
			path.lineTo(border, bbox.size.height - wTopRight / 2.0f);
		} else {
			path.lineTo(border, bbox.size.height);
		}
		path.lineTo(border / 2.0f, bbox.size.height);
		if (wTopLeft > 0.0f) {
			path.lineTo(0.0f, bbox.size.height - wTopLeft / 2.0f);
		} else {
			path.lineTo(0.0f, bbox.size.height);
		}
	}
	path.closePath();
}

void PathObject::drawHorizontalLineSegment(const Vec2 &origin, float width, const Color4B &color, float border, style::BorderStyle style,
		float wLeftBottom, float wLeft, float wLeftTop, float wRightTop, float wRight, float wRightBottom) {
	path.setStyle(Path::Style::Fill);
	path.setFillColor(color);
	path.reserve(7, 1);

	const float extraLeftSize = (wLeft == 0.0f && ((wLeftBottom == 0.0f && wLeftTop != 0.0f) || (wLeftBottom != 0.0f && wLeftTop == 0.0f)))
			? max(wLeftTop, wLeftBottom) / 2.0f : 0.0f;
	const float extraRightSize = (wRight == 0.0f && ((wRightBottom == 0.0f && wRightTop != 0.0f) || (wRightBottom != 0.0f && wRightTop == 0.0f)))
			? max(wRightTop, wRightBottom) / 2.0f : 0.0f;

	const float originX = origin.x - extraLeftSize;
	const float originY = origin.y - border / 2.0f;

	bbox = Rect( originX, originY, width + extraLeftSize + extraRightSize, border);

	if (wLeft == 0.0f) {
		if ((wLeftBottom == 0.0f && wLeftTop != 0.0f) || (wLeftBottom != 0.0f && wLeftTop == 0.0f)) {
			if (wLeftTop > 0.0f) {
				path.moveTo(0.0f, 0.0f);
				path.lineTo(wLeftTop, border);
			} else {
				path.moveTo(wLeftBottom, 0.0f);
				path.lineTo(0.0f, border);
			}
		} else {
			path.moveTo((wLeftBottom == 0.0f) ? border / 2.0f : wLeftBottom / 2.0f, 0.0f);
			path.lineTo(0.0f, border / 2.0f);
			path.lineTo((wLeftTop == 0.0f) ? border / 2.0f : wLeftTop / 2.0f, border);
		}
	} else {
		if (wLeftBottom > 0.0f) {
			path.moveTo(wLeftBottom / 2.0f, 0.0f);
		} else {
			path.moveTo(0.0f, 0.0f);
		}
		path.lineTo(0.0f, border / 2.0f);
		if (wLeftTop > 0.0f) {
			path.lineTo(wLeftTop / 2.0f, border);
		} else {
			path.lineTo(0.0f, border);
		}
	}

	if (wRight == 0.0f) {
		if ((wRightBottom == 0.0f && wRightTop != 0.0f) || (wRightBottom != 0.0f && wRightTop == 0.0f)) {
			if (wRightTop > 0.0f) {
				path.lineTo(bbox.size.width - wRightTop, border);
				path.lineTo(bbox.size.width, 0.0f);
			} else {
				path.lineTo(bbox.size.width, border);
				path.lineTo(bbox.size.width - wRightBottom, 0.0f);
			}
		} else {
			path.lineTo(bbox.size.width - ((wRightTop == 0.0f) ? border : wRightTop) / 2.0f, border);
			path.lineTo(bbox.size.width, border / 2.0f);
			path.lineTo(bbox.size.width - ((wRightBottom == 0.0f) ? border : wRightBottom) / 2.0f, 0.0f);
		}
	} else {
		if (wRightTop > 0.0f) {
			path.lineTo(bbox.size.width - wRightTop / 2.0f, border);
		} else {
			path.lineTo(bbox.size.width, border);
		}
		path.lineTo(bbox.size.width, border / 2.0f);
		if (wRightBottom > 0.0f) {
			path.lineTo(bbox.size.width - wRightBottom / 2.0f, 0.0f);
		} else {
			path.lineTo(bbox.size.width, 0.0f);
		}
	}
	path.closePath();
}

float Label::getTextIndent(float density) const {
	return format.chars.front().pos / density;
}

Rect Label::getLineRect(uint32_t lineId, float density, const Vec2 &origin) const {
	return format.getLineRect(lineId, density, origin);
}

Rect Label::getLineRect(const LineSpec &line, float density, const Vec2 &origin) const {
	return format.getLineRect(line, density, origin);
}

uint32_t Label::getLineForCharId(uint32_t id) const {
	return format.getLineNumber(id);
}

Vector<Rect> Label::getLabelRects(uint32_t firstCharId, uint32_t lastCharId, float density, const Vec2 &origin, const Padding &p) const {
	return format.getLabelRects(firstCharId, lastCharId, density, origin, p);
}

void Label::getLabelRects(Vector<Rect> &rect, uint32_t firstCharId, uint32_t lastCharId, float density, const Vec2 &origin, const Padding &p) const {
	return format.getLabelRects(rect, firstCharId, lastCharId, density, origin, p);
}

float Label::getLinePosition(uint32_t firstCharId, uint32_t lastCharId, float density) const {
	auto firstLine = format.getLine(firstCharId);
	auto lastLine = format.getLine(lastCharId);

	return ((firstLine->pos) / density + (lastLine->pos) / density) / 2.0f;
}

NS_LAYOUT_END
