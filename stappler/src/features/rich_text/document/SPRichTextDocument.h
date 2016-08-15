/*
 * SPHtmlDocument.h
 *
 *  Created on: 14 апр. 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTDOCUMENT_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTDOCUMENT_H_

#include "SPRichTextNode.h"
#include "SPRichTextMultipartParser.h"
#include "SPImage.h"

#include "base/CCMap.h"

NS_SP_EXT_BEGIN(rich_text)

class Document : public Ref, public RendererInterface {
public:
	struct Image {
		enum Type {
			Embed,
			Local,
			Web,
		};

		Type type;

		uint16_t width;
		uint16_t height;

		size_t offset;
		size_t length;
		String encoding;

		String name;
		String ref;

		Image(const MultipartParser::Image &);
		Image(uint16_t width, uint16_t height, size_t size, const String &path);
	};

	struct FontConfigValue {
		style::FontStyleParameters style;
		Vector<const style::FontFace *> face;
		WideString chars;
		Set<char16_t> set;
	};

	using FontConfig = Map<String, FontConfigValue>;
	using CssStrings = Map<CssStringId, String>;
	using MediaQueries = Vector<style::MediaQuery>;
	using ImageMap = Map<String, Image>;
	using GalleryMap = Map<String, Vector<String>>;
	using NamedStyles = Map<String, style::ParameterList>;
	using CssMap = Map<String, style::CssData>;

	static String getImageName(const String &);
	static Vector<String> getImageOptions(const String &);

public:
	Document();
	virtual ~Document();

	virtual bool init(const String &);
	virtual bool init(const FilePath &, const String & = "");
	virtual bool init(const Bytes &, const String & = "");

	virtual bool prepare();

	virtual bool isFileExists(const String &) const;
	virtual Bytes getFileData(const String &);
	virtual Bitmap getImageBitmap(const String &, const Bitmap::StrideFn &fn = nullptr);

	void setDefaultFontFaces(const HtmlPage::FontMap &);
	void setDefaultFontFaces(HtmlPage::FontMap &&);

	const Node &getRoot() const;
	const Vector<HtmlPage> &getContent() const;
	const HtmlPage *getContentPage(const String &) const;
	const Node *getNodeById(const String &) const;

	const Vector<style::MediaQuery> &getMediaQueries() const;
	const CssStrings &getCssStrings() const;
	const FontConfig &getFontConfig() const;

	virtual bool resolveMediaQuery(MediaQueryId queryId) const;
	virtual String getCssString(CssStringId) const;

	bool hasImage(const String &) const;
	Pair<uint16_t, uint16_t> getImageSize(const String &) const;

	const ImageMap & getImages() const;
	const GalleryMap & getGalleryMap() const;

protected:
	Bytes readData(size_t offset, size_t len);

	virtual void processCss(const String &, const CharReaderBase &);
	virtual void processHtml(const String &, const CharReaderBase &, bool linear = true);
	virtual void processMeta(HtmlPage &c, const Vector<Pair<String, String>> &);
	void updateNodes();

	void processLists(FontConfig &, const Node &, const HtmlPage::FontMap &);
	void processList(FontConfig &, style::ListStyleType, const style::FontStyleParameters &, const HtmlPage::FontMap &);

	void processSpans(FontConfig &, const Node &, const HtmlPage::FontMap &);
	void processSpan(FontConfig &, const Node &, const style::FontStyleParameters &, const HtmlPage::FontMap &);

	void forEachFontStyle(const style::ParameterList &style, const Function<void(const style::FontStyleParameters &)> &);

	Vector<const style::FontFace *> selectFontFace(const style::FontStyleParameters &params, const HtmlPage::FontMap &page, const HtmlPage::FontMap &def);

	CssStrings _cssStrings;
	MediaQueries _mediaQueries;

	HtmlPage::FontMap _defaultFontFaces;
	FontConfig _fontConfig;
	Set<String> _fontFamily;
	ImageMap _images;

	GalleryMap _gallery;
	Map<String, Node *> _ids;
	Vector<HtmlPage> _content;
	CssMap _css;

	Bytes _data;
	String _filePath;
	String _contentType;
};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTDOCUMENT_H_ */
