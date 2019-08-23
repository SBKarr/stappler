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
#include "MaterialRoundedProgress.h"
#include "MaterialButtonIcon.h"

#include "RTEpubNavigation.h"

#include "RTRenderer.h"
#include "RTDrawer.h"
#include "SPScrollView.h"
#include "SPScrollController.h"
#include "SPActions.h"
#include "SPRoundedSprite.h"
#include "SPGestureListener.h"
#include "SPDevice.h"
#include "SPLayer.h"
#include "SPRoundedSprite.h"
#include "RTView.h"

NS_RT_BEGIN

class EpubNavigation::Scroll : public material::MaterialNode {
public:
	static float getScrollSize() {
		if (Device::getInstance()->isTablet()) {
			return 192.0f;
		} else {
			return 96.0f;
		}
	}

	virtual bool init(const Callback &cb, const Callback &scb) {
		if (!MaterialNode::init()) {
			return false;
		}

		_callback = cb;
		_scrollCallback = scb;
		setShadowZIndex(0.0f);

		auto scissor = Rc<StrictNode>::create();
		scissor->setAnchorPoint(Vec2::ZERO);
		scissor->setPosition(Vec2::ZERO);

		auto view = Rc<ScrollView>::create(ScrollView::Horizontal);
		view->setController(Rc<ScrollController>::create());
		view->getGestureListener()->setSwallowTouches(true);
		view->setIndicatorVisible(false);
		view->setTapCallback([this] (int count, const Vec2 &loc) {
			const float len = _view->getScrollLength();
			const auto pos = _view->getRoot()->convertToNodeSpace(loc);
			const auto val = fabs(_view->getNodeScrollPosition(pos));
			if (_callback) {
				_callback(val/len);
			}
		});
		view->setScrollCallback(std::bind(&EpubNavigation::Scroll::onScroll, this));
		_view = scissor->addChildNode(view);
		_scissor = addChildNode(scissor);

		return true;
	}

	virtual void onContentSizeDirty() override {
		MaterialNode::onContentSizeDirty();
		_scissor->setContentSize(_contentSize);
		if (_view->getLayout() == ScrollView::Horizontal) {
			_view->setPosition(0.0f, 20.0f);
			_view->setAnchorPoint(Vec2::ZERO);
			_view->setContentSize(Size(_contentSize.width, getScrollSize()));
			_view->setEnabled(_contentSize.height > getScrollSize());
			_view->setVisible(_contentSize.height > 20.0f);
		} else {
			_view->setPosition(_contentSize.width - 20.0f, 0.0f);
			_view->setAnchorPoint(Vec2(1.0f, 0.0f));
			_view->setContentSize(Size(getScrollSize(), _contentSize.height - material::metrics::appBarHeight() - screen::statusBarHeight()));
			_view->setEnabled(_contentSize.width > getScrollSize());
			_view->setVisible(_contentSize.width > 20.0f);
		}
	}

	virtual void setRenderer(rich_text::Renderer *r) {
		_renderer = r;
	}

	virtual void setLayout(ScrollView::Layout l) {
		if (_view->getLayout() != l) {
			_view->setLayout(l);
			_view->getController()->clear();
			_contentSizeDirty = true;
		}
	}

	virtual void onResult(rich_text::Result *res) {
		_result = res;

		auto controller = _view->getController();
		controller->clear();

		auto surface = res->getSurfaceSize();
		if (res->getMedia().flags & layout::RenderFlag::PaginatedLayout) {
			auto num = res->getNumPages();
			bool isSplitted = _renderer->isPageSplitted();
			auto page = Size(surface.width + _renderer->getPageMargin().horizontal(), surface.height + _renderer->getPageMargin().vertical());
			_scale = getScrollSize() / page.height;

			for (size_t i = 0; i < num; ++ i) {
				controller->addItem(std::bind(&Scroll::onPageNode, this, i), page.width * _scale, page.width * _scale * i);
			}

			if (isSplitted && num % 2 == 1) {
				controller->addItem(std::bind(&Scroll::onPageNode, this, num), page.width * _scale, page.width * num * _scale);
			}
		} else {
			size_t idx = 0;
			auto size = res->getContentSize();
			_scale = getScrollSize() / size.width;
			float origin = 0;
			while (origin < size.height) {
				float offset = surface.height;
				if (offset > size.height - origin) {
					offset = size.height - origin;
				}

				controller->addItem(std::bind(&Scroll::onPageNode, this, idx), offset * _scale, origin * _scale);
				origin += offset;
				++ idx;
			}
		}

		controller->onScrollPosition();
	}

	rich_text::Renderer *getRenderer() const {
		return _renderer;
	}

