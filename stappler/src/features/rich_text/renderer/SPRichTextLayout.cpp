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
#include "SPRichTextLayout.h"

NS_SP_EXT_BEGIN(rich_text)

Object::Value::Value() {
	memset((void*)this, 0, sizeof(Value));
}

Object::Value::Value(Ref &&v) : ref(std::move(v)) { }
Object::Value::Value(Outline &&v) : outline(std::move(v)) { }
Object::Value::Value(const Background &v) : background(v) { }
Object::Value::Value(Background &&v) : background(std::move(v)) { }
Object::Value::Value(Label &&v) : label(std::move(v)) { }

Object::Value::~Value() { }

Object::~Object() {
	switch(type) {
	case Type::Empty: break;
	case Type::Ref: value.ref.~Ref(); break;
	case Type::Outline: value.outline.~Outline(); break;
	case Type::Background: value.background.~Background(); break;
	case Type::Label: value.label.~Label(); break;
	}
}

Object::Object(const cocos2d::Rect &bbox) : bbox(bbox), type(Type::Empty) { }
Object::Object(const cocos2d::Rect &bbox, Ref &&v) : bbox(bbox), type(Type::Ref), value(std::move(v)) { }
Object::Object(const cocos2d::Rect &bbox, Outline &&v) : bbox(bbox), type(Type::Outline), value(std::move(v)) { }
Object::Object(const cocos2d::Rect &bbox, const Background &v) : bbox(bbox), type(Type::Background), value(v) { }
Object::Object(const cocos2d::Rect &bbox, Background &&v) : bbox(bbox), type(Type::Background), value(std::move(v)) { }
Object::Object(const cocos2d::Rect &bbox, Label &&v) : bbox(bbox), type(Type::Label), value(std::move(v)) { }

Object::Object(Object &&other) : bbox(other.bbox), type(other.type) {
	switch(other.type) {
	case Type::Empty: break;
	case Type::Ref: new (&value.ref) Ref(std::move(other.value.ref)); other.value.ref.~Ref(); break;
	case Type::Outline: new (&value.outline) Outline(std::move(other.value.outline)); other.value.outline.~Outline(); break;
	case Type::Background: new (&value.background) Background(std::move(other.value.background)); other.value.background.~Background(); break;
	case Type::Label: new (&value.label) Label(std::move(other.value.label)); other.value.label.~Label(); break;
	}
	other.type = Type::Empty;
}

Object & Object::operator = (Object &&other) {
	bbox = other.bbox;
	switch(type) {
	case Type::Empty: break;
	case Type::Ref: value.ref.~Ref(); break;
	case Type::Outline: value.outline.~Outline(); break;
	case Type::Background: value.background.~Background(); break;
	case Type::Label: value.label.~Label(); break;
	}
	switch(other.type) {
	case Type::Empty: break;
	case Type::Ref: new (&value.ref) Ref(std::move(other.value.ref)); other.value.ref.~Ref(); break;
	case Type::Outline: new (&value.outline) Outline(std::move(other.value.outline)); other.value.outline.~Outline(); break;
	case Type::Background: new (&value.background) Background(std::move(other.value.background)); other.value.background.~Background(); break;
	case Type::Label: new (&value.label) Label(std::move(other.value.label)); other.value.label.~Label(); break;
	}
	type = other.type;
	other.type = Type::Empty;
	return *this;
}

bool Object::isTransparent() const {
	return type == Type::Ref;
}
bool Object::isDrawable() const {
	return type == Type::Outline || type == Type::Label ||
			(type == Type::Background && value.background.backgroundImage.empty());
}
bool Object::isExternal() const {
	return (type == Type::Background && !value.background.backgroundImage.empty());
}

bool Object::isEmpty() const {
	return (type == Type::Empty);
}

