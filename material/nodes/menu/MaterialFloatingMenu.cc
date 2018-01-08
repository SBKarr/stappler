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
#include "MaterialFloatingMenu.h"
#include "MaterialForegroundLayer.h"
#include "MaterialScene.h"
#include "MaterialResize.h"
#include "MaterialMenuButton.h"
#include "2d/CCActionInterval.h"
#include "2d/CCActionInstant.h"

#include "SPScrollView.h"
#include "SPDataListener.h"

NS_MD_BEGIN

void FloatingMenu::push(MenuSource *source, const Vec2 &globalOrigin, Binding b, FloatingMenu *root) {
	auto m = Rc<FloatingMenu>::create(source, root);
	m->pushMenu(globalOrigin, b);
}

bool FloatingMenu::init(MenuSource *source, FloatingMenu *root) {
	if (!Menu::init(source)) {
		return false;
	}

	setMenuButtonCallback(std::bind(&FloatingMenu::onMenuButton, this, std::placeholders::_1));

	float w = getMenuWidth(source);
	float h = getMenuHeight(w, source);

	_fullSize = Size(w, h);
	_root = root;
	_scroll->setVisible(false);

	setShadowZIndex(1.5f);

	return true;
}

void FloatingMenu::pushMenu(const Vec2 &o, Binding b) {
	_binding = b;

	auto scene = Scene::getRunningScene();
	auto f = scene->getForegroundLayer();

	auto origin = f->convertToNodeSpace(o);

	float incr = metrics::horizontalIncrement();
	auto &size = scene->getViewSize();

	switch (b) {
	case Binding::Relative:
		setPositionY(origin.y);
		if (origin.x < incr / 4) {
			setPositionX(incr / 4);
			setAnchorPoint(Vec2(0, 1.0f));
		} else if (origin.x > size.width - incr / 4) {
			setPositionX(size.width - incr / 4);
			setAnchorPoint(Vec2(1, 1.0f));
		} else {
			float rel = (origin.x - incr / 4) / (size.width - incr / 2);
			setPositionX(origin.x);
			setAnchorPoint(Vec2(rel, 1.0f));
		}
		setContentSize(Size(1, 1));
		break;
	case Binding::OriginLeft:
		setPositionY(origin.y);
		if (origin.x - incr / 4 < _fullSize.width) {
			setAnchorPoint(Vec2(0, 1.0f));
			setPositionX(incr / 4);
		} else {
			setAnchorPoint(Vec2(1, 1.0f));
			setPositionX(origin.x);
		}
		setContentSize(Size(_fullSize.width, 1));
		break;
	case Binding::OriginRight:
		setPositionY(origin.y);
		if (size.width - origin.x - incr / 4 < _fullSize.width) {
			setAnchorPoint(Vec2(1, 1.0f));
			setPositionX(size.width - incr / 4);
		} else {
			setAnchorPoint(Vec2(0, 1.0f));
			setPositionX(origin.x);
		}
		setContentSize(Size(_fullSize.width, 1));
		break;
	case Binding::Anchor:
		setPosition(origin);

		break;
	}

	if (b != Binding::Anchor) {
		if (_fullSize.height > origin.y - incr / 4) {
			if (origin.y - incr / 4 < incr * 4) {
				if (_fullSize.height > incr * 4) {
					_fullSize.height = incr * 4;
				}

				setPositionY(_fullSize.height + incr / 4);
			} else {
				_fullSize.height = origin.y - incr / 4;
			}
		}
	}

	if (origin.y > size.height - incr / 4) {
		setPositionY(size.height - incr / 4);
	}

	if (!f->isActive()) {
		f->setBackgroundOpacity(0);
	}

	f->pushNode(this, std::bind(&FloatingMenu::close, this));

	auto a = Rc<ResizeTo>::create(0.25, _fullSize);
	runAction(action::sequence(a, [this] {
		_scroll->setVisible(true);
	}));
}

void FloatingMenu::setCloseCallback(const CloseCallback &cb) {
	_closeCallback = cb;
}
const FloatingMenu::CloseCallback & FloatingMenu::getCloseCallback() const {
	return _closeCallback;
}

