// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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
#include "AppLayout.h"

#include "MaterialFloatingActionButton.h"

#include "SPActions.h"

NS_SP_EXT_BEGIN(app)

bool AppLayout::init() {
	if (!Layout::init()) {
		return false;
	}

	/*auto button = Rc<material::IconSprite>::create();
	button->setColor(material::Color::Green_500);
	button->setAnchorPoint(Anchor::Middle);
	button->setIconName(material::IconName::Action_3d_rotation);
	_button = addChildNode(button);

	runAction(cocos2d::RepeatForever::create((cocos2d::ActionInterval *)action::sequence(2.0f, [this] {
		auto t = _button->getNodeToParentTransform();
		std::cout <<  t.m[12] << " " <<  t.m[13] << " " << _button->getAnchorPointInPoints() << "\n";
		_button->setIconName(material::IconName(toInt(_button->getIconName()) + 1));
	})));*/

	auto node = Rc<draw::PathNode>::create(100, 150);
	node->setAntialiased(false);
	node->addPath().moveTo(4.670644,147.500198).lineTo(104.670647,-2.499797).lineTo(-4.670644,-2.499797).lineTo(95.329353,147.500198)
		.moveTo(-4.670644,152.499802).lineTo(104.670647,-2.499797).lineTo(4.670644,2.499797).lineTo(104.670647,152.499802)
			.closePath().setWindingRule(layout::Winding::EvenOdd);
	node->setContentSize(Size(100, 150));
	node->setAnchorPoint(Anchor::Middle);
	node->setColor(Color::Black);
	_node = addChildNode(node);

	return true;
}

void AppLayout::onContentSizeDirty() {
	Layout::onContentSizeDirty();

	//_button->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
	_node->setPosition(_contentSize.width / 2.0f, _contentSize.height / 4.0f);
}

NS_SP_EXT_END(app)
