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

#include "SPDefine.h"
#include "SLLayout.h"

#include "SPScrollController.h"
#include "SPDynamicSprite.h"
#include "SPEventListener.h"

#include "SPLayer.h"
#include "SPScreen.h"
#include "SPDevice.h"
#include "SPActions.h"

#include "2d/CCActionInterval.h"
#include "base/CCDirector.h"
#include "RTCommonView.h"
#include "RTDrawer.h"
#include "RTRenderer.h"

NS_SP_EXT_BEGIN(rich_text)

bool CommonView::Page::init(const PageData &data, float d) {
	if (!Node::init()) {
		return false;
	}

	_data = data;
	if (data.isSplit) {
		if (data.num % 2 == 1) {
			_data.margin.left -= data.margin.right / 3.0f;
			_data.margin.right += data.margin.right / 3.0f;
		} else {
			_data.margin.left += data.margin.right / 3.0f;
		}
	}

	auto background = Rc<Layer>::create(Color4B(128, 128, 128, 255));
	background->setAnchorPoint(Vec2(0, 0));
	background->setPosition(Vec2(0, 0));
	background->setVisible(false);
	addChild(background, -1);
	_background = background;

	auto sprite = Rc<DynamicSprite>::create(nullptr, Rect::ZERO, d);
	sprite->setNormalized(true);
	sprite->setFlippedY(true);
	sprite->setOpacity(0);
	sprite->setContentSize(_data.texRect.size);
	sprite->setPosition(Vec2(_data.margin.left, _data.margin.bottom));
	addChild(sprite);
	_sprite = sprite;

	setContentSize(_data.viewRect.size);
	return true;
}

void CommonView::Page::setTexture(cocos2d::Texture2D *tex) {
	_sprite->setTexture(tex);
	_sprite->runAction(cocos2d::FadeIn::create(0.1f));
}
void CommonView::Page::setBackgroundColor(const Color3B &c) {
	_background->setColor(c);
}
Vec2 CommonView::Page::convertToObjectSpace(const Vec2 &vec) const {
	if (node::isTouched(_sprite, vec)) {
		auto loc = _sprite->convertToNodeSpace(vec);
		loc.y = _data.texRect.origin.y + _data.texRect.size.height - loc.y;
		return loc;
	}
	return Vec2::ZERO;
}

CommonView::~CommonView() {
	_renderer->setRenderingCallback(nullptr);
}

bool CommonView::init(Layout l, CommonSource *source, const Vector<String> &ids) {
	if (!ScrollView::init(l)) {
		return false;
	}

	setController(Rc<ScrollController>::create());

	auto renderer = Rc<Renderer>::create(ids);
	renderer->setRenderingCallback(std::bind(&CommonView::onRenderer, this,
			std::placeholders::_1, std::placeholders::_2));
	_renderer = addComponentItem(renderer);

	auto background = Rc<Layer>::create(Color4B(238, 238, 238, 255));
	background->setAnchorPoint(Vec2(0, 0));
	background->setPosition(Vec2(0, 0));
	_background = addChildNode(background, -1);

	_pageMargin = Margin(8.0f, 8.0f);

	if (source) {
		setSource(source);
	}

	auto el = Rc<EventListener>::create();
	el->onEvent(Device::onRegenerateTextures, [this] (const Event &) {
		if (auto res = _renderer->getResult()) {
			onRenderer(res, false);
			_controller->onScrollPosition(true);
		}
	});
	addComponentItem(el);

	return true;
}

void CommonView::setLayout(Layout l) {
	if (l != _layout) {
		_savedRelativePosition = getScrollRelativePosition();
		_controller->clear();
		ScrollView::setLayout(l);
	}
}

Vec2 CommonView::convertToObjectSpace(const Vec2 &vec) const {
	if (_layout == Vertical) {
		auto loc = _root->convertToNodeSpace(vec);
		loc.y = _root->getContentSize().height - loc.y - _objectsOffset;
		return loc;
	} else {
		for (auto &node : _root->getChildren()) {
			if (node::isTouched(node, vec)) {
				auto page = dynamic_cast<Page *>(node);
				if (page) {
					return page->convertToObjectSpace(vec);
				}
			}
		}
	}
	return Vec2::ZERO;
}

