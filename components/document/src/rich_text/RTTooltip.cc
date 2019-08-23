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
#include "MaterialResize.h"
#include "MaterialScene.h"
#include "MaterialForegroundLayer.h"
#include "MaterialToolbarLayout.h"
#include "MaterialToolbar.h"
#include "MaterialMenuSource.h"

#include "RTListenerView.h"
#include "RTTooltip.h"

#include "RTCommonView.h"
#include "RTRenderer.h"
#include "SLResult.h"
#include "SPScrollView.h"
#include "SPGestureListener.h"
#include "SPEventListener.h"
#include "SPActions.h"

NS_RT_BEGIN

SP_DECLARE_EVENT_CLASS(Tooltip, onCopy);

Tooltip::~Tooltip() { }

bool Tooltip::init(CommonSource *s, const Vector<String> &ids) {
	if (!MaterialNode::init()) {
		return false;
	}

	auto l = Rc<gesture::Listener>::create();
	l->setTouchCallback([this] (gesture::Event ev, const gesture::Touch &) {
		if (ev == gesture::Event::Ended || ev == gesture::Event::Cancelled) {
			if (!_expanded) {
				onDelayedFadeOut();
			}
		} else {
			onFadeIn();
		}
		return true;
	});
	//l->setOpacityFilter(1);
	l->setSwallowTouches(true);
	addComponent(l);
	_listener = l;

	Rc<ListenerView> view;
	if (!ids.empty()) {
		view = Rc<ListenerView>::create(ListenerView::Vertical, s, ids);
		view->setAnchorPoint(Vec2(0, 0));
		view->setPosition(Vec2(0, 0));
		view->setResultCallback(std::bind(&Tooltip::onResult, this, std::placeholders::_1));
		view->setUseSelection(true);
		view->getGestureListener()->setSwallowTouches(true);

		auto el = Rc<EventListener>::create();
		el->onEventWithObject(ListenerView::onSelection, view, std::bind(&Tooltip::onSelection, this));
		view->addComponent(el);

		auto r = view->getRenderer();
		r->addOption("tooltip");
		r->addFlag(layout::RenderFlag::NoImages);
		r->addFlag(layout::RenderFlag::NoHeightCheck);
		r->addFlag(layout::RenderFlag::RenderById);
	}

	auto toolbar = Rc<material::Toolbar>::create();
	toolbar->setShadowZIndex(1.0f);
	toolbar->setMaxActionIcons(2);

	auto layout = Rc<material::ToolbarLayout>::create(toolbar);
	layout->setFlexibleToolbar(false);
	layout->setStatusBarTracked(false);

	_actions = Rc<material::MenuSource>::create();
	_actions->addButton("Close", material::IconName::Navigation_close, std::bind(&Tooltip::close, this));

	_selectionActions = Rc<material::MenuSource>::create();
	_selectionActions->addButton("Copy", material::IconName::Content_content_copy, std::bind(&Tooltip::copySelection, this));
	_selectionActions->addButton("Close", material::IconName::Content_remove_circle_outline, std::bind(&Tooltip::cancelSelection, this));

	toolbar->setActionMenuSource(_actions);
	toolbar->setNavButtonIcon(material::IconName::None);
	toolbar->setTitle("");
	toolbar->setMinified(true);
	toolbar->setBarCallback([this] {
		onFadeIn();
	});

	if (view) {
		layout->setBaseNode(view);
		layout->setOpacity(0);
		_view = view;
	} else {
		layout->setOpacity(255);
		toolbar->setShadowZIndex(0.0f);
		toolbar->setSwallowTouches(false);
	}
	layout->setAnchorPoint(Vec2(0, 0));
	layout->setPosition(0, 0);
	_layout = addChildNode(layout);
	_toolbar = toolbar;

	_contentSizeDirty = true;

	return true;
}

void Tooltip::onContentSizeDirty() {
	MaterialNode::onContentSizeDirty();
	if (_originPosition.y - material::metrics::horizontalIncrement() / 4 < _contentSize.height * _anchorPoint.y) {
		setAnchorPoint(Vec2(_anchorPoint.x,
				(_originPosition.y - material::metrics::horizontalIncrement() / 4) / _contentSize.height));
	}

	_layout->onContentSizeDirty();
	if (_maxContentSize.width != 0.0f) {
		_layout->setContentSize(Size(_maxContentSize.width,
				MAX(_contentSize.height, _layout->getCurrentFlexibleMax())));
	} else {
		_layout->setContentSize(Size(_contentSize.width,
				MAX(_contentSize.height, _layout->getCurrentFlexibleMax())));
	}
}

