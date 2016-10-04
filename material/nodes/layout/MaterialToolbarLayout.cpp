/*
 * MaterialToolbarLayout.cpp
 *
 *  Created on: 21 июля 2015 г.
 *      Author: sbkarr
 */

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
	_toolbarMinPortrait = portrait;
	if (isnan(landscape)) {
		_toolbarMinLandscape = portrait;
	} else {
		_toolbarMinLandscape = landscape;
	}
	_contentSizeDirty = true;
}
void ToolbarLayout::setMaxToolbarHeight(float portrait, float landscape) {
	_toolbarMaxPortrait = portrait;
	if (isnan(landscape)) {
		_toolbarMaxLandscape = portrait;
	} else {
		_toolbarMaxLandscape = landscape;
	}
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
		toolbar->setColor(material::Color::Grey_300);
		toolbar->setShadowZIndex(1.5f);
		toolbar->setMaxActionIcons(2);
		toolbar->setTitle("Title");
		toolbar->setNavButtonIcon(material::IconName::Dynamic_Navigation);
		toolbar->setNavCallback(std::bind(&ToolbarLayout::onToolbarNavButton, this));
		return toolbar;
	} else {
		return toolbar;
	}
}

std::pair<float, float> ToolbarLayout::onToolbarHeight() {
	float min, max;
	if (_contentSize.width > _contentSize.height) { // landscape
		min = _toolbarMinLandscape;
		max = _toolbarMaxLandscape;
	} else { //portrait;
		min = _toolbarMinPortrait;
		max = _toolbarMaxPortrait;
	}

	if (isnan(max)) {
		max = material::metrics::appBarHeight();
	}
	_toolbar->setBasicHeight(material::metrics::appBarHeight());

	if (_flexibleToolbar) {
		return std::make_pair(min, max);
	} else {
		return std::make_pair(max, max);
	}
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
