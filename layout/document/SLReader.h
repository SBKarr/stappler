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

#ifndef LAYOUT_DOCUMENT_SLREADER_H_
#define LAYOUT_DOCUMENT_SLREADER_H_

#include "SLNode.h"

NS_LAYOUT_BEGIN

class Reader {
public:
	using Tag = style::Tag;
	using Style = style::ParameterList;

	using CssStrings = Map<CssStringId, String>;
	using NamedStyles = Map<String, style::ParameterList>;
	using MediaQueries = Vector<style::MediaQuery>;
	using MetaPairs = Vector<Pair<String, String>>;
	using CssMap = Map<String, style::CssData>;

	virtual ~Reader() { }

	virtual bool readHtml(HtmlPage &page, const StringView &str, CssStrings &, MediaQueries &, MetaPairs &, CssMap &);
	virtual style::CssData readCss(const String &path, const StringView &str, CssStrings &, MediaQueries &);

	MediaQueryId addMediaQuery(style::MediaQuery &&);
	void addCssString(CssStringId id, const String &str);
	void addCssString(const String &origStr);

	void onStyleObject(const String &, const style::StyleVec &, const MediaQueryId &);

protected:
	Tag onTag(StringReader &tag);

	virtual void onPushTag(Tag &);
	virtual void onPopTag(Tag &);
	virtual void onInlineTag(Tag &);
	virtual void onTagContent(Tag &, StringReader &);

	void onStyleTag(StringReader &);

	bool hasParentTag(const String &);
	bool shouldParseRefs();

	Style specializeStyle(const Tag &tag, const Style &parentStyle);

	void onMeta(String &&name, String &&content);
	void onCss(const String & href);

	virtual bool isStyleAttribute(const String &tagName, const String &name) const;
	virtual void addStyleAttribute(Tag &, const String &name, const String &value);

	String _path;
	StringReader _origin;
	StringReader _current;

	Vector<Node *> _nodeStack;
	Vector<Style> _styleStack;

	Vector<Tag> _tagStack;
	NamedStyles _namedStyles;
	CssStrings * _cssStrings;
	MediaQueries * _mediaQueries;
	MetaPairs *_meta;
	CssMap *_css = nullptr;
	HtmlPage::FontMap _fonts;

	bool _htmlTag = false;
	uint32_t _pseudoId = 0;
};

NS_LAYOUT_END

#endif /* LAYOUT_DOCUMENT_SLREADER_H_ */
