// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPAccelerated.h"

#define SP_ACCELERATED_LOG(...)
//#define SP_ACCELERATED_LOG(...) stappler::log(__VA_ARGS__)

NS_SP_BEGIN

cocos2d::ActionInterval *Accelerated::createBounce(float acceleration, const Vec2 &from, const Vec2 &to, const Vec2 &velocity, float bounceAcceleration, std::function<void(cocos2d::Node *)> callback) {
	Vec2 diff = to - from;
	float distance = diff.getLength();

	if (fabsf(distance) < std::numeric_limits<float>::epsilon()) {
		return cocos2d::DelayTime::create(0.0f);
	}

    Vec2 normal = diff.getNormalized();
	normal.normalize();

    Vec2 velProject = velocity.project(normal);

    float startSpeed;
    if (fabsf(normal.getAngle(velProject)) < M_PI_2) {
    	startSpeed = velProject.getLength();
    } else {
    	startSpeed = -velProject.getLength();
    }

    return Accelerated::createBounce(acceleration, from, to, startSpeed, bounceAcceleration, callback);
}

cocos2d::ActionInterval *Accelerated::createBounce(float acceleration, const Vec2 &from, const Vec2 &to, float velocity, float bounceAcceleration, std::function<void(cocos2d::Node *)> callback) {
	Vec2 diff = to - from;
	float distance = diff.getLength();

	if (fabsf(distance) < std::numeric_limits<float>::epsilon()) {
		return cocos2d::DelayTime::create(0.0f);
	}

    Vec2 normal = diff.getNormalized();
	normal.normalize();

    float startSpeed = velocity;

	if (startSpeed == 0) {
		float duration = sqrtf(distance / acceleration);

		Accelerated *a = Accelerated::createWithDuration(duration, normal, from, startSpeed, acceleration);
		Accelerated *b = Accelerated::createWithDuration(duration, normal, a->getEndPosition(), a->getEndVelocity(), -acceleration);

		a->setCallback(callback);
		b->setCallback(callback);

		return cocos2d::Sequence::createWithTwoActions(a, b);
	} else {
		float t = startSpeed / acceleration;
		float result = (startSpeed * t) - (acceleration * t * t * 0.5);

		if (startSpeed > 0 && distance > result) {
			float pseudoDistance = distance + acceleration * t * t * 0.5;
			float pseudoDuration = sqrtf(pseudoDistance / acceleration);

			Accelerated *a = Accelerated::createAccelerationTo(normal, from, startSpeed, acceleration * pseudoDuration, acceleration);
			Accelerated *b = Accelerated::createWithDuration(pseudoDuration, normal, a->getEndPosition(), a->getEndVelocity(), -acceleration);

			a->setCallback(callback);
			b->setCallback(callback);

			return cocos2d::Sequence::createWithTwoActions(a, b);
		} else if (startSpeed > 0 && distance <= result) {
			if (bounceAcceleration == 0) {
				acceleration = -acceleration;
				float pseudoDistance = distance - result; // < 0
				float pseudoDuration = sqrtf(pseudoDistance / acceleration); // > 0

				Accelerated *a = Accelerated::createAccelerationTo(normal, from, startSpeed, acceleration * pseudoDuration, acceleration);
				Accelerated *b = Accelerated::createWithDuration(pseudoDuration, normal, a->getEndPosition(), a->getEndVelocity(), -acceleration);

				a->setCallback(callback);
				b->setCallback(callback);

				return cocos2d::Sequence::createWithTwoActions(a, b);
			} else {
				Accelerated *a0 = Accelerated::createAccelerationTo(from, to, startSpeed, -acceleration);
				Accelerated *a1 = Accelerated::createDecceleration(normal, a0->getEndPosition(), a0->getEndVelocity(), bounceAcceleration);

				Vec2 tmpFrom = a1->getEndPosition();
				diff = to - tmpFrom;
				distance = diff.getLength();
			    normal = diff.getNormalized();
				float duration = sqrtf(distance / acceleration);

				Accelerated *a = Accelerated::createWithDuration(duration, normal, tmpFrom, 0, acceleration);
				Accelerated *b = Accelerated::createWithDuration(duration, normal, a->getEndPosition(), a->getEndVelocity(), -acceleration);

				a0->setCallback(callback);
				a1->setCallback(callback);
				a->setCallback(callback);
				b->setCallback(callback);

				return cocos2d::Sequence::create(a0, a1, a, b, NULL);
			}
		} else {
			float pseudoDistance = 0;

			if (bounceAcceleration) {
				t = startSpeed / bounceAcceleration;
				pseudoDistance = distance + fabsf((float)((startSpeed * t) - bounceAcceleration * t * t * 0.5));
			} else {
				pseudoDistance = distance + fabsf((float)((startSpeed * t) - acceleration * t * t * 0.5));
			}

			float pseudoDuration = sqrtf(pseudoDistance / acceleration);

			Accelerated *a1 = NULL;
			Accelerated *a2 = NULL;
			if (bounceAcceleration) {
				a1 = Accelerated::createDecceleration(-normal, from, -startSpeed, bounceAcceleration);
				a2 = Accelerated::createAccelerationTo(normal, a1->getEndPosition(), 0, acceleration * pseudoDuration, acceleration);
			} else {
				a2 = Accelerated::createAccelerationTo(normal, from, startSpeed, acceleration * pseudoDuration, acceleration);
			}

			Accelerated *b = Accelerated::createDecceleration(normal, a2->getEndPosition(), a2->getEndVelocity(), acceleration);

			a2->setCallback(callback);
			b->setCallback(callback);

			if (a1) {
				a1->setCallback(callback);
				return cocos2d::Sequence::create(a1, a2, b, NULL);
			} else {
				return cocos2d::Sequence::createWithTwoActions(a2, b);
			}
		}
	}
}

