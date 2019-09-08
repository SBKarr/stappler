
#ifndef UTILS_SRC_OPENDOCUMENT_ODMMDPROCESSOR_H_
#define UTILS_SRC_OPENDOCUMENT_ODMMDPROCESSOR_H_

#include "MMDHtmlProcessor.h"
#include "ODDocument.h"

typedef struct _HyphenDict HyphenDict;

namespace opendocument {

class MmdProcessor : public stappler::mmd::HtmlProcessor {
public:
	using Content = stappler::mmd::Content;
	using Token = stappler::mmd::Token;
	using token = stappler::mmd::token;
	using Engine = stappler::mmd::Engine;
	using Extensions = stappler::mmd::Extensions;
	using HyphenDicts = Map<stappler::CharGroupId, HyphenDict *>;

	struct ImageData {
		String name;
		String type;
		uint16_t width;
		uint16_t height;

		operator bool() const { return !name.empty() && !type.empty(); }
	};

	struct Config {
		Document *document = nullptr;
		Node *root = nullptr;
		Function<ImageData(const StringView &)> onImage;
		Function<ImageData(const StringView &)> onVideo;
		Function<String(const StringView &)> onLink;
		Function<String(const StringView &)> onId;
		Function<void(Node *, const StringView &)> onInsert;
		Function<void(Node *, const StringView &)> onMath;
		StringStream *errorStream = nullptr;

		Config() = default;
		Config(Document *, Node *, Function<ImageData(const StringView &)> &&, Function<ImageData(const StringView &)> &&,
				Function<String(const StringView &)> &&, Function<String(const StringView &)> &&,
				Function<void(Node *, const StringView &)> &&, Function<void(Node *, const StringView &)> &&, StringStream * = nullptr);

		Config(const Config &) = default;
		Config &operator=(const Config &) = default;
	};

	static void run(const Config &cfg, pool_t *pool, const StringView &str, const Value *meta, const Extensions &ext = stappler::mmd::DefaultExtensions);

	virtual ~MmdProcessor() { }

	virtual bool init(const Config &cfg, const Value *meta);

protected:
	Node *makeNode(const StringView &name, InitList &&attr, VecList &&vec);
	Node *makeLinkNode(const StringView &name, InitList &&attr, VecList &&vec);
	Node *makeImageNode(const StringView &name, InitList &&attr, VecList &&vec);

	virtual void processHtml(const Content &, const StringView &, const Token &);
	virtual void exportMath(std::ostream &, token *t);

	virtual void pushNode(token *, const StringView &name, InitList &&attr = InitList(), VecList && = VecList());
	virtual void pushInlineNode(token *, const StringView &name, InitList &&attr = InitList(), VecList && = VecList());
	virtual void popNode();
	virtual void flushBuffer();

	virtual void exportPairBracketCitation(std::ostream &, token *t);
	virtual void exportPairBracketFootnote(std::ostream &, token *t);

	style::Style *getParagraphStyle();
	style::Style *getHeaderStyle(size_t);
	style::Style *getNoteStyle();
	style::Style *getEmStyle();
	style::Style *getStrongStyle();
	style::Style *getOrderedListStyle();
	style::Style *getUnorderedListStyle();

	style::Style *getFigureStyle();
	style::Style *getFigureContentStyle();
	style::Style *getFigureLabelStyle();
	style::Style *getImageStyle();
	style::Style *getLinkStyle();

	style::Style *getTableStyle();
	style::Style *getTableRowStyle();
	style::Style *getTableHeaderCellStyle();
	style::Style *getTableBodyCellStyle(size_t n);
	style::Style *getTableCaptionStyle();

	Config _config;
	Vector<Node *> _nodeStack;
	uint32_t _tableIdx = 0;
	Node _tmpNode;
	const Value *_meta = nullptr;
	bool _inFootnote = false;

	style::Style *_tableStyle = nullptr;
	style::Style *_rowStyle = nullptr;
	style::Style *_cellStyle1 = nullptr;
	style::Style *_cellStyle2 = nullptr;
	style::Style *_headerStyle = nullptr;
};

}

#endif /* UTILS_SRC_OPENDOCUMENT_ODMMDPROCESSOR_H_ */
