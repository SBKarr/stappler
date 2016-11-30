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
