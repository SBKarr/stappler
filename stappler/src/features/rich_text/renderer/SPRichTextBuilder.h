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

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERERIMPLEMENTATION_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERERIMPLEMENTATION_H_

#include "SPRichTextRendererTypes.h"
#include "SPRichTextDocument.h"
#include "SPRichTextLayout.h"
#include "SPRichTextResult.h"
#include "SPFontFormatter.h"

NS_SP_EXT_BEGIN(rich_text)

class Builder : public RendererInterface {
public:
	using Formatter = font::Formatter;

	Builder(Document *, const MediaParameters &, font::Source *set, const Vector<String> & = Vector<String>());
	virtual ~Builder();

	void setHyphens(font::HyphenMap *);
	void setMargin(const Margin &);

	void render();

	Result *getResult() const;

	virtual bool resolveMediaQuery(MediaQueryId queryId) const override;
	virtual String getCssString(CssStringId) const override;

	void resolveMediaQueries(const Vector<style::MediaQuery> &);

	const MediaParameters &getMedia() const;

	Document *getDocument() const;
	font::Source *getFontSet() const;

	TimeInterval getReaderTime() const;

protected:
	void generateFontConfig();
	void buildBlockModel();

	// build layout
	bool processNode(Layout &l, const Vec2 &origin, const Size &size, float collapsableMarginTop);
	bool processFloatNode(Layout &l, const Node &, BlockStyle &&, Vec2 &pos);
	bool processBlockNode(Layout &l, const Node &, BlockStyle &&, Vec2 &, float &height, float &margin);
	bool processInlineNode(Layout &l, const Node &, BlockStyle &&, const Vec2 &pos);
	bool processInlineBlockNode(Layout &l, const Node &, BlockStyle &&, const Vec2 &pos);

	void processChilds(Layout &l, const Node &);
	void processChilds(Layout &l, const Vector<const Node *> &, bool pageBreak = false);
	bool processChildNode(Layout &l, const Node &, Vec2 &pos, float &height, float &collapsableMarginTop);
	void finalizeChilds(Layout &l, float height);

	bool initLayout(Layout &l, const Vec2 &origin, const Size &size, float collapsableMarginTop);
	bool finalizeLayout(Layout &l, const Vec2 &, float collapsableMarginTop);

	void processBackground(Layout &l, float parentPosY);
	void processOutline(Layout &l);
	void processRef(Layout &l, const String &);

	void pushFloatingNode(Layout &l, const BlockStyle &, const Vec2 &);

	Pair<uint16_t, uint16_t> getTextPosition(const Layout *l, uint16_t &linePos, uint16_t &lineHeight, float density, float parentPosY);

	// add objects
	void addLayoutObjects(Layout &l);

protected:
	Pair<float, float> getFloatBounds(const Layout *l, float y, float height);

	void applyVerticalMargin(Layout &l, float base, float collapsableMarginTop, float pos);

	WideString getListItemString(Layout *parent, Layout &l);
	int64_t getListItemCount(const Node &node);
	style::Display getLayoutContext(const Node &node) const;
	style::Display getLayoutContext(const Vector<const Node *> &, Vector<const Node *>::const_iterator, style::Display) const;

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
	Rc<font::Source>_fontSet;

	Vector<bool> _mediaQueries;
	Vector<String> _spine;

	Vector<Layout *> _layoutStack;
	Vector<FloatContext *> _floatStack;
	Rc<font::HyphenMap> _hyphens;
	TimeInterval _readerAccum;
};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTRENDERERIMPLEMENTATION_H_ */
