/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef MMD_PROCESSORS_MMDHTMLPROCESSOR_H_
#define MMD_PROCESSORS_MMDHTMLPROCESSOR_H_

#include "MMDProcessor.h"

NS_MMD_BEGIN

class HtmlProcessor : public Processor {
public:
	using InitList = std::initializer_list<Pair<StringView, StringView>>;
	using VecList = Vector<Pair<StringView, StringView>>;

	HtmlProcessor();
	virtual ~HtmlProcessor() { }

	virtual bool init(std::ostream *);
	virtual void process(const Content &, const StringView &, const Token &);

protected:
	virtual void processMeta(const StringView &, const StringView &);
	virtual void processHtml(const Content &, const StringView &, const Token &);

	void pad(std::ostream &, uint16_t num);
	void printHtml(std::ostream &, const StringView &);
	void printLocalizedChar(std::ostream &, uint16_t type);

	bool shouldWriteMeta(const StringView &);

	void startCompleteHtml(const Content &c);
	void endCompleteHtml();

	void exportToken(std::ostream &, token *t);
	void exportTokenTree(std::ostream &, token *t);

	void exportTokenRaw(std::ostream &, token *t);
	void exportTokenTreeRaw(std::ostream &, token *t);

	void makeTokenHash(const Function<void(const StringView &)> &, token *t);
	void makeTokenTreeHash(const Function<void(const StringView &)> &, token *t);

	void exportTokenMath(std::ostream &, token *t);
	void exportTokenTreeMath(std::ostream &, token *t);

	void exportFootnoteList(std::ostream &);
	void exportGlossaryList(std::ostream &);
	void exportCitationList(std::ostream &);

	void exportBlockquote(std::ostream &, token *t);
	void exportDefinition(std::ostream &, token *t);
	void exportDefList(std::ostream &, token *t);
	void exportDefTerm(std::ostream &, token *t);
	void exportFencedCodeBlock(std::ostream &,token *t);
	void exportIndentedCodeBlock(std::ostream &, token *t);
	void exportHeader(std::ostream &, token *t);
	void exportHr(std::ostream &);
	void exportHtml(std::ostream &, token *t);
	void exportListBulleted(std::ostream &, token *t);
	void exportListEnumerated(std::ostream &, token *t);
	void exportListItem(std::ostream &, token *t, bool tight);
	void exportDefinitionBlock(std::ostream &, token *t);
	void exportHeaderText(std::ostream &, token *t, uint8_t level);
	void exportTable(std::ostream &, token *t);
	void exportTableHeader(std::ostream &, token *t);
	void exportTableSection(std::ostream &, token *t);
	void exportTableCell(std::ostream &, token *t);
	void exportTableRow(std::ostream &, token *t);
	void exportToc(std::ostream &, token *t);
	void exportTocEntry(std::ostream &, size_t &counter, uint16_t level);

	void exportBacktick(std::ostream &, token *t);
	void exportPairBacktick(std::ostream &, token *t);
	void exportPairAngle(std::ostream &, token *t);
	void exportPairBracketImage(std::ostream &, token *t);
	void exportPairBracketAbbreviation(std::ostream &, token *t);
	void exportPairBracketCitation(std::ostream &, token *t);
	void exportPairBracketFootnote(std::ostream &, token *t);
	void exportPairBracketGlossary(std::ostream &, token *t);
	void exportPairBracketVariable(std::ostream &, token *t);

	void exportCriticAdd(std::ostream &, token *t);
	void exportCriticDel(std::ostream &, token *t);
	void exportCriticCom(std::ostream &, token *t);
	void exportCriticHi(std::ostream &, token *t);
	void exportCriticPairSubDel(std::ostream &, token *t);
	void exportCriticPairSubAdd(std::ostream &, token *t);

	void exportMath(std::ostream &, token *t);
	void exportSubscript(std::ostream &, token *t);
	void exportSuperscript(std::ostream &, token *t);
	void exportLineBreak(std::ostream &, token *t);

	void exportLink(std::ostream &, token * text, Content::Link * link);
	void exportImage(std::ostream &, token * text, Content::Link * link, bool is_figure);

	virtual void pushHtmlEntity(std::ostream &, token *t);

	virtual void pushNode(token *t, const StringView &name, InitList &&attr = InitList(), VecList && = VecList()) = 0;
	virtual void pushInlineNode(token *t, const StringView &name, InitList &&attr = InitList(), VecList && = VecList()) = 0;
	virtual void popNode() = 0;
	virtual void flushBuffer() = 0;

protected:
	virtual void pushHtmlEntityText(std::ostream &out, StringView r, token *t = nullptr);

	bool spExt = false;
	uint8_t html_header_level = maxOf<uint8_t>();
	std::ostream *output = nullptr;
	StringStream buffer;
	uint32_t figureId = 0;
};

NS_MMD_END

#endif /* MMD_PROCESSORS_MMDHTMLPROCESSOR_H_ */