bool CommonView::isObjectActive(const Object &obj) const {
	return obj.type == layout::Object::Type::Ref;
}

bool CommonView::isObjectTapped(const Vec2 & loc, const Object &obj) const {
	if (obj.type == layout::Object::Type::Ref) {
		if (loc.x >= obj.bbox.getMinX() - 8.0f && loc.x <= obj.bbox.getMaxX() + 8.0f && loc.y >= obj.bbox.getMinY() - 8.0f && loc.y <= obj.bbox.getMaxY() + 8.0f) {
			return true;
		}
		return false;
	} else {
		return obj.bbox.containsPoint(loc);
	}
}

void CommonView::onObjectPressBegin(const Vec2 &, const Object &obj) { }

void CommonView::onObjectPressEnd(const Vec2 &, const Object &obj) { }

void CommonView::onSwipeBegin() {
	ScrollView::onSwipeBegin();
	_gestureStart = getScrollPosition();
}

bool CommonView::onPressBegin(const Vec2 &vec) {
	ScrollView::onPressBegin(vec);
	auto res = _renderer->getResult();
	if (!res) {
		return true;
	}
	auto &refs = res->getRefs();
	if (refs.empty()) {
		return true;
	}

	if (_linksEnabled) {
		auto loc = convertToObjectSpace(vec);
		for (auto &it : refs) {
			if (isObjectTapped(loc, *it)) {
				onObjectPressBegin(vec, *it);
				return true;
			}
		}
	}

	return true;
}

bool CommonView::onPressEnd(const Vec2 &vec, const TimeInterval &r) {
	ScrollView::onPressEnd(vec, r);
	if (_layout == Horizontal && _movement == Movement::None) {
		_gestureStart = nan();
		onSwipe(0.0f, 0.0f, true);
	}

	auto res = _renderer->getResult();
	if (!res) {
		return false;
	}
	auto &refs = res->getRefs();
	if (refs.empty()) {
		return false;
	}

	if (_linksEnabled) {
		auto loc = convertToObjectSpace(vec);
		for (auto &it : refs) {
			if (isObjectActive(*it)) {
				if (isObjectTapped(loc, *it)) {
					onObjectPressEnd(vec, *it);
					return true;
				}
			}
		}
	}

	return false;
}

bool CommonView::onPressCancel(const Vec2 &vec, const TimeInterval &r) {
	ScrollView::onPressCancel(vec, r);
	return false;
}

void CommonView::onContentSizeDirty() {
	if (_layout == Layout::Horizontal) {
		if (!_renderer->hasFlag(layout::RenderFlag::PaginatedLayout)) {
			_renderer->addFlag(layout::RenderFlag::PaginatedLayout);
		}
	} else {
		if (_renderer->hasFlag(layout::RenderFlag::PaginatedLayout)) {
			_renderer->removeFlag(layout::RenderFlag::PaginatedLayout);
		}
	}

	auto m = (_layout == Layout::Horizontal)?_pageMargin:Margin(_paddingGlobal.top, 0.0f, 0.0f);
	//m.top += (_paddingGlobal.top);
	_renderer->setPageMargin(m);

	ScrollView::onContentSizeDirty();
	_background->setContentSize(getContentSize());

	_gestureStart = nan();
}

void CommonView::setSource(CommonSource *source) {
	if (_source != source) {
		_source = source;
		_renderer->setSource(_source);
	}
}

CommonSource *CommonView::getSource() const {
	return _source;
}

Renderer *CommonView::getRenderer() const {
	return _renderer;
}

Document *CommonView::getDocument() const {
	return _renderer->getDocument();
}
Result *CommonView::getResult() const {
	return _renderer->getResult();
}

void CommonView::refresh() {
	if (_source) {
		_source->refresh();
	}
}

void CommonView::setResultCallback(const ResultCallback &cb) {
	_callback = cb;
}

const CommonView::ResultCallback &CommonView::getResultCallback() const {
	return _callback;
}

