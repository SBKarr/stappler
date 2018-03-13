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

#ifndef STAPPLER_SRC_FEATURES_NETWORKING_SPASSETLIBRARY_H_
#define STAPPLER_SRC_FEATURES_NETWORKING_SPASSETLIBRARY_H_

#include "SPEventHeader.h"
#include "SPScheme.h"
#include "SPAsset.h"

NS_SP_BEGIN

class AssetLibrary : public Ref {
public:
	static EventHeader onLoaded;

	struct AssetRequest;

	using AssetCallback = Function<void (Asset *)>;

	using AssetVec = Vector<Rc<Asset>>;
	using AssetVecCallback = Function<void (const AssetVec &)>;

	using AssetRequestVec = Vector<AssetRequest>;
	using AssetMultiRequestVec = Vector<Pair<AssetRequestVec, AssetVecCallback>>;

    using DownloadCallback = Asset::DownloadCallback;
	using Assets = Map<uint64_t, Asset *>;
	using Downloads = Map<Asset *, Rc<AssetDownload>>;

	using CallbackVec = Vector<AssetCallback>;
	using CallbacksList = Map<uint64_t, CallbackVec>;

	struct AssetRequest {
		AssetCallback callback;
		uint64_t id;
		String url;
		String path;
		TimeInterval ttl;
		String cacheDir;
		DownloadCallback download;

		AssetRequest(const AssetCallback &, const String &url, const String &path,
				TimeInterval ttl = TimeInterval(), const String &cacheDir = "", const DownloadCallback & = nullptr);
	};

public:
	static void setAssetSignKey(Bytes &&);
	static const Bytes &getAssetSignKey();

	static AssetLibrary *getInstance();

	void setServerDate(const Time &t);

	/* get asset from db, new asset is autoreleased */
	bool getAsset(const AssetCallback &cb, const String &url, const String &path,
			TimeInterval ttl = TimeInterval(), const String &cacheDir = "", const DownloadCallback & = nullptr);

	bool getAssets(const AssetRequestVec &, const AssetVecCallback & = nullptr);

	Asset *acquireLiveAsset(const String &url, const String &path);

	bool isLoaded() const { return _loaded; }
	uint64_t getAssetId(const String &url, const String &path) const;

	String getTempPath(const String &) const;

	bool isLiveAsset(uint64_t) const;
	bool isLiveAsset(const String &url, const String &path) const;

	void updateAssets();

	void addAssetFile(const String &, const String &, uint64_t asset, uint64_t ctime);
	void removeAssetFile(const String &);

	void finalize();

protected:
	friend class Asset;

	void removeAsset(Asset *);
	void saveAsset(Asset *);
	void touchAsset(Asset *);

	AssetDownload * downloadAsset(Asset *);

protected:
	AssetLibrary();
	void init();
	void restoreDownloads(data::Value &, Downloads &);
	void cleanup();

	void startDownload(AssetDownload *);
	void removeDownload(AssetDownload *);

	Time getCorrectTime() const;
	Asset *getLiveAsset(uint64_t id) const;
	Asset *getLiveAsset(const String &, const String &) const;

	void onAssetCreated(Asset *);

	friend class storage::Handle;

	void performGetAssets(AssetVec &, const std::vector<AssetRequest> &);

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

#endif /* STAPPLER_SRC_FEATURES_NETWORKING_SPASSETLIBRARY_H_ */