cocos2d::ActionInterval *Accelerated::createFreeBounce(float acceleration, const Vec2 &from, const Vec2 &to, const Vec2 &velocity, float bounceAcceleration, std::function<void(cocos2d::Node *)> callback) {

	Vec2 diff = to - from;
	float distance = diff.getLength();
	Vec2 normal = diff.getNormalized();
	Vec2 velProject = velocity.project(normal);

    float startSpeed;
    if (fabsf(normal.getAngle(velProject)) < M_PI_2) {
    	startSpeed = velProject.getLength();
    } else {
    	startSpeed = -velProject.getLength();
    }

    if (startSpeed < 0) {
    	return createBounce(acceleration, from, to, velocity, bounceAcceleration, callback);
    } else {
    	float duration = startSpeed / acceleration;
    	float deccelerationPath = startSpeed * duration - acceleration * duration * duration * 0.5;
    	if (deccelerationPath < distance) {
    		Accelerated *a = createWithDuration(duration, normal, from, startSpeed, -acceleration);
			a->setCallback(callback);
			return a;
    	} else {
    		return createBounce(acceleration, from, to, velocity, bounceAcceleration, callback);
    	}
    }
}

cocos2d::ActionInterval *Accelerated::createWithBounds(float acceleration, const Vec2 &from, const Vec2 &velocity, const Rect &bounds, std::function<void(cocos2d::Node *)> callback) {
	Vec2 normal = velocity.getNormalized();

	float angle = normal.getAngle();

	Vec2 x, y, z, a, b, i;

	if (bounds.size.width == 0 && bounds.size.height == 0) {
		return nullptr;
	}

	if (bounds.size.width == 0) {
		float start = bounds.origin.y;
		float end = bounds.origin.y + bounds.size.height;

		float pos = from.y;

		float v = velocity.y;
		float t = fabsf(v) / fabsf(acceleration);
		float dist = fabsf(v) * t - fabsf(acceleration) * t * t * 0.5;

		if (velocity.y > 0) {
			if (pos == end) {
				return nullptr;
			} else if (pos + dist < end) {
				return createDecceleration(cocos2d::Vec2(0, 1), from, velocity.y, -acceleration);
			} else {
				return createAccelerationTo(from, cocos2d::Vec2(from.x, end), v, -acceleration);
			}
		} else {
			if (pos == start) {
				return nullptr;
			} else if (pos - dist > start) {
				return createDecceleration(cocos2d::Vec2(0, -1), from, velocity.y, -acceleration);
			} else {
				return createAccelerationTo(from, cocos2d::Vec2(from.x, start), fabs(v), -acceleration);
			}
		}
	}

	if (bounds.size.height == 0) {
		float start = bounds.origin.x;
		float end = bounds.origin.x + bounds.size.width;

		float pos = from.x;

		float v = fabsf(velocity.x);
		float t = v / fabsf(acceleration);
		float dist = v * t - fabsf(acceleration) * t * t * 0.5;

		if (velocity.x > 0) {
			if (pos == end) {
				return nullptr;
			} else if (pos + dist < end) {
				return createDecceleration(cocos2d::Vec2(1, 0), from, v, -acceleration);
			} else {
				return createAccelerationTo(from, cocos2d::Vec2(end, from.y), v, -acceleration);
			}
		} else {
			if (pos == start) {
				return nullptr;
			} else if (pos - dist > start) {
				return createDecceleration(cocos2d::Vec2(-1, 0), from, v, -acceleration);
			} else {
				return createAccelerationTo(from, cocos2d::Vec2(start, from.y), v, -acceleration);
			}
		}
	}

	if (angle < -M_PI_2) {
		x = Vec2(bounds.getMinX(), bounds.getMaxY());
		y = Vec2(bounds.getMinX(), bounds.getMinY());
		z = Vec2(bounds.getMaxX(), bounds.getMinY());

		a = (normal.x == 0)?from:Vec2::getIntersectPoint(from, from + normal, x, y);
		b = (normal.y == 0)?from:Vec2::getIntersectPoint(from, from + normal, y, z);
	} else if (angle < 0) {
		x = Vec2(bounds.getMinX(), bounds.getMinY());
		y = Vec2(bounds.getMaxX(), bounds.getMinY());
		z = Vec2(bounds.getMaxX(), bounds.getMaxY());

		a = (normal.x == 0)?from:Vec2::getIntersectPoint(from, from + normal, x, y);
		b = (normal.y == 0)?from:Vec2::getIntersectPoint(from, from + normal, y, z);
	} else if (angle < M_PI_2) {
		x = Vec2(bounds.getMaxX(), bounds.getMinY());
		y = Vec2(bounds.getMaxX(), bounds.getMaxY());
		z = Vec2(bounds.getMinX(), bounds.getMaxY());

		a = (normal.y == 0)?from:Vec2::getIntersectPoint(from, from + normal, x, y);
		b = (normal.x == 0)?from:Vec2::getIntersectPoint(from, from + normal, y, z);
	} else {
		x = Vec2(bounds.getMaxX(), bounds.getMaxY());
		y = Vec2(bounds.getMinX(), bounds.getMaxY());
		z = Vec2(bounds.getMinX(), bounds.getMinY());

		a = (normal.y == 0)?from:Vec2::getIntersectPoint(from, from + normal, x, y);
		b = (normal.x == 0)?from:Vec2::getIntersectPoint(from, from + normal, y, z);
	}

	float a_len = from.getDistance(a);
	float b_len = from.getDistance(b);
	float i_len, s_len;

	if (a_len < b_len) {
		i = a; i_len = a_len; s_len = b_len;
	} else {
		i = b; i_len = b_len; s_len = a_len;
	}

	float v = velocity.getLength();
	float t = v / acceleration;
	float path = v * t - acceleration * t * t * 0.5;

	if (path < i_len) {
		Accelerated *a1 = createDecceleration(normal, from, v, acceleration);
		a1->setCallback(callback);
		return a1;
	} else {
		Accelerated *a1 = createAccelerationTo(from, i, v, -acceleration);
		a1->setCallback(callback);

		if (s_len > 0) {
			Vec2 diff = y - i;
			if (diff.length() < std::numeric_limits<float>::epsilon()) {
				return a1;
			}
			Vec2 newNormal = diff.getNormalized();

			i_len = diff.getLength();

			acceleration = (normal * acceleration).project(newNormal).getLength();
			if (acceleration == 0.0f) {
				return a1;
			}
			v = (normal * a1->getEndVelocity()).project(newNormal).getLength();
			t = v / acceleration;
			path = v * t - acceleration * t * t * 0.5;

			if (path < i_len) {
				Accelerated *a2 = createDecceleration(newNormal, i, v, acceleration);
				a2->setCallback(callback);
				return cocos2d::Sequence::createWithTwoActions(a1, a2);
			} else {
				Accelerated *a2 = createAccelerationTo(i, y, v, -acceleration);
				a2->setCallback(callback);
				return cocos2d::Sequence::createWithTwoActions(a1, a2);
			}
		} else {
			return a1;
		}
	}
}

