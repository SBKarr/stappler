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

#ifndef STAPPLER_SRC_FEATURES_NETWORKING_SPASSET_H_
#define STAPPLER_SRC_FEATURES_NETWORKING_SPASSET_H_

#include "SPFilesystem.h"
#include "SPEventHeader.h"
#include "SPEventHandler.h"
#include "SPData.h"
#include "SPSyncRWLock.h"

NS_SP_BEGIN

class AssetFile : public Ref {
public:
	~AssetFile();

	bool init(Asset *);

	Bytes readFile() const;
	filesystem::ifile open() const;

	void remove();

	bool match(const Asset *) const;
	bool exists() const { return _exists; }
	operator bool() const { return _exists; }

	uint64_t getCTime() const { return _ctime; }
	uint64_t getMTime() const { return _mtime; }
	uint64_t getAssetId() const { return _id; }
	size_t getSize() const { return _size; }

	StringView getPath() const { return _path; }
	StringView getUrl() const { return _url; }
	StringView getContentType() const { return _contentType; }
	StringView getETag() const { return _etag; }

protected:
	friend class Asset;

	bool _exists = false;
	uint64_t _ctime = 0;
	uint64_t _mtime = 0;
	uint64_t _id = 0;
	size_t _size = 0;

	String _url;
	String _path;
	String _contentType;
	String _etag;
};

class Asset : public SyncRWLock {
public:
    enum Update : uint8_t {
    	CacheDataUpdated = 2,
    	FileUpdated,
		DownloadStarted,
		DownloadProgress,
		DownloadCompleted,
		DownloadSuccessful,
		DownloadFailed,
		WriteLocked,
		ReadLocked,
		Unlocked,
    };

    using DownloadCallback = Function<bool(AssetDownload &)>;

public:
	~Asset();

    bool download();
    void clear();
	void checkFile();

	bool isReadAvailable() const;
	bool isDownloadAvailable() const;
	bool isUpdateAvailable() const;

	void setDownloadCallback(const DownloadCallback &);

	StringView getFilePath() const { return _path; }
	StringView getCachePath() const { return _cachePath; }
	StringView getUrl() const { return _url; }
	StringView getContentType() const { return _contentType; }

    bool isDownloadInProgress() const { return _downloadInProgress; }
	float getProgress() const { return _progress; }

	uint64_t getMTime() const { return _mtime; }
	uint64_t getId() const { return _id; }
	size_t getSize() const { return _size; }
	StringView getETag() const { return _etag; }

	Time getTouch() const { return _touch; }
	TimeInterval getTtl() const { return _ttl; }

	void setData(const data::Value &d) { _data = d; _storageDirty = true; }
	void setData(data::Value &&d) { _data = std::move(d); _storageDirty = true; }
	const data::Value &getData() const { return _data; }

	bool isFileExists() const { return _fileExisted; }
	bool isFileUpdate() const { return _fileUpdate; }

	void onStarted();
	void onProgress(float progress);
	void onCompleted(bool success, bool cacheRequest, const StringView &file, const StringView &ct, const StringView &etag, uint64_t mtime, size_t size);
	void onFile();

	void save();
	void touch();

	bool isStorageDirty() const { return _storageDirty; }
	void setStorageDirty(bool value) { _storageDirty = value; }

	Rc<AssetFile> cloneFile();

protected:
	friend class AssetLibrary;

    Asset(const data::Value &, const DownloadCallback &);

    void update(Update);
    bool swapFiles(const StringView &file, const StringView &ct, const StringView &etag, uint64_t mtime, size_t size);
    void touchWithTime(Time t);

    virtual void onLocked(Lock) override;

	uint64_t _id = 0;

	String _url;
	String _path;
	String _cachePath;
	String _contentType;

	Time _touch;
	TimeInterval _ttl;

	uint64_t _mtime = 0;
	size_t _size = 0;
	String _etag;

	float _progress = 0;

	bool _storageDirty = false;

	bool _fileExisted = false;
	bool _unupdated = false;
	bool _waitFileSwap = false;
	bool _downloadInProgress = false;
	bool _fileUpdate = false;

	String _tempPath; // if set - we should swap files on write lock or in destructor
	DownloadCallback _downloadFunction = nullptr;

	data::Value _data;
};

NS_SP_END

#endif /* STAPPLER_SRC_FEATURES_NETWORKING_SPASSET_H_ */
