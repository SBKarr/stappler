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

	void addAssetFile(const String &, const String &, uint64_t asset, uint64_t ctime);
	void removeAssetFile(const String &);

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
	storage::Scheme _stateClass;

	bool _correctionNegative = false;
	TimeInterval _correctionTime;

	AssetRequestVec _tmpRequests;
	AssetMultiRequestVec _tmpMultiRequest;

	CallbacksList _callbacks;
};

NS_SP_END

#endif /* defined(__stappler__SPAssetLibrary__) */