cocos2d::Vec2 Accelerated::computeEndPoint() {
	return _startPoint + (_normalPoint * ((_startVelocity * _duration) + (_acceleration * _duration * _duration * 0.5)));
}

cocos2d::Vec2 Accelerated::computeNormalPoint() {
	return (_endPoint - _startPoint).getNormalized();
}

float Accelerated::computeEndVelocity() {
	return _startVelocity + _acceleration * _duration;
}

Accelerated *Accelerated::createDecceleration(const Vec2 &normal, const Vec2 &startPoint, float startVelocity, float acceleration) {
	Accelerated *pRet = new Accelerated();
	pRet->initDecceleration(normal, startPoint, startVelocity, acceleration);
	pRet->autorelease();
	return pRet;
}

bool Accelerated::initDecceleration(const Vec2 &normal, const Vec2 &startPoint, float startVelocity, float acceleration) {
	acceleration = fabsf(acceleration);
	startVelocity = fabsf(startVelocity);
	if (startVelocity < 0 || acceleration < 0) {
		SP_ACCELERATED_LOG("Deceleration failed: velocity:%f acceleration:%f", startVelocity, acceleration);
		return false;
	}

	_duration = startVelocity / acceleration;

	if (!ActionInterval::initWithDuration(_duration)) {
		return false;
	}

	_acceleration = -acceleration;
	_startVelocity = startVelocity;
	_endVelocity = 0;

	_normalPoint = normal;
	_startPoint = startPoint;

	_endPoint = computeEndPoint();

	SP_ACCELERATED_LOG("%s %d Acceleration:%f velocity:%f->%f duration:%f position:(%f %f)->(%f %f)",
			__FUNCTION__, __LINE__,
			_acceleration, _startVelocity, _endVelocity, _duration,
			_startPoint.x, _startPoint.y, _endPoint.x, _endPoint.y);

	if (isnan(_endPoint.x) || isnan(_endPoint.y) || isnan(_duration)) {
		SP_ACCELERATED_LOG("Failed!");
		return false;
	}

	return true;
}

