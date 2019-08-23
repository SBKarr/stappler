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
#include "MaterialMetrics.h"
#include "MaterialToolbar.h"
#include "MaterialMenuSource.h"
#include "MaterialScene.h"
#include "MaterialSidebarLayout.h"
#include "MaterialButtonLabelSelector.h"

#include "SPAsset.h"
#include "SPStorage.h"
#include "SPString.h"
#include "SPEventListener.h"
#include "SPScreen.h"
#include "SPProgressAction.h"
#include "SPGestureListener.h"
#include "SPActions.h"
#include "SPAssetLibrary.h"
#include "SPLocale.h"

#include "2d/CCActionEase.h"
#include "RTFontSizeMenu.h"
#include "RTArticleLayout.h"
#include "RTView.h"
#include "RTTooltip.h"

NS_RT_BEGIN

SP_DECLARE_EVENT_CLASS(ArticleLayout, onOpened);
SP_DECLARE_EVENT_CLASS(ArticleLayout, onClosed);
SP_DECLARE_EVENT_CLASS(ArticleLayout, onImageLink);
SP_DECLARE_EVENT_CLASS(ArticleLayout, onExternalLink);
SP_DECLARE_EVENT_CLASS(ArticleLayout, onContentLink);

static bool s_localeInit = false;
static void defineLocaleStrings() {
	locale::define("ru-ru", {
		pair("RichTextCopy", "Копировать"),
		pair("RichTextMakeBookmark", "В закладки"),
		pair("RichTextSendEmail", "Отправить письмом"),
		pair("RichTextShare", "Поделиться"),
		pair("RichTextReportMisprint", "Сообщить об ошибке"),

		pair("RichTextMisprintReported", "Ваше сообщение об опечатке отправлено. Опечатка будет исправлена в ближайшее время."),
		pair("RichTextBookmarkCreated", "Закладка создана."),
		pair("RichTextNoNetworkConnection", "Нет соединения с интернетом, попробуйте позже."),
	});

	locale::define("en-us", {
		pair("RichTextCopy", "Copy"),
		pair("RichTextMakeBookmark", "Make bookmark"),
		pair("RichTextSendEmail", "Send as email"),
		pair("RichTextShare", "Share"),
		pair("RichTextReportMisprint", "Report misprint"),

		pair("RichTextMisprintReported", "Your misprint report has been sent. Misprint will be corrected soon."),
		pair("RichTextBookmarkCreated", "Bookmark created"),
		pair("RichTextNoNetworkConnection", "No connection with internet, try again later."),
	});
}

static void initLocale() {
	if (!s_localeInit) {
		defineLocaleStrings();
		s_localeInit = true;
	}
}


bool ArticleLayout::ArticleLoader::init(size_t num) {
	number = num;
	return true;
}

void ArticleLayout::ArticleLoader::setAsset(Asset *a) {
	asset = a;
}
void ArticleLayout::ArticleLoader::setCallback(const AssetCallback &cb) {
	callback = cb;
}

Rc<Source> ArticleLayout::ArticleLoader::constructSource() const {
	Rc<Source> source;
	if (asset) {
		source = Rc<Source>::create(Rc<SourceNetworkAsset>::create(asset), false);
	} else if (callback) {
		source = Rc<Source>::create(Rc<SourceNetworkAsset>::create([this] (const Function<void(Asset *)> &cb) {
			callback(this, cb);
		}), false);
	}
	return source;
}

Rc<View> ArticleLayout::ArticleLoader::constructView() const {
	if (auto source = constructSource()) {
		return Rc<View>::create(source);
	}
	return nullptr;
}

size_t ArticleLayout::ArticleLoader::getNumber() const {
	return number;
}

ArticleLayout::~ArticleLayout() { }

