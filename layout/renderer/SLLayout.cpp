// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPLayout.h"
#include "SLLayout.h"

NS_LAYOUT_BEGIN

Object::Value::Value() {
	memset((void*)this, 0, sizeof(Value));
}

Object::Value::Value(Link &&v) : ref(std::move(v)) { }
Object::Value::Value(Outline &&v) : outline(std::move(v)) { }
Object::Value::Value(const BackgroundStyle &v) : background(v) { }
Object::Value::Value(BackgroundStyle &&v) : background(std::move(v)) { }
Object::Value::Value(Label &&v) : label(std::move(v)) { }

Object::Value::~Value() { }

Object::~Object() {
	switch(type) {
	case Type::Empty: break;
	case Type::Ref: value.ref.~Link(); break;
	case Type::Outline: value.outline.~Outline(); break;
	case Type::Background: value.background.~BackgroundStyle(); break;
	case Type::Label: value.label.~Label(); break;
	}
}

Object::Object(const Rect &bbox) : bbox(bbox), type(Type::Empty) { }
Object::Object(const Rect &bbox, Link &&v) : bbox(bbox), type(Type::Ref), value(std::move(v)) { }
Object::Object(const Rect &bbox, Outline &&v) : bbox(bbox), type(Type::Outline), value(std::move(v)) { }
Object::Object(const Rect &bbox, const BackgroundStyle &v) : bbox(bbox), type(Type::Background), value(v) { }
Object::Object(const Rect &bbox, BackgroundStyle &&v) : bbox(bbox), type(Type::Background), value(std::move(v)) { }
Object::Object(const Rect &bbox, Label &&v) : bbox(bbox), type(Type::Label), value(std::move(v)) { }

Object::Object(Object &&other) : bbox(other.bbox), type(other.type), index(other.index) {
	switch(other.type) {
	case Type::Empty: break;
	case Type::Ref: new (&value.ref) Link(std::move(other.value.ref)); other.value.ref.~Link(); break;
	case Type::Outline: new (&value.outline) Outline(std::move(other.value.outline)); other.value.outline.~Outline(); break;
	case Type::Background: new (&value.background) BackgroundStyle(std::move(other.value.background)); other.value.background.~BackgroundStyle(); break;
	case Type::Label: new (&value.label) Label(std::move(other.value.label)); other.value.label.~Label(); break;
	}
	other.type = Type::Empty;
}