void FloatingMenu::close() {
	stopAllActions();
	_scroll->setVisible(false);
	auto a = Rc<ResizeTo>::create(0.25f, (_binding == Binding::Relative?Size(1, 1):Size(_fullSize.width, 1)));
	runAction(action::sequence(a, [this] {
		if (_closeCallback) {
			_closeCallback();
		}
		Scene::getRunningScene()->popForegroundNode(this);
	}));
}

void FloatingMenu::closeRecursive() {
	if (_root) {
		stopAllActions();
		_scroll->setVisible(false);
		auto a = Rc<ResizeTo>::create(0.15f, (_binding == Binding::Relative?Size(1, 1):Size(_fullSize.width, 1)));
		runAction(action::sequence(a, [this] {
			auto r = _root;
			Scene::getRunningScene()->popForegroundNode(this);
			if (r) {
				r->closeRecursive();
			}
		}));
	} else {
		close();
	}
}

void FloatingMenu::onEnter() {
	Menu::onEnter();
}

void FloatingMenu::onExit() {
	Menu::onExit();
}

void FloatingMenu::onCapturedTap() {
	close();
}

float FloatingMenu::getMenuWidth(MenuSource *source) {
	auto metrics = getMetrics();
	auto font = FontType::Subhead;

	float minWidth = 0;
	auto &items = source->getItems();
	for (auto &item : items) {
		if (item->getType() == MenuSourceItem::Type::Custom) {
			float w = static_cast<MenuSourceCustom *>(item.get())->getMinWidth();
			if (w > minWidth) {
				minWidth = w;
			}
		} else if (item->getType() == MenuSourceItem::Type::Button) {
			auto btn = static_cast<MenuSourceButton *>(item.get());
			float nameLen = Label::getStringWidth(font, btn->getName(), 0.0f, true);
			float valueLen = Label::getStringWidth(font, btn->getValue(), 0.0f, true);

			float offset = metrics::menuFirstLeftKeyline(metrics);
			if (btn->getNameIcon() != IconName::None) {
				offset = metrics::menuSecondLeftKeyline(metrics);
			}

			if (valueLen < 64.0f) {
				if (btn->getValueIcon() != IconName::None || btn->getNextMenu() != nullptr) {
					valueLen += 64.0f;
				}
			}

			if (valueLen == 0.0f) {
				valueLen = metrics::menuRightKeyline(metrics);
			}

			float w = offset + nameLen + valueLen;
			if (w > minWidth) {
				minWidth = w;
			}
		}
	}

	const float incr = metrics::horizontalIncrement();
	const auto &size = Scene::getRunningScene()->getViewSize();

	minWidth = incr * ceilf(minWidth / incr);
	if (minWidth > size.width - incr / 2) {
		minWidth = size.width - incr / 2;
	}

	return minWidth;
}

float FloatingMenu::getMenuHeight(float width, MenuSource *source) {
	float height = metrics::menuVerticalPadding(getMetrics()) * 2;
	auto &items = source->getItems();
	for (auto &item : items) {
		if (item->getType() == MenuSourceItem::Type::Custom) {
			height += static_cast<MenuSourceCustom *>(item.get())->getHeight(width);
		} else if (item->getType() == MenuSourceItem::Type::Button) {
			height += metrics::menuItemHeight(getMetrics());
		} else {
			height += metrics::menuVerticalPadding(getMetrics());
		}
	}

	const float incr = metrics::horizontalIncrement();
	const auto &size = Scene::getRunningScene()->getViewSize();

	if (height > size.height - incr / 2) {
		height = size.height - incr / 2;
	}

	return height;
}

void FloatingMenu::layoutSubviews() {
	auto &size = _fullSize;
	_scroll->setPosition(Vec2(0, size.height - metrics::menuVerticalPadding(getMetrics())));
	_scroll->setContentSize(Size(size.width, size.height - metrics::menuVerticalPadding(getMetrics()) * 2));

	if (_menu) {
		auto &items = _menu->getItems();
		for (auto &item : items) {
			item.get()->setDirty();
		}
	}
}

void FloatingMenu::onMenuButton(MenuButton *btn) {
	if (!btn->getMenuSourceButton()->getNextMenu()) {
		setEnabled(false);
		runAction(action::sequence(0.3f, std::bind(&FloatingMenu::closeRecursive, this)));
	}
}

NS_MD_END