	Rc<cocos2d::Node> onPageNode(size_t idx) {
		auto source = _renderer->getSource();
		auto drawer = _renderer->getDrawer();
		rich_text::Result::PageData data = _result->getPageData(idx, 0.0f);

		if (_result->getMedia().flags & layout::RenderFlag::PaginatedLayout) {
			auto page = Rc<cocos2d::Node>::create();
			if (data.isSplit) {
				if (data.num % 2 == 1) {
					data.margin.left -= data.margin.right / 3.0f;
					data.margin.right += data.margin.right / 3.0f;
				} else {
					data.margin.left += data.margin.right / 3.0f;
				}
			}

			data.margin *= _scale;

			auto sprite = Rc<DynamicSprite>::create(nullptr, Rect::ZERO, _result->getMedia().density);
			sprite->setNormalized(true);
			sprite->setFlippedY(true);
			sprite->setOpacity(0);
			sprite->setPosition(Vec2(data.margin.left, data.margin.bottom));
			page->addChild(sprite);

			if (idx < _result->getNumPages()) {
				page->retain();

				drawer->thumbnail(source, _result, data.texRect, _scale, [page, sprite] (cocos2d::Texture2D *tex) {
					if (page->isRunning()) {
						sprite->setTexture(tex);
						sprite->runAction(cocos2d::FadeIn::create(0.1f));
					}
					page->release();
				}, this);
			}

			auto label = Rc<material::Label>::create(material::FontType::Caption);
			label->setNormalizedPosition(Vec2(0.5f, 0.0f));
			label->setAnchorPoint(Anchor::MiddleBottom);
			label->setString(toString(data.num + 1));
			label->tryUpdateLabel();
			page->addChild(label, 2);

			auto layer = Rc<Layer>::create(material::Color::White);
			layer->setNormalizedPosition(Vec2(0.5f, 0.0f));
			layer->setAnchorPoint(Anchor::MiddleBottom);
			layer->setContentSize(label->getContentSize() + Size(8.0f, 2.0f));
			page->addChild(layer, 1);

			return page;
		} else {
			auto sprite = Rc<DynamicSprite>::create(nullptr, Rect::ZERO, _result->getMedia().density);
			sprite->setNormalized(true);
			sprite->setOpacity(0);
			sprite->retain();
			sprite->setFlippedY(true);

			drawer->thumbnail(source, _result, data.texRect, _scale, [sprite] (cocos2d::Texture2D *tex) {
				if (sprite->isRunning()) {
					sprite->setTexture(tex);
					sprite->runAction(cocos2d::FadeIn::create(0.1f));
				}
				sprite->release();
			}, this);

			return sprite;
		}
	}

	virtual bool isEnabled() const {
		return _view->isEnabled();
	}

	virtual void setProgress(float p) const {
		if (!_view->isMoved()) {
			_view->setScrollRelativePosition(p);
		}
	}

	virtual void onScroll() {
		if (_scrollCallback) {
			_scrollCallback(_view->getScrollRelativePosition());
		}
	}

protected:
	Rc<rich_text::Renderer> _renderer;
	Rc<rich_text::Result> _result;
	ScrollView *_view = nullptr;
	StrictNode *_scissor = nullptr;
	float _scale = 1.0f;
	Callback _callback = nullptr;
	Callback _scrollCallback = nullptr;
};

class EpubNavigation::Bookmarks : public cocos2d::Node {
public:
	virtual bool init() override;
	virtual void onContentSizeDirty() override;

	virtual void clearBookmarks();
	virtual void addBookmark(float start, float end);

	virtual void setBorderRadius(uint32_t);
protected:
	Vector<Pair<float, float>> _positions;
	uint32_t _borderRadius = 0;
};

bool EpubNavigation::Bookmarks::init() {
	if (!Node::init()) {
		return false;
	}

	setCascadeColorEnabled(true);
	setCascadeOpacityEnabled(true);

	return true;
}

void EpubNavigation::Bookmarks::onContentSizeDirty() {
	Node::onContentSizeDirty();

	bool vertical = (_contentSize.width < _contentSize.height);

	auto field = vertical?_contentSize.width:_contentSize.height;
	auto line = vertical?_contentSize.height:_contentSize.width - field;

	removeAllChildren();
	for (auto &it : _positions) {
		if (!isnan(it.first) && !isnan(it.second)) {
			auto s = Rc<RoundedSprite>::create(0);
			s->setColor(material::Color::White);
			s->setTextureSize(_borderRadius);
			s->setAnchorPoint(Anchor::BottomLeft);

			auto length = (it.second - it.first) * line;
			if (length < field) {
				s->setContentSize(Size(field, field));
			} else {
				s->setContentSize(vertical?Size(field, length):Size(length, field));
			}

			if (vertical) {
				s->setPosition(0.0f, (1.0f - it.second) * (line - field));
			} else {
				s->setPosition(it.first * line, 0.0f);
			}

			addChild(s);
		}
	}
	updateColor();
}

