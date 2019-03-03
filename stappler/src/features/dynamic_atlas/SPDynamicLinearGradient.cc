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

#include "SPDynamicLinearGradient.h"
#include "renderer/CCGLProgramState.h"

NS_SP_BEGIN

bool DynamicLinearGradient::init(bool isAbsolute) {
	_absolute = isAbsolute;
	return true;
}

bool DynamicLinearGradient::init(bool isAbsolute, const Vec2 &normalizedStart, const Vec2 &normalizedEnd, Vector<Stop> &&stops, const Size &size) {
	_absolute = isAbsolute;
	if (!size.equals(Size::ZERO)) {
		updateWithSize(size, Vec2::ZERO);
	}

	if (!updateWithData(normalizedStart, normalizedEnd, move(stops))) {
		return false;
	}

	return true;
}

bool DynamicLinearGradient::init(bool isAbsolute, const Vec2 &origin, float angle, float rad, Vector<Stop> &&stops, const Size &size) {
	return init(isAbsolute, origin, origin + Vec2::forAngle(angle) * rad, move(stops), size);
}

bool DynamicLinearGradient::updateWithData(const Vec2 &normalizedStart, const Vec2 &normalizedEnd, Vector<Stop> &&stops) {
	if (stops.size() > MAX_STOPS) {
		log::format("DynamicLinearGradient", "Only up to %d stops allowed (you provide %d)", int(MAX_STOPS), int(stops.size()));
		return false;
	}

	if (stops.size() < 2) {
		log::format("DynamicLinearGradient", "Gradient should specify at least 2 stops");
		return false;
	}

	_start = normalizedStart;
	_end = normalizedEnd;
	_stops = move(stops);

	_dirty = true;

	return true;
}

bool DynamicLinearGradient::updateWithData(const Vec2 &origin, float angle, float rad, Vector<Stop> &&stops) {
	return updateWithData(origin, origin + Vec2::forAngle(angle) * rad, move(stops));
}

void DynamicLinearGradient::updateWithSize(const Size &size, const Vec2 &origin) {
	if (!_targetSize.equals(size)) {
		_targetSize = size;
		_dirty = true;
	}
	_targetOrigin = origin;
}

bool DynamicLinearGradient::isAbsolute() const {
	return _absolute;
}

const Vec2 &DynamicLinearGradient::getStart() const {
	return _start;
}
const Vec2 &DynamicLinearGradient::getEnd() const {
	return _end;
}
const Size &DynamicLinearGradient::getTargetSize() const {
	return _targetSize;
}
const Vector<DynamicLinearGradient::Stop> &DynamicLinearGradient::getSteps() const {
	return _stops;
}

float DynamicLinearGradient::getAngle() const {
	return _angle;
}
const Vector<Vec4> &DynamicLinearGradient::getStopColors() const {
	return _stopsColors;
}
const Vector<float> &DynamicLinearGradient::getStopValues() const {
	return _stopsPositions;
}

void DynamicLinearGradient::updateUniforms(cocos2d::GLProgramState *prog) {
	if (_dirty) {
		rebuild();
		_dirty = false;
	}

	if (_stopsColors.size() < 2) {
		return;
	}

	if (_absolute) {
		prog->setUniformVec2("grad_origin", _targetOrigin);
		prog->setUniformVec2("grad_size",  Vec2(_targetSize.width, _targetSize.height));

		if (std::abs(_targetNormal.y) > std::abs(_targetNormal.x)) {
			prog->setUniformFloat("grad_axis", 0);
			prog->setUniformFloat("grad_tg_alpha", std::tan(M_PI_2 - _angle));
		} else {
			prog->setUniformFloat("grad_axis", 1);
			prog->setUniformFloat("grad_tg_alpha", std::tan(_angle));
		}
	} else {
		if (std::abs(_targetNormal.y) > std::abs(_targetNormal.x)) {
			prog->setUniformFloat("grad_axis", 0);
			prog->setUniformFloat("grad_tg_alpha", std::tan(M_PI_2 - _angle) * (_targetSize.width / _targetSize.height));
		} else {
			prog->setUniformFloat("grad_axis", 1);
			prog->setUniformFloat("grad_tg_alpha", std::tan(_angle) * (_targetSize.height / _targetSize.width));
		}
	}

	prog->setUniformInt("grad_numStops", int(_stopsColors.size()));
	prog->setUniformFloatv("grad_stops", _stopsPositions.size(), _stopsPositions.data());
	prog->setUniformVec4v("grad_colors", _stopsColors.size(), _stopsColors.data());
}

void DynamicLinearGradient::rebuild() {
	_stopsPositions.clear();
	_stopsColors.clear();

	Vec2 start(_start.x * _targetSize.width, _start.y * _targetSize.height);
	Vec2 end(_end.x * _targetSize.width, _end.y * _targetSize.height);

	auto norm = end - start;
	_angle = norm.getAngle();

	if (std::abs(norm.y) > std::abs(norm.x)) {
		if (_angle >= 0) {
			float newAngle = M_PI_2 - _angle;

			const float s = start.y + std::tan(newAngle) * start.x;
			const float e = end.y + std::tan(newAngle) * end.x;
			const float d = e - s;

			for (auto &it : _stops) {
				_stopsPositions.emplace_back((s + d * it.first) / _targetSize.height);
				_stopsColors.emplace_back(Vec4(it.second.r / 255.0f, it.second.g / 255.0f, it.second.b / 255.0f, it.second.a / 255.0f));
			}
		} else {
			Vec2 tmp = start; start = end; end = tmp; // swap bounds

			auto norm = end - start; // rebuild normal and angle
			_angle = norm.getAngle();
			float newAngle = M_PI_2 - _angle;

			const float s = start.y + std::tan(newAngle) * start.x;
			const float e = end.y + std::tan(newAngle) * end.x;
			const float d = e - s;

			for (auto it = _stops.rbegin(); it != _stops.rend(); ++ it) {
				_stopsPositions.emplace_back((s + d * (1.0f - it->first)) / _targetSize.height);
				_stopsColors.emplace_back(Vec4(it->second.r / 255.0f, it->second.g / 255.0f, it->second.b / 255.0f, it->second.a / 255.0f));
			}
		}
	} else {
		if (std::abs(_angle) < M_PI_2) {
			const float s = start.x + std::tan(_angle) * start.y;
			const float e = end.x + std::tan(_angle) * end.y;
			const float d = e - s;

			for (auto &it : _stops) {
				_stopsPositions.emplace_back((s + d * it.first) / _targetSize.width);
				_stopsColors.emplace_back(Vec4(it.second.r / 255.0f, it.second.g / 255.0f, it.second.b / 255.0f, it.second.a / 255.0f));
			}
		} else {
			Vec2 tmp = start; start = end; end = tmp; // swap bounds

			auto norm = end - start; // rebuild normal and angle
			_angle = norm.getAngle();

			const float s = start.x + std::tan(_angle) * start.y;
			const float e = end.x + std::tan(_angle) * end.y;
			const float d = e - s;

			for (auto it = _stops.rbegin(); it != _stops.rend(); ++ it) {
				_stopsPositions.emplace_back((s + d * (1.0f - it->first)) / _targetSize.width);
				_stopsColors.emplace_back(Vec4(it->second.r / 255.0f, it->second.g / 255.0f, it->second.b / 255.0f, it->second.a / 255.0f));
			}
		}
	}

	_targetNormal = norm;
}

NS_SP_END
