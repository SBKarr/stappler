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
#include "MaterialLabel.h"
#include "MaterialScroll.h"
#include "MaterialScrollSlice.h"
#include "MaterialScrollFixed.h"
#include "MaterialResourceManager.h"
#include "MaterialButton.h"
#include "MaterialTabBar.h"
#include "MaterialRecyclerScroll.h"

#include "RTEpubContentsView.h"

#include "SLDocument.h"
#include "SLResult.h"
#include "SPActions.h"
#include "SPGestureListener.h"

NS_RT_BEGIN

Size EpubContentsNode::compute(const data::Value &data, float width) {
	auto page = data.getInteger("page");
	auto font = material::FontType::Body_1;
	auto str = data.getString("label");
	if (page != maxOf<int64_t>()) {
		str += toString(" — %Shortcut:Pages%. ", page + 1);
	}
	auto labelSize = material::Label::getLabelSize(font, str, width - 32.0f, 0.0f, true);
	return Size(width, labelSize.height + 32.0f);
}

bool EpubContentsNode::init(const data::Value &data, Callback & cb) {
	if (!MaterialNode::init()) {
		return false;
	}

	auto page = data.getInteger("page");

	_callback = cb;
	_href = data.getString("href");

	auto label = Rc<material::Label>::create(material::FontType::Body_1);
	label->setLocaleEnabled(true);
	label->setString(data.getString("label"));
	label->setAnchorPoint(Vec2(0.0f, 1.0f));
	if (page != maxOf<int64_t>()) {
		label->appendTextWithStyle(toString(" — %Shortcut:Pages%. ", page + 1), material::Label::Style(material::Label::Opacity(100)));
	}
	label->tryUpdateLabel();
	_label = addChildNode(label);

	auto button = Rc<material::Button>::create([this] {
		if (_available && _callback) {
			_callback(_href);
		}
	});
	button->setStyle(material::Button::Style::FlatBlack);
	button->setAnchorPoint(cocos2d::Vec2(0.0f, 0.0f));
	button->setSwallowTouches(false);
	button->setOpacity(255);
	button->setEnabled(!_href.empty());
	_button = addChildNode(button, 2);

	setPadding(Padding(1.0f));
	setShadowZIndex(0.5f);

	return true;
}

void EpubContentsNode::onContentSizeDirty() {
	MaterialNode::onContentSizeDirty();

	auto size = getContentSize();
	_label->setPosition(Vec2(16.0f, size.height - 16.0f));
	_label->setWidth(size.width - 32.0f);
	_label->tryUpdateLabel();

	_button->setPosition(getAnchorPositionWithPadding());
	_button->setContentSize(getContentSizeWithPadding());
}

bool EpubContentsScroll::init(const Callback &cb) {
	if (!Scroll::init()) {
		return false;
	}

	getGestureListener()->setSwallowTouches(true);
	_callback = cb;

	setHandlerCallback([] (Scroll *s) -> Rc<Handler> {
		class Handler : public material::ScrollHandlerSlice {
		public:
			virtual bool init(Scroll *s) override {
				if (!ScrollHandlerSlice::init(s, nullptr)) {
					return false;
				}
				return true;
			}
		protected:
			virtual Rc<Scroll::Item> onItem(data::Value &&data, const Vec2 &origin) override {
				auto width = _size.width;
				Size size = EpubContentsNode::compute(data, width);
				return Rc<Scroll::Item>::create(std::move(data), origin, size);
			}
		};

		return Rc<Handler>::create(s);
	});

	setItemCallback( [this] (Item *item) -> Rc<material::MaterialNode> {
		return Rc<EpubContentsNode>::create(item->getData(), _callback);
	});

	setLookupLevel(10);
	setItemsForSubcats(true);
	setMaxSize(64);

	return true;
}

void EpubContentsScroll::onResult(layout::Result *res) {
	_result = res;
	if (!res) {
		setSource(nullptr);
	} else {
		setSource(Rc<data::Source>::create(data::Source::ChildsCount(res->getBounds().size()),
				[this] (const data::Source::DataCallback &cb, data::Source::Id id) {
			if (_result) {
				auto &bounds = _result->getBounds();
				if (id.get() < bounds.size()) {
					auto &d = bounds.at(size_t(id.get()));
					cb(data::Value{
						pair("page", data::Value(d.page)),
						pair("label", data::Value(d.label)),
						pair("href", data::Value(d.href)),
					});
					return;
				}
			}
			cb(data::Value());
		}));
	}
}

