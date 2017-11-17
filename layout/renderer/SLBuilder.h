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

#ifndef LAYOUT_RENDERER_SLBUILDER_H_
#define LAYOUT_RENDERER_SLBUILDER_H_

#include "SLDocument.h"
#include "SLRendererTypes.h"
#include "SLResult.h"

NS_LAYOUT_BEGIN

class Builder : public RendererInterface {
public:
	using ExternalAssetsMap = Map<String, Document::AssetMeta>;

	static void compileNodeStyle(Style &style, const ContentPage *page, const Node &node,
			const Vector<const Node *> &stack, const MediaParameters &media, const Vector<bool> &resolved);

	Builder(Document *, const MediaParameters &, FontSource *set, const Vector<String> & = Vector<String>());
	virtual ~Builder();

	void setExternalAssetsMeta(ExternalAssetsMap &&);
	void setHyphens(HyphenMap *);
	void setMargin(const Margin &);

	void render();

	Result *getResult() const;

	virtual bool resolveMediaQuery(MediaQueryId queryId) const override;
	virtual String getCssString(CssStringId) const override;

	const MediaParameters &getMedia() const;

	Document *getDocument() const;
	FontSource *getFontSet() const;

	TimeInterval getReaderTime() const;

protected:
	bool isFileExists(const String &) const;
	Pair<uint16_t, uint16_t> getImageSize(const String &) const;

	const Style *compileStyle(const Node &);

	const Vector<bool> * resolvePage(const ContentPage *page);
	void setPage(const ContentPage *);

	void generateFontConfig();
	void buildBlockModel();

	// build layout
	bool processNode(Layout &l, const Vec2 &origin, const Size &size, float collapsableMarginTop);
	bool processFloatNode(Layout &l, Layout::NodeInfo &&, Vec2 &pos);
	bool processBlockNode(Layout &l, Layout::NodeInfo &&, Vec2 &, float &height, float &margin);
	bool processInlineNode(Layout &l, Layout::NodeInfo &&, const Vec2 &pos);
	bool processInlineBlockNode(Layout &l, Layout::NodeInfo &&, const Vec2 &pos);

	void processChilds(Layout &l, const Node &);
	void processChilds(Layout &l, const Vector<const Node *> &, bool pageBreak = false);
	bool processChildNode(Layout &l, const Node &, Vec2 &pos, float &height, float &collapsableMarginTop, bool pageBreak);
	void finalizeChilds(Layout &l, float height);

	bool initLayout(Layout &l, const Vec2 &origin, const Size &size, float collapsableMarginTop);
	bool finalizeLayout(Layout &l, const Vec2 &, float collapsableMarginTop);

	void processBackground(Layout &l, float parentPosY);
	void processOutline(Layout &l);
	void processRef(Layout &l, const String &ref, const String &target);

	void pushFloatingNode(Layout &l, const BlockStyle &, const Vec2 &);

	Pair<uint16_t, uint16_t> getTextPosition(const Layout *l, uint16_t &linePos, uint16_t &lineHeight, float density, float parentPosY);

	// add objects
	void addLayoutObjects(Layout &l);

protected:
	Pair<float, float> getFloatBounds(const Layout *l, float y, float height);

	void applyVerticalMargin(Layout &l, float base, float collapsableMarginTop, float pos);

	WideString getListItemString(Layout *parent, Layout &l);
	int64_t getListItemCount(const Node &node, const Style &);

	style::Display getNodeDisplay(const Node &node, const Style *parentStyle);
	style::Display getLayoutContext(const Layout::NodeInfo &node);
	style::Display getLayoutContext(const Vector<Node> &, Vector<Node>::const_iterator, const Layout::NodeInfo &p);

	void initFormatter(Layout &, const ParagraphStyle &, float, Formatter &, bool initial);
	InlineContext &makeInlineContext(Layout &l, float parentPosY, const Node &node);
	float fixLabelPagination(Layout &l, Label &label);

	// Завершает рисование строковых элементов и подготавливает их к переносу в укладку
	float freeInlineContext(Layout &l);

	// Добавляет в укладку строковые объекты и очищает буфер строкового контекста
	void finalizeInlineContext(Layout &l);

	void drawListItemBullet(Layout &, float);

	void doPageBreak(Layout *, Vec2 &);

	MediaParameters _media;
	Margin _margin;

	Rc<Document>_document;
	Rc<Result>_result;
	Rc<FontSource>_fontSet;
	ExternalAssetsMap _externalAssets;

	Vector<String> _spine;

	Vector<Layout *> _layoutStack;
	Vector<FloatContext *> _floatStack;
	Rc<HyphenMap> _hyphens;
	TimeInterval _readerAccum;

	Vector<const Node *> _nodeStack;
	Map<NodeId, Style> styles;

	Map<CssStringId, String> _cssStrings;
	const ContentPage *_currentPage = nullptr;
	const Vector<bool> *_currentMedia = nullptr;
	Map<const ContentPage *, Vector<bool>> _resolvedMedia;
};

NS_LAYOUT_END

#endif /* LAYOUT_RENDERER_SLBUILDER_H_ */
