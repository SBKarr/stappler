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

#ifndef MMD_PROCESSORS_MMDHTMLOUTPUTPROCESSOR_H_
#define MMD_PROCESSORS_MMDHTMLOUTPUTPROCESSOR_H_

#include "MMDHtmlProcessor.h"

NS_MMD_BEGIN

class HtmlOutputProcessor : public HtmlProcessor {
public:
	static void run(std::ostream *, const StringView &, const Extensions & = DefaultExtensions);
	static void run(std::ostream *, memory::pool_t *, const StringView &, const Extensions & = DefaultExtensions);

protected:
	virtual void pushNode(token *t, const StringView &name, InitList &&attr, VecList &&) override;
	virtual void pushInlineNode(token *t, const StringView &name, InitList &&attr, VecList &&) override;
	virtual void popNode() override;

	virtual void flushBuffer() override;

	Vector<Pair<StringView, size_t>> tagStack;
};

NS_MMD_END

#endif /* MMD_PROCESSORS_MMDHTMLOUTPUTPROCESSOR_H_ */
