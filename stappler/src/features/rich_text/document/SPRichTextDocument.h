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

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTDOCUMENT_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTDOCUMENT_H_

#include "SPRichTextNode.h"
#include "SPRichTextMultipartParser.h"
#include "SPBitmap.h"

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

	struct ContentRecord {
		String label;
		String href;
		Vector<ContentRecord> childs;
	};

	using FontConfig = Map<String, FontConfigValue>;
	using CssStrings = Map<CssStringId, String>;
	using MediaQueries = Vector<style::MediaQuery>;
	using ImageMap = Map<String, Image>;
	using GalleryMap = Map<String, Vector<String>>;
	using NamedStyles = Map<String, style::ParameterList>;
	using CssMap = Map<String, style::CssData>;
	using FontFaceMap = Map<String, Vector<style::FontFace>>;

	static String getImageName(const String &);
	static Vector<String> getImageOptions(const String &);

public:
	Document();
	virtual ~Document();

	virtual bool init(const String &);
	virtual bool init(const FilePath &, const String & = "");
	virtual bool init(const Bytes &, const String & = "");

	virtual bool prepare();

	virtual const FontFaceMap &getFontFaces() const;

	virtual bool isFileExists(const String &) const;
	virtual Bytes getFileData(const String &);
	virtual Bitmap getImageBitmap(const String &, const Bitmap::StrideFn &fn = nullptr);

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
	const ContentRecord & getTableOfContents() const;

	size_t getSizeInMemory();

protected:
	Bytes readData(size_t offset, size_t len);

	virtual void processCss(const String &, const CharReaderBase &);
	virtual void processHtml(const String &, const CharReaderBase &, bool linear = true);
	virtual void processMeta(HtmlPage &c, const Vector<Pair<String, String>> &);
	void updateNodes();

	CssStrings _cssStrings;
	MediaQueries _mediaQueries;

	ImageMap _images;
	GalleryMap _gallery;
	Map<String, Node *> _ids;
	Vector<HtmlPage> _content;
	CssMap _css;
	FontFaceMap _fontFaces;

	Bytes _data;
	String _filePath;
	String _contentType;

	ContentRecord _contents;
};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_SPRICHTEXTDOCUMENT_H_ */
