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