void EpubNavigation::Bookmarks::clearBookmarks() {
	_positions.clear();
	_contentSizeDirty = true;
}

void EpubNavigation::Bookmarks::addBookmark(float start, float end) {
	_positions.emplace_back(start, end);
	_contentSizeDirty = true;
}

void EpubNavigation::Bookmarks::setBorderRadius(uint32_t r) {
	_borderRadius = r;
	_contentSizeDirty = true;
}

bool EpubNavigation::init(const Callback &cb) {
	if (!Node::init()) {
		return false;
	}

	_callback = cb;

	auto gl = Rc<gesture::Listener>::create();
	gl->setSwallowTouches(true);
	gl->setTouchCallback([this] (gesture::Event ev, const gesture::Touch &p) -> bool {
		if (ev == gesture::Event::Began) {
			if (_touchCaptured) {
				return false;
			}
			if (!node::isTouched(_progress, p.location(), 8.0f)) {
				return false;
			} else {
				_touchCaptured = true;
				float progress = getProgressForPoint(p.location());
				onManualProgress(progress, 0.15f);
				if (!_scroll->isEnabled()) {
					_autoHideView = true;
					open();
				}
			}
		} else if (ev == gesture::Event::Activated) {
			float progress = getProgressForPoint(p.location());
			onManualProgress(progress);
		} else if (ev == gesture::Event::Ended) {
			if (_autoHideView) {
				if (node::isTouched(_progress, p.location(), 16.0f)) {
					float progress = getProgressForPoint(p.location());
					propagateProgress(progress);
				} else {
					onManualProgress(_view->getScrollRelativePosition(), 0.15f);
				}
				close();
				_autoHideView = false;
			}
			_touchCaptured = false;
		} else if (ev == gesture::Event::Cancelled) {
			onManualProgress(_view->getScrollRelativePosition(), 0.15f);
			if (_autoHideView) {
				close();
				_autoHideView = false;
			}
			_touchCaptured = false;
		}
		return true;
	});
	_listener = gl;
	addComponent(gl);

	auto progress = Rc<material::RoundedProgress>::create();
	progress->setBorderRadius(6);
	progress->setBarColor(material::Color::Grey_500);
	progress->setLineColor(material::Color::Grey_300);
	_progress = addChildNode(progress, 6);

	auto bookmarks = Rc<Bookmarks>::create();
	bookmarks->setBorderRadius(6);
	bookmarks->setColor(material::Color::LightGreen_500);
	bookmarks->setOpacity(168);
	_bookmarks = addChildNode(bookmarks, 7);

	auto icon = Rc<material::ButtonIcon>::create(material::IconName::Navigation_expand_less, std::bind(&EpubNavigation::onButton, this));
	icon->setIconColor(material::Color::Black);
	icon->setSwallowTouches(true);
	icon->setBackgroundVisible(false);
	icon->setAnimationOpacity(0);
	//_icon->setTouchPadding(24.0f);
	_icon = addChildNode(icon, 5);

	auto scroll = Rc<Scroll>::create([this] (float v) {
		if (_callback) {
			_progress->setProgress(v);
			_callback(v);
		}
		close();
	}, [this] (float v) {
		_progress->setProgress(v);
	});
	_scroll = addChildNode(scroll, 4);

	return true;
}

void EpubNavigation::onContentSizeDirty() {
	Node::onContentSizeDirty();

	if (_contentSize.width > _contentSize.height) {
		_progress->setContentSize(Size(_contentSize.width - 8.0f - 48.0f, 12.0f));
		_progress->setPosition(Vec2(4.0f, 4.0f));
		_progress->setAnchorPoint(Vec2::ZERO);
		_progress->setInverted(false);

		_icon->setAnchorPoint(Vec2(1.0f, 0.5f));
		_icon->setPosition(_contentSize.width, 10.0f);
		_icon->setContentSize(cocos2d::Size(48.0f, 48.0f));

		_scroll->setPosition(0.0f, 0.0f);
		_scroll->setAnchorPoint(Vec2::ZERO);
	} else {
		_progress->setContentSize(Size(12.0f, _contentSize.height - 8.0f - material::metrics::appBarHeight() - screen::statusBarHeight() - 48.0f));
		_progress->setPosition(Vec2(_contentSize.width - 4.0f, 4.0f + 48.0f));
		_progress->setAnchorPoint(Vec2(1.0f, 0.0f));
		_progress->setInverted(true);

		_icon->setAnchorPoint(Vec2(0.5f, 0.0f));
		_icon->setPosition(_contentSize.width - 10.0f, 0.0f);
		_icon->setContentSize(Size(48.0f, 48.0f));

		_scroll->setPosition(_contentSize.width, 0.0f);
		_scroll->setAnchorPoint(Vec2(1.0f, 0.0f));
	}

	_bookmarks->setContentSize(_progress->getContentSize());
	_bookmarks->setPosition(_progress->getPosition());
	_bookmarks->setAnchorPoint(_progress->getAnchorPoint());

	onOpenProgress(_openProgress);
}

