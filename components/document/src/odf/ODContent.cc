
#include "ODContent.h"

namespace opendocument {

Node::Node() { }

Node::Node(const StringView &tag, const style::Style *s)
: tag(tag.str<Interface>()), style(s) { }

Node::Node(const StringView &tag, Map<String, String> &&a, const style::Style *s)
: tag(tag.str<Interface>()), style(s), attr(std::move(a)) { }

void Node::setStyle(const style::Style *s) {
	style = s;
}

Node & Node::addContent(const StringView &c) {
	pool::push(nodes.get_allocator());
	if (nodes.empty()) {
		if (value.empty()) {
			value = c.str<Interface>();
		} else {
			value += c.str<Interface>();
		}
	} else {
		nodes.emplace_back(new (nodes.get_allocator()) Node())->addContent(c);
	}
	pool::pop();
	return *this;
}

Node & Node::setAttribute(const StringView &key, const StringView &value) {
	pool::push(nodes.get_allocator());
	attr.emplace(key.str<Interface>(), value.str<Interface>());
	pool::pop();
	return *this;
}

Node & Node::setNote(const StringView &key, const StringView &value) {
	pool::push(nodes.get_allocator());
	notes.emplace(key.str<Interface>(), value.str<Interface>());
	pool::pop();
	return *this;
}

StringView Node::getNote(const StringView &key) const {
	auto it = notes.find(key);
	if (it != notes.end()) {
		return it->second;
	}
	return StringView();
}

String Node::popNote(const StringView &key) {
	auto it = notes.find(key);
	if (it != notes.end()) {
		auto ret = it->second;
		notes.erase(it);
		return ret;
	}
	return String();
}

Node * Node::addNode(const StringView &tag, const style::Style *s) {
	pool::push(nodes.get_allocator());
	if (!value.empty()) {
		nodes.emplace_back(new (nodes.get_allocator()) Node())->setContent(std::move(value));
		value = String();
	}

	auto ret = nodes.emplace_back(new (nodes.get_allocator()) Node(tag, s));
	pool::pop();
	return ret;
}

Node * Node::addNode(const StringView &tag, Map<String, String> &&a, const style::Style *s) {
	pool::push(nodes.get_allocator());
	if (!value.empty()) {
		nodes.emplace_back(new (nodes.get_allocator()) Node())->setContent(std::move(value));
		value = String();
	}

	auto ret = nodes.emplace_back(new (nodes.get_allocator()) Node(tag, std::move(a), s));
	pool::pop();
	return ret;
}

StringView Node::getTag() const { return tag; }
StringView Node::getValue() const { return value; }
const style::Style *Node::getStyle() const { return style; }

const Map<String, String> &Node::getAttribute() const { return attr; }
const Vector<Node *> &Node::getNodes() const { return nodes; }

bool Node::empty() const {
	return nodes.empty() && value.empty() && attr.empty();
}

bool Node::hasContent() const {
	return !value.empty() || !nodes.empty();
}

void Node::setContent(String &&str) {
	pool::push(nodes.get_allocator());
	value = std::move(str);
	pool::pop();
}


MasterPage::MasterPage(const StringView &n, const style::Style *s)
: header("style:header"), footer("style:footer"), name(n.str<Interface>()), pageLayout(s) { }

}
