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

#ifndef LIBS_STAPPLER_CORE_STRUCT_SPPADDING_H_
#define LIBS_STAPPLER_CORE_STRUCT_SPPADDING_H_

#include "SPDefine.h"
#include "math/CCGeometry.h"

NS_SP_BEGIN

struct Padding {
	float top = 0.0f;
	float right = 0.0f;
	float bottom = 0.0f;
	float left = 0.0f;

	inline float horizontal() const { return right + left; }
	inline float vertical() const { return top + bottom; }

	inline Vec2 getBottomLeft(const Size &size) const { return Vec2(left, bottom); }
	inline Vec2 getTopLeft(const Size &size) const { return Vec2(left, size.height - top); }

	inline Vec2 getBottomRight(const Size &size) const { return Vec2(size.width - right, bottom); }
	inline Vec2 getTopRight(const Size &size) const { return Vec2(size.width - right, size.height - top); }

	inline Padding &setTop(float value) { top = value; return *this; }
	inline Padding &setBottom(float value) { bottom = value; return *this; }
	inline Padding &setLeft(float value) { left = value; return *this; }
	inline Padding &setRight(float value) { right = value; return *this; }

	inline Padding &set(float vtop, float vright, float vbottom, float vleft) {
		top = vtop; right = vright; bottom = vbottom; left = vleft; return *this;
	}

	inline Padding &set(float vtop, float rightAndLeft, float vbottom) {
		top = vtop; right = rightAndLeft; bottom = vbottom; left = rightAndLeft; return *this;
	}

	inline Padding &set(float topAndBottom, float rightAndLeft) {
		top = topAndBottom; right = rightAndLeft; bottom = topAndBottom; left = rightAndLeft; return *this;
	}

	inline Padding &set(float all) {
		top = all; right = all; bottom = all; left = all; return *this;
	}

	Padding(float vtop, float vright, float vbottom, float vleft) {
		top = vtop; right = vright; bottom = vbottom; left = vleft;
	}

	Padding(float vtop, float rightAndLeft, float vbottom) {
		top = vtop; right = rightAndLeft; bottom = vbottom; left = rightAndLeft;
	}

	Padding(float topAndBottom, float rightAndLeft) {
		top = topAndBottom; right = rightAndLeft; bottom = topAndBottom; left = rightAndLeft;
	}

	Padding(float all) {
		top = all; right = all; bottom = all; left = all;
	}

	Padding() : Padding(0) { }

	inline bool operator == (const Padding &p) const { return top == p.top && bottom == p.bottom && left == p.left && right == p.right; }
	inline bool operator != (const Padding &p) const { return top != p.top || bottom != p.bottom || left != p.left || right != p.right; }

	inline Padding & operator *= (const float &v) { top *= v; right *= v; bottom *= v; left *= v; return *this; }
};

using Margin = Padding;

NS_SP_END

#endif /* LIBS_STAPPLER_CORE_STRUCT_SPPADDING_H_ */