void CommonView::onRenderer(Result *res, bool) {
	if (res) {
		auto color = res->getBackgroundColor();

		_background->setColor(Color3B(color.r, color.g, color.b));
		_background->setOpacity(color.a);

		_controller->clear();
		_root->removeAllChildren();

		onPageData(res, 0.0f);

		if (_callback) {
			_callback(res);
		}

		if (_running) {
			_controller->onScrollPosition();
		}
	}
}

void CommonView::onPageData(Result *res, float contentOffset) {
	auto surface = res->getSurfaceSize();
	if (surface.height == 0) {
		surface.height = res->getContentSize().height;
	}
	auto &media = res->getMedia();
	if (media.flags & layout::RenderFlag::PaginatedLayout) {
		auto num = res->getNumPages();
		bool isSplitted = media.flags & layout::RenderFlag::SplitPages;
		auto page = Size(surface.width + res->getMedia().pageMargin.horizontal(), surface.height + res->getMedia().pageMargin.vertical());
		for (size_t i = 0; i < num; ++ i) {
			_controller->addItem(std::bind(&CommonView::onPageNode, this, i), page.width, page.width * i);
		}

		if (isSplitted && num % 2 == 1) {
			_controller->addItem(std::bind(&CommonView::onPageNode, this, num), page.width, page.width * num);
		}
		_objectsOffset = 0;
	} else {
		if (isnan(contentOffset)) {
			contentOffset = _controller->getNextItemPosition();
		}

		size_t idx = 0;
		auto size = res->getContentSize();
		float origin = 0;
		while (size.height - origin > 1.0f) {
			float offset = surface.height;
			if (offset > size.height - origin) {
				offset = size.height - origin;
			}

			_controller->addItem(std::bind(&CommonView::onPageNode, this, idx), offset, origin + contentOffset);
			origin += offset;
			++ idx;
		}
		_objectsOffset = contentOffset;
	}
}

PageData CommonView::getPageData(size_t idx) const {
	return _renderer->getResult()->getPageData(idx, _objectsOffset);
}

Rc<cocos2d::Node> CommonView::onPageNode(size_t idx) {
	auto source = _renderer->getSource();
	auto result = _renderer->getResult();
	auto drawer = _renderer->getDrawer();
	PageData data = getPageData(idx);

	if (data.texRect.equals(Rect::ZERO)) {
		return nullptr;
	}

	if (result->getMedia().flags & layout::RenderFlag::PaginatedLayout) {
		auto page = onConstructPageNode(data, result->getMedia().density);
		if (idx < result->getNumPages()) {
			page->retain();

			drawer->draw(source, result, data.texRect, [page] (cocos2d::Texture2D *tex) {
				if (page->isRunning()) {
					page->setTexture(tex);
				}
				page->release();
			}, this);
		}

		return page;
	} else {
		auto sprite = Rc<DynamicSprite>::create(nullptr, Rect::ZERO, result->getMedia().density);
		sprite->setNormalized(true);
		sprite->setFlippedY(true);
		sprite->setOpacity(0);

		drawer->draw(source, result, data.texRect, [sprite] (cocos2d::Texture2D *tex) {
			if (sprite->isRunning()) {
				sprite->setTexture(tex);
				sprite->runAction(cocos2d::FadeIn::create(0.1f));
			}
		}, this);

		return sprite;
	}
}

Rc<CommonView::Page> CommonView::onConstructPageNode(const PageData &data, float density) {
	return Rc<Page>::create(data, density);
}

