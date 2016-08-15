//
//  SPAsset.h
//  chieftime-federal
//
//  Created by SBKarr on 09.07.13.
//
//

#ifndef __chieftime_federal__SPAsset__
#define __chieftime_federal__SPAsset__

#include "SPEventHeader.h"
#include "SPEventHandler.h"
#include "SPData.h"
#include "SPSyncRWLock.h"

NS_SP_BEGIN

class AssetDownload;

class Asset : public SyncRWLock {
public:
    enum Update : uint8_t {
    	CacheDataUpdated = 1,
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

    using DownloadCallback = std::function<bool(Asset *)>;

public:
	~Asset();

    bool download();
    void clear();
	void checkFile();

	bool isReadAvailable() const;
	bool isDownloadAvailable() const;
	bool isUpdateAvailable() const;

	void setDownloadCallback(const DownloadCallback &);

	const std::string &getFilePath() const { return _path; }
	const std::string &getCachePath() const { return _cachePath; }
	const std::string &getUrl() const { return _url; }
    const std::string &getContentType() const { return _contentType; }

    bool isDownloadInProgress() const { return _downloadInProgress; }
	float getProgress() const { return _progress; }

	uint64_t getMTime() const { return _mtime; }
	uint64_t getId() const { return _id; }
	size_t getSize() const { return _size; }
	const std::string getETag() const { return _etag; }

	Time getTouch() const { return _touch; }
	TimeInterval getTtl() const { return _ttl; }

	void setData(const data::Value &d) { _data = d; _storageDirty = true; }
	void setData(data::Value &&d) { _data = std::move(d); _storageDirty = true; }
	const data::Value &getData() const { return _data; }

	bool isFileExists() const { return _fileExisted; }
	bool isFileUpdate() const { return _fileUpdate; }

	void onCacheData(uint64_t mtime, size_t size, const std::string &etag, const std::string &ct);
	void onStarted();
	void onProgress(float progress);
	void onCompleted(bool success, const std::string &, const std::string &, bool cacheRequest);
	void onFile();

	void save();
	void touch();
	void syncWithNetwork();

	bool isStorageDirty() const { return _storageDirty; }
	void setStorageDirty(bool value) { _storageDirty = value; }

protected:
	friend class AssetLibrary;

    Asset(const data::Value &, const DownloadCallback &);

    void update(Update);
    bool swapFiles();
    void touchWithTime(Time t);

    virtual void onLocked(Lock) override;

	uint64_t _id = 0;

	std::string _url;
	std::string _path;
	std::string _cachePath;
	std::string _contentType;

	Time _touch;
	TimeInterval _ttl;

	uint64_t _mtime = 0;
	size_t _size = 0;
	std::string _etag;

	float _progress = 0;

	bool _storageDirty = false;

	bool _fileExisted = false;
	bool _unupdated = false;
	bool _downloadInProgress = false;
	bool _fileUpdate = false;

	std::string _tempPath; // if set - we should swap files on write lock or in destructor
	DownloadCallback _downloadFunction = nullptr;

	data::Value _data;
};

NS_SP_END

#endif /* defined(__chieftime_federal__SPAsset__) */