bool ArticleLayout::init(size_t num, size_t count, font::HyphenMap *hmap) {
	if (!ToolbarLayout::init()) {
		return false;
	}

	initLocale();

	_hyphens = hmap;
	_count = count;
	_currentNumber = num;

	auto el = Rc<EventListener>::create();
	el->onEvent(rich_text::Tooltip::onCopy, [this] (const Event &ev) {
		if (auto obj = dynamic_cast<rich_text::Tooltip *>(ev.getObject())) {
			onCopyButton(obj->getSelectedString());
		}
	});
	addComponent(el);

	setStatusBarTracked(true);
	setBaseNodePadding(0.0f);
	updateActionsMenu();

	// configure sidebar

	auto sidebar = Rc<material::SidebarLayout>::create(material::SidebarLayout::Right);
	sidebar->setNodeWidthCallback([] (const cocos2d::Size &size) -> float {
		return MIN(size.width - 56.0f, material::metrics::horizontalIncrement() * 6);
	});
	sidebar->setNodeVisibleCallback([this] (bool value) {
		if (value) {
			setFlexibleLevelAnimated(1.0f, 0.25f);
		}
	});
	sidebar->setBackgroundActiveOpacity(0);
	onSidebar(sidebar);
	addChild(sidebar, 2);
	_sidebar = sidebar;

	auto g = Rc<gesture::Listener>::create();
	g->setSwipeCallback([this] (gesture::Event ev, const gesture::Swipe &s) -> bool {
		if (ev == gesture::Event::Began) {
			if (_richTextView) {
				if (_richTextView->isSelectionEnabled()) {
					return false;
				}
			}
			auto diff = s.firstTouch.point - s.firstTouch.startPoint;
			if (fabs(diff.x) < fabs(diff.y)) {
				return false;
			} else {
				setFlexibleLevelAnimated(1.0f, 0.25f);
			}
		} else if (ev == gesture::Event::Activated) {
			setSwipeProgress(s.delta / screen::density());
		} else {
			endSwipeProgress(s.delta / screen::density(), s.velocity / screen::density());
		}
		return true;
	}, 20.0f, false);
	addComponent(g);

	return true;
}

bool ArticleLayout::init(Asset *a, font::HyphenMap *hmap) {
	if (!init(0, 1, hmap)) {
		return false;
	}

	loadArticle(0, a);
	return true;
}

void ArticleLayout::loadArticle(size_t num, Asset *a, const AssetCallback &cb) {
	auto loader = constructLoader(num);
	if (a) {
		loader->setAsset(a);
	} else if (cb) {
		loader->setCallback(cb);
	}

	loadArticle(loader);
}

void ArticleLayout::loadArticle(const ArticleLoader *loader) {
	_currentNumber = loader->getNumber();
	_swipeProgress = 0;
	onSwipeProgress();

	if (_richTextView) {
		setBaseNode(nullptr);
		_richTextView = nullptr;
	}

	_sidebar->hide();

	auto rt = loadView(loader);
	_richTextView = rt;
	onViewSelected(rt, loader->getNumber());
	setBaseNode(rt, 1);
}

void ArticleLayout::onScroll(float value, bool ended) {
	FlexibleLayout::onScroll(value, ended);

	float newPos = _richTextView->getScrollRelativePosition();

	AssetFlags flags = ((fabs(newPos - _currentPos) > 0.005f) ? AssetFlags::Offset : AssetFlags::None)
			| ((newPos == 1.0f || _richTextView->getScrollMaxPosition() <= _richTextView->getScrollMinPosition()) ? AssetFlags::Read : AssetFlags::None);

	if (_richTextView->getResult() && flags != AssetFlags::None) {
		updateAssetInfo(_richTextView->getSource()->getAsset(), flags, newPos);
	}

	if ((flags | AssetFlags::Offset) != AssetFlags::None) {
		_currentPos = newPos;
	}
}

void ArticleLayout::setArticleCallback(const ArticleCallback &cb) {
	_articleCallback = cb;
}
const ArticleLayout::ArticleCallback &ArticleLayout::getArticleCallback() const {
	return _articleCallback;
}

void ArticleLayout::setAssetCallback(const AssetCallback &cb) {
	_assetCallback = cb;
}
const ArticleLayout::AssetCallback &ArticleLayout::getAssetCallback() const {
	return _assetCallback;
}

void ArticleLayout::setCount(size_t c) {
	_count = c;
}

void ArticleLayout::setCloseCallback(const Function<void()> &cb) {
	_cancelCallback = cb;

	auto scene = material::Scene::getRunningScene();
	if (!scene || !scene->getContentLayer()->isActive()) {
		_toolbar->setNavButtonIconProgress(1.0f, 0.0f);
	}
}