Accelerated *Accelerated::createDecceleration(const Vec2 & startPoint, const Vec2 & endPoint, float acceleration) {
	Accelerated *pRet = new Accelerated();
	pRet->initDecceleration(startPoint, endPoint, acceleration);
	pRet->autorelease();
	return pRet;
}

bool Accelerated::initDecceleration(const Vec2 & startPoint, const Vec2 & endPoint, float acceleration) {
	float distance = startPoint.distance(endPoint);
	acceleration = fabsf(acceleration);

	if (distance == 0.0f) {
		_duration = 0.0f;
	} else {
		_duration =  sqrtf((distance * 2.0f) / acceleration);
	}

	if (!ActionInterval::initWithDuration(_duration)) {
		return false;
	}

	_acceleration = -acceleration;
	_startVelocity = _duration * acceleration;
	_endVelocity = 0;

	_startPoint = startPoint;
	_endPoint = endPoint;
	_normalPoint = computeNormalPoint();

	SP_ACCELERATED_LOG("%s %d Acceleration:%f velocity:%f->%f duration:%f position:(%f %f)->(%f %f)",
			__FUNCTION__, __LINE__,
			_acceleration, _startVelocity, _endVelocity, _duration,
			_startPoint.x, _startPoint.y, _endPoint.x, _endPoint.y);

	if (isnan(_endPoint.x) || isnan(_endPoint.y) || isnan(_duration)) {
		SP_ACCELERATED_LOG("Failed!");
		return false;
	}

	return true;
}

