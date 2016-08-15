//
//  SPNetworkSprite.h
//  stappler
//
//  Created by SBKarr on 13.09.14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#ifndef __stappler__SPNetworkSprite__
#define __stappler__SPNetworkSprite__

#include "SPEventHandler.h"
#include "SPAsset.h"
#include "SPDynamicSprite.h"
#include "SPDataListener.h"

NS_SP_BEGIN

class NetworkSprite : public DynamicSprite {
public:
	static std::string getPathForUrl(const std::string &);
	static bool isCachedTextureUrl(const std::string &);

	virtual ~NetworkSprite();

	virtual bool init(const std::string &url, float density = 0.0f);
    virtual void visit(cocos2d::Renderer *, const cocos2d::Mat4 &, uint32_t, ZPath &zPath) override;

    virtual void setUrl(const std::string &, bool force = false);
	virtual const std::string &getUrl() const;

protected:
	virtual void onAsset(Asset *, bool force);
	virtual void onAssetUpdated(data::Subscription::Flags);

	virtual void updateSprite(bool force = false);
	virtual void loadTexture();

	std::string _url;
	std::string _filePath;

	bool _assetDirty = true;

	data::Listener<Asset> _asset;
};

NS_SP_END

#endif /* defined(__stappler__SPNetworkSprite__) */