void ArticleLayout::onDocument(View *view) {
	_documentOpened = true;
}

void ArticleLayout::updateAssetInfo(SourceAsset *a, AssetFlags flags, float pos) {
	if (!a) {
		return;
	}

	auto d = a->getExtraData();
	if ((flags & AssetFlags::Read) != AssetFlags::None) {
		d.setBool(true, "read");
	}
	if ((flags & AssetFlags::Offset) != AssetFlags::None) {
		d.setDouble(pos, "value");
	}
	if ((flags & AssetFlags::Open) != AssetFlags::None) {
		d.setBool(true, "open");
	}
	a->setExtraData(std::move(d));
}

void ArticleLayout::updateActionsMenu() {
	_viewSource = Rc<material::MenuSource>::create();
	_viewSource->addItem(Rc<FontSizeMenuButton>::create());
	_viewSource->addButton("", material::IconName::Action_list, std::bind([this] {
		if (_sidebar->getProgress() == 0.0f) {
			_sidebar->show();
		} else {
			_sidebar->hide();
		}
	}));
	_toolbar->setActionMenuSource(_viewSource);

	_selectionSource = Rc<material::MenuSource>::create();
	_selectionSource->addButton("Copy", material::IconName::Content_content_copy, std::bind([this] () {
		if (_richTextView && _richTextView->isSelectionEnabled()) {
			onCopyButton(_richTextView->getSelectedString(maxOf<size_t>()));
			_richTextView->disableSelection();
		}
	}));
	_selectionSource->addButton("Bookmark", material::IconName::Action_bookmark_border, std::bind(&ArticleLayout::onBookmarkButton, this));
	_selectionSource->addButton("RichTextCopy"_locale, material::IconName::Content_content_copy, std::bind([this] () {
		if (_richTextView && _richTextView->isSelectionEnabled()) {
			onCopyButton(_richTextView->getSelectedString(maxOf<size_t>()));
			_richTextView->disableSelection();
		}
	}));
	_selectionSource->addButton("RichTextMakeBookmark"_locale, material::IconName::Action_bookmark_border, std::bind(&ArticleLayout::onBookmarkButton, this));
	_selectionSource->addButton("RichTextReportMisprint"_locale, material::IconName::Content_report, std::bind(&ArticleLayout::onMisprintButton, this));
}

void ArticleLayout::onEnter() {
	FlexibleLayout::onEnter();
	onOpened(this);
	if (!Screen::getInstance()->isStatusBarEnabled()) {
		if (getCurrentFlexibleMax() != _flexibleMaxHeight) {
			setFlexibleHeight(_flexibleMaxHeight);
		}
	}
}

void ArticleLayout::onExit() {
	FlexibleLayout::onExit();
	onClosed(this);
}

void ArticleLayout::onContentSizeDirty() {
	FlexibleLayout::onContentSizeDirty();
	_sidebar->setContentSize(cocos2d::Size(_contentSize.width, _contentSize.height - getCurrentFlexibleMax()));
}

bool ArticleLayout::onBackButton() {
	if (_richTextView && _richTextView->isSelectionEnabled()) {
		_richTextView->disableSelection();
		return true;
	}
	retain();
	if (_cancelCallback) {
		_cancelCallback();
	}
	bool ret = FlexibleLayout::onBackButton();
	release();
	return ret;
}

Rc<ArticleLayout::ArticleLoader> ArticleLayout::constructLoader(size_t number) {
	return nullptr;
}