void Tooltip::onEnter()  {
	if (!_view) {
		setShadowZIndex(3.0f);
	}
	MaterialNode::onEnter();
}
void Tooltip::onResult(rich_text::Result *r) {
	auto bgColor = r->getBackgroundColor();
	setBackgroundColor(bgColor);
	_toolbar->setColor(bgColor);

	Size cs = r->getContentSize();

	cs.height += _layout->getFlexibleMaxHeight() + 16.0f;
	if (cs.height > _maxContentSize.height) {
		cs.height = _maxContentSize.height;
	}

	_defaultSize = cs;
	if (!_expanded) {
		cs.height = material::metrics::appBarHeight();
	}

	stopAllActions();
	setShadowZIndexAnimated(3.0f, 0.25f);
	runAction(action::sequence(Rc<material::ResizeTo>::create(0.25, cs), [this] {
		if (_layout->getOpacity() != 255) {
			_layout->setOpacity(255);
		}
	}));
}

void Tooltip::setMaxContentSize(const Size &size) {
	_maxContentSize = size;
	if (_layout->getContentSize().width != _maxContentSize.width) {
		_layout->setContentSize(Size(_maxContentSize.width,
				MAX(_layout->getContentSize().height, _layout->getCurrentFlexibleMax())));
	}
}

void Tooltip::setOriginPosition(const Vec2 &pos, const Size &parentSize, const Vec2 &worldPos) {
	_originPosition = pos;
	_worldPos = worldPos;
	_parentSize = parentSize;

	setAnchorPoint(Vec2(_originPosition.x / _parentSize.width, 1.0f));
}

void Tooltip::setExpanded(bool value) {
	if (value != _expanded) {
		_expanded = value;

		if (_expanded) {
			stopAllActions();
			runAction(action::sequence(Rc<material::ResizeTo>::create(0.25, _defaultSize), [this] {
				_view->setVisible(true);
				_view->setOpacity(0);
				_view->runAction(cocos2d::FadeIn::create(0.15f));
			}));
			_toolbar->setShadowZIndex(1.5f);
			onFadeIn();
		} else {
			_view->setVisible(false);

			auto newSize = Size(_defaultSize.width, _toolbar->getDefaultToolbarHeight());
			if (!newSize.equals(_defaultSize)) {
				stopAllActions();
				runAction(action::sequence(Rc<material::ResizeTo>::create(0.25, newSize), [this] {
					_toolbar->setShadowZIndex(0.0f);
					onDelayedFadeOut();
				}));
			} else {
				onDelayedFadeOut();
			}
		}
	}
}

void Tooltip::pushToForeground() {
	auto foreground = material::Scene::getRunningScene()->getForegroundLayer();
	setPosition(foreground->convertToNodeSpace(_worldPos));
	foreground->pushNode(this, std::bind(&Tooltip::close, this));
}

void Tooltip::close() {
	if (_closeCallback) {
		_closeCallback();
	}

	_layout->setVisible(false);
	stopAllActions();
	setShadowZIndexAnimated(0.0f, 0.25f);
	runAction(action::sequence(Rc<material::ResizeTo>::create(0.25, Size(0, 0)), [this] {
		auto foreground = material::Scene::getRunningScene()->getForegroundLayer();
		foreground->popNode(this);
	}));
}

void Tooltip::onDelayedFadeOut() {
	stopActionByTag(2);
	runAction(action::sequence(3.0f, cocos2d::FadeTo::create(0.25f, 48)), 2);
}

void Tooltip::onFadeIn() {
	stopActionByTag(1);
	auto a = getActionByTag(3);
	if (!a || getOpacity() != 255) {
		runAction(cocos2d::FadeTo::create(0.15f, 255), 3);
	}
}

void Tooltip::onFadeOut() {
	stopActionByTag(3);
	auto a = getActionByTag(2);
	if (!a || getOpacity() != 48) {
		runAction(cocos2d::FadeTo::create(0.15f, 48), 2);
	}
}

void Tooltip::setCloseCallback(const CloseCallback &cb) {
	_closeCallback = cb;
}

rich_text::Renderer *Tooltip::getRenderer() const {
	return _view?_view->getRenderer():nullptr;
}

String Tooltip::getSelectedString() const {
	return (_view && _view->isSelectionEnabled())?_view->getSelectedString():String();
}

void Tooltip::onLightLevel() {
	MaterialNode::onLightLevel();
}

void Tooltip::copySelection() {
	if (_view && _view->isSelectionEnabled()) {
		onCopy(this);
		_view->disableSelection();
	}
}
void Tooltip::cancelSelection() {
	if (_view) {
		_view->disableSelection();
	}
}
void Tooltip::onSelection() {
	if (_view) {
		if (_view->isSelectionEnabled()) {
			_toolbar->replaceActionMenuSource(_selectionActions, 2);
		} else {
			_toolbar->replaceActionMenuSource(_actions, 2);
		}
	}
}

NS_RT_END
