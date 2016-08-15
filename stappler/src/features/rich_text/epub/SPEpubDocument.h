/*
 * EpubDocument.h
 *
 *  Created on: 11 июл. 2016 г.
 *      Author: sbkarr
 */

#ifndef LIBS_EPUB_SRC_DOCUMENT_EPUBDOCUMENT_H_
#define LIBS_EPUB_SRC_DOCUMENT_EPUBDOCUMENT_H_

#include "SPEpubInfo.h"
#include "SPRichTextDocument.h"

NS_EPUB_BEGIN

class Document : public rich_text::Document {
public:
	static bool isEpub(const String &path);

	Document();

	virtual bool init(const FilePath &);
	virtual bool isFileExists(const String &) const override;
	virtual Bytes getFileData(const String &) override;
	virtual Bitmap getImageBitmap(const String &, const Bitmap::StrideFn &fn) override;

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
	virtual void processHtml(const String &, const CharReaderBase &, bool linear = true) override;

	Rc<Info> _info;
};

NS_EPUB_END

#endif /* LIBS_EPUB_SRC_DOCUMENT_EPUBDOCUMENT_H_ */
