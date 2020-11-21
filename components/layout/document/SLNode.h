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

#ifndef LAYOUT_INTERNAL_SLNODE_H_
#define LAYOUT_INTERNAL_SLNODE_H_

#include "SLStyle.h"

NS_LAYOUT_BEGIN

using NodeId = uint32_t;
inline NodeId NodeIdNone() { return maxOf<NodeId>(); }

class Node : public AllocBase {
public:
	using NodeVec = Vector<Node>;
	using AttrMap = Map<String, String>;
	using Style = style::ParameterList;

	using ForeachIter = Function<void(Node &, size_t level)>;
	using ForeachConstIter = Function<void(const Node &, size_t level)>;

	Node & pushNode(StringView htmlName, StringView htmlId, Style &&style, AttrMap &&map) {
		dropValue();
		_nodes.emplace_back(htmlName, htmlId, std::move(style), std::move(map));
		return _nodes.back();
	}

	void pushValue(StringView str);
	void pushValue(WideString &&str);
	void pushValue(WideString &&str, Style &&s);
	void pushLineBreak();

	void pushStyle(const style::StyleVec &, const MediaQueryId &);
	void pushStyle(const Style &);

	void setNodeId(NodeId id);
	NodeId getNodeId() const;

	const Style &getStyle() const;
	StringView getHtmlId() const;
	StringView getHtmlName() const;
	const NodeVec &getNodes() const;
	WideStringView getValue() const;
	const AttrMap &getAttributes() const;
	StringView getAttribute(StringView) const;

	bool empty() const;
	bool hasValue() const;
	bool isVirtual() const;
	inline operator bool () const { return !empty(); }

	void foreach(const ForeachIter &);
	void foreach(const ForeachConstIter &) const;

	size_t getChildIndex(const Node &) const;

public:
	Node();
	Node(Node &&);
	Node &operator = (Node &&);

	Node(const Node &) = default;
	Node &operator = (const Node &) = default;

	Node(Style &&style, WideString &&);

	Node(StringView htmlName, StringView htmlId)
	: _htmlId(htmlId.str())
	, _htmlName(htmlName.str()) { }

	Node(StringView htmlName, StringView htmlId, Style &&style, AttrMap &&map)
	: _htmlId(htmlId.str())
	, _htmlName(htmlName.str())
	, _style(move(style))
	, _attributes(move(map)) { }

protected:
	void dropValue();
	void foreach(const ForeachIter &, size_t level);
	void foreach(const ForeachConstIter &, size_t level) const;

	NodeId _nodeId = NodeIdNone();
	String _htmlId;
	String _htmlName;
	Style _style;
	NodeVec _nodes;
	WideString _value;
	AttrMap _attributes;
	bool _isVirtual = false;
	bool _hasValue = false;
};

struct ContentPage {
	using StyleMap = Map<String, style::ParameterList>;
	using FontMap = Map<String, Vector<style::FontFace>>;

	String path;
	Node root;
	bool linear = true;

	Map<CssStringId, String> strings;
	Vector<style::MediaQuery> queries;
	StyleMap styles;
	FontMap fonts;

	Vector<String> styleReferences;
	Vector<String> assets;
	Map<String, Node *> ids;
};

NS_LAYOUT_END

#endif /* LAYOUT_INTERNAL_SLNODE_H_ */