Object & Object::operator = (Object &&other) {
	bbox = other.bbox;
	switch(type) {
	case Type::Empty: break;
	case Type::Ref: value.ref.~Link(); break;
	case Type::Outline: value.outline.~Outline(); break;
	case Type::Background: value.background.~BackgroundStyle(); break;
	case Type::Label: value.label.~Label(); break;
	}
	switch(other.type) {
	case Type::Empty: break;
	case Type::Ref: new (&value.ref) Link(std::move(other.value.ref)); other.value.ref.~Link(); break;
	case Type::Outline: new (&value.outline) Outline(std::move(other.value.outline)); other.value.outline.~Outline(); break;
	case Type::Background: new (&value.background) BackgroundStyle(std::move(other.value.background)); other.value.background.~BackgroundStyle(); break;
	case Type::Label: new (&value.label) Label(std::move(other.value.label)); other.value.label.~Label(); break;
	}
	type = other.type;
	index = other.index;
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

Outline Outline::border(const OutlineStyle &style, const Size &vp, float swidth, float emBase, float rootEmBase, float &borderWidth) {
	Border border;
	if (style.borderTopStyle != style::BorderStyle::None) {
		float width = style.borderTopWidth.computeValueAuto(swidth, vp, emBase, rootEmBase);
		borderWidth = std::max(borderWidth, width);
		border.top = Border::Params{style.borderTopStyle, width, style.borderTopColor};
	}
	if (style.borderRightStyle != style::BorderStyle::None) {
		float width = style.borderRightWidth.computeValueAuto(swidth, vp, emBase, rootEmBase);
		borderWidth = std::max(borderWidth, width);
		border.right = Border::Params{style.borderRightStyle, width, style.borderRightColor};
	}
	if (style.borderBottomStyle != style::BorderStyle::None) {
		float width = style.borderBottomWidth.computeValueAuto(swidth, vp, emBase, rootEmBase);
		borderWidth = std::max(borderWidth, width);
		border.bottom = Border::Params{style.borderBottomStyle, width, style.borderBottomColor};
	}
	if (style.borderLeftStyle != style::BorderStyle::None) {
		float width = style.borderLeftWidth.computeValueAuto(swidth, vp, emBase, rootEmBase);
		borderWidth = std::max(borderWidth, width);
		border.left = Border::Params{style.borderLeftStyle, width, style.borderLeftColor};
	}
	return border;
}

Rect Label::getLineRect(uint32_t lineId, float density, const Vec2 &origin) const {
	return format.getLineRect(lineId, density, origin);
}

Rect Label::getLineRect(const LineSpec &line, float density, const Vec2 &origin) const {
	return format.getLineRect(line, density, origin);
}

uint32_t Label::getLineForCharId(uint32_t id) const {
	return format.getLineNumber(id);
}

Vector<Rect> Label::getLabelRects(uint32_t firstCharId, uint32_t lastCharId, float density, const Vec2 &origin, const Padding &p) const {
	return format.getLabelRects(firstCharId, lastCharId, density, origin, p);
}

void Label::getLabelRects(Vector<Rect> &rect, uint32_t firstCharId, uint32_t lastCharId, float density, const Vec2 &origin, const Padding &p) const {
	return format.getLabelRects(rect, firstCharId, lastCharId, density, origin, p);
}

float Label::getLinePosition(uint32_t firstCharId, uint32_t lastCharId, float density) const {
	auto firstLine = format.getLine(firstCharId);
	auto lastLine = format.getLine(lastCharId);

	return ((firstLine->pos) / density + (lastLine->pos) / density) / 2.0f;
}


Layout::NodeInfo::NodeInfo(const Node *n, const Style *s, const RendererInterface *r)
: node(n), style(s), block(style->compileBlockModel(r)), context(block.display) { }

Layout::NodeInfo::NodeInfo(const Node *n, const Style *s, BlockStyle &&b)
: node(n), style(s), block(move(b)), context(block.display) { }


Rect Layout::PositionInfo::getBoundingBox() const {
	return Rect(position.x - padding.left - margin.left,
			position.y - padding.top - margin.top,
			size.width + padding.left + padding.right + margin.left + margin.right,
			(!isnanf(size.height)?( size.height + padding.top + padding.bottom + margin.top + margin.bottom):nan()));
}
Rect Layout::PositionInfo::getPaddingBox() const {
	return Rect(position.x - padding.left, position.y - padding.top,
			size.width + padding.left + padding.right,
			(!isnanf(size.height)?( size.height + padding.top + padding.bottom):nan()));
}
Rect Layout::PositionInfo::getContentBox() const {
	return Rect(position.x, position.y, size.width, size.height);
}

Layout::Layout(NodeInfo &&n, bool d) : node(move(n)) {
	pos.disablePageBreak = d;
}

Layout::Layout(Layout &&l)
: node(move(l.node))
, pos(move(l.pos))
, preObjects(std::move(l.preObjects))
, layouts(std::move(l.layouts))
, postObjects(std::move(l.postObjects))
, listItemIndex(l.listItemIndex)
, listItem(l.listItem)
, context(std::move(l.context))
, inlineBlockLayouts(std::move(l.inlineBlockLayouts))
, charBinding(l.charBinding) { }

Layout & Layout::operator = (Layout &&l) {
	node = move(l.node);
	pos = move(l.pos);
	preObjects = (std::move(l.preObjects));
	layouts = (std::move(l.layouts));
	postObjects = (std::move(l.postObjects));
	listItemIndex = (l.listItemIndex);
	listItem = (l.listItem);
	context = std::move(l.context);
	inlineBlockLayouts = std::move(l.inlineBlockLayouts);
	charBinding = (l.charBinding);
	return *this;
}

Rect Layout::getBoundingBox() const {
	return pos.getBoundingBox();
}
Rect Layout::getPaddingBox() const {
	return pos.getPaddingBox();
}
Rect Layout::getContentBox() const {
	return pos.getContentBox();
}

void Layout::updatePosition(float p) {
	pos.position.y += p;
	for (auto &l : layouts) {
		l.updatePosition(p);
	}
}

void Layout::setBoundPosition(const Vec2 &p) {
	auto newPos = p;
	newPos.x += (pos.margin.left + pos.padding.left);
	newPos.y += (pos.margin.top + pos.padding.top);
	auto diff = p - getBoundingBox().origin;
	pos.position = newPos;

	for (auto &l : layouts) {
		auto nodePos = l.getBoundingBox().origin + diff;
		l.setBoundPosition(nodePos);
	}
}

Pair<float, float> FloatContext::getAvailablePosition(float yPos, float height) const {
	float x = root->pos.position.x;
	float width = root->pos.size.width;

	float topStackWidth = 0;
	for (auto &stack : floatLeft) {
		if (stack.origin.y <= yPos + height && stack.origin.y + stack.size.height >= yPos) {
			if (topStackWidth < stack.origin.x + stack.size.width) {
				x = stack.origin.x + stack.size.width;
				width = root->pos.size.width - x;
				topStackWidth = stack.origin.x + stack.size.width;
			}
		}
	}

	topStackWidth = 0;
	for (auto &stack : floatRight) {
		if (stack.origin.y <= yPos + height && stack.origin.y + stack.size.height >= yPos) {
			if (topStackWidth < stack.size.width) {
				topStackWidth = root->pos.size.width - stack.origin.x;
			}
		}
	}
	width -= topStackWidth;
	return pair(x, width);
}

bool FloatContext::pushFloatingNode(Layout &origin, Layout &l, Vec2 &vec) {
	auto &stacks = (l.node.block.floating==style::Float::Left)?floatLeft:floatRight;
	auto bbox = l.getBoundingBox();

	if (l.node.block.clear == style::Clear::Both
			|| (l.node.block.floating == style::Float::Left && l.node.block.clear == style::Clear::Left)
			|| (l.node.block.floating == style::Float::Right && l.node.block.clear == style::Clear::Right) ) {
		return pushFloatingNodeToNewStack(origin, l, stacks, bbox, vec);
	}

	// can we use current stack?
	if (!stacks.empty()) {
		auto &stack = stacks.back();
		if (root->pos.size.width - stack.size.width > bbox.size.width) {
			if (pushFloatingNodeToStack(origin, l, stack, bbox, vec)) {
				return true;
			}
		}
	}
	return pushFloatingNodeToNewStack(origin, l, stacks, bbox, vec);
}

bool FloatContext::pushFloatingNodeToStack(Layout &origin, Layout &l, Rect &s, const Rect &bbox, Vec2 &vec) {
	auto &stacks = (l.node.block.floating==style::Float::Left)?floatRight:floatLeft;

	float newHeight = std::max(s.size.height, bbox.size.height);
	float oppositeWidth = 0;

	for (auto &stack : stacks) {
		if (stack.origin.y <= s.origin.y + newHeight && stack.origin.y + stack.size.height >= s.origin.y) {
			if (oppositeWidth < stack.size.width) {
				oppositeWidth = stack.size.width;
			}
		}
	}

	if (root->pos.size.width - s.size.width - oppositeWidth > bbox.size.width) {
		s.size.height = newHeight;
		l.setBoundPosition(Vec2(
				(l.node.block.floating==style::Float::Left)?(s.origin.x + s.size.width):(s.origin.x - s.size.width)
						, s.origin.y));
		s.size.width += bbox.size.width;

		if (fabsf(root->pos.size.width - s.size.width) < root->pos.size.width * 0.025) {
			vec.y = s.origin.y + s.size.height;
		}

		return true;
	}
	return false;
}

bool FloatContext::pushFloatingNodeToNewStack(Layout &lo, Layout &l, FloatStack &stacks, const Rect &bbox, Vec2 &vec) {
	auto &oppositeStacks = (l.node.block.floating==style::Float::Left)?floatRight:floatLeft;

	Vec2 origin = Vec2(
			(l.node.block.floating==style::Float::Left)?(lo.pos.position.x):(lo.pos.position.x + lo.pos.size.width - bbox.size.width),
			bbox.origin.y);

	Size bsize = bbox.size;

	for (auto &stack : stacks) {
		if (origin.y < stack.origin.y + stack.size.height) {
			origin.y = stack.origin.y + stack.size.height;
		}
	}

	bool oppositeClear = false;
	if (l.node.block.clear == style::Clear::Both
			|| (l.node.block.floating == style::Float::Left && l.node.block.clear == style::Clear::Right)
			|| (l.node.block.floating == style::Float::Right && l.node.block.clear == style::Clear::Left) ) {
		oppositeClear = true;
	}

	for (auto &stack : oppositeStacks) {
		if (stack.origin.y <= origin.y && stack.origin.y + stack.size.height >= origin.y) {
			if (oppositeClear || root->pos.size.width - stack.size.width < bsize.width) {
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

	if (fabsf(root->pos.size.width - bsize.width)  < root->pos.size.width * 0.025) {
		vec.y = origin.y + bsize.height;
	}

	return true;
}

InlineContext::InlineContext() { }

bool InlineContext::init(FontSource *set, float density) {
	reader.init(set, &(label.format), density);
	return true;
}

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

NS_LAYOUT_END
