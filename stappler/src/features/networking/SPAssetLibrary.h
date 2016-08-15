//
//  SPAssetLibrary.h
//  stappler
//
//  Created by SBKarr on 17.09.14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#ifndef __stappler__SPAssetLibrary__
#define __stappler__SPAssetLibrary__

#include "SPEventHeader.h"
#include "SPScheme.h"

#include "base/CCMap.h"

NS_SP_BEGIN

class Asset;
class AssetDownload;

class AssetLibrary : public cocos2d::Ref {
public:
	static EventHeader onLoaded;

	struct AssetRequest;

	using AssetCallback = std::function<void (Asset *)>;

	using AssetVec = std::vector<Asset *>;
	using AssetVecCallback = std::function<void (const AssetVec &)>;

	using AssetRequestVec = std::vector<AssetRequest>;
	using AssetMultiRequestVec = std::vector<std::pair<AssetRequestVec, AssetVecCallback>>;

    using DownloadCallback = std::function<bool(Asset *)>;
	using Assets = std::map<uint64_t, Asset *>;
	using Downloads = cocos2d::Map<Asset *, AssetDownload *>;

	using CallbackVec = std::vector<AssetCallback>;
	using CallbacksList = std::map<uint64_t, CallbackVec>;

	struct AssetRequest {
		AssetCallback callback;
		uint64_t id;
		std::string url;
		std::string path;
		TimeInterval ttl;
		std::string cacheDir;
		DownloadCallback download;

		AssetRequest(const AssetCallback &, const std::string &url, const std::string &path,
				TimeInterval ttl = TimeInterval(), const std::string &cacheDir = "", const DownloadCallback & = nullptr);
	};

public:
	static AssetLibrary *getInstance();

	void setServerDate(const Time &t);

	/* get asset from db, new asset is autoreleased */
	bool getAsset(const AssetCallback &cb, const std::string &url, const std::string &path,
			TimeInterval ttl = TimeInterval(), const std::string &cacheDir = "", const DownloadCallback & = nullptr);

	bool getAssets(const AssetRequestVec &, const AssetVecCallback & = nullptr);

	Asset *acquireLiveAsset(const std::string &url, const std::string &path);

	bool isLoaded() const { return _loaded; }
	uint64_t getAssetId(const std::string &url, const std::string &path) const;

	std::string getTempPath(const std::string &) const;

	bool isLiveAsset(uint64_t) const;
	bool isLiveAsset(const std::string &url, const std::string &path) const;

	void updateAssets();
protected:
	friend class Asset;

	void removeAsset(Asset *);
	bool downloadAsset(Asset *);
	void saveAsset(Asset *);
	void touchAsset(Asset *);

protected:
	AssetLibrary();
	void init();
	void restoreDownloads(data::Value &, Downloads &);
	void cleanup();

	void startDownload(AssetDownload *);
	void removeDownload(AssetDownload *);

	Time getCorrectTime() const;
	Asset *getLiveAsset(uint64_t id) const;
	Asset *getLiveAsset(const std::string &, const std::string &) const;

	void onAssetCreated(Asset *);

	friend class storage::Handle;
	static void importAssetData(data::Value &);

	void performGetAssets(std::vector<Asset *> &, const std::vector<AssetRequest> &);

	bool _loaded = false;

	Assets _assets;
	Downloads _downloads;

	std::mutex _mutex;

	storage::Scheme _assetsClass;

	bool _correctionNegative = false;
	TimeInterval _correctionTime;

	AssetRequestVec _tmpRequests;
	AssetMultiRequestVec _tmpMultiRequest;

	CallbacksList _callbacks;
};

NS_SP_END

#endif /* defined(__stappler__SPAssetLibrary__) */
