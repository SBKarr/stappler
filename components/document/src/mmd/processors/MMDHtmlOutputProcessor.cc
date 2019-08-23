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

#include "SPCommon.h"
#include "MMDHtmlOutputProcessor.h"
#include "MMDEngine.h"

NS_MMD_BEGIN

void HtmlOutputProcessor::run(std::ostream *stream, const StringView &str, const Extensions &ext) {
	Engine e; e.init(str, ext);

	e.process([&] (const Content &c, const StringView &s, const Token &t) {
		HtmlOutputProcessor p; p.init(stream);
		p.process(c, s, t);
	});
}

void HtmlOutputProcessor::run(std::ostream *stream, memory::pool_t *pool, const StringView &str, const Extensions &ext) {
	Engine e; e.init(pool, str, ext);

	e.process([&] (const Content &c, const StringView &s, const Token &t) {
		HtmlOutputProcessor p; p.init(stream);
		p.process(c, s, t);
	});
}

void HtmlOutputProcessor::pushNode(token *t, const StringView &name, InitList &&attr, VecList && vec) {
	flushBuffer();
	*output << "<" << name;
	if (attr.size() > 0) {
		for (auto &it : attr) {
			*output << " " << it.first << "=\"" << it.second << "\"";
		}
	}
	if (!vec.empty()) {
		for (auto &it : vec) {
			*output << " " << it.first << "=\"" << it.second << "\"";
		}
	}
	*output << ">";
	tagStack.emplace_back(name, 0);
}

void HtmlOutputProcessor::pushInlineNode(token *t, const StringView &name, InitList &&attr, VecList && vec) {
	flushBuffer();
	*output << "<" << name;
	if (attr.size() > 0) {
		for (auto &it : attr) {
			*output << " " << it.first << "=\"" << it.second << "\"";
		}
	}
	if (!vec.empty()) {
		for (auto &it : vec) {
			*output << " " << it.first << "=\"" << it.second << "\"";
		}
	}
	*output << " />";
}

void HtmlOutputProcessor::popNode() {
	flushBuffer();
	*output << "</" << tagStack.back().first << ">";
	tagStack.pop_back();
}

void HtmlOutputProcessor::flushBuffer() {
	if (buffer.size() > 0) {
		output->write(buffer.data(), buffer.size());
		if (!tagStack.empty()) {
			++ tagStack.back().second;
		}
	}
	buffer.clear();
}

NS_MMD_END
