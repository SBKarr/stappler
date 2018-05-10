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

#ifndef LAYOUT_RENDERER_SLFLOATCONTEXT_H_
#define LAYOUT_RENDERER_SLFLOATCONTEXT_H_

#include "SLLayout.h"

NS_LAYOUT_BEGIN

using FloatStack = Vector< Rect >;

struct FloatContext {
	Layout * root;
	float pageHeight = nan();
	FloatStack floatRight;
	FloatStack floatLeft;

	Pair<float, float> getAvailablePosition(float yPos, float height) const;

	bool pushFloatingNode(Layout &origin, Layout &l, Vec2 &vec);
	bool pushFloatingNodeToStack(Layout &origin, Layout &l, Rect &s, const Rect &bbox, Vec2 &vec);
	bool pushFloatingNodeToNewStack(Layout &origin, Layout &l, FloatStack &s, const Rect &bbox, Vec2 &vec);
};

NS_LAYOUT_END

#endif /* LAYOUT_RENDERER_SLFLOATCONTEXT_H_ */
