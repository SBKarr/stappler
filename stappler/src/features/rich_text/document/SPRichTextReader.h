/*
 * SPRichTextDocumentReader.h
 *
 *  Created on: 27 июля 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTREADER_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTREADER_H_

#include "SPRichTextNode.h"

NS_SP_EXT_BEGIN(rich_text)

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

	virtual bool readHtml(HtmlPage &page, const CharReaderBase &str, CssStrings &, MediaQueries &, MetaPairs &, CssMap &);
	virtual style::CssData readCss(const String &path, const CharReaderBase &str, CssStrings &, MediaQueries &);

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

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTREADER_H_ */
