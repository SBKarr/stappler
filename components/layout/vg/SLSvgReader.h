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

#ifndef COMPONENTS_LAYOUT_VG_SLSVGREADER_H_
#define COMPONENTS_LAYOUT_VG_SLSVGREADER_H_

#include "SPBitmap.h"
#include "SPHtmlParser.h"
#include "SLPath.h"

NS_LAYOUT_BEGIN

struct SvgTag : public html::Tag<StringView> {
	SvgTag(StringView &r) : Tag(r) { }

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
	Mat4 mat = Mat4::INVALID;
	StringView id;
	StringView ref;
	Path rpath;

	Path &getPath();
};

struct SvgReader {
	using Parser = html::Parser<SvgReader, StringView, SvgTag>;
	using Tag = SvgTag;
	using StringReader = Parser::StringReader;

	SvgReader();

	void onBeginTag(Parser &p, Tag &tag);
	void onEndTag(Parser &p, Tag &tag, bool isClosed);
	void onStyleParameter(Tag &tag, StringReader &name, StringReader &value);
	void onStyle(Tag &tag, StringReader &value);
	void onTagAttribute(Parser &p, Tag &tag, StringReader &name, StringReader &value);
	void onPushTag(Parser &p, Tag &tag);
	void onPopTag(Parser &p, Tag &tag);
	void onInlineTag(Parser &p, Tag &tag);

	bool _defs = false;
	float _squareLength = 0.0f;
	float _width = 0;
	float _height = 0;
	uint16_t _nextId = 0;

	Rect _viewBox;
	Vector<PathXRef> _drawOrder;
	Map<String, Path> _paths;
};

NS_LAYOUT_END

#endif /* COMPONENTS_LAYOUT_VG_SLSVGREADER_H_ */
