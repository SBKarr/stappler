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
#include "MaterialToolbarLayout.h"
#include "MaterialToolbar.h"
#include "MaterialSearchToolbar.h"
#include "MaterialMenuSource.h"
#include "MaterialMetrics.h"
#include "MaterialScene.h"
#include "SPScrollView.h"
#include "SPActions.h"
#include "SPIME.h"

NS_MD_BEGIN

bool ToolbarLayout::init(ToolbarBase *toolbar) {
	if (!FlexibleLayout::init()) {
		return false;
	}

	auto t = setupToolbar(toolbar);
	_toolbar = t;
	setFlexibleNode(t);

	setFlexibleHeightFunction([this] () -> std::pair<float, float> {
		return onToolbarHeight();
	});

	setStatusBarTracked(true);

	return true;
}

void ToolbarLayout::onEnter() {
	FlexibleLayout::onEnter();
}

ToolbarBase *ToolbarLayout::getToolbar() const {
	return _toolbar;
}

MenuSource *ToolbarLayout::getActionMenuSource() const {
	return _toolbar->getActionMenuSource();
}

void ToolbarLayout::setTitle(const std::string &str) {
	_toolbar->setTitle(str);
}
const std::string &ToolbarLayout::getTitle() const {
	return _toolbar->getTitle();
}

void ToolbarLayout::setToolbarColor(const Color &c) {
	_toolbar->setColor(c);
	if (_statusBarTracked) {
		setStatusBarBackgroundColor(c.darker(2));
	}
}
void ToolbarLayout::setToolbarColor(const Color &c, const Color &text) {
	_toolbar->setColor(c);
	_toolbar->setTextColor(text);
}

void ToolbarLayout::setMaxActions(size_t n) {
	_toolbar->setMaxActionIcons(n);
}

void ToolbarLayout::setFlexibleToolbar(bool value) {
	if (value != _flexibleBaseNode) {
		_flexibleBaseNode = value;
		_contentSizeDirty = true;
	}
}
bool ToolbarLayout::getFlexibleToolbar() const {
	return _flexibleBaseNode;
}

void ToolbarLayout::setMinToolbarHeight(float portrait, float landscape) {
	_toolbar->setMinToolbarHeight(portrait, landscape);
	_contentSizeDirty = true;
}
void ToolbarLayout::setMaxToolbarHeight(float portrait, float landscape) {
	_toolbar->setMaxToolbarHeight(portrait, landscape);
	_contentSizeDirty = true;
}

void ToolbarLayout::setPopOnNavButton(bool value) {
	_popOnNavButton = value;
}

void ToolbarLayout::onToolbarNavButton() {
	if (_popOnNavButton) {
		material::Scene::getRunningScene()->getContentLayer()->popNode(this);
	} else if (_toolbar->getNavButtonIcon() == IconName::Dynamic_Navigation && _toolbar->getNavButtonIconProgress() > 0.0f) {
		onBackButton();
	} else if (_toolbar->getNavButtonIcon() == IconName::Dynamic_Navigation && _toolbar->getNavButtonIconProgress() == 0.0f) {
		runAction(action::sequence(0.15f, [] {
			material::Scene::getRunningScene()->getNavigationLayer()->show();
		}));
	}
}

Rc<ToolbarBase> ToolbarLayout::setupToolbar(ToolbarBase *iToolbar) {
	Rc<ToolbarBase> toolbar(iToolbar);

	if (!toolbar) {
		toolbar.set(Rc<material::Toolbar>::create());
	}

	toolbar->setColor(material::Color::Grey_300);
	toolbar->setShadowZIndex(1.5f);
	toolbar->setMaxActionIcons(2);

	if (toolbar->getNavButtonIcon() == IconName::Empty || toolbar->getNavButtonIcon() == IconName::None) {
		toolbar->setNavButtonIcon(material::IconName::Dynamic_Navigation);
	}

	toolbar->setNavCallback(std::bind(&ToolbarLayout::onToolbarNavButton, this));

	if (!toolbar->getActionMenuSource()) {
		toolbar->setActionMenuSource(Rc<material::MenuSource>::create());
	}

	return toolbar;
}

