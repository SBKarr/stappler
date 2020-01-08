/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#include "XLGestureData.h"

namespace stappler::xenolith::gesture {

String Touch::description() const {
	if (id == INT_MAX) {
		return toString(std::fixed, std::setprecision(2), "[N: (",
				startPoint.x, " : ", startPoint.y, ") -> (", point.x, " : ", point.y, ")]");
	} else if (id == -1) {
		return "[Invalid touch]";
	} else {
		return toString(std::fixed, std::setprecision(2), "[", id, ": (",
				startPoint.x, " : ", startPoint.y, ") -> (", point.x, " : ", point.y, ")]");
	}
}

const Vec2 &Touch::location() const {
	return point;
}

void Touch::cleanup() {
	id = -1;
	startPoint = Vec2::ZERO;
	prevPoint = Vec2::ZERO;
	point = Vec2::ZERO;
}

Touch::operator bool() const {
	return id == -1;
}

String Tap::description() const {
	return toString("Tap: ", touch.description(), " : [", count, "]");
}

const Vec2 &Tap::location() const {
	return touch.location();
}

void Tap::cleanup() {
	touch.cleanup();
	count = 0;
}

String Press::description() const {
	return toString("Press: ", touch.description(), " : [", time.toFloatSeconds(), "]");
}

const Vec2 &Press::location() const {
	return touch.point;
}

void Press::cleanup() {
	touch.cleanup();
	time.clear();
	count = 0;
}

String Swipe::description() const {
	return toString(std::fixed, std::setprecision(2), "Swipe: ", firstTouch.description(), " : [",
			delta.x, " : ", delta.y, " | ", velocity.x, " : ", velocity.y, "]");
}

const Vec2 &Swipe::location() const {
	return midpoint;
}

void Swipe::cleanup() {
	firstTouch.cleanup();
	secondTouch.cleanup();
	delta = Vec2::ZERO;
	velocity = Vec2::ZERO;
	midpoint = Vec2::ZERO;
}

void Pinch::set(const Touch &t1, const Touch &t2, float vel) {
	first = t1;
	second = t2;
	center = first.point.getMidpoint(second.point);

	distance = first.point.getDistance(second.point);
	startDistance = first.startPoint.getDistance(second.startPoint);
	prevDistance = first.prevPoint.getDistance(second.prevPoint);

	scale = distance / startDistance;
	velocity = vel;
}

String Pinch::description() const {
	return toString(std::fixed, std::setprecision(2), "Pinch: ", startDistance, " -> ", distance,
			" (", std::setprecision(4), scale, ") ", first.description(), " : ", second.description());
}

const Vec2 &Pinch::location() const {
	return center;
}

void Pinch::cleanup() {
	first.cleanup();
	second.cleanup();
	center = Vec2::ZERO;
	startDistance = 0.0f;
	prevDistance = 0.0f;
	distance = 0.0f;
	scale = 0.0f;
	velocity = 0.0f;
}

const Vec2 &Rotate::location() const {
	return center;
}
void Rotate::cleanup() {
	first.cleanup();
	second.cleanup();
	center = Vec2::ZERO;
	startAngle = 0.0f;
	prevAngle = 0.0f;
	angle = 0.0f;
	velocity = 0.0f;
}

const Vec2 &Wheel::location() const {
	return position.location();
}

void Wheel::cleanup() {
	position.cleanup();
	amount = Vec2::ZERO;
}

}