void EpubContentsScroll::setSelectedPosition(float value) {
	if (_result) {
		auto bounds = _result->getBoundsForPosition(value);
		setOriginId(data::Source::Id(bounds.idx));
		resetSlice();
	}
}

Size EpubBookmarkNode::compute(const data::Value &data, float width) {
	auto name = data.getString("item");
	auto label = data.getString("label");

	auto l = material::Label::getLabelSize(material::FontType::Body_2, name, width - 16.0f);
	auto n = material::Label::getLabelSize(material::FontType::Caption, label, width - 16.0f);

	return Size(width, 20.0f + l.height + n.height);
}

bool EpubBookmarkNode::init(const data::Value &data, const Size &size, const Callback &cb) {
	if (!MaterialNode::init()) {
		return false;
	}

	_callback = cb;
	_object = size_t(data.getInteger("object"));
	_position = size_t(data.getInteger("position"));
	_scroll = data.getDouble("scroll");

	auto name = Rc<material::Label>::create(material::FontType::Body_2);
	name->setString(data.getString("item"));
	name->setWidth(size.width - 16.0f);
	name->setAnchorPoint(Vec2(0.0f, 1.0f));
	name->setPosition(8.0f, size.height - 8.0f);
	_name = addChildNode(name, 1);

	auto label = Rc<material::Label>::create(material::FontType::Caption);
	label->setString(data.getString("label"));
	label->setWidth(size.width - 16.0f);
	label->setAnchorPoint(Vec2(0.0f, 0.0f));
	label->setPosition(8.0f, 8.0f);
	_label = addChildNode(label, 1);

	auto button = Rc<material::Button>::create(std::bind(&EpubBookmarkNode::onButton, this));
	button->setStyle(material::Button::Style::FlatBlack);
	button->setSwallowTouches(false);
	_button = addChildNode(button, 2);

	setPadding(1.0f);
	setShadowZIndex(1.0f);

	return true;
}

void EpubBookmarkNode::onContentSizeDirty() {
	MaterialNode::onContentSizeDirty();

	_button->setPosition(getAnchorPositionWithPadding());
	_button->setContentSize(getContentSizeWithPadding());
}

void EpubBookmarkNode::onButton() {
	if (_callback) {
		_callback(_object, _position, _scroll);
	}
}

bool EpubBookmarkScroll::init(const Callback &cb) {
	if (!RecyclerScroll::init()) {
		return false;
	}

	_callback = cb;
	getGestureListener()->setSwallowTouches(true);

	setHandlerCallback([] (Scroll *s) -> Handler * {
		return Rc<material::ScrollHandlerSlice>::create(s, [] (material::ScrollHandlerSlice *h, data::Value &&data, const Vec2 &origin) -> Rc<Scroll::Item> {
			auto width = h->getContentSize().width;
			Size size = EpubBookmarkNode::compute(data, width);
			return Rc<Scroll::Item>::create(std::move(data), origin, size);
		});
	});

	setItemCallback( [this] (Item *item) -> material::MaterialNode * {
		return Rc<EpubBookmarkNode>::create(item->getData(), item->getContentSize(), _callback);
	});

	return true;
}

bool EpubContentsView::init(const Callback &cb, const BookmarkCallback &bcb) {
	if (!MaterialNode::init()) {
		return false;
	}

	auto header = Rc<MaterialNode>::create();
	header->setShadowZIndex(1.5f);
	header->setBackgroundColor(material::Color::Grey_200);
	header->setAnchorPoint(Vec2(0.0f, 1.0f));

	auto title = Rc<material::Label>::create(material::FontType::Subhead);
	title->setString("Содержание");
	title->setAnchorPoint(Anchor::MiddleLeft);
	title->setVisible(false);
	header->addChild(title);
	_title = title;

	auto tabBar = constructTabBar();
	tabBar->setAnchorPoint(Vec2(0.0f, 0.0f));
	tabBar->setPosition(Vec2(0.0f, 0.0f));
	header->addChild(tabBar);
	_tabBar = tabBar;

	_header = addChildNode(header, 2);

	auto contentNode = Rc<StrictNode>::create();
	contentNode->setAnchorPoint(Vec2(0.0f, 0.0f));

	if (auto contentsScroll = constructContentsScroll(cb)) {
		contentsScroll->setAnchorPoint(Vec2(0.0f, 0.0f));
		contentNode->addChild(contentsScroll, 1);
		_contentsScroll = contentsScroll;
	}

	if (auto bookmarksScroll = constructBookmarkScroll(bcb)) {
		bookmarksScroll->setAnchorPoint(Vec2(0.0f, 0.0f));
		contentNode->addChild(bookmarksScroll, 2);
		_bookmarksScroll = bookmarksScroll;
	}

	_contentNode = addChildNode(contentNode, 1);

	setShadowZIndex(1.5f);

	return true;
}

