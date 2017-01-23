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

#include "Material.h"
#include "MaterialLineField.h"
#include "MaterialInputMenu.h"

#include "SPGestureListener.h"
#include "SPLayer.h"
#include "SPString.h"
#include "SPStrictNode.h"
#include "SPActions.h"
#include "SPDevice.h"

NS_MD_BEGIN

float LineField::getMaxLabelHeight(bool dense) {
	return dense?54.0f:64.0f;
}

void LineField::onInput() {
	auto labelWidth = _label->getContentSize().width;
	auto width = _node->getContentSize().width;
	auto cursor = _label->getCursor();
	if (cursor.start >= _label->getCharsCount()) {
		if (labelWidth > width) {
			runAdjust(width - labelWidth);
			return;
		}
	} else {
		auto pos = _label->getCursorMarkPosition();
		auto labelPos = pos.x + width / 2;
		if (labelWidth > width && labelPos > width) {
			auto min = width - labelWidth;
			auto max = 0.0f;
			auto newpos = width - labelPos;
			if (newpos < min) {
				newpos = min;
			} else if (newpos > max) {
				newpos = max;
			}

			runAdjust(newpos);
			return;
		}
	}

	FormField::onInput();
	runAdjust(0.0f);
}

void LineField::onMenuVisible() {
	auto pos = _label->getCursorMarkPosition();
	auto &tmp = _node->getNodeToParentTransform();
	Vec3 vec3(pos.x + _label->getPositionX(), pos.y + _label->getPositionY(), 0);
	Vec3 ret;
	tmp.transformPoint(vec3, &ret);

	setMenuPosition(Vec2(ret.x, _label->getPositionY() + _label->getFontHeight() / _label->getDensity() + 6.0f));
}

bool LineField::onInputString(const std::u16string &nstr, const Cursor &nc) {
	auto str = nstr;
	auto c = nc;

	auto pos = std::min(str.find(u'\n'), str.find(u'\r'));
	if (pos != std::u16string::npos) {
		str = str.substr(0, pos);
		c = Cursor(str.length(), 0);
		_label->releaseInput();
		if (_onInput) {
			_onInput();
		}
		return false;
	}

	return true;
}

bool LineField::onSwipeBegin(const Vec2 &loc, const Vec2 &delta) {
	if (FormField::onSwipeBegin(loc, delta)) {
		return true;
	}

	auto size = _label->getContentSize();
	if (size.width > _contentSize.width - _padding.horizontal() && fabsf(delta.x) > fabsf(delta.y)) {
		_swipeCaptured = true;
		return true;
	}

	return false;
}
bool LineField::onSwipe(const Vec2 &loc, const Vec2 &delta) {
	if (_swipeCaptured) {
		auto labelWidth = _label->getContentSize().width;
		auto width = _node->getContentSize().width;
		auto min = width - labelWidth - 2.0f;
		auto max = 0.0f;
		auto newpos = _label->getPositionX() + delta.x;
		if (newpos < min) {
			newpos = min;
		} else if (newpos > max) {
			newpos = max;
		}
		_label->stopAllActionsByTag("LineFieldAdjust"_tag);
		_label->setPositionX(newpos);
		return true;
	} else {
		if (FormField::onSwipe(loc, delta)) {
			auto labelWidth = _label->getContentSize().width;
			auto width = _node->getContentSize().width;
			if (labelWidth > width) {
				auto pos = convertToNodeSpace(loc);
				if (pos.x < _padding.left + 24.0f) {
					scheduleAdjust(Left, loc, _padding.left + 24.0f - pos.x);
				} else if (pos.x > _contentSize.width - _padding.right - 24.0f) {
					scheduleAdjust(Right, loc, pos.x - (_contentSize.width - _padding.right - 24.0f));
				} else {
					scheduleAdjust(None, loc, 0.0f);
				}
			}
			return true;
		}
		return false;
	}
}

bool LineField::onSwipeEnd(const Vec2 &vel) {
	if (_swipeCaptured) {
		_swipeCaptured = false;
		return true;
	} else {
		scheduleAdjust(None, Vec2(0.0f, 0.0f), 0.0f);
		return FormField::onSwipeEnd(vel);
	}
}

void LineField::runAdjust(float pos) {
	auto dist = fabs(_label->getPositionX() - pos);
	auto t = 0.1f;
	if (dist < 20.0f) {
		t = 0.1f;
	} else if (dist > 220.0f) {
		t = 0.35f;
	} else {
		t = progress(0.1f, 0.35f, (dist - 20.0f) / 200.0f);
	}
	auto a = cocos2d::MoveTo::create(t, Vec2(pos, _label->getPositionY()));
	_label->stopAllActionsByTag("LineFieldAdjust"_tag);
	_label->runAction(a, "LineFieldAdjust"_tag);
}

void LineField::scheduleAdjust(Adjust a, const Vec2 &vec, float pos) {
	_adjustValue = vec;
	_adjustPosition = pos;
	if (a != _adjust) {
		_adjust = a;
		switch (_adjust) {
		case None: unscheduleUpdate(); break;
		default: scheduleUpdate(); break;
		}
	}
}

void LineField::update(float dt) {
	auto labelWidth = _label->getContentSize().width;
	auto width = _node->getContentSize().width;
	auto min = width - labelWidth - 2.0f;
	auto max = 0.0f;
	auto newpos = _label->getPositionX();

	auto factor = std::min(32.0f, _adjustPosition);

	switch (_adjust) {
	case Left:
		newpos += (45.0f + progress(0.0f, 200.0f, factor / 32.0f)) * dt;
		break;
	case Right:
		newpos -= (45.0f + progress(0.0f, 200.0f, factor / 32.0f)) * dt;
		break;
	default:
		break;
	}

	if (newpos != _label->getPositionX()) {
		if (newpos < min) {
			newpos = min;
		} else if (newpos > max) {
			newpos = max;
		}
		_label->stopAllActionsByTag("LineFieldAdjust"_tag);
		_label->setPositionX(newpos);
		FormField::onSwipe(_adjustValue, Vec2::ZERO);
	}
}

NS_MD_END