Rc<View> ArticleLayout::loadView(const ArticleLoader *loader) {
	Rc<View> view = loader->constructView();
	view->getSource()->setHyphens(_hyphens);
	view->setTapCallback([this] (int, const Vec2 &) {
		onTap();
	});
	View *viewPtr = view.get();

	auto el = Rc<EventListener>::create();
	el->onEventWithObject(View::onContentLink, view, [this] (const Event &ev) {
		onContentLink(this, ev.getStringValue());
	});
	el->onEventWithObject(View::onImageLink, view, [this] (const Event &ev) {
		onImageLink(this, ev.getStringValue());
	});
	el->onEventWithObject(View::onExternalLink, view, [this] (const Event &ev) {
		onExternalLink(this, ev.getStringValue());
	});
	el->onEventWithObject(View::onDocument, view, [this, viewPtr] (const Event &ev) {
		if (viewPtr->getScrollMaxPosition() <= viewPtr->getScrollMinPosition()) {
			auto a = viewPtr->getSource()->getAsset();
			updateAssetInfo(a, AssetFlags::Read, 0.0f);
		}
	});
	el->onEventWithObject(View::onSelection, view, [this, viewPtr] (const Event &ev) {
		onSelection(viewPtr, viewPtr->isSelectionEnabled());
	});
	el->onEventWithObject(View::onError, viewPtr, [] (const Event &ev) {
		if (auto scene = material::Scene::getRunningScene()) {
			auto err = (rich_text::Source::Error)ev.getIntValue();
			if (err == rich_text::Source::Error::DocumentError) {
				scene->showSnackbar("Не удалось загрузить статью: ошибка в документе");
			} else if (err == rich_text::Source::Error::NetworkError) {
				scene->showSnackbar("Не удалось загрузить статью: нет соединения с сетью");
			} else {
				scene->showSnackbar("Не удалось загрузить статью");
			}
		}
	});
	view->addComponent(el);

	updateAssetInfo(view->getSource()->getAsset(), AssetFlags::Open, 0.0f);
	return view;
}

void ArticleLayout::onSwipeProgress() {
	if (!_richTextView) {
		return;
	}

	_richTextView->setPositionX(_swipeProgress);

	if (_swipeProgress > 0.0f) {
		if (_nextRichText) {
			_nextRichText->removeFromParent();
			_nextRichText = nullptr;
		}

		if (!_prevRichText && _currentNumber > 0) {
			auto loader = constructLoader(_currentNumber - 1);
			auto rt = loadView(loader);
			rt->setContentSize(_richTextView->getContentSize());
			rt->setPosition(_richTextView->getPosition());
			rt->setPadding(_richTextView->getPadding());
			addChild(rt, 1);
			_prevRichText = rt;
		}

		if (_prevRichText) {
			applyViewProgress(_richTextView, _prevRichText, fabs(_swipeProgress) / _contentSize.width);
		}
	} else if (_swipeProgress < 0.0f) {
		if (_prevRichText) {
			_prevRichText->removeFromParent();
			_prevRichText = nullptr;
		}

		if (!_nextRichText && _currentNumber < _count - 1) {
			auto loader = constructLoader(_currentNumber + 1);
			auto rt = loadView(loader);
			rt->setContentSize(_richTextView->getContentSize());
			rt->setPosition(_richTextView->getPosition());
			rt->setPadding(_richTextView->getPadding());
			addChild(rt, 1);
			_nextRichText = rt;
		}

		if (_nextRichText) {
			applyViewProgress(_richTextView, _nextRichText, fabs(_swipeProgress) / _contentSize.width);
		}
	} else {
		if (_nextRichText) {
			_nextRichText->removeFromParent();
			_nextRichText = nullptr;
		}

		if (_prevRichText) {
			_prevRichText->removeFromParent();
			_prevRichText = nullptr;
		}

		applyView(_richTextView);
	}
	if (_nextRichText) {
		_nextRichText->setPositionX(_swipeProgress + _contentSize.width);
	}
	if (_prevRichText) {
		_prevRichText->setPositionX(_swipeProgress - _contentSize.width);
	}
}

void ArticleLayout::applyView(View *) { }

void ArticleLayout::applyViewProgress(View *, View *, float) { }

void ArticleLayout::onTap() {
	setFlexibleLevelAnimated(1.0f, 0.25f);
	if (_sidebar->isNodeVisible()) {
		_sidebar->hide();
	}
}

void ArticleLayout::setSwipeProgress(const Vec2 &delta) {
	_richTextView->stopActionByTag("ListAction"_tag);
	float diff = delta.x;
	if (_currentNumber == 0 && _swipeProgress + diff > 0.0f) {
		_swipeProgress = 0.0f;
		onSwipeProgress();
	} else if (_currentNumber == _count - 1 && _swipeProgress + diff < 0.0f) {
		_swipeProgress = 0.0f;
		onSwipeProgress();
	} else {
		_swipeProgress += diff;
		onSwipeProgress();
	}
}
void ArticleLayout::onSwipeAction(cocos2d::Node *node) {
	if (node == _richTextView) {
		_swipeProgress = _richTextView->getPositionX();
		onSwipeProgress();
	}
}