void EpubContentsView::onContentSizeDirty() {
	MaterialNode::onContentSizeDirty();
	auto size = getContentSizeWithPadding();

	float height = 48.0f;
	if (_tabBar->getButtonStyle() == material::TabBar::ButtonStyle::TitleIcon) {
		height = 64.0f;
	}

	if (_header) {
		_header->setContentSize(Size(size.width, height));
		_header->setPosition(getAnchorPositionWithPadding() + Vec2(0.0f, size.height));

		_tabBar->setContentSize(_header->getContentSize());
		_title->setPosition(16.0f, 24.0f);
	}

	_contentNode->setContentSize(Size(size.width, size.height - height));
	_contentNode->setPosition(getAnchorPositionWithPadding());

	if (_contentsScroll) {
		_contentsScroll->setVisible(fabs(_progress - 1.0f) > std::numeric_limits<float>::epsilon());
		_contentsScroll->setContentSize(_contentNode->getContentSize());
		_contentsScroll->setPosition(Vec2(- size.width * _progress, 0.0f));
	}

	if (_bookmarksScroll) {
		_bookmarksScroll->setVisible(fabs(_progress) > std::numeric_limits<float>::epsilon());
		_bookmarksScroll->setContentSize(_contentNode->getContentSize());
		_bookmarksScroll->setPosition(Vec2(size.width * (1.0f - _progress), 0.0f));
	}
}

void EpubContentsView::onResult(layout::Result *res) {
	if (_contentsScroll) {
		_contentsScroll->onResult(res);
	}
}

void EpubContentsView::setSelectedPosition(float value) {
	if (_contentsScroll) {
		_contentsScroll->setSelectedPosition(value);
	}
}

void EpubContentsView::setColors(const material::Color &bar, const material::Color &accent) {
	_header->setBackgroundColor(bar);
	_tabBar->setAccentColor(accent);
	_tabBar->setTextColor(bar.text());
	_tabBar->setSelectedColor(bar.text());
	_title->setColor(bar.text());
}

void EpubContentsView::setBookmarksSource(data::Source *source) {
	if (_bookmarksScroll) {
		_bookmarksScroll->setSource(source);
	}
}

void EpubContentsView::showBookmarks() {
	auto a = cocos2d::EaseQuadraticActionInOut::create(Rc<ProgressAction>::create(0.25f, _progress, 1.0f, [this] (ProgressAction *a, float p) {
		_progress = p;
		_contentSizeDirty = true;
	}));
	runAction(a, "SwitchAction"_tag);
}

void EpubContentsView::showContents() {
	auto a = cocos2d::EaseQuadraticActionInOut::create(Rc<ProgressAction>::create(0.25f, _progress, 0.0f, [this] (ProgressAction *a, float p) {
		_progress = p;
		_contentSizeDirty = true;
	}));
	runAction(a, "SwitchAction"_tag);
}

void EpubContentsView::setBookmarksEnabled(bool value) {
	if (_bookmarksEnabled != value) {
		_bookmarksEnabled = value;
		_tabBar->setVisible(_bookmarksEnabled);
		_title->setVisible(!_bookmarksEnabled);
	}
}

bool EpubContentsView::isBookmarksEnabled() const {
	return _bookmarksEnabled;
}

Rc<material::TabBar> EpubContentsView::constructTabBar() {
	auto source = Rc<material::MenuSource>::create();
	source->addButton("Содержание", [this] (material::Button *b, material::MenuSourceButton *) {
		showContents();
	})->setSelected(true);
	source->addButton("Закладки", [this] (material::Button *b, material::MenuSourceButton *) {
		showBookmarks();
	});

	Rc<material::TabBar> tabBar = Rc<material::TabBar>::create(source);
	tabBar->setAlignment(material::TabBar::Alignment::Justify);
	return tabBar;
}

Rc<EpubContentsScroll> EpubContentsView::constructContentsScroll(const Callback &cb) {
	return Rc<EpubContentsScroll>::create(cb);
}

Rc<EpubBookmarkScroll> EpubContentsView::constructBookmarkScroll(const BookmarkCallback &cb) {
	return Rc<EpubBookmarkScroll>::create(cb);
}

NS_RT_END