void EpubNavigation::setView(View *view) {
	_view = view;
	_scroll->setRenderer(_view->getRenderer());
}

void EpubNavigation::open() {
	if (_openProgress < 1.0f) {
		stopActionByTag("CloseAction"_tag);
		auto tmp = getActionByTag("OpenAction"_tag);
		if (!tmp) {
			auto a = Rc<ProgressAction>::create(progress(0.0f, 0.25f, 1.0f - _openProgress), _openProgress, 1.0f, [this] (ProgressAction *a, float time) {
				onOpenProgress(time);
			});
			runAction(cocos2d::EaseQuadraticActionOut::create(a), "OpenAction"_tag);
		}
	}
}

void EpubNavigation::close() {
	if (_openProgress > 0.0f) {
		stopActionByTag("OpenAction"_tag);
		auto tmp = getActionByTag("CloseAction"_tag);
		if (!tmp) {
			auto a = Rc<ProgressAction>::create(progress(0.0f, 0.25f, _openProgress), _openProgress, 0.0f, [this] (ProgressAction *a, float time) {
				onOpenProgress(time);
			});
			runAction(cocos2d::EaseQuadraticActionIn::create(a), "CloseAction"_tag);
		}

	}
}

void EpubNavigation::clearBookmarks() {
	_bookmarks->clearBookmarks();
}
void EpubNavigation::addBookmark(float start, float end) {
	_bookmarks->addBookmark(start, end);
}

void EpubNavigation::setReaderProgress(float p) {
	if (p < 0.0f) {
		p = 0.0f;
	} else if (p > 1.0f) {
		p = 1.0f;
	}
	_scroll->setProgress(p);
	_progress->setProgress(p);
	_readerProgress = p;
	close();
}

void EpubNavigation::onResult(layout::Result *res) {
	if (_scroll->getRenderer()->isPageSplitted()) {
		_progress->setBarScale(2.0f / res->getNumPages());
	} else {
		_progress->setBarScale(1.0f / res->getNumPages());
	}
	if (_scroll) {
		_scroll->onResult(res);
	}
}

void EpubNavigation::onButton() {
	if (_openProgress == 0.0f) {
		open();
	} else {
		close();
	}
}

void EpubNavigation::onOpenProgress(float val) {
	_openProgress = val;
	if (_contentSize.width > _contentSize.height) {
		_icon->setIconName(val==0.0f ? material::IconName::Navigation_expand_less : material::IconName::Navigation_expand_more);
		_scroll->setContentSize(Size(_contentSize.width, 20.0f + Scroll::getScrollSize() * _openProgress));
		_scroll->setLayout(ScrollView::Horizontal);
	} else {
		_icon->setIconName(val==0.0f ? material::IconName::Navigation_chevron_left : material::IconName::Navigation_chevron_right);
		_scroll->setContentSize(Size(20.0f + Scroll::getScrollSize() * _openProgress, _contentSize.height));
		_scroll->setLayout(ScrollView::Vertical);
	}

	if (_openProgress < 0.5f && _openProgress > 0.2f) {
		_scroll->setShadowZIndex(progress(0.0f, 1.5f, (_openProgress - 0.2f) / 0.3f));
		_scroll->getBackground()->setOpacity(255);
	} else if (_openProgress <= 0.2f) {
		_scroll->getBackground()->setOpacity(progress(0, 255, _openProgress / 0.2f));
		_scroll->setShadowZIndex(0.0f);
	} else {
		_scroll->setShadowZIndex(1.5f);
		_scroll->getBackground()->setOpacity(255);
	}
}

void EpubNavigation::onManualProgress(float progress, float anim) {
	if (progress < 0.0f) {
		progress = 0.0f;
	} else if (progress > 1.0f) {
		progress = 1.0f;
	}

	_progress->setProgress(progress, anim);
	_scroll->setProgress(progress);
}

float EpubNavigation::getProgressForPoint(const Vec2 &loc) const {
	auto pos = _progress->convertToNodeSpace(loc);
	float value = (_contentSize.width > _contentSize.height)
			? (pos.x / _progress->getContentSize().width)
			: ((_progress->getContentSize().height - pos.y) / _progress->getContentSize().height);
	if (value < 0.0f) {
		value = 0.0f;
	} else if (value > 1.0f) {
		value = 1.0f;
	}
	return value;
}

void EpubNavigation::propagateProgress(float value) const {
	if (_callback) {
		_callback(value);
	}
}

NS_RT_END