void ArticleLayout::setNextRichTextView(size_t id) {
	if (_nextRichText) {
		_richTextView = _nextRichText;
		_nextRichText = nullptr;

		onViewSelected(_richTextView, id);
		setBaseNode(_richTextView, 1);
	}
	_currentNumber = id;
	_swipeProgress = 0;
	onSwipeProgress();
	if (_articleCallback) {
		_articleCallback(_currentNumber);
	}
}

void ArticleLayout::setPrevRichTextView(size_t id) {
	if (_prevRichText) {
		_richTextView = _prevRichText;
		_prevRichText = nullptr;

		onViewSelected(_richTextView, id);
		setBaseNode(_richTextView, 1);
	}
	_currentNumber = id;
	_swipeProgress = 0;
	onSwipeProgress();
	if (_articleCallback) {
		_articleCallback(_currentNumber);
	}
}

void ArticleLayout::onViewSelected(View *view, size_t) {
	auto source = view->getSource();
	source->setEnabled(true);
}

void ArticleLayout::endSwipeProgress(const Vec2 &delta, const Vec2 &velocity) {
	if (!_richTextView) {
		return;
	}

	float v = fabsf(velocity.x);
	float a = 5000;
	float t = v / a;
	float dx = v * t - (a * t * t) / 2;

	float pos = _swipeProgress + ((velocity.x > 0) ? (dx) : (-dx));

	cocos2d::FiniteTimeAction *action = nullptr;
	if (pos > _contentSize.width / 2 && _currentNumber > 0) {
		action = action::sequence(
				Accelerated::createBounce(5000, Vec2(_swipeProgress, 0), Vec2(_contentSize.width, 0.0f), Vec2(velocity.x, 0),
						200000, std::bind(&ArticleLayout::onSwipeAction, this, std::placeholders::_1)),
				[this] {
			setPrevRichTextView(_currentNumber - 1);
		});
	} else if (pos < -_contentSize.width / 2 && _currentNumber < _count - 1) {
		action = action::sequence(
				Accelerated::createBounce(5000, Vec2(_swipeProgress, 0), Vec2(-_contentSize.width, 0.0f), Vec2(velocity.x, 0),
						200000, std::bind(&ArticleLayout::onSwipeAction, this, std::placeholders::_1)),
				[this] {
			setNextRichTextView(_currentNumber + 1);
		});
	} else {
		action = action::sequence(
				Accelerated::createBounce(5000, Vec2(_swipeProgress, 0), Vec2(0, 0), Vec2(velocity.x, 0), 50000,
						std::bind(&ArticleLayout::onSwipeAction, this, std::placeholders::_1)),
				[this] {
			_swipeProgress = 0;
			onSwipeProgress();
		});
	}
	_richTextView->runAction(action, "ListAction"_tag);
}

void ArticleLayout::onSidebar(material::SidebarLayout *sidebar) { }

void ArticleLayout::onSelection(View *view, bool enabled) {
	if (view == _richTextView) {
		if (enabled) {
			showSelectionToolbar();
		} else {
			hideSelectionToolbar();
		}
	}
}

void ArticleLayout::showSelectionToolbar() {
	_tmpCallback = _toolbar->getNavCallback();
	_tmpIconProgress = _toolbar->getNavButtonIconProgress();
	_toolbar->replaceActionMenuSource(_selectionSource, _selectionSourceCount);
	_toolbar->setNavButtonIconProgress(2.0f, 0.25f);
	_toolbar->setNavCallback([this] {
		_richTextView->disableSelection();
	});
}
void ArticleLayout::hideSelectionToolbar() {
	_toolbar->replaceActionMenuSource(_viewSource, _viewSourceCount);
	_toolbar->setNavCallback(_tmpCallback);
	_toolbar->setNavButtonIconProgress(_tmpIconProgress, 0.25f);
}

void ArticleLayout::onCopyButton(const String &) { }
void ArticleLayout::onBookmarkButton() { }
void ArticleLayout::onMisprintButton() { }

NS_RT_END
