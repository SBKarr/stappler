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

#ifndef RICH_TEXT_COMMON_RTCOMMONSOURCE_H_
#define RICH_TEXT_COMMON_RTCOMMONSOURCE_H_

#include "RTSourceAsset.h"
#include "SPEventHandler.h"

NS_RT_BEGIN

class CommonSource : public font::FontController, protected EventHandler {
public:
	using Document = layout::Document;
	using StringDocument = Document::StringDocument;
	using AssetMeta = Document::AssetMeta;
	using AssetCallback = Function<void(const Function<void(SourceAsset *)> &)>;

	static EventHeader onError;
	static EventHeader onDocument;

	enum Error : int64_t /* event compatible */ {
		DocumentError,
		NetworkError,
	};

	struct AssetData {
		String originalUrl;
		data::Listener<SourceAsset> asset;
		AssetMeta meta;
	};

	static String getPathForUrl(const String &url);

	virtual ~CommonSource();

	virtual bool init();
	virtual bool init(SourceAsset *, bool enabled = true);

	void setHyphens(layout::HyphenMap *);
	layout::HyphenMap *getHyphens() const;

	Document *getDocument() const;
	SourceAsset *getAsset() const;
	Asset *getNetworkAsset() const;

	Map<String, AssetMeta> getExternalAssetMeta() const;
	const Map<String, AssetData> &getExternalAssets() const;

	bool isReady() const;
	bool isActual() const;
	bool isDocumentLoading() const;

	void refresh();

	bool tryReadLock(Ref *);
	void retainReadLock(Ref *, const Function<void()> &);
	void releaseReadLock(Ref *);

	void setEnabled(bool val);
	bool isEnabled() const;

	bool isFileExists(const StringView &url) const;
	Pair<uint16_t, uint16_t> getImageSize(const StringView &url) const;
	Bytes getImageData(const StringView &url) const;

	virtual void update(float dt) override;

protected:
	virtual void onDocumentAsset(SourceAsset *);
	virtual void onDocumentAssetUpdated(data::Subscription::Flags);
	virtual void onDocumentLoaded(Document *);

	virtual void acquireAsset(const String &, const Function<void(SourceAsset *)> &);
	virtual bool isExternalAsset(Document *doc, const String &); // true is asset is external (not stored in document itself)

	virtual bool onExternalAssets(Document *doc, const Set<String> &); // true if no asset requests is performed
	virtual void onExternalAssetUpdated(AssetData *, data::Subscription::Flags);

	virtual bool readExternalAsset(AssetData &); // true if asset meta was updated

	virtual void tryLoadDocument();
	virtual void updateDocument();

	virtual void onSourceUpdated(FontSource *) override;

	virtual Rc<font::FontSource> makeSource(AssetMap &&, bool schedule = false) override;

	bool hasAssetRequests() const;
	void addAssetRequest(AssetData *);
	void removeAssetRequest(AssetData *);
	void waitForAssets(Function<void()> &&);

	Vector<SyncRWLock *> getAssetsVec() const;

	data::Listener<SourceAsset> _documentAsset;
	Rc<Document> _document;
	Map<String, AssetData> _networkAssets;

	int64_t _loadedAssetMTime = 0;
	bool _documentLoading = false;
	bool _enabled = true;

	Set<AssetData *> _assetRequests;
	Vector<Function<void()>> _assetWaiters;

	float _retryUpdate = nan();

	Rc<font::HyphenMap> _hyphens;

	struct LockStruct {
		Vector<SyncRWLock *> vec;
		Rc<CommonSource> source;
		size_t count = 1;
		bool acquired = false;
		Vector<Function<void()>> waiters;
	};

	Map<Rc<Ref>, LockStruct> _readLocks;
};

NS_RT_END

#endif /* RICH_TEXT_COMMON_RTCOMMONSOURCE_H_ */
