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

#ifndef EPUB_EPUBDOCUMENT_H_
#define EPUB_EPUBDOCUMENT_H_

#include "EpubInfo.h"
#include "SLDocument.h"
#include "SLRendererTypes.h"

NS_EPUB_BEGIN

class Document : public layout::Document {
public:
	static DocumentFormat EpubFormat;

	using Style = layout::Style;
	using Node = layout::Node;
	using MediaParameters = layout::MediaParameters;
	using FilePath = layout::FilePath;

	static bool isEpub(const StringView &path);

	Document();

	virtual bool init(const FilePath &);
	virtual bool isFileExists(const StringView &) const override;
	virtual Bytes getFileData(const StringView &) override;
	virtual Bytes getImageData(const StringView &) override;
	virtual Pair<uint16_t, uint16_t> getImageSize(const StringView &) override;

	bool valid() const;
	operator bool () const;

	data::Value encode() const;

	const String & getUniqueId() const;
	const String & getModificationTime() const;
	const String & getCoverFile() const;

	bool isFileExists(const String &path, const String &root = "") const;
	size_t getFileSize(const String &path, const String &root = "") const;
	Bytes getFileData(const String &path, const String &root = "") const;

	bool isImage(const String &path, const String &root = "") const;
	bool isImage(const String &path, size_t &width, size_t &height, const String &root = "") const;

	Vector<SpineFile> getSpine(bool linearOnly = true) const;

public: // meta
	String getTitle() const;
	String getCreators() const;
	String getLanguage() const;

protected:
	virtual void onStyleAttribute(Style &style, const StringView &tag, const StringView &name, const StringView &value,
		const MediaParameters &) const override;

	virtual void processHtml(const String &, const StringView &, bool linear = true) override;

	void readTocFile(const String &);
	void readNcxNav(const String &filePath);
	void readXmlNav(const String &filePath);

	data::Value encodeContents(const ContentRecord &);

	Rc<Info> _info;
};

NS_EPUB_END

#endif /* EPUB_EPUBDOCUMENT_H_ */