Outline Outline::border(const OutlineStyle &style, float swidth, float emBase, float &borderWidth) {
	Border border;
	if (style.borderTopStyle != style::BorderStyle::None) {
		float width = style.borderTopWidth.computeValue(swidth, emBase, true);
		borderWidth = std::max(borderWidth, width);
		border.top = Border::Params{style.borderTopStyle, width, style.borderTopColor};
	}
	if (style.borderRightStyle != style::BorderStyle::None) {
		float width = style.borderRightWidth.computeValue(swidth, emBase, true);
		borderWidth = std::max(borderWidth, width);
		border.right = Border::Params{style.borderRightStyle, width, style.borderRightColor};
	}
	if (style.borderBottomStyle != style::BorderStyle::None) {
		float width = style.borderBottomWidth.computeValue(swidth, emBase, true);
		borderWidth = std::max(borderWidth, width);
		border.bottom = Border::Params{style.borderBottomStyle, width, style.borderBottomColor};
	}
	if (style.borderLeftStyle != style::BorderStyle::None) {
		float width = style.borderLeftWidth.computeValue(swidth, emBase, true);
		borderWidth = std::max(borderWidth, width);
		border.left = Border::Params{style.borderLeftStyle, width, style.borderLeftColor};
	}
	return border;
}

static Rect getLabelLineStartRect(const Label &l, uint16_t lineId, float density, uint16_t c) {
	Rect rect;
	const font::LineSpec &line = l.format.lines.at(lineId);
	if (line.count > 0) {
		const font::CharSpec & firstChar = l.format.chars.at(MAX(line.start, c));
		const font::CharSpec & lastChar = l.format.chars.at(line.start + line.count - 1);
		rect.origin = Vec2((firstChar.pos) / density, (line.pos) / density - line.height / density);
		rect.size = Size((lastChar.pos + lastChar.advance - firstChar.pos) / density, line.height / density);
	}

	return rect;
}

static Rect getLabelLineEndRect(const Label &l, uint16_t lineId, float density, uint16_t c) {
	Rect rect;
	const font::LineSpec &line = l.format.lines.at(lineId);
	if (line.count > 0) {
		const font::CharSpec & firstChar = l.format.chars.at(line.start);
		const font::CharSpec & lastChar = l.format.chars.at(MIN(line.start + line.count - 1, c));
		rect.origin = Vec2((firstChar.pos) / density, (line.pos) / density - line.height / density);
		rect.size = Size((lastChar.pos + lastChar.advance - firstChar.pos) / density, line.height / density);
	}
	return rect;
}

static cocos2d::Rect getCharsRect(const Label &l, uint16_t lineId, uint16_t firstCharId, uint16_t lastCharId, float density) {
	Rect rect;
	const font::LineSpec & line = l.format.lines.at(lineId);
	const font::CharSpec & firstChar = l.format.chars.at(firstCharId);
	const font::CharSpec & lastChar = l.format.chars.at(lastCharId);
	rect.origin = Vec2((firstChar.pos) / density, (line.pos) / density - line.height / density);
	rect.size = Size((lastChar.pos + lastChar.advance - firstChar.pos) / density, line.height / density);
	return rect;
}

Rect Label::getLineRect(uint16_t lineId, float density, const Vec2 &origin) const {
	if (lineId >= format.lines.size()) {
		return Rect::ZERO;
	}
	return getLineRect(format.lines[lineId], density, origin);
}

Rect Label::getLineRect(const font::LineSpec &line, float density, const Vec2 &origin) const {
	Rect rect;
	if (line.count > 0) {
		const font::CharSpec & firstChar = format.chars.at(line.start);
		const font::CharSpec & lastChar = format.chars.at(line.start + line.count - 1);
		rect.origin = Vec2((firstChar.pos) / density + origin.x, (line.pos) / density - line.height / density + origin.y);
		rect.size = Size((lastChar.pos + lastChar.advance - firstChar.pos) / density, line.height / density);
	}
	return rect;
}

