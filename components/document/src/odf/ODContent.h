
#ifndef UTILS_SRC_ODCONTENT_H_
#define UTILS_SRC_ODCONTENT_H_

#include "ODStyle.h"
#include "SPLayout.h"

namespace opendocument {

class Node : public AllocBase {
public:
	Node();
	Node(const StringView &tag, const style::Style * = nullptr);
	Node(const StringView &tag, Map<String, String> &&, const style::Style * = nullptr);

	void setStyle(const style::Style *);

	Node & addContent(const StringView &);
	Node & setAttribute(const StringView &, const StringView &);

	Node & setNote(const StringView &, const StringView &);
	StringView getNote(const StringView &) const;
	String popNote(const StringView &);

	Node * addNode(const StringView &tag, const style::Style * = nullptr);
	Node * addNode(const StringView &tag, Map<String, String> &&, const style::Style * = nullptr);

	StringView getTag() const;
	StringView getValue() const;
	const style::Style * getStyle() const;

	const Map<String, String> &getAttribute() const;
	const Vector<Node *> &getNodes() const;

	bool empty() const;
	bool hasContent() const;

protected:
	void setContent(String &&);

	String tag; // can be empty
	String value;
	const style::Style *style = nullptr;
	Map<String, String> attr;
	Vector<Node *> nodes;
	Map<String, String> notes;
};

class MasterPage : public AllocBase {
public:
	MasterPage(const StringView &, const style::Style *);

	Node &getHeader() { return header; }
	Node &getFooter() { return footer; }

	const Node &getHeader() const { return header; }
	const Node &getFooter() const { return footer; }

	StringView getName() const { return name; }
	const style::Style *getPageLayout() const { return pageLayout; }

protected:
	Node header;
	Node footer;

	String name;
	const style::Style *pageLayout = nullptr;
};

}

#endif /* UTILS_SRC_ODCONTENT_H_ */
