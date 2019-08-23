/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef STAPPLER_SRC_FEATURES_DYNAMIC_ATLAS_SPDYNAMICLINEARGRADIENT_H_
#define STAPPLER_SRC_FEATURES_DYNAMIC_ATLAS_SPDYNAMICLINEARGRADIENT_H_

#include "SPDefine.h"

NS_SP_BEGIN

class DynamicLinearGradient : public Ref {
public:
	static constexpr size_t MAX_STOPS = 16;

	using Stop = Pair<float, Color4B>;

	// By default, gradient uses texture coords for drawing (so, it transforms with texture
	//  (or correctly attributed quad array)
	// In absolute mode, gradient uses node screen space (useful to apply on objects in atlas, like font labels)
	// Default or absolute mode can be defined only once per gradient object

	bool init(bool isAbsolute);
	bool init(bool isAbsolute, const Vec2 &normalizedStart, const Vec2 &normalizedEnd, Vector<Stop> &&, const Size & = Size::ZERO);
	bool init(bool isAbsolute, const Vec2 &origin, float angle, float rad, Vector<Stop> &&, const Size & = Size::ZERO);

	bool updateWithData(const Vec2 &normalizedStart, const Vec2 &normalizedEnd, Vector<Stop> &&);
	bool updateWithData(const Vec2 &origin, float angle, float rad, Vector<Stop> &&);
	void updateWithSize(const Size &, const Vec2 &origin);

	bool isAbsolute() const;

	const Vec2 &getStart() const;
	const Vec2 &getEnd() const;
	const Size &getTargetSize() const;

	const Vector<Stop> &getSteps() const;

	float getAngle() const;
	const Vector<Vec4> &getStopColors() const;
	const Vector<float> &getStopValues() const;

protected:
	friend class DynamicBatchCommand;
	void updateUniforms(cocos2d::GLProgramState *);

	void rebuild();

protected:
	bool _absolute = false;
	bool _dirty = false;

	Vec2 _start; // normalized
	Vec2 _end; // normalized
	Vector<Stop> _stops;

	Vec2 _targetOrigin;
	Size _targetSize;
	Vec2 _targetNormal;

	float _angle = 0.0f;
	Vector<Vec4> _stopsColors;
	Vector<float> _stopsPositions;
};

NS_SP_END

#endif /* STAPPLER_SRC_FEATURES_DYNAMIC_ATLAS_SPDYNAMICLINEARGRADIENT_H_ */