uint16_t Label::getLineForCharId(uint16_t id) const {
	uint16_t n = 0;
	for (auto &it : format.lines) {
		if (id >= it.start && id < it.start + it.count) {
			return n;
		}
		n++;
	}
	if (n >= format.lines.size()) {
		n = format.lines.size() - 1;
	}
	return n;
}

Vector<Rect> Label::getLabelRects(uint16_t firstCharId, uint16_t lastCharId, float density, const Vec2 &origin, const Padding &p) const {
	auto &l = *this;
	Vector<Rect> ret;

	auto firstLine = getLineForCharId(firstCharId);
	auto lastLine = getLineForCharId(lastCharId);

	if (firstLine == lastLine) {
		auto rect = getCharsRect(l, firstLine, firstCharId, lastCharId, density);
		rect.origin.x += origin.x - p.left;
		rect.origin.y += origin.y - p.top;
		rect.size.width += p.left + p.right;
		rect.size.height += p.bottom + p.top;
		if (!rect.equals(cocos2d::Rect::ZERO)) {
			ret.push_back(rect);
		}
	} else {
		auto first = getLabelLineStartRect(l, firstLine, density, firstCharId);
		if (!first.equals(cocos2d::Rect::ZERO)) {
			first.origin.x += origin.x;
			first.origin.y += origin.y;
			if (first.origin.x - p.left < 0.0f) {
				first.size.width += (first.origin.x);
				first.origin.x = 0.0f;
			} else {
				first.origin.x -= p.left;
				first.size.width += p.left;
			}
			first.origin.y -= p.top;
			first.size.height += p.bottom + p.top;
			ret.push_back(first);
		}

		for (auto i = firstLine + 1; i < lastLine; i++) {
			auto rect = getLineRect(i, density);
			rect.origin.x += origin.x;
			rect.origin.y += origin.y - p.top;
			rect.size.height += p.bottom + p.top;
			if (!rect.equals(cocos2d::Rect::ZERO)) {
				ret.push_back(rect);
			}
		}

		auto last = getLabelLineEndRect(l, lastLine, density, lastCharId);
		if (!last.equals(cocos2d::Rect::ZERO)) {
			last.origin.x += origin.x;
			last.origin.y += origin.y - p.top;
			last.size.width += p.right;
			last.size.height += p.bottom + p.top;
			ret.push_back(last);
		}
	}
	return ret;
}

Layout::Layout() { }
Layout::Layout(const RendererInterface *r, const Node *n, bool d)
: node(n), style(n->getStyle().compileBlockModel(r)), disablePageBreak(d) { }

Layout::Layout(const Node *n, BlockStyle &&s, bool d)
: node(n), style(std::move(s)), disablePageBreak(d) { }

Layout::Layout(Layout &&l)
: node(l.node)
, style(std::move(l.style))
, padding(l.padding)
, margin(l.margin)
, layoutContext(l.layoutContext)
, position(l.position)
, origin(l.origin)
, size(l.size)
, minHeight(l.minHeight)
, maxHeight(l.maxHeight)
, preObjects(std::move(l.preObjects))
, layouts(std::move(l.layouts))
, postObjects(std::move(l.postObjects))
, collapsableMarginTop(l.collapsableMarginTop)
, collapsableMarginBottom(l.collapsableMarginBottom)
, listItemIndex(l.listItemIndex)
, listItem(l.listItem)
, context(std::move(l.context))
, inlineBlockLayouts(std::move(l.inlineBlockLayouts))
, charBinding(l.charBinding)
, disablePageBreak(l.disablePageBreak) { }

Layout & Layout::operator = (Layout &&l) {
	node = (l.node);
	style = (std::move(l.style));
	padding = (l.padding);
	margin = (l.margin);
	layoutContext = (l.layoutContext);
	position = (l.position);
	origin = (l.origin);
	size = (l.size);
	minHeight = (l.minHeight);
	maxHeight = (l.maxHeight);
	preObjects = (std::move(l.preObjects));
	layouts = (std::move(l.layouts));
	postObjects = (std::move(l.postObjects));
	collapsableMarginTop = (l.collapsableMarginTop);
	listItemIndex = (l.listItemIndex);
	listItem = (l.listItem);
	context = std::move(l.context);
	inlineBlockLayouts = std::move(l.inlineBlockLayouts);
	charBinding = (l.charBinding);
	disablePageBreak = (l.disablePageBreak);
	return *this;
}

