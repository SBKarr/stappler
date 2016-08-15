//
//  SPNetworkDownloadTask.h
//  chiertime-federal
//
//  Created by SBKarr on 6/24/13.
//
//

#ifndef __chiertime_federal__SPNetworkDownloadTask__
#define __chiertime_federal__SPNetworkDownloadTask__

#include "SPNetworkTask.h"

NS_SP_BEGIN

class NetworkDownload : public NetworkTask {
public:
    NetworkDownload();
    
    virtual bool init(const std::string &url, const std::string &fileName);
    
    virtual bool execute() override;
    virtual bool performQuery() override;
    virtual void onComplete() override;

    virtual void notifyOnStarted(bool bind = false);
    virtual void notifyOnProgress(float progress, bool bind = false);
    virtual void notifyOnComplete(bool success, bool bind = false);

public:
	typedef std::function<void(const NetworkDownload *download)> StartedCallback;
	typedef std::function<void(const NetworkDownload *download, float progress)> ProgressCallback;
	typedef std::function<void(const NetworkDownload *download, bool success)> CompletedCallback;

	virtual void setStartedCallback(StartedCallback func);
	virtual void setProgressCallback(ProgressCallback func);
	virtual void setCompletedCallback(CompletedCallback func);

protected:
	StartedCallback _onStarted;
	ProgressCallback _onProgress;
	CompletedCallback _onCompleted;
};

NS_SP_END

#endif /* defined(__chiertime_federal__SPNetworkDownloadTask__) */
