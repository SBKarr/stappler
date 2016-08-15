/*
 * MaterialLayout.cpp
 *
 *  Created on: 29 мая 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialLayout.h"
#include "MaterialScene.h"

NS_MD_BEGIN

bool Layout::onBackButton() {
	if (_backButtonCallback) {
		return _backButtonCallback();
	} else {
		auto scene = Scene::getRunningScene();
		auto content = scene->getContentLayer();
		if (content->getNodesCount() > 2 && content->getRunningNode() == this) {
			content->popLastNode();
			return true;
		}
	}
	return false;
}

void Layout::setBackButtonCallback(const BackButtonCallback &cb) {
	_backButtonCallback = cb;
}
const Layout::BackButtonCallback &Layout::getBackButtonCallback() const {
	return _backButtonCallback;
}

void Layout::onPush(ContentLayer *l, bool replace) { }
void Layout::onPushTransitionEnded(ContentLayer *l, bool replace) { }

void Layout::onPopTransitionBegan(ContentLayer *l, bool replace) { }
void Layout::onPop(ContentLayer *l, bool replace) { }

void Layout::onBackground(ContentLayer *l, Layout *overlay) { }
void Layout::onBackgroundTransitionEnded(ContentLayer *l, Layout *overlay) { }

void Layout::onForegroundTransitionBegan(ContentLayer *l, Layout *overlay) { }
void Layout::onForeground(ContentLayer *l, Layout *overlay) { }

NS_MD_END
