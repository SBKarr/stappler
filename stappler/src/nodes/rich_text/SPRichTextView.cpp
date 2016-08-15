/*
 * SPRichTextView.cpp
 *
 *  Created on: 31 июля 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPRichTextView.h"
#include "SPRichTextRenderer.h"
#include "SPRichTextLayout.h"

#include "SPScrollController.h"
#include "SPRichTextDrawer.h"
#include "SPDynamicSprite.h"

#include "SPLayer.h"
#include "SPScreen.h"
#include "SPActions.h"
#include "SPRichLabel.h"

#include "2d/CCActionInterval.h"
#include "base/CCDirector.h"

NS_SP_EXT_BEGIN(rich_text)

bool View::Page::init(const PageData &data, float d) {
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

	_background = construct<Layer>();
	_background->setColor(Color3B(128, 128, 128));
	_background->setAnchorPoint(Vec2(0, 0));
	_background->setPosition(Vec2(0, 0));
	addChild(_background, -1);

	_sprite = construct<DynamicSprite>(nullptr, Rect::ZERO, d);
	_sprite->setNormalized(true);
	_sprite->setOpacity(0);
	_sprite->setContentSize(_data.texRect.size);
	_sprite->setPosition(Vec2(_data.margin.left, _data.margin.bottom));
	addChild(_sprite);

	setContentSize(_data.viewRect.size);
	return true;
}

void View::Page::setTexture(cocos2d::Texture2D *tex) {
	_sprite->setTexture(tex);
	_sprite->runAction(cocos2d::FadeIn::create(0.1f));
}
void View::Page::setBackgroundColor(const Color3B &c) {
	_background->setColor(c);
}
Vec2 View::Page::convertToObjectSpace(const Vec2 &vec) const {
	if (node::isTouched(_sprite, vec)) {
		auto loc = _sprite->convertToNodeSpace(vec);
		loc.y = _data.texRect.origin.y + _data.texRect.size.height - loc.y;
		return loc;
	}
	return Vec2::ZERO;
}

View::~View() {
	_renderer->setRenderingCallback(nullptr);
}

bool View::init(Layout l, Source *source, const Vector<String> &ids) {
	if (!ScrollView::init(l)) {
		return false;
	}

	setController(construct<ScrollController>());

	_renderer = construct<Renderer>(ids);
	_renderer->setRenderingCallback(std::bind(&View::onRenderer, this,
			std::placeholders::_1, std::placeholders::_2));
	addComponent(_renderer);

	_background = construct<Layer>();
	_background->setColor(Color3B(238, 238, 238));
	_background->setAnchorPoint(Vec2(0, 0));
	_background->setPosition(Vec2(0, 0));
	addChild(_background, -1);

	_pageMargin = Margin(8.0f, 18.0f);

	if (source) {
		setSource(source);
	}

	return true;
}

void View::setLayout(Layout l) {
	if (l != _layout) {
		_savedRelativePosition = getScrollRelativePosition();
		_controller->clear();
		ScrollView::setLayout(l);
	}
}

void View::setHyphens(rich_text::HyphenMap *map) {
	_renderer->setHyphens(map);
}

Vec2 View::convertToObjectSpace(const Vec2 &vec) const {
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

bool View::isObjectActive(const Object &obj) const {
	return obj.type == Object::Type::Ref;
}

bool View::isObjectTapped(const Vec2 & loc, const Object &obj) const {
	if (obj.type == Object::Type::Ref) {
		Rect bbox = obj.bbox;
		bbox.origin.x -= 8.0f;
		bbox.origin.y -= 8.0f;
		bbox.size.width += 16.0f;
		bbox.size.height += 16.0f;
		return bbox.containsPoint(loc);
	} else {
		return obj.bbox.containsPoint(loc);
	}
}

void View::onObjectPressBegin(const Vec2 &, const Object &obj) {
	if (obj.type == Object::Type::Ref) {
		//log("ref: %s", obj.value.ref.target.c_str());
	}
}

void View::onObjectPressEnd(const Vec2 &, const Object &obj) {
	if (obj.type == Object::Type::Ref) {
		//log("ref: %s", obj.value.ref.target.c_str());
	}
}

bool View::onPressBegin(const Vec2 &vec) {
	ScrollView::onPressBegin(vec);
	auto res = _renderer->getResult();
	if (!res) {
		return true;
	}
	auto &refs = res->getRefs();
	if (refs.empty()) {
		return true;
	}

	auto loc = convertToObjectSpace(vec);
	for (auto &it : refs) {
		if (isObjectTapped(loc, it)) {
			onObjectPressBegin(vec, it);
			return true;
		}
	}

	return true;
}

bool View::onPressEnd(const Vec2 &vec, const TimeInterval &r) {
	ScrollView::onPressEnd(vec, r);
	if (_layout == Horizontal && _movement == Movement::None) {
		onSwipe(0.0f, 0.0f, true);
	}

	auto res = _renderer->getResult();
	if (!res) {
		return false;
	}
	auto &objs = res->getRefs();
	if (objs.empty()) {
		return false;
	}

	auto loc = convertToObjectSpace(vec);
	for (auto &it : objs) {
		if (isObjectActive(it)) {
			if (isObjectTapped(loc, it)) {
				onObjectPressEnd(vec, it);
				return true;
			}
		}
	}

	return false;
}

bool View::onPressCancel(const Vec2 &vec, const TimeInterval &r) {
	ScrollView::onPressCancel(vec, r);
	return false;
}

void View::onContentSizeDirty() {
	if (_layout == Layout::Horizontal) {
		if (!_renderer->hasFlag(rich_text::RenderFlag::PaginatedLayout)) {
			_renderer->addFlag(rich_text::RenderFlag::PaginatedLayout);
		}
	} else {
		if (_renderer->hasFlag(rich_text::RenderFlag::PaginatedLayout)) {
			_renderer->removeFlag(rich_text::RenderFlag::PaginatedLayout);
		}
	}

	auto m = (_layout == Layout::Horizontal)?_pageMargin:Margin();
	m.top += _paddingGlobal.top;
	_renderer->setPageMargin(m);

	ScrollView::onContentSizeDirty();
	_background->setContentSize(getContentSize());
}

void View::setSource(Source *source) {
	if (_source != source) {
		_source = source;
		_renderer->setSource(_source);
	}
}

Source *View::getSource() const {
	return _source;
}

Renderer *View::getRenderer() const {
	return _renderer;
}

Document *View::getDocument() const {
	return _renderer->getDocument();
}
Result *View::getResult() const {
	return _renderer->getResult();
}

void View::refresh() {
	if (_source) {
		_source->refresh();
	}
}

void View::setResultCallback(const ResultCallback &cb) {
	_callback = cb;
}

const View::ResultCallback &View::getResultCallback() const {
	return _callback;
}

void View::onRenderer(Result *res, bool) {
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

		_controller->onScrollPosition();
	}
}

void View::onPageData(Result *res, float contentOffset) {
	auto surface = res->getSurfaceSize();
	if (res->getMedia().flags & RenderFlag::PaginatedLayout) {
		auto num = res->getNumPages();
		bool isSplitted = _renderer->isPageSplitted();
		auto page = Size(surface.width + _renderer->getPageMargin().horizontal(), surface.height + _renderer->getPageMargin().vertical());
		for (size_t i = 0; i < num; ++ i) {
			_controller->addItem(std::bind(&View::onPageNode, this, i), page.width, page.width * i);
		}

		if (isSplitted && num % 2 == 1) {
			_controller->addItem(std::bind(&View::onPageNode, this, num), page.width, page.width * num);
		}
		_objectsOffset = 0;
	} else {
		size_t idx = 0;
		auto size = res->getContentSize();
		float origin = 0;
		while (origin < size.height) {
			float offset = surface.height;
			if (offset > size.height - origin) {
				offset = size.height - origin;
			}

			_controller->addItem(std::bind(&View::onPageNode, this, idx), offset, origin + contentOffset);
			origin += offset;
			++ idx;
		}
		_objectsOffset = contentOffset;
	}
}

View::PageData View::getPageData(size_t idx) const {
	return _renderer->getResult()->getPageData(_renderer, idx, _objectsOffset);
}

cocos2d::Node * View::onPageNode(size_t idx) {
	auto source = _renderer->getSource();
	auto result = _renderer->getResult();
	PageData data = getPageData(idx);

	if (result->getMedia().flags & RenderFlag::PaginatedLayout) {
		auto page = construct<Page>(data, result->getMedia().density);
		if (idx < result->getNumPages()) {
			page->retain();

			construct<Drawer>(source, result, data.texRect, [this, page] (cocos2d::Texture2D *tex) {
				if (page->isRunning()) {
					page->setTexture(tex);
				}
				page->release();
			}, this);
		}

		return page;
	} else {
		auto sprite = construct<DynamicSprite>(nullptr, Rect::ZERO, result->getMedia().density);
		sprite->setNormalized(true);
		sprite->setOpacity(0);
		sprite->retain();

		construct<Drawer>(source, result, data.texRect, [this, sprite] (cocos2d::Texture2D *tex) {
			if (sprite->isRunning()) {
				sprite->setTexture(tex);
				sprite->runAction(cocos2d::FadeIn::create(0.1f));
			}
			sprite->release();
		}, this);

		return sprite;
	}
}

cocos2d::ActionInterval *View::onSwipeFinalizeAction(float velocity) {
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

	if (!isnan(boundary)) {
		auto from = _root->getPosition();
		auto to = getPointForScrollPosition(boundary);

		float distance = from.distance(to);
		if (distance < 2.0f) {
			setScrollPosition(boundary);
			return nullptr;
		}

		if ((velocity > 0 && pos + path > boundary) || (velocity < 0 && pos + path < boundary)) {
			_movementAction = Accelerated::createAccelerationTo(from, to, fabsf(velocity), -fabsf(acceleration));

			auto overscrollPath = path + ((velocity < 0)?(distance):(-distance));
			if (overscrollPath) {
				auto cb = cocos2d::CallFunc::create(std::bind(&View::onOverscroll, this, overscrollPath));
				a = cocos2d::Sequence::createWithTwoActions(_movementAction, cb);
			}
		}
	}

	if (!_movementAction) {
		float newPos = pos + path;
		float segment = _contentSize.width;

		int num = roundf(newPos / segment);
		auto from = _root->getPosition();
		auto to = getPointForScrollPosition(num * segment);

		return Accelerated::createBounce(fabsf(acceleration), from, to, Vec2(-velocity, 0.0f), 25000);
	}

	if (!a) {
		a = _movementAction;
	}

	return a;
}

void View::setScrollRelativePosition(float value) {
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
	float size = getScrollSize();

	auto &padding = getPadding();
	auto paddingFront = (isVertical())?padding.top:padding.left;
	auto paddingBack = (isVertical())?padding.bottom:padding.right;

	if (!isnan(areaSize) && !isnan(areaOffset) && areaSize > 0) {
		float liveSize = areaSize + paddingFront + paddingBack - size;
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

bool View::showNextPage() {
	if (isHorizontal() && !isMoved() && !_animationAction) {
		_movement = Movement::Auto;
		_animationAction = action::callback(
				Accelerated::createBounce(7500, _root->getPosition(), _root->getPosition() - Vec2(_contentSize.width, 0), Vec2(0, 0)),
				[this] {
			onAnimationFinished();
		});
		_root->runAction(_animationAction);
		return true;
	}
	return false;
}

bool View::showPrevPage() {
	if (isHorizontal() && !isMoved() && !_animationAction) {
		_movement = Movement::Auto;
		_animationAction = action::callback(
				Accelerated::createBounce(7500, _root->getPosition(), _root->getPosition() + Vec2(_contentSize.width, 0), Vec2(0, 0)),
				[this] {
			onAnimationFinished();
		});
		_root->runAction(_animationAction);
		return true;
	}
	return false;
}

NS_SP_EXT_END(rich_text)
