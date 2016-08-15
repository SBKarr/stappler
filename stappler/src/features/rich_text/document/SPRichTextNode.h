/*
 * SPRichTextNode.h
 *
 *  Created on: 27 июля 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTNODE_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTNODE_H_

#include "SPRichTextStyle.h"

NS_SP_EXT_BEGIN(rich_text)

using NodeId = uint32_t;
inline NodeId NodeIdNone() { return maxOf<NodeId>(); }

class Node {
public:
	using NodeVec = Vector<Node>;
	using AttrMap = Map<String, String>;
	using Style = style::ParameterList;

	using ForeachIter = Function<void(Node &, size_t level)>;

	template <typename Name, typename Id, typename TStyle>
	Node & pushNode(Name &&htmlName, Id &&htmlId, TStyle &&style, AttrMap &&map) {
		dropValue();
		_nodes.emplace_back(std::forward<Name>(htmlName), std::forward<Id>(htmlId), std::forward<TStyle>(style), std::move(map));
		return _nodes.back();
	}

	void pushValue(const String &str);
	void pushValue(WideString &&str);
	void pushValue(WideString &&str, Style &&s);
	void pushLineBreak();

	void pushStyle(const style::StyleVec &, const MediaQueryId &);
	void pushStyle(const Style &);

	void setNodeId(NodeId id);
	NodeId getNodeId() const;

	const Style &getStyle() const;
	const String &getHtmlId() const;
	const String &getHtmlName() const;
	const NodeVec &getNodes() const;
	const WideString &getValue() const;
	const AttrMap &getAttributes() const;

	bool empty() const;
	bool hasValue() const;
	inline operator bool () const { return !empty(); }

	void foreach(const ForeachIter &);

public:
	Node();
	Node(Node &&);
	Node &operator = (Node &&);

	Node(const Node &) = delete;
	Node &operator = (const Node &) = delete;

	Node(Style &&style, WideString &&);

	template <typename Name, typename Id, typename TStyle>
	Node(Name &&htmlName, Id &&htmlId, TStyle &&style, AttrMap &&map)
	: _htmlId(std::forward<Id>(htmlId))
	, _htmlName(std::forward<Name>(htmlName))
	, _style(std::forward<TStyle>(style))
	, _attributes(std::move(map)) { }

protected:
	void dropValue();
	void foreach(const ForeachIter &, int level);

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

struct HtmlPage {
	using FontMap = Map<String, Vector<style::FontFace>>;

	String path;
	Node root;
	FontMap fonts;
	bool linear = true;
};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTNODE_H_ */
