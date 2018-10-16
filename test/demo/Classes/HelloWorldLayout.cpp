// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "Material.h"
#include "HelloWorldLayout.h"

#include "MaterialLabel.h"

#include "SPGestureListener.h"
#include "SPActions.h"

NS_SP_EXT_BEGIN(app)

bool HelloWorldLayout::init(const data::Value &) {
	if (!Layout::init()) {
		return false;
	}

	auto l = Rc<gesture::Listener>::create();
	l->setTapCallback([this] (gesture::Event ev, const gesture::Tap &tap) -> bool {
		onTap(tap.location());
		return true;
	});
	_listener = addComponentItem(l);

	auto label = Rc<material::Label>::create(material::FontType::Title);
	label->setString("Hello world");
	label->setAnchorPoint(Anchor::Middle);
	label->setStandalone(true); // animated label should be standalone
	_label = addChildNode(label);

	return true;
}

void HelloWorldLayout::onContentSizeDirty() {
	Layout::onContentSizeDirty();

	_label->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
}

void HelloWorldLayout::onTap(const Vec2 &pos) {
	if (node::isTouched(_label, pos)) {
		_label->stopActionByTag("HelloWorldLayout::onTap"_tag);
		if (_label->getFontSize() > 20) {
			auto s = _label->getFontSize();
			auto a = Rc<ProgressAction>::create(0.35f, [this, s] (ProgressAction *a, float p) {
				_label->setFontSize(progress(int(s), 20, p));
			});
			_label->runAction(a, "HelloWorldLayout::onTap"_tag);
		} else {
			auto a = Rc<ProgressAction>::create(0.35f, [this] (ProgressAction *a, float p) {
				_label->setFontSize(progress(20, 28, p));
			});
			_label->runAction(a, "HelloWorldLayout::onTap"_tag);
		}
	}
}

NS_SP_EXT_END(app)