std::pair<float, float> ToolbarLayout::onToolbarHeight() {
	return _toolbar->onToolbarHeight(_flexibleBaseNode, _contentSize.width > _contentSize.height);
}

void ToolbarLayout::onPush(ContentLayer *l, bool replace) {
	FlexibleLayout::onPush(l, replace);
	if (replace) {
		auto prev = dynamic_cast<ToolbarLayout *>(l->getPrevNode());
		if (!prev) {
			return;
		}

		auto toolbar = prev->getToolbar();
		if (_toolbar && toolbar && toolbar->getNavButtonIconProgress() > 0.0f) {
			auto prog = toolbar->getNavButtonIconProgress();
			_toolbar->setNavButtonIconProgress(prog, 0.0f);
			_toolbar->setNavButtonIconProgress(0.0f, progress(0.0f, 0.35f, prog));
		}
	}
}

void ToolbarLayout::onBackground(ContentLayer *l, Layout *overlay) {
	FlexibleLayout::onBackground(l, overlay);

	auto next = dynamic_cast<ToolbarLayout *>(overlay);
	if (!next) {
		return;
	}

	auto nextToolbar = next->getToolbar();

	if (_toolbar->isNavProgressSupported() && nextToolbar->isNavProgressSupported()) {
		auto p = _toolbar->getNavButtonIconProgress();
		if (p < 1.0f) {
			nextToolbar->setNavButtonIconProgress(1.0f, 0.35f);
			_toolbar->setNavButtonIconProgress(1.0f, 0.0f);
			_forwardProgress = true;
		} else {
			nextToolbar->setNavButtonIconProgress(1.0f, 0.0f);
		}

		next->setPopOnNavButton(true);
	}
}

void ToolbarLayout::onForegroundTransitionBegan(ContentLayer *l, Layout *overlay) {
	if (!_flexibleBaseNode) {
		_flexibleLevel = 1.0f;
	}
	FlexibleLayout::onForegroundTransitionBegan(l, overlay);
}

void ToolbarLayout::onForeground(ContentLayer *l, Layout *overlay) {
	FlexibleLayout::onForeground(l, overlay);

	auto next = dynamic_cast<ToolbarLayout *>(overlay);
	if (!next) {
		return;
	}

	auto nextToolbar = next->getToolbar();
	if (_toolbar->isNavProgressSupported() && nextToolbar->isNavProgressSupported()) {
		if (_forwardProgress) {
			_toolbar->setNavButtonIconProgress(0.0f, 0.35f);
			_forwardProgress = false;
		}
	}
}

void ToolbarLayout::onScroll(float delta, bool finished) {
    if (_flexibleBaseNode) {
        FlexibleLayout::onScroll(delta, finished);
    }
}

void ToolbarLayout::onKeyboard(bool enabled, const Rect &rect, float duration) {
	bool tmpEnabled = _keyboardEnabled;
	FlexibleLayout::onKeyboard(enabled, rect, duration);
	if (tmpEnabled != enabled && _toolbar->isNavProgressSupported()) {
		if (enabled) {
			_savedNavCallback = _toolbar->getNavCallback();
			_savedNavProgress = _toolbar->getNavButtonIconProgress();

			_toolbar->setNavCallback(std::bind(&ToolbarLayout::closeKeyboard, this));
			_toolbar->setNavButtonIconProgress(2.0f, 0.35f);
		} else {
			_toolbar->setNavCallback(_savedNavCallback);
			_toolbar->setNavButtonIconProgress(_savedNavProgress, 0.35f);

			_savedNavCallback = nullptr;
			_savedNavProgress = 0.0f;
		}
	}
}

void ToolbarLayout::closeKeyboard() {
	ime::cancel();
}

NS_MD_END
