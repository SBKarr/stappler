/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef EXTENSIONS_DOCUMENT_SRC_RICH_TEXT_COMMON_RTSOURCEASSET_H_
#define EXTENSIONS_DOCUMENT_SRC_RICH_TEXT_COMMON_RTSOURCEASSET_H_

#include "RTCommon.h"
#include "SPEventHeader.h"
#include "SPDataListener.h"
#include "SPAsset.h"

NS_RT_BEGIN

class SourceAsset : public data::Subscription {
public:
	using Document = layout::Document;
	using StringDocument = Document::StringDocument;

	virtual Rc<Document> openDocument() = 0;

	virtual SyncRWLock *getRWLock() const { return nullptr; }

	virtual int64_t getMTime() const { return 0; }

	virtual bool tryLockDocument(int64_t mtime) { retain(); return true; }
	virtual void releaseDocument() { release(); }

	virtual bool tryReadLock() { retain(); return true; }
	virtual void releaseReadLock() { release(); }

	virtual bool download() { return false; }

	virtual bool isDownloadAvailable() const { return false; }
	virtual bool isDownloadInProgress() const { return false; }
	virtual bool isUpdateAvailable() const { return false; }
	virtual bool isReadAvailable() const { return true; }

	virtual StringView getContentType() const { return StringView(); }

	virtual Bytes getData() const { return Bytes(); }

	virtual bool getImageSize(size_t &w, size_t &h) const { return false; }

	virtual void setExtraData(data::Value &&) { }
	virtual data::Value getExtraData() const { return data::Value(); }
};

class SourceMemoryAsset : public SourceAsset {
public:
	virtual bool init(const StringDocument &str, StringView type = StringView());
	virtual bool init(const StringView &str, StringView type = StringView());
	virtual bool init(const BytesView &data, StringView type = StringView());

	virtual Rc<Document> openDocument() override;

	virtual StringView getContentType() const override { return _type; }

	virtual Bytes getData() const override;

	virtual bool getImageSize(size_t &w, size_t &h) const override;

protected:
	String _type;
	Bytes _data;
};

class SourceFileAsset : public SourceAsset {
public:
	virtual bool init(const FilePath &file, StringView type = StringView());
	virtual bool init(const StringView &str, StringView type = StringView());

	virtual Rc<Document> openDocument() override;

	virtual StringView getContentType() const override { return _type; }

	virtual Bytes getData() const override;

	virtual bool getImageSize(size_t &w, size_t &h) const override;

protected:
	String _type;
	String _file;
};

class SourceNetworkAsset : public SourceAsset {
public:
	using AssetCallback = Function<void(const Function<void(Asset *)> &)>;

	virtual bool init(const String &url, const String &path,
			TimeInterval ttl = TimeInterval(), const String &cacheDir = "", const Asset::DownloadCallback & = nullptr);
	virtual bool init(Asset *a);
	virtual bool init(const AssetCallback &cb);

	virtual void setAsset(Asset *a);
	virtual void setAsset(const AssetCallback &cb);

	virtual Asset *getAsset() const;

	virtual Rc<Document> openDocument() override;

	virtual SyncRWLock *getRWLock() const override;

	virtual int64_t getMTime() const override;

	virtual bool tryLockDocument(int64_t mtime) override;
	virtual void releaseDocument() override;

	virtual bool tryReadLock() override;
	virtual void releaseReadLock() override;

	virtual bool download() override;

	virtual bool isDownloadAvailable() const override;
	virtual bool isDownloadInProgress() const override;
	virtual bool isUpdateAvailable() const override;
	virtual bool isReadAvailable() const override;

	virtual StringView getContentType() const override;

	virtual Bytes getData() const override;

	virtual bool getImageSize(size_t &w, size_t &h) const override;

	virtual void setExtraData(data::Value &&) override;
	virtual data::Value getExtraData() const override;

protected:
	virtual void onAsset(Asset *);

	String _savedFilePath;
	String _savedType;

	Rc<Asset> _asset;
};

NS_RT_END

#endif /* EXTENSIONS_DOCUMENT_SRC_RICH_TEXT_COMMON_RTSOURCEASSET_H_ */
