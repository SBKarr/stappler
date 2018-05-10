/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SLTable.h"
#include "SLFloatContext.h"

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

	Result *getResult() const;

	const MediaParameters &getMedia() const;
	const MediaParameters &getOriginalMedia() const;
	void hookMedia(const MediaParameters &);
	void restoreMedia();
	bool isMediaHooked() const;

	Document *getDocument() const;
	FontSource *getFontSet() const;
	HyphenMap *getHyphens() const;

	void incrementNodeId(NodeId);
	NodeId getMaxNodeId() const;

	Layout &makeLayout(Layout::NodeInfo &&, bool disablePageBreaks);
	Layout &makeLayout(Layout::NodeInfo &&, Layout::PositionInfo &&);
	Layout &makeLayout(const Node *, style::Display = style::Display::None, bool disablePageBreaks = false);

	InlineContext *acquireInlineContext(FontSource *, float);

	bool isFileExists(const StringView &) const;
	Pair<uint16_t, uint16_t> getImageSize(const StringView &) const;

	const Style *compileStyle(const Node &);
	const Style *getStyle(const Node &) const;

	virtual bool resolveMediaQuery(MediaQueryId queryId) const override;
	virtual StringView getCssString(CssStringId) const override;

	style::Float getNodeFloating(const Node &node) const;
	style::Display getNodeDisplay(const Node &node) const;
	style::Display getNodeDisplay(const Node &node, style::Display p) const;

	style::Display getLayoutContext(const Node &node);
	style::Display getLayoutContext(const Vector<Node> &, Vector<Node>::const_iterator, style::Display p);

	uint16_t getLayoutDepth() const;
	Layout *getTopLayout() const;

	void render();

	Pair<float, float> getFloatBounds(const Layout *l, float y, float height);
	Pair<uint16_t, uint16_t> getTextBounds(const Layout *l, uint16_t &linePos, uint16_t &lineHeight, float density, float parentPosY);

	void pushNode(const Node *);
	void popNode();

	void processChilds(Layout &l, const Node &);

protected:
	const Vector<bool> * resolvePage(const ContentPage *page);
	void setPage(const ContentPage *);

	void addLayoutObjects(Layout &l);
	bool processChildNode(Layout &l, const Node &, Vec2 &pos, float &height, float &collapsableMarginTop, bool pageBreak);
	void doPageBreak(Layout *, Vec2 &);

	// build layout
	bool processNode(Layout &l, const Vec2 &origin, const Size &size, float collapsableMarginTop);
	bool processFloatNode(Layout &l, Layout::NodeInfo &&, Vec2 &pos);
	bool processBlockNode(Layout &l, Layout::NodeInfo &&, Vec2 &, float &height, float &margin);
	bool processInlineNode(Layout &l, Layout::NodeInfo &&, const Vec2 &pos);
	bool processInlineBlockNode(Layout &l, Layout::NodeInfo &&, const Vec2 &pos);
	bool processTableNode(Layout &l, Layout::NodeInfo &&, Vec2 &pos, float &height, float &margin);
	void pushFloatingNode(Layout &l, const BlockStyle &, const Vec2 &);

protected:
	Vector<MediaParameters> _originalMedia;
	MediaParameters _media;
	Margin _margin;

	Rc<Document> _document;
	Rc<Result> _result;
	Rc<FontSource>_fontSet;
	ExternalAssetsMap _externalAssets;

	Vector<String> _spine;

	Vector<Layout *> _layoutStack;
	Vector<FloatContext *> _floatStack;
	Rc<HyphenMap> _hyphens;

	Vector<const Node *> _nodeStack;
	Map<NodeId, Style> styles;

	const ContentPage *_currentPage = nullptr;
	const Vector<bool> *_currentMedia = nullptr;
	Map<const ContentPage *, Vector<bool>> _resolvedMedia;
	NodeId _maxNodeId = 0;

	MemoryStorage<Layout, 8_KiB> _layoutStorage;
	Vector<Rc<InlineContext>> _contextStorage;
};

NS_LAYOUT_END

#endif /* LAYOUT_RENDERER_SLBUILDER_H_ */
