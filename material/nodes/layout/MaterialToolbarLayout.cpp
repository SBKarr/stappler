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
#include "MaterialMenuSource.h"
#include "MaterialMetrics.h"
#include "MaterialScene.h"
#include "SPPadding.h"
#include "SPScrollView.h"
#include "SPActions.h"

NS_MD_BEGIN

bool ToolbarLayout::init(Toolbar *toolbar) {
	if (!FlexibleLayout::init()) {
		return false;
	}

	_toolbar = setupToolbar(toolbar);
	setFlexibleNode(_toolbar);

	auto actions = construct<material::MenuSource>();
	_toolbar->setActionMenuSource(actions);

	setFlexibleHeightFunction([this] () -> std::pair<float, float> {
		return onToolbarHeight();
	});

	setStatusBarTracked(true);

	return true;
}

void ToolbarLayout::onContentSizeDirty() {
	auto tmp = _baseNode;
	if (!_flexibleToolbar) {
		_baseNode = nullptr;
	}
	FlexibleLayout::onContentSizeDirty();
	_baseNode = tmp;

	if (!_flexibleToolbar && _baseNode) {
		stappler::Padding padding;
		if (_baseNode) {
			padding = _baseNode->getPadding();
		}

		_baseNode->setAnchorPoint(cocos2d::Vec2(0, 0));
		_baseNode->setPosition(0, 0);
		_baseNode->setContentSize(Size(_contentSize.width, _contentSize.height - getCurrentFlexibleMax() - 0.0f));
		_baseNode->setPadding(padding.setTop(6.0f));
		_baseNode->setOverscrollFrontOffset(0.0f);
	}
}

void ToolbarLayout::onEnter() {
	FlexibleLayout::onEnter();
}

Toolbar *ToolbarLayout::getToolbar() const {
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
}
void ToolbarLayout::setToolbarColor(const Color &c, const Color &text) {
	_toolbar->setColor(c);
	_toolbar->setTextColor(text);
}

void ToolbarLayout::setMaxActions(size_t n) {
	_toolbar->setMaxActionIcons(n);
}

void ToolbarLayout::setFlexibleToolbar(bool value) {
	if (value != _flexibleToolbar) {
		_flexibleToolbar = value;
		_contentSizeDirty = true;
	}
}
bool ToolbarLayout::getFlexibleToolbar() const {
	return _flexibleToolbar;
}

void ToolbarLayout::setMinToolbarHeight(float portrait, float landscape) {
	_toolbar->setMinToolbarHeight(portrait, landscape);
	_contentSizeDirty = true;
}
void ToolbarLayout::setMaxToolbarHeight(float portrait, float landscape) {
	_toolbar->setMaxToolbarHeight(portrait, landscape);
	_contentSizeDirty = true;
}

void ToolbarLayout::onToolbarNavButton() {
	if (_toolbar->getNavButtonIcon() == IconName::Dynamic_Navigation && _toolbar->getNavButtonIconProgress() > 0.0f) {
		onBackButton();
	} else if (_toolbar->getNavButtonIcon() == IconName::Dynamic_Navigation && _toolbar->getNavButtonIconProgress() == 0.0f) {
		runAction(action::callback(0.15f, [] {
			material::Scene::getRunningScene()->getNavigationLayer()->show();
		}));
	}
}

Toolbar *ToolbarLayout::setupToolbar(Toolbar *toolbar) {
	if (!toolbar) {
		toolbar = construct<material::Toolbar>();
	}
	toolbar->setColor(material::Color::Grey_300);
	toolbar->setShadowZIndex(1.5f);
	toolbar->setMaxActionIcons(2);
	toolbar->setTitle("Title");
	toolbar->setNavButtonIcon(material::IconName::Dynamic_Navigation);
	toolbar->setNavCallback(std::bind(&ToolbarLayout::onToolbarNavButton, this));
	return toolbar;
}

std::pair<float, float> ToolbarLayout::onToolbarHeight() {
	return _toolbar->onToolbarHeight(_flexibleToolbar);
}

void ToolbarLayout::onPush(ContentLayer *l, bool replace) {
	FlexibleLayout::onPush(l, replace);
	if (replace) {
		auto prev = dynamic_cast<ToolbarLayout *>(l->getPrevNode());
		if (!prev) {
			return;
		}

		auto toolbar = prev->getToolbar();
		auto prog = toolbar->getNavButtonIconProgress();
		if (_toolbar && toolbar && toolbar->getNavButtonIconProgress() > 0.0f) {
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

	if (_toolbar->getNavButtonIcon() == IconName::Dynamic_Navigation
			&& _toolbar->getNavButtonIcon() == nextToolbar->getNavButtonIcon()) {
		auto p = _toolbar->getNavButtonIconProgress();
		if (p < 1.0f) {
			nextToolbar->setNavButtonIconProgress(1.0f, 0.35f);
			_toolbar->setNavButtonIconProgress(1.0f, 0.0f);
			_forwardProgress = true;
		} else {
			nextToolbar->setNavButtonIconProgress(1.0f, 0.0f);
		}

		nextToolbar->setNavCallback([this, next] {
			material::Scene::getRunningScene()->getContentLayer()->popNode(next);
		});
	}
}

void ToolbarLayout::onForegroundTransitionBegan(ContentLayer *l, Layout *overlay) {
	if (!_flexibleToolbar) {
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
	if (_toolbar->getNavButtonIcon() == IconName::Dynamic_Navigation
			&& _toolbar->getNavButtonIcon() == nextToolbar->getNavButtonIcon()) {
		if (_forwardProgress) {
			_toolbar->setNavButtonIconProgress(0.0f, 0.35f);
			_forwardProgress = false;
		}
	}
}

void ToolbarLayout::onScroll(float delta, bool finished) {
    if (_flexibleToolbar) {
        FlexibleLayout::onScroll(delta, finished);
    }
}
NS_MD_END
