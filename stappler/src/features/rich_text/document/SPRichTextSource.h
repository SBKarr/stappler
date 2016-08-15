/*
 * SPRichTextSource.h
 *
 *  Created on: 30 июля 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTSOURCE_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTSOURCE_H_

#include "SPFont.h"
#include "SPAsset.h"
#include "SPRichTextDocument.h"
#include "SPRichTextStyle.h"
#include "SPEventHeader.h"
#include "SPDataListener.h"

NS_SP_EXT_BEGIN(rich_text)

class Source : public FontSource {
public:
	static EventHeader onError;
	static EventHeader onUpdate;
	static EventHeader onAsset;

	enum Error : int64_t /* event compatible */ {
		DocumentError,
		NetworkError,
	};

	using FontCallback = Function<FontRequest::Receipt(const Document::FontConfigValue &type)>;
	using StringDocument = ValueWrapper<String, class DocumentStringDocumentTag>;
	using FontConfigStyle = Document::FontConfigValue;

	virtual ~Source();

	virtual bool init(const StringDocument &str);
	virtual bool init(const FilePath &file);
	virtual bool init(const String &url, const String &path,
			TimeInterval ttl = TimeInterval(), const String &cacheDir = "", const Asset::DownloadCallback & = nullptr);
	virtual bool init(Asset *a, bool enabled = true);

	void addFontFace(const String &family, style::FontFace &&);

	Document *getDocument() const;
	Asset *getAsset() const;

	void setFontScale(float);
	float getFontScale() const;

	bool isReady() const;
	bool isActual() const;
	bool isDocumentLoading() const;

	void refresh();

	bool tryReadLock(Ref *);
	void retainReadLock(Ref *, const Function<void()> &);
	void releaseReadLock(Ref *);

	void setEnabled(bool val);
	bool isEnabled() const;

protected:
	virtual void onDocumentAsset(Asset *);
	virtual void onDocumentAssetUpdated(Subscription::Flags);
	virtual void onDocument(Document *);

	virtual void tryLoadDocument();
	virtual void updateFontSource();
	virtual void updateDocument();

	virtual void onFontSet(FontSet *);
	virtual void updateRequest();

	virtual Rc<Document> openDocument(const String &path, const String &ct);

	virtual FontRequest::Receipt onFontRequest(const FontConfigStyle &style);
	virtual Bytes onReceiptFile(Document *doc, const String &str);

	String _string;
	String _file;

	data::Listener<Asset> _documentAsset;
	Rc<Document> _document;

	HtmlPage::FontMap _defaultFontFaces;
	uint64_t _loadedAssetMTime = 0;
	bool _documentLoading = false;
	float _fontScale = 1.0f;

	bool _enabled = true;
};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTSOURCE_H_ */
