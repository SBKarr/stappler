//
//  SPAssetDownload.h
//  stappler
//
//  Created by SBKarr on 7/27/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#ifndef __stappler__SPAssetDownload__
#define __stappler__SPAssetDownload__

#include "SPNetworkDownload.h"
#include "SPAsset.h"

NS_SP_BEGIN

class AssetDownload : public NetworkDownload {
public:
    enum CacheRequestType {
    	CacheRequest
    };

    virtual bool init(Asset *, CacheRequestType);
    virtual bool init(Asset *, const std::string &);


    virtual ~AssetDownload();

    virtual bool execute() override;
    virtual bool performQuery() override;

    enum class Validation {
    	Valid,
		Invalid,
		NotSupported,
		Unavailable,
    };

    Validation validateDownload(bool fileOnly = false, bool bind = false);
	void updateCache(bool bind);

    Asset *getAsset() const { return _asset; }
    bool isCacheRequest() const { return _cacheRequest; }

    virtual void notifyOnCacheData(uint64_t, size_t, const std::string &, const std::string &ct, bool bind = false);
    virtual void notifyOnStarted(bool bind = false) override;
    virtual void notifyOnProgress(float progress, bool bind = false) override;
    virtual void notifyOnComplete(bool success, bool bind = false) override;
protected:

	bool executeLoop();

	Rc<Asset> _asset;
	bool _cacheRequest = false;
};

NS_SP_END

#endif /* defined(__stappler__SPAssetDownload__) */