Accelerated *Accelerated::createAccelerationTo( const Vec2 & normal, const Vec2 & startPoint, float startVelocity, float endVelocity, float acceleration) {

	Accelerated *pRet = new Accelerated();
	pRet->initAccelerationTo(normal, startPoint, startVelocity, endVelocity, acceleration);
	pRet->autorelease();
	return pRet;
}

bool Accelerated::initAccelerationTo(const Vec2 & normal, const Vec2 & startPoint, float startVelocity, float endVelocity, float acceleration) {

	_duration = (endVelocity - startVelocity) / acceleration;

	if (_duration < 0) {
		SP_ACCELERATED_LOG("AccelerationTo failed: velocity:(%f:%f) acceleration:%f", startVelocity, endVelocity, acceleration);
		return false;
	}

	if (!ActionInterval::initWithDuration(_duration)) {
		return false;
	}

	_acceleration = acceleration;
	_startVelocity = startVelocity;
	_endVelocity = endVelocity;

	_normalPoint = normal;
	_startPoint = startPoint;

	_endPoint = computeEndPoint();

	SP_ACCELERATED_LOG("%s %d Acceleration:%f velocity:%f->%f duration:%f position:(%f %f)->(%f %f)",
			__FUNCTION__, __LINE__,
			_acceleration, _startVelocity, _endVelocity, _duration,
			_startPoint.x, _startPoint.y, _endPoint.x, _endPoint.y);

	if (isnan(_endPoint.x) || isnan(_endPoint.y) || isnan(_duration)) {
		SP_ACCELERATED_LOG("Failed!");
		return false;
	}

	return true;
}

Accelerated *Accelerated::createAccelerationTo(const Vec2 & startPoint, const Vec2 & endPoint, float startVelocity, float acceleration) {

	Accelerated *pRet = new Accelerated();
	pRet->initAccelerationTo(startPoint, endPoint, startVelocity, acceleration);
	pRet->autorelease();
	return pRet;
}

bool Accelerated::initAccelerationTo(const Vec2 & startPoint, const Vec2 & endPoint, float startVelocity, float acceleration) {

	float distance = - endPoint.getDistance(startPoint);
	float d = startVelocity * startVelocity - 2 * acceleration * distance;

	if (distance == 0.0f) {
		SP_ACCELERATED_LOG("zero distance");
	}

	if (d < 0) {
		SP_ACCELERATED_LOG("AccelerationTo failed: acceleration:%f velocity:%f D:%f", acceleration, startVelocity, d);
		return false;
	}

	float t1 = (-startVelocity + sqrtf(d)) / acceleration;
	float t2 = (-startVelocity - sqrtf(d)) / acceleration;


	if (distance != 0.0f) {
		_duration = t1 < 0 ? t2 : (t2 < 0 ? t1 : MIN(t1, t2));
		if (isnan(_duration)) {
			_duration = 0.0f;
		}
	} else {
		_duration = 0.0f;
	}

	if (_duration < 0) {
		if (d < 0) {
			SP_ACCELERATED_LOG("AccelerationTo failed: acceleration:%f velocity:%f duration:%f", acceleration, startVelocity, _duration);
			return false;
		}
	}

	if (!ActionInterval::initWithDuration(_duration)) {
		return false;
	}

	_startPoint = startPoint;
	_endPoint = endPoint;
	_normalPoint = computeNormalPoint();

	_acceleration = acceleration;
	_startVelocity = startVelocity;
	_endVelocity = computeEndVelocity();

	SP_ACCELERATED_LOG("%s %d Acceleration:%f velocity:%f->%f duration:%f position:(%f %f)->(%f %f)",
			__FUNCTION__, __LINE__,
			_acceleration, _startVelocity, _endVelocity, _duration,
			_startPoint.x, _startPoint.y, _endPoint.x, _endPoint.y);

	if (isnan(_endPoint.x) || isnan(_endPoint.y) || isnan(_duration)) {
		SP_ACCELERATED_LOG("Failed!");
		return false;
	}

	return true;
}

