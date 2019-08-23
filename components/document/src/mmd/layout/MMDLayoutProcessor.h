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

#ifndef MMD_LAYOUT_MMDLAYOUTPROCESSOR_H_
#define MMD_LAYOUT_MMDLAYOUTPROCESSOR_H_

#include "SLDocument.h"
#include "MMDHtmlProcessor.h"

NS_MMD_BEGIN

class LayoutProcessor : public HtmlProcessor {
public:
	using Page = layout::ContentPage;

	virtual ~LayoutProcessor() { }

	virtual bool init(LayoutDocument *);

protected:
	template <typename T>
	friend void LayoutProcessor_processAttr(LayoutProcessor &p, const T &container, const StringView &name,
			stappler::Map<stappler::String, stappler::String> &attributes, layout::Style &style, StringView &id);

	virtual void processHtml(const Content &, const StringView &, const Token &);

	void addCssString(const stappler::String &);

	void processTagStyle(const StringView &name, layout::Style &);
	void processStyle(const StringView &name, layout::Style &, const StringView &);

	layout::Node *makeNode(const StringView &name, InitList &&attr, VecList &&);

	virtual void pushNode(token *, const StringView &name, InitList &&attr = InitList(), VecList && = VecList());
	virtual void pushInlineNode(token *, const StringView &name, InitList &&attr = InitList(), VecList && = VecList());
	virtual void popNode();
	virtual void flushBuffer();

	Vector<layout::Node *> _nodeStack;
	LayoutDocument *_document = nullptr;
	Page *_page;
	uint32_t _tableIdx = 0;
};

NS_MMD_END

#endif /* MMD_LAYOUT_MMDLAYOUTPROCESSOR_H_ */