cocos2d::Rect Layout::getBoundingBox() const {
	return cocos2d::Rect(position.x - padding.left - margin.left,
			position.y - padding.top - margin.top,
			size.width + padding.left + padding.right + margin.left + margin.right,
			(!isnanf(size.height)?( size.height + padding.top + padding.bottom + margin.top + margin.bottom):nan()));
}
cocos2d::Rect Layout::getPaddingBox() const {
	return cocos2d::Rect(position.x - padding.left, position.y - padding.top,
			size.width + padding.left + padding.right,
			(!isnanf(size.height)?( size.height + padding.top + padding.bottom):nan()));
}
cocos2d::Rect Layout::getContentBox() const {
	return cocos2d::Rect(position.x, position.y, size.width, size.height);
}

void Layout::updatePosition(float pos) {
	position.y += pos;
	for (auto &l : layouts) {
		l.updatePosition(pos);
	}
}

void Layout::setBoundPosition(const Vec2 &pos) {
	auto newPos = pos;
	newPos.x += (margin.left + padding.left);
	newPos.y += (margin.top + padding.top);
	auto diff = pos - getBoundingBox().origin;
	position = newPos;

	for (auto &l : layouts) {
		auto nodePos = l.getBoundingBox().origin + diff;
		l.setBoundPosition(nodePos);
	}
}

Pair<float, float> FloatContext::getAvailablePosition(float yPos, float height) const {
	float x = root->position.x;
	float width = root->size.width;

	float topStackWidth = 0;
	for (auto &stack : floatLeft) {
		if (stack.origin.y <= yPos + height && stack.origin.y + stack.size.height >= yPos) {
			if (topStackWidth < stack.origin.x + stack.size.width) {
				x = stack.origin.x + stack.size.width;
				width = root->size.width - x;
				topStackWidth = stack.origin.x + stack.size.width;
			}
		}
	}

	topStackWidth = 0;
	for (auto &stack : floatRight) {
		if (stack.origin.y <= yPos + height && stack.origin.y + stack.size.height >= yPos) {
			if (topStackWidth < stack.size.width) {
				topStackWidth = root->size.width - stack.origin.x;
			}
		}
	}
	width -= topStackWidth;
	return pair(x, width);
}

bool FloatContext::pushFloatingNode(Layout &origin, Layout &l, Vec2 &vec) {
	auto &stacks = (l.style.floating==style::Float::Left)?floatLeft:floatRight;
	auto bbox = l.getBoundingBox();

	if (l.style.clear == style::Clear::Both
			|| (l.style.floating == style::Float::Left && l.style.clear == style::Clear::Left)
			|| (l.style.floating == style::Float::Right && l.style.clear == style::Clear::Right) ) {
		return pushFloatingNodeToNewStack(origin, l, stacks, bbox, vec);
	}

	// can we use current stack?
	if (!stacks.empty()) {
		auto &stack = stacks.back();
		if (root->size.width - stack.size.width > bbox.size.width) {
			if (pushFloatingNodeToStack(origin, l, stack, bbox, vec)) {
				return true;
			}
		}
	}
	return pushFloatingNodeToNewStack(origin, l, stacks, bbox, vec);
}

