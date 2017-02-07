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
#include "SLNode.h"
#include "SPString.h"

//#define SP_RTNODE_LOG(...) logTag("RTNode", __VA_ARGS__)
#define SP_RTNODE_LOG(...)

NS_LAYOUT_BEGIN

void Node::pushValue(const String &str) {
	pushValue(string::toUtf16Html(str));
}

void Node::pushValue(WideString &&str) {
	Style s;
	s.set<style::ParameterName::Display>(style::Display::Inline);
	s.merge(_style, true);
	pushValue(std::move(str), std::move(s));
}

void Node::pushValue(WideString &&str, Style &&s) {
	dropValue();
	_nodes.emplace_back(std::move(s), std::move(str));
	_hasValue = true;
}

void Node::pushLineBreak() {
	Style s;
	s.set<style::ParameterName::Display>(style::Display::Inline);
	s.merge(_style, true);
	s.merge(style::getStyleForTag("br", style::Tag::Block));

	pushValue(u"\n", std::move(s));
}

void Node::pushStyle(const style::StyleVec &vec, const MediaQueryId &query) {
	_style.read(vec, query);
}

void Node::pushStyle(const Style &style) {
	_style.merge(style, false);
}

void Node::setNodeId(NodeId id) {
	_nodeId = id;
}
NodeId Node::getNodeId() const {
	return _nodeId;
}

const Node::Style &Node::getStyle() const {
	return _style;
}
const String &Node::getHtmlId() const {
	return _htmlId;
}
const String &Node::getHtmlName() const {
	return _htmlName;
}
const Node::NodeVec &Node::getNodes() const {
	return _nodes;
}
const WideString &Node::getValue() const {
	return _value;
}

const Node::AttrMap &Node::getAttributes() const {
	return _attributes;
}

bool Node::empty() const {
	return _value.empty() && _nodes.empty();
}

bool Node::hasValue() const {
	return _hasValue;
}

Node::Node() { }

Node::Node(Node &&other)
: _nodeId(other._nodeId)
, _htmlId(std::move(other._htmlId))
, _htmlName(std::move(other._htmlName))
, _style(std::move(other._style))
, _nodes(std::move(other._nodes))
, _value(std::move(other._value))
, _attributes(std::move(other._attributes))
, _isVirtual(other._isVirtual)
, _hasValue(other._hasValue) { }

Node &Node::operator = (Node &&other) {
	_nodeId = other._nodeId;
	_htmlId = std::move(other._htmlId);
	_htmlName = std::move(other._htmlName);
	_style = std::move(other._style);
	_nodes = std::move(other._nodes);
	_value = std::move(other._value);
	_attributes = std::move(other._attributes);
	_isVirtual = other._isVirtual;
	_hasValue = other._hasValue;
	return *this;
}

Node::Node(Style &&style, WideString &&value) : _style(std::move(style)), _value(std::move(value)) { }

void Node::foreach(const ForeachIter &onNode) {
	onNode(*this, 0);
	foreach(onNode, 0);
}

void Node::foreach(const ForeachConstIter &onNode) const {
	onNode(*this, 0);
	foreach(onNode, 0);
}

void Node::dropValue() {
	if (!_value.empty()) {
		Style s;
		s.set<style::ParameterName::Display>(style::Display::Inline);
		s.merge(_style, true);
		_nodes.emplace_back(std::move(s), std::move(_value));
		_value.clear();
		_isVirtual = false;
		_hasValue = true;
	}
}

void Node::foreach(const ForeachIter &onNode, size_t level) {
	for (auto &it : _nodes) {
		onNode(it, level + 1);
		it.foreach(onNode, level + 1);
	}
}

void Node::foreach(const ForeachConstIter &onNode, size_t level) const {
	for (auto &it : _nodes) {
		onNode(it, level + 1);
		it.foreach(onNode, level + 1);
	}
}

NS_LAYOUT_END
