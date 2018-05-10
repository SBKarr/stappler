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
	_node->onInput();
	FormField::onInput();
}
void LineField::onCursor(const Cursor &c) {
	_node->onCursor();
	FormField::onCursor(c);
}

void LineField::onMenuVisible() {
	auto pos = _label->getCursorMarkPosition();
	auto &tmp = _node->getNodeToParentTransform();
	Vec3 vec3(pos.x + _label->getPositionX(), pos.y + _label->getPositionY(), 0);
	Vec3 ret;
	tmp.transformPoint(vec3, &ret);

	setMenuPosition(Vec2(ret.x, _label->getPositionY() + _label->getFontHeight() / _label->getDensity() + 6.0f));
}

bool LineField::onInputString(const WideStringView &nstr, const Cursor &nc) {
	auto str = nstr;
	auto c = nc;

	auto pos = std::min(str.find(u'\n'), str.find(u'\r'));
	if (pos != std::u16string::npos) {
		str = str.sub(0, pos);
		c = Cursor(uint32_t(str.size()), 0);
		_label->releaseInput();
		if (_onInput) {
			_onInput();
		}
		return false;
	}

	return true;
}

bool LineField::onSwipeBegin(const Vec2 &loc, const Vec2 &delta) {
	return _node->onSwipeBegin(loc, delta);
}
bool LineField::onSwipe(const Vec2 &loc, const Vec2 &delta) {
	return _node->onSwipe(loc, delta);
}
bool LineField::onSwipeEnd(const Vec2 &vel) {
	return _node->onSwipeEnd(vel);
}

Rc<InputLabel> LineField::makeLabel(FontType font) {
	auto l = FormField::makeLabel(font);
	l->setAllowMultiline(false);
	return l;
}

NS_MD_END
