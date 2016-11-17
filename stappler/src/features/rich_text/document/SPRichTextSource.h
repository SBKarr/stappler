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

class Source : public font::Controller {
public:
	static EventHeader onError;
	static EventHeader onDocument;

	enum Error : int64_t /* event compatible */ {
		DocumentError,
		NetworkError,
	};

	using StringDocument = ValueWrapper<String, class DocumentStringDocumentTag>;
	using FontConfigStyle = Document::FontConfigValue;

	virtual ~Source();

	virtual bool init();
	virtual bool init(const StringDocument &str);
	virtual bool init(const FilePath &file);
	virtual bool init(const String &url, const String &path,
			TimeInterval ttl = TimeInterval(), const String &cacheDir = "", const Asset::DownloadCallback & = nullptr);
	virtual bool init(Asset *a, bool enabled = true);

	Document *getDocument() const;
	Asset *getAsset() const;

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
	virtual void onDocumentAssetUpdated(data::Subscription::Flags);
	virtual void onDocumentLoaded(Document *);

	virtual void tryLoadDocument();
	virtual void updateDocument();

	virtual Rc<font::Source> makeSource(AssetMap &&);

	virtual Rc<Document> openDocument(const String &path, const String &ct);

	String _string;
	String _file;

	data::Listener<Asset> _documentAsset;
	Rc<Document> _document;

	uint64_t _loadedAssetMTime = 0;
	bool _documentLoading = false;
	bool _enabled = true;
};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTSOURCE_H_ */
