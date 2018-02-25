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

NS_SP_EXT_BEGIN(app)

bool AppLayout::init() {
	if (!Layout::init()) {
		return false;
	}

	auto button = Rc<material::FloatingActionButton>::create([this] { });
	button->setBackgroundColor(material::Color::Green_500);
	button->setAnchorPoint(Anchor::Middle);
	_button = addChildNode(button);

	auto button2 = Rc<material::FloatingActionButton>::create([this] { });
	button2->setBackgroundColor(material::Color::Green_500);
	button2->setAnchorPoint(Anchor::Middle);
	button2->setOpacity(127);
	_button2 = addChildNode(button2);

	return true;
}

void AppLayout::onContentSizeDirty() {
	Layout::onContentSizeDirty();

	_button->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
	_button2->setPosition(_contentSize.width / 2.0f, _contentSize.height / 4.0f);
}

NS_SP_EXT_END(app)