Accelerated *Accelerated::createWithDuration(float duration, const Vec2 &normal, const Vec2 &startPoint, float startVelocity, float acceleration) {

	Accelerated *pRet = new Accelerated();
	pRet->initWithDuration(duration, normal, startPoint, startVelocity, acceleration);
	pRet->autorelease();
	return pRet;
}

bool Accelerated::initWithDuration(float duration, const Vec2 &normal, const Vec2 &startPoint, float startVelocity, float acceleration) {

	_duration = duration;

	if (!ActionInterval::initWithDuration(_duration)) {
		return false;
	}

	_normalPoint = normal;
	_startPoint = startPoint;

	_acceleration = acceleration;
	_startVelocity = startVelocity;

	_endVelocity = computeEndVelocity();
	_endPoint = computeEndPoint();

	SP_ACCELERATED_LOG("%s %d Acceleration:%f velocity:%f->%f duration:%f position:(%f %f)->(%f %f)",
			__FUNCTION__, __LINE__,
			_acceleration, _startVelocity, _endVelocity, _duration,
			_startPoint.x, _startPoint.y, _endPoint.x, _endPoint.y);

	if (isnan(_endPoint.x) || isnan(_endPoint.y) || isnan(_duration)) {
		SP_ACCELERATED_LOG("Failed!");
		return false;
	}

	return true;
}

float Accelerated::getDuration() const {
	return _duration;
}

cocos2d::Vec2 Accelerated::getPosition(float timePercent) const {
	float t = timePercent * _duration;
	return _startPoint + _normalPoint * ((_startVelocity * t) + (_acceleration * t * t * 0.5));
}

const cocos2d::Vec2 &Accelerated::getStartPosition() const {
	return _startPoint;
}

const cocos2d::Vec2 &Accelerated::getEndPosition() const {
	return _endPoint;
}

const cocos2d::Vec2 &Accelerated::getNormal() const {
	return _normalPoint;
}

float Accelerated::getStartVelocity() const {
	return _startVelocity;
}

float Accelerated::getEndVelocity() const {
	return _endVelocity;
}

float Accelerated::getCurrentVelocity() const {
	return _startVelocity + _acceleration * _elapsed;
}

void Accelerated::startWithTarget(cocos2d::Node *target) {
	cocos2d::ActionInterval::startWithTarget(target);
	SP_ACCELERATED_LOG("Acceleration:%f velocity:%f->%f duration:%f position:(%f %f)->(%f %f)",
			_acceleration, _startVelocity, _endVelocity, _duration,
			_startPoint.x, _startPoint.y, _endPoint.x, _endPoint.y);
}

cocos2d::ActionInterval* Accelerated::clone() const {
	auto copy = new Accelerated();
    copy->initWithDuration(_duration, _normalPoint, _startPoint, _startVelocity, _acceleration);
    return copy;
}

cocos2d::ActionInterval* Accelerated::reverse(void) const {
    return Accelerated::createWithDuration(_duration, - _normalPoint,
										   _endPoint, _endVelocity, _acceleration);
}

void Accelerated::update(float t) {
    if (_target) {
        Vec2 pos = getPosition(t);
    	_target->setPosition(pos);
		if (_callback) {
			_callback(_target);
		}
    }
}

void Accelerated::setCallback(std::function<void(cocos2d::Node *)> callback) {
	_callback = callback;
}

NS_SP_END