bool FloatContext::pushFloatingNodeToStack(Layout &origin, Layout &l, Rect &s, const Rect &bbox, Vec2 &vec) {
	auto &stacks = (l.style.floating==style::Float::Left)?floatRight:floatLeft;

	float newHeight = MAX(s.size.height, bbox.size.height);
	float oppositeWidth = 0;

	for (auto &stack : stacks) {
		if (stack.origin.y <= s.origin.y + newHeight && stack.origin.y + stack.size.height >= s.origin.y) {
			if (oppositeWidth < stack.size.width) {
				oppositeWidth = stack.size.width;
			}
		}
	}

	if (root->size.width - s.size.width - oppositeWidth > bbox.size.width) {
		s.size.height = newHeight;
		l.setBoundPosition(cocos2d::Vec2(
				(l.style.floating==style::Float::Left)?(s.origin.x + s.size.width):(s.origin.x - s.size.width)
						, s.origin.y));
		s.size.width += bbox.size.width;

		if (fabsf(root->size.width - s.size.width) < root->size.width * 0.025) {
			vec.y = s.origin.y + s.size.height;
		}

		return true;
	}
	return false;
}

bool FloatContext::pushFloatingNodeToNewStack(Layout &lo, Layout &l, FloatStack &stacks, const Rect &bbox, Vec2 &vec) {
	auto &oppositeStacks = (l.style.floating==style::Float::Left)?floatRight:floatLeft;

	Vec2 origin = Vec2(
			(l.style.floating==style::Float::Left)?(lo.position.x):(lo.position.x + lo.size.width - bbox.size.width),
			bbox.origin.y);

	Size bsize = bbox.size;

	for (auto &stack : stacks) {
		if (origin.y < stack.origin.y + stack.size.height) {
			origin.y = stack.origin.y + stack.size.height;
		}
	}

	bool oppositeClear = false;
	if (l.style.clear == style::Clear::Both
			|| (l.style.floating == style::Float::Left && l.style.clear == style::Clear::Right)
			|| (l.style.floating == style::Float::Right && l.style.clear == style::Clear::Left) ) {
		oppositeClear = true;
	}

	for (auto &stack : oppositeStacks) {
		if (stack.origin.y <= origin.y && stack.origin.y + stack.size.height >= origin.y) {
			if (oppositeClear || root->size.width - stack.size.width < bsize.width) {
				origin.y = stack.origin.y + stack.size.height;
			}
		}
	}

	if (!isnan(pageHeight)) {
		uint32_t curr1 = (uint32_t)std::floor(origin.y / pageHeight);
		uint32_t curr2 = (uint32_t)std::floor((origin.y + bsize.height) / pageHeight);
		if (curr1 != curr2) {
			origin.y = curr2 * pageHeight;
		}
	}

	stacks.push_back(Rect(origin.x, origin.y, bsize.width, bsize.height));
	l.setBoundPosition(origin);

	if (fabsf(root->size.width - bsize.width)  < root->size.width * 0.025) {
		vec.y = origin.y + bsize.height;
	}

	return true;
}

InlineContext::InlineContext(font::Source *set, float density)
: reader(set, &(label.format), density) { }

void InlineContext::pushNode(const Node *node, const NodeCallback &cb) {
	if (!finalized) {
		nodes.push_back(pair(node, cb));
	}
}
void InlineContext::popNode(const Node *node) {
	if (!finalized && !nodes.empty()) {
		if (nodes.back().first == node) {
			if (nodes.back().second) {
				nodes.back().second(*this);
			}
			nodes.pop_back();
		} else {
			for (auto it = nodes.begin(); it != nodes.end(); it ++) {
				if (it->first == node) {
					if (it->second) {
						it->second(*this);
					}
					nodes.erase(it);
					break;
				}
			}
		}
	}
}
void InlineContext::finalize() {
	if (!nodes.empty()) {
		for (auto it = nodes.rbegin(); it != nodes.rend(); it ++) {
			if (it->second) {
				it->second(*this);
			}
		}
		nodes.clear();
	}
	refPos.clear();
	outlinePos.clear();
	borderPos.clear();
	backgroundPos.clear();
	finalized = true;
}
void InlineContext::reset() {
	finalized = false;
	reader.reset(&label.format);
}

NS_SP_EXT_END(rich_text)