cocos2d::ActionInterval *CommonView::onSwipeFinalizeAction(float velocity) {
	if (_layout == Layout::Vertical) {
		return ScrollView::onSwipeFinalizeAction(velocity);
	}

	float acceleration = (velocity > 0)?-3500.0f:3500.0f;
	float boundary = (velocity > 0)?_scrollMax:_scrollMin;

	Vec2 normal = (isVertical())
			?(Vec2(0.0f, (velocity > 0)?1.0f:-1.0f))
			:(Vec2((velocity > 0)?-1.0f:1.0f, 0.0f));

	cocos2d::ActionInterval *a = nullptr;

	if (!isnan(_maxVelocity)) {
		if (velocity > fabs(_maxVelocity)) {
			velocity = fabs(_maxVelocity);
		} else if (velocity < -fabs(_maxVelocity)) {
			velocity = -fabs(_maxVelocity);
		}
	}

	float pos = getScrollPosition();
	float duration = fabsf(velocity / acceleration);
	float path = velocity * duration + acceleration * duration * duration * 0.5f;
	float newPos = pos + path;
	float segment = _contentSize.width;
	int num = roundf(newPos / segment);

	if (!isnan(_gestureStart)) {
		int prev = roundf(_gestureStart / segment);
		if (num - prev < -1) {
			num = prev - 1;
		}
		if (num - prev > 1) {
			num = prev + 1;
		}
		_gestureStart = nan();
	}
	newPos = num * segment;

	if (!isnan(boundary)) {
		auto from = _root->getPosition();
		auto to = getPointForScrollPosition(boundary);

		float distance = from.distance(to);
		if (distance < 2.0f) {
			setScrollPosition(boundary);
			return nullptr;
		}

		if ((velocity > 0 && newPos > boundary) || (velocity < 0 && newPos < boundary)) {
			_movementAction = Accelerated::createAccelerationTo(from, to, fabsf(velocity), -fabsf(acceleration));

			auto overscrollPath = path + ((velocity < 0)?(distance):(-distance));
			if (fabs(overscrollPath) < std::numeric_limits<float>::epsilon() ) {
				auto cb = cocos2d::CallFunc::create(std::bind(&CommonView::onOverscroll, this, overscrollPath));
				a = cocos2d::Sequence::createWithTwoActions(_movementAction, cb);
			}
		}
	}

	if (!_movementAction) {
		auto from = _root->getPosition();
		auto to = getPointForScrollPosition(newPos);

		return Accelerated::createBounce(fabsf(acceleration), from, to, Vec2(-velocity, 0.0f), 25000);
	}

	if (!a) {
		a = _movementAction;
	}

	return a;
}

void CommonView::setScrollRelativePosition(float value) {
	if (isVertical()) {
		ScrollView::setScrollRelativePosition(value);
		return;
	}

	if (!isnan(value)) {
		if (value < 0.0f) {
			value = 0.0f;
		} else if (value > 1.0f) {
			value = 1.0f;
		}
	} else {
		value = 0.0f;
	}

	float areaSize = getScrollableAreaSize();
	float areaOffset = getScrollableAreaOffset();

	auto &padding = getPadding();
	auto paddingFront = (isVertical())?padding.top:padding.left;
	auto paddingBack = (isVertical())?padding.bottom:padding.right;

	if (!isnan(areaSize) && !isnan(areaOffset) && areaSize > 0) {
		float liveSize = areaSize + paddingFront + paddingBack;
		float pos = (value * liveSize) - paddingFront + areaOffset;
		float segment = _contentSize.width;

		int num = roundf(pos / segment);
		if (num < 0) {
			num = 0;
		}

		setScrollPosition(num * segment);
	} else {
		_savedRelativePosition = value;
	}
}

bool CommonView::showNextPage() {
	if (isHorizontal() && !isMoved() && !_animationAction) {
		_movement = Movement::Auto;
		_animationAction = action::sequence(
				Accelerated::createBounce(7500, _root->getPosition(), _root->getPosition() - Vec2(_contentSize.width, 0), Vec2(0, 0)),
				[this] {
			onAnimationFinished();
		});
		_root->runAction(_animationAction);
		return true;
	}
	return false;
}

bool CommonView::showPrevPage() {
	if (isHorizontal() && !isMoved() && !_animationAction) {
		_movement = Movement::Auto;
		_animationAction = action::sequence(
				Accelerated::createBounce(7500, _root->getPosition(), _root->getPosition() + Vec2(_contentSize.width, 0), Vec2(0, 0)),
				[this] {
			onAnimationFinished();
		});
		_root->runAction(_animationAction);
		return true;
	}
	return false;
}

float CommonView::getObjectsOffset() const {
	return _objectsOffset;
}

void CommonView::setLinksEnabled(bool value) {
	_linksEnabled = value;
}
bool CommonView::isLinksEnabled() const {
	return _linksEnabled;
}

void CommonView::onPosition() {
	ScrollView::onPosition();
	if (_movement == Movement::None) {
		_gestureStart = nan();
	}
}

NS_SP_EXT_END(rich_text)
